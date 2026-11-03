"""Registry, two-phase validation/import, dependency ordering and audit reports."""
import hashlib
import json
from datetime import datetime
from pathlib import Path
from . import progression, waves

PLUGIN = Path(__file__).resolve().parents[3]
DATA_DIR = PLUGIN / "Data"
ADAPTERS = {"progression": progression, "waves": waves}


def registry(directory=DATA_DIR):
    directory = Path(directory).resolve()
    data = json.loads((directory / "ConfigRegistry.json").read_text(encoding="utf-8-sig"))
    if data.get("version") != 1 or not isinstance(data.get("groups"), list):
        raise ValueError("ConfigRegistry.json: unsupported version or missing groups")
    groups, owned_sources, owned_outputs = {}, set(), set()
    for group in data["groups"]:
        identity = group["id"]
        if identity not in ADAPTERS or identity in groups:
            raise ValueError(f"Unknown or duplicate configuration group: {identity}")
        if not group.get("label") or not group.get("sources") or not group.get("outputs"):
            raise ValueError(f"Incomplete configuration group: {identity}")
        if set(group["sources"]) != ADAPTERS[identity].SOURCES:
            raise ValueError(f"{identity}: registered sources do not match adapter inputs")
        for source in group["sources"]:
            path = (directory / source).resolve()
            if not path.is_relative_to(directory) or path.suffix.lower() != ".csv":
                raise ValueError(f"Invalid CSV path: {source}")
            if source.casefold() in owned_sources:
                raise ValueError(f"CSV belongs to multiple groups: {source}")
            owned_sources.add(source.casefold())
        for output in group["outputs"]:
            if not output.startswith("/MonsterArena/") or output.casefold() in owned_outputs:
                raise ValueError(f"Invalid or shared output asset: {output}")
            owned_outputs.add(output.casefold())
        groups[identity] = group
    unmanaged = sorted(str(p.relative_to(directory)) for p in directory.rglob("*.csv") if str(p.relative_to(directory)).replace("\\", "/").casefold() not in owned_sources)
    if unmanaged:
        raise ValueError(f"CSV files missing from ConfigRegistry.json: {unmanaged}")
    return groups


def ordered_groups(groups, selected=None):
    selected = list(groups) if selected is None else list(selected)
    ordered, visiting = [], set()

    def visit(identity):
        if identity not in groups:
            raise ValueError(f"Unknown group/dependency: {identity}")
        if identity in visiting:
            raise ValueError(f"Configuration dependency cycle: {identity}")
        if identity in ordered:
            return
        visiting.add(identity)
        for dependency in groups[identity].get("depends_on", []):
            visit(dependency)
        visiting.remove(identity)
        ordered.append(identity)

    for identity in selected:
        visit(identity)
    if not ordered:
        raise ValueError("No configuration groups selected")
    return ordered


def fingerprints(groups, selected, directory):
    names = {"ConfigRegistry.json"}
    for identity in selected:
        names.update(groups[identity]["sources"])
    return {name: hashlib.sha256((directory / name).read_bytes()).hexdigest() for name in sorted(names)}


def prepare(selected=None, directory=DATA_DIR):
    """Pure validation. Returns immutable-by-convention snapshots; writers never reread CSV."""
    directory = Path(directory)
    groups = registry(directory)
    selected = ordered_groups(groups, selected)
    before = fingerprints(groups, selected, directory)
    snapshots, errors = {}, []
    for identity in selected:
        try:
            snapshots[identity] = ADAPTERS[identity].validate(directory)
        except (ValueError, OSError, KeyError) as error:
            errors.append(f"[{identity}] {error}")
    if errors:
        raise ValueError("CSV validation failed; no assets changed:\n" + "\n".join(errors))
    if before != fingerprints(groups, selected, directory):
        raise ValueError("CSV changed during validation; retry before importing")
    return groups, selected, snapshots, before


