"""Expose MonsterArena CSV tools when the editor menu becomes available."""
import time
import runpy
from pathlib import Path
import unreal

def _install_csv_menu():
    # Unreal removes __file__ after executing startup scripts. Capture it before ticking.
    script = str(Path(__file__).with_name("manage_csv.py"))
    started = time.monotonic()
    handle = None

    def register(_delta):
        try:
            api = runpy.run_path(script)
            if api["register_menu"]() or time.monotonic() - started > 30:
                unreal.unregister_slate_post_tick_callback(handle)
        except Exception:
            unreal.unregister_slate_post_tick_callback(handle)
            raise

    handle = unreal.register_slate_post_tick_callback(register)


if not unreal.SystemLibrary.is_unattended() and "-run=" not in unreal.SystemLibrary.get_command_line().lower():
    _install_csv_menu()
