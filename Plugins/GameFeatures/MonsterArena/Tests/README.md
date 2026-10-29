# MonsterArena 测试

测试内容归属 MonsterArena 插件，目录如下：

| 位置 | 用途 |
| --- | --- |
| `../Source/MonsterArenaRuntime/Private/Tests/MonsterArenaSmokeTest.cpp` | 击杀、经验、显式选卡、升级奖励和基础资源回归；调用选卡专项测试 |
| `../Source/MonsterArenaRuntime/Private/Tests/MonsterArenaRewardChoiceTest.cpp` | 待选次数、关闭重开、候选去重、即时效果、无效请求、卡池限制和输入锁回归 |
| `../Source/MonsterArenaRuntime/Private/Tests/MonsterArenaRewardTestHelpers.h` | 临时卡池和显式选卡的测试辅助代码 |
| `../Source/MonsterArenaRuntime/Private/Tests/MonsterArenaAmmoTest.cpp` | 自动射击、松开停火、空弹匣和换弹回归 |
| `../Source/MonsterArenaRuntime/Private/Tests/MonsterArenaLifecycleTest.cpp` | 真实怪物击杀与自动复活，检查已选永久奖励、未选候选和即时奖励不重复发放 |
| `../Source/MonsterArenaEditor/Private/Tests/MonsterArenaRewardTestLibrary.h/.cpp` | 启停真实 PIE、枚举 PIE World、施加正常 GAS 伤害、查询输入锁和伤害免疫状态的 Editor 测试桥接 |
| `Python/validate_monster_arena_editor.py` | Editor 模块和 Python 配置接口校验 |
| `Python/validate_reward_multiplayer.py` | 两名玩家的监听服务器／客户端 PIE 选卡、复制和界面生命周期回归 |
| `Python/preview_reward_ui.py` | 保持编辑器运行的单人选卡界面预览，供真实鼠标、键盘、手柄和画面验收 |
| [ACCEPTANCE.md](ACCEPTANCE.md) | 实际验收证据、结果及未完成项 |

## 运行 Runtime 玩法测试

先编译 `LyraEditor Win64 Development`。以下命令均从项目根目录执行，`$UE` 为引擎安装目录：

```powershell
& "$UE/Engine/Binaries/Win64/UnrealEditor-Cmd.exe" "$PWD/LyraStarterGame.uproject" /MonsterArena/Maps/L_MonsterArena -game -MonsterArenaSmokeTest -unattended -NullRHI -nosound -SCCProvider=None -abslog="$PWD/Saved/Logs/MonsterArenaSmoke.log"
```

`-MonsterArenaSmokeTest` 覆盖实际步枪伤害效果、经验溢出、等级上限和显式选卡后的强化；升到 3 级时先检查有两次待选机会且属性未自动增加，再执行选择。它同时调用 `RunMonsterArenaRewardChoiceTests()`，无需额外启动参数。

选卡专项覆盖同轮去重、关闭重开不刷新、重复／伪造／旧对局请求不发奖、治疗和备用弹药、满血治疗过滤、叠层限制、空池兜底、保留其他系统的输入阻断，以及始终不暂停游戏。

| 启动参数 | 完成时的成功标记 |
| --- | --- |
| `-MonsterArenaSmokeTest` | `MONSTER_ARENA_SMOKE_SUCCEEDED`，其中选卡专项另输出 `MONSTER_ARENA_REWARD_CHOICES_SUCCEEDED` |
| `-MonsterArenaAmmoTest` | `MONSTER_ARENA_AMMO_SUCCEEDED` |
| `-MonsterArenaLifecycleTest` | `MONSTER_ARENA_LIFECYCLE_SUCCEEDED` |

将上述命令中的测试参数替换为对应项。每次单独启动一个进程，并使用不同日志文件。Runtime 测试结束后主动退出，成功退出码为 0，失败为 1；必须同时检查退出码、对应成功／失败标记和完整日志。

Runtime C++ 测试及调用入口受 `WITH_DEV_AUTOMATION_TESTS` 保护。当前配置下 Shipping 不包含这些测试逻辑；Development 中也需显式启动参数才运行。

## 运行 Editor 配置校验

```powershell
& "$UE/Engine/Binaries/Win64/UnrealEditor-Cmd.exe" "$PWD/LyraStarterGame.uproject" -run=pythonscript -script="$PWD/Plugins/GameFeatures/MonsterArena/Tests/Python/validate_monster_arena_editor.py" -unattended -NullRHI -nosound -SCCProvider=None -abslog="$PWD/Saved/Logs/MonsterArenaEditorModuleValidation.log"
```

成功输出 `MONSTER_ARENA_EDITOR_MODULE_VALIDATION_SUCCEEDED`。脚本使用临时对象，不修改已保存资源。资源生成工具 `Content/Python/setup_monster_arena.py` 继续放在工具目录。