def run(action="import", selected=None, directory=DATA_DIR):
    if action not in ("list", "validate", "import"):
        raise ValueError(f"Unknown action: {action}")
    report = {"action": action, "selected": selected, "success": False, "groups": [], "error": None}
    stamp = datetime.now().astimezone().strftime("%Y%m%d-%H%M%S-%f")
    report_path = PLUGIN.parents[2] / "Saved/ConfigImports/MonsterArena" / (stamp + ".json")
    try:
        if action == "list":
            groups = registry(directory)
            report["groups"] = list(groups.values())
        else:
            groups, identities, snapshots, hashes = prepare(selected, directory)
            report["source_sha256"] = hashes
            report["groups"] = [dict(id=i, sources=groups[i]["sources"], outputs=groups[i]["outputs"], status="validated") for i in identities]
            if action == "import":
                import unreal
                # Unlike LevelEditorSubsystem.IsInPlayInEditor, this is safe without an open Level Editor.
                if unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_game_world() is not None:
                    raise RuntimeError("Stop PIE before importing CSV configuration")
                for entry in report["groups"]:
                    entry["status"] = "importing"
                    try:
                        ADAPTERS[entry["id"]].apply(snapshots[entry["id"]])
                    except Exception:
                        entry["status"] = "failed"
                        raise
                    entry["status"] = "imported"
        report["success"] = True
        return report
    except Exception as error:
        report["error"] = str(error)
        raise
    finally:
        report_path.parent.mkdir(parents=True, exist_ok=True)
        report["report_path"] = str(report_path)
        report_path.write_text(json.dumps(report, ensure_ascii=False, indent=2), encoding="utf-8")
        print(json.dumps(report, ensure_ascii=False, indent=2))


def register_menu():
    """Register idempotently when the Level Editor Tools menu is available."""
    import unreal
    menus = unreal.ToolMenus.get()
    if menus is None:
        return False
    parent = menus.find_menu("LevelEditor.MainMenu.Tools")
    if parent is None:
        return False
    menu = parent.add_sub_menu("MonsterArenaCSV", "Tools", "MonsterArenaCSV", "MonsterArena CSV 配置")
    groups = registry()

    def add(target, name, label, action, selected=None):
        entry = unreal.ToolMenuEntry(name=name, type=unreal.MultiBlockType.MENU_ENTRY)
        entry.set_label(label)
        command = "import runpy; runpy.run_path(" + repr(str(PLUGIN / "Content/Python/manage_csv.py")) + ")[\"run\"](" + repr(action) + ", " + repr(selected) + ")"
        entry.set_string_command(unreal.ToolMenuStringCommandType.PYTHON, "", command)
        target.add_menu_entry("Configuration", entry)

    add(menu, "List", "在输出日志列出配置清单", "list")
    add(menu, "ValidateAll", "校验全部（不改资产）", "validate")
    add(menu, "ImportAll", "导入全部", "import")
    for identity, group in groups.items():
        sub = menu.add_sub_menu("MonsterArenaCSV", "Groups", identity, group["label"])
        add(sub, "Validate", "校验此组", "validate", [identity])
        add(sub, "Import", "导入此组", "import", [identity])
    for location, label in (("data", "打开 CSV 配置目录"), ("reports", "打开导入报告目录")):
        entry = unreal.ToolMenuEntry(name="Open_" + location, type=unreal.MultiBlockType.MENU_ENTRY)
        entry.set_label(label)
        command = "import runpy; runpy.run_path(" + repr(str(PLUGIN / "Content/Python/manage_csv.py")) + ")[\"open_location\"](" + repr(location) + ")"
        entry.set_string_command(unreal.ToolMenuStringCommandType.PYTHON, "", command)
        menu.add_menu_entry("Files", entry)
    menus.refresh_all_widgets()
    return True


def open_location(location):
    import os
    paths = {"data": DATA_DIR, "reports": PLUGIN.parents[2] / "Saved/ConfigImports/MonsterArena"}
    if location not in paths:
        raise ValueError("Unknown configuration location")
    paths[location].mkdir(parents=True, exist_ok=True)
    os.startfile(str(paths[location]))
