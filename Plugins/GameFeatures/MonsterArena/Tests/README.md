# MonsterArena 测试

测试内容按 Unreal 的编译目录组织，仍归属 MonsterArena 插件：

| 位置 | 用途 |
| --- | --- |
| `../Source/MonsterArenaRuntime/Private/Tests/MonsterArenaSmokeTest.cpp` | 击杀、经验、升级与奖励回归 |
| `../Source/MonsterArenaRuntime/Private/Tests/MonsterArenaAmmoTest.cpp` | 自动射击、松开停火、空弹匣和换弹回归 |
| `../Source/MonsterArenaRuntime/Private/Tests/MonsterArenaLifecycleTest.cpp` | 怪物击杀玩家、自动复活、奖励与装备恢复回归 |
| `Python/validate_monster_arena_editor.py` | Editor 模块和 Python 配置接口校验 |
| [ACCEPTANCE.md](ACCEPTANCE.md) | 验收证据、结果及未完成项 |

## 运行玩法测试

从项目根目录执行，`$UE` 为引擎安装目录：

```powershell
& "$UE/Engine/Binaries/Win64/UnrealEditor-Cmd.exe" "$PWD/LyraStarterGame.uproject" /MonsterArena/Maps/L_MonsterArena -game -MonsterArenaSmokeTest -unattended -NullRHI -nosound -SCCProvider=None -abslog="$PWD/Saved/Logs/MonsterArenaSmoke.log"
```

将 `-MonsterArenaSmokeTest` 换成 `-MonsterArenaAmmoTest` 或 `-MonsterArenaLifecycleTest` 可运行对应测试；每次单独启动一个进程，并使用不同日志文件。成功标记分别为 `MONSTER_ARENA_SMOKE_SUCCEEDED`、`MONSTER_ARENA_AMMO_SUCCEEDED` 和 `MONSTER_ARENA_LIFECYCLE_SUCCEEDED`，测试完成后退出进程。

C++ 测试及调用入口受 `WITH_DEV_AUTOMATION_TESTS` 保护。当前配置下 Shipping 不包含这些测试逻辑；Development 中也需显式启动参数才运行。

## 运行编辑器校验

```powershell
& "$UE/Engine/Binaries/Win64/UnrealEditor-Cmd.exe" "$PWD/LyraStarterGame.uproject" -run=pythonscript -script="$PWD/Plugins/GameFeatures/MonsterArena/Tests/Python/validate_monster_arena_editor.py" -unattended -NullRHI -nosound -SCCProvider=None -abslog="$PWD/Saved/Logs/MonsterArenaEditorModuleValidation.log"
```

成功输出 `MONSTER_ARENA_EDITOR_MODULE_VALIDATION_SUCCEEDED`。脚本使用临时对象，不修改已保存资源。资源生成工具 `Content/Python/setup_monster_arena.py` 继续放在工具目录。

无界面测试不能代替画面、真实鼠标键盘输入和联网验收；检查本轮完整日志，不只检查成功标记。具体限制和历史结果见验收记录。