## 运行双人选卡 PIE 回归

使用单独的、有渲染的编辑器进程运行；脚本完成后会退出该进程。先保存当前编辑器中的工作，再运行：

```powershell
& "$UE/Engine/Binaries/Win64/UnrealEditor.exe" "$PWD/LyraStarterGame.uproject" /MonsterArena/Maps/L_MonsterArena -ExecutePythonScript="$PWD/Plugins/GameFeatures/MonsterArena/Tests/Python/validate_reward_multiplayer.py" -unattended -nosplash -SCCProvider=None -abslog="$PWD/Saved/Logs/MonsterArenaRewardNetwork.log"
```

这个脚本通过 `MonsterArenaEditor` 的 `UMonsterArenaRewardTestLibrary` 在同一进程启动监听服务器与客户端两个 PIE World。它使用 Slate post-tick 回调等待网络复制，启动时调用 `unreal.EditorPythonScripting.set_keep_python_script_alive(True)` 保持异步脚本存活；不能将入口替换为同步 `-run=pythonscript` 命令行校验。

所有涉及 RPC 的操作经测试库恢复正常游戏调用规则后执行。不能从 Python 直接调用客户端的选卡／开关界面接口：Python 的编辑器脚本保护会把嵌套的 Server RPC 当作本地调用，形成错误的网络测试结果。测试库仅位于 Editor 模块，不参与游戏打包。

检查包括：升级只累计次数、owner-only 奖励状态、手动打开真实 CommonUI、服务端候选去重、世界时间继续前进、选卡期间正常受伤、关闭重开保留候选、通过客户端 RPC 确认、重复／无效请求、另一玩家状态隔离、死亡自动关闭、复活后手动恢复选择、最后一次领取关闭界面、输入锁清理，以及新对局拒绝旧请求。为了稳定验证网络和菜单状态，脚本暂时停止刷怪，伤害由正常 GAS 路径显式施加；它不是持续刷怪压力测试。

脚本输出：

- 日志标记：`MONSTER_ARENA_REWARD_NETWORK_SUCCEEDED` 或 `MONSTER_ARENA_REWARD_NETWORK_FAILED`。
- 结构化结果：`Saved/Logs/MonsterArenaRewardNetworkResult.json`，包含 `passed`、每项检查及异常信息。
- 整体超时为 180 秒；退出编辑器前请求结束测试 PIE。

该入口通过反射调用界面和选卡函数，不模拟物理鼠标、键盘或手柄。成功标记也不覆盖独立进程的专用服务器、断线重连或丢包压力场景。真实按键打开、按住射击时打开与关闭、焦点切换、手柄导航以及画面布局仍需交互验收。

## 交互界面预览

```powershell
& "$UE/Engine/Binaries/Win64/UnrealEditor.exe" "$PWD/LyraStarterGame.uproject" /MonsterArena/Maps/L_MonsterArena -ExecutePythonScript="$PWD/Plugins/GameFeatures/MonsterArena/Tests/Python/preview_reward_ui.py" -nosplash -SCCProvider=None -abslog="$PWD/Saved/Logs/MonsterArenaRewardUIPreview.log"
```

该入口异步启动一个有渲染的 PIE 玩家，等待角色、成长组件和 HUD 就绪后只增加一次 325 经验，得到三次待选机会，界面仍保持关闭。出现 `MONSTER_ARENA_REWARD_UI_PREVIEW_READY` 后，默认按 `U` 或手柄上方向键打开界面，检查排版、焦点、确认、关闭重开，以及按住开火时打开／关闭的行为。项目中的 `K` 是自毁调试快捷键，不作为默认选卡键。准备完成后卸载回调，编辑器和 PIE 持续运行；通过编辑器的正常停止入口结束预览。

为了给画面与输入验收保留稳定环境，脚本仅在当前 PIE World 临时禁用刷怪并移除已有怪物，不保存资源，也不赋予无敌。此入口不覆盖选卡期间的战斗危险性、怪物攻击或多人复制；这些行为应由上面的双人回归及正常战斗试玩另行验证。`READY` 只代表预览准备成功，不能作为交互验收通过标记。

## 日志与验收边界

以上内容说明测试入口和预期覆盖，不表示这些检查已经执行或通过。实际结果以 [ACCEPTANCE.md](ACCEPTANCE.md) 中对应版本和运行记录为准。

检查新一轮从启动到退出的完整日志，分别统计 Error/Fatal、断言／Ensure、蓝图异常和脚本警告。`-NullRHI` 中的 Niagara 特效空引用、独立 `-game` 中的实验性编辑器工具初始化问题不能被成功标记掩盖，也不能通过关闭日志消除。无界面测试和反射操作不能替代真实画面、输入及完整网络验收。
