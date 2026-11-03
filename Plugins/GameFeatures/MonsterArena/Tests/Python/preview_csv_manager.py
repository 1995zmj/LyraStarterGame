"""Checks live PIE import guard, then leaves the real Tools menu for manual input validation."""
import time
import runpy
from pathlib import Path
import unreal

unreal.load_module("MonsterArenaEditor")
unreal.EditorPythonScripting.set_keep_python_script_alive(True)
api = runpy.run_path(str(Path(__file__).resolve().parents[2] / "Content/Python/manage_csv.py"))
assert api["register_menu"]()
assert api["register_menu"]()
session = unreal.MonsterArenaRewardTestLibrary
assert session.start_session(1)
start = time.monotonic()
stage = 0


def tick(_delta):
    global stage
    if time.monotonic() - start > 90:
        unreal.unregister_slate_post_tick_callback(handle)
        raise RuntimeError("CSV manager preview startup timed out")
    if stage == 0 and session.get_test_worlds():
        try:
            api["run"]("import")
        except RuntimeError as error:
            assert "Stop PIE" in str(error)
            unreal.log("MONSTER_ARENA_CSV_PIE_GUARD_PASSED")
        else:
            raise AssertionError("CSV import was permitted in PIE")
        stage = 1
        session.stop_session()
    elif stage == 1 and not session.get_test_worlds():
        assert api["register_menu"]()
        unreal.log("MONSTER_ARENA_CSV_MENU_READY")
        unreal.unregister_slate_post_tick_callback(handle)


handle = unreal.register_slate_post_tick_callback(tick)
