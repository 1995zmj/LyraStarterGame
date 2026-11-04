# MonsterArena 测试

`Python/validate_base_health_hud.py` 通过编辑器 `-ExecutePythonScript` 入口运行，两轮双人实际 PIE 校验大本营血量在 HUD 首行的初始值、受伤、自定义上限复制、归零、重置与重进；成功标记 `MONSTER_ARENA_BASE_HEALTH_HUD_PASSED`，截图输出到 `Saved/Screenshots/WindowsEditor/`。

## 统一 CSV 管理验收

`Python/validate_csv_manager.py` 覆盖注册清单、分组与依赖、未注册文件、全组预校验、源文件变更、部分写入失败报告、重复真实导入和旧入口兼容。在项目根目录执行（`$UE` 为引擎目录）：

```powershell
& "$UE/Engine/Binaries/Win64/UnrealEditor-Cmd.exe" "$PWD/LyraStarterGame.uproject" -run=pythonscript -script="$PWD/Plugins/GameFeatures/MonsterArena/Tests/Python/validate_csv_manager.py" -unattended -NullRHI -nosound -SCCProvider=None -abslog="$PWD/Saved/Logs/MonsterArenaCSVManager.log"
```

成功标记为 `MONSTER_ARENA_CSV_MANAGER_SUCCEEDED: 17 checks`。重复执行使用新的日志文件名，保留失败记录。

交互检查使用 `UnrealEditor.exe` 和 `-ExecutePythonScript=".../Tests/Python/preview_csv_manager.py"`：脚本验证 PIE 期间拒绝导入，停止 PIE 后保留编辑器，供真实点击 Tools → MonsterArena CSV 配置。脚本调用与真实菜单输入分别记录，报告位于 `Saved/ConfigImports/MonsterArena/`。

测试内容归属 MonsterArena 插件，目录如下：

| 位置 | 用途 |
| --- | --- |
| `../Source/MonsterArenaRuntime/Private/Tests/MonsterArenaSmokeTest.cpp` | 击杀、经验、显式选卡、升级奖励和基础资源回归；调用选卡专项测试 |
| `../Source/MonsterArenaRuntime/Private/Tests/MonsterArenaRewardChoiceTest.cpp` | 待选次数、关闭重开、候选去重、即时效果、无效请求、卡池限制和输入锁回归 |
| `../Source/MonsterArenaRuntime/Private/Tests/MonsterArenaRewardTestHelpers.h` | 临时卡池和显式选卡的测试辅助代码 |
| `../Source/MonsterArenaRuntime/Private/Tests/MonsterArenaAmmoTest.cpp` | 自动射击、松开停火、空弹匣和换弹回归 |
| `../Source/MonsterArenaRuntime/Private/Tests/MonsterArenaLifecycleTest.cpp` | 真实怪物击杀与自动复活，检查已选永久奖励、未选候选和即时奖励不重复发放 |
| `../Source/MonsterArenaEditor/Private/Tests/MonsterArenaRewardTestLibrary.h/.cpp` | 启停真实 PIE、枚举 PIE World、施加正常 GAS 伤害、查询输入锁和伤害免疫状态的 Editor 测试桥接 |
| `../Source/MonsterArenaEditor/Private/Tests/MonsterArenaAITestLibrary.h/.cpp` | StateTree 测试用怪物生成、活动状态与目标查询、视线遮挡体；仅存在于 Editor 模块 |
| `Python/validate_monster_statetree.py` | 双人实际 StateTree 追逐、近战、冷却、遮挡、目标切换、服务端执行／客户端复制、死亡复活和重进验收 |
| `Python/validate_monster_arena_editor.py` | Editor 模块和 Python 配置接口校验 |
| `Python/validate_monster_arena_hud.py` | 保存的 HUD Widget Blueprint 父类、必要控件绑定与编译校验；命令行用法与 Editor 配置校验相同 |
| `Python/validate_reward_screen_blueprint.py` | 选卡页、独立卡牌、效果行的 Widget Blueprint 父类和必要控件绑定、编译及 HUD 默认类绑定检查 |
| `Python/validate_monster_arena_input.py` | 输入动作父类、Value Type、两个玩家映射名、IMC 默认映射，以及 `InputData_MonsterArena` 的选卡／波次 InputTag、`AbilitySet_MonsterArena` 的两个技能授予、动作集输入项和 Experience 技能集绑定检查 |
| `Python/validate_csv_configuration.py` | CSV 重复导入、每条行为资产值、配置引用、等级断档／负经验／重复 ID／未知行为／悬空卡牌引用／空行为／非法数值和合计越界校验；使用 Editor 配置校验的命令行入口替换脚本路径 |
| `Python/validate_reward_multiplayer.py` | 两名玩家的监听服务器／客户端 PIE 选卡、复制和界面生命周期回归 |
| `Python/reward_card_view_cases.py` | 双人入口调用：实际卡牌与效果行蓝图实例、候选快照、七条行为合并成六行、比例格式、重复配置与只显示不发奖检查 |
| `Python/kill_accumulation_cases.py` | 由双人回归入口调用：使用导入的火力积累卡验证怪物／玩家击杀、重复通知、叠卡、队友／自杀排除、复活、复制和新对局清零 |
| `Python/preview_reward_ui.py` | 保持编辑器运行的单人选卡界面预览，供真实鼠标、键盘、手柄和画面验收 |
| `Python/preview_monster_ai.py` | 保留一个正常怪物的单人战斗预览，供真实射击／追逐画面验收；不赋予无敌、不自动开火 |
| [ACCEPTANCE.md](ACCEPTANCE.md) | 实际验收证据、结果及未完成项 |

## 运行 Runtime 玩法测试

先编译 `LyraEditor Win64 Development`。以下命令均从项目根目录执行，`$UE` 为引擎安装目录：

```powershell
& "$UE/Engine/Binaries/Win64/UnrealEditor-Cmd.exe" "$PWD/LyraStarterGame.uproject" /MonsterArena/Maps/L_MonsterArena -game -MonsterArenaSmokeTest -unattended -NullRHI -nosound -SCCProvider=None -abslog="$PWD/Saved/Logs/MonsterArenaSmoke.log"
```

`-MonsterArenaSmokeTest` 覆盖实际步枪伤害效果、经验溢出、等级上限和显式选卡后的强化；升到 3 级时先检查有两次待选机会且属性未自动增加，再执行选择。它同时调用 `RunMonsterArenaRewardChoiceTests()`，无需额外启动参数。

选卡专项覆盖同轮去重、关闭重开不刷新、重复／伪造／旧对局请求不发奖、治疗和备用弹药、满血治疗过滤、叠层限制、空池兜底、保留其他系统的输入阻断，以及始终不暂停游戏。

行为列表回归还覆盖单卡六条行为（含重复伤害）、持续效果先于治疗结算、治疗使用新生命上限、补弹不重放、未知行为拒绝以及重复弹药的精度越界。双人 PIE 比较服务端和客户端的完整候选行为与已领取行为列表。

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

输入资产校验使用同一入口，替换脚本路径：

```powershell
& "$UE/Engine/Binaries/Win64/UnrealEditor-Cmd.exe" "$PWD/LyraStarterGame.uproject" -run=pythonscript -script="$PWD/Plugins/GameFeatures/MonsterArena/Tests/Python/validate_monster_arena_input.py" -unattended -NullRHI -nosound -SCCProvider=None -abslog="$PWD/Saved/Logs/MonsterArenaInputValidation.log"
```

成功输出 `MONSTER_ARENA_INPUT_VALIDATION_SUCCEEDED`。重新生成输入资产使用 `Content/Python/setup_monster_arena_input.py`，成功输出 `MONSTER_ARENA_INPUT_SETUP_SUCCEEDED`。

## 运行 StateTree AI 验收

先执行 `Content/Python/setup_monster_arena_ai.py` 生成资产；使用上面的 Editor 配置校验命令替换脚本路径即可。该工具保存资源，运行前须保存工作并关闭使用该资产的其他编辑器进程，不能和 PIE 进程同时写同一资产。完成后运行：

```powershell
& "$UE/Engine/Binaries/Win64/UnrealEditor.exe" "$PWD/LyraStarterGame.uproject" /MonsterArena/Maps/L_MonsterArena -ExecutePythonScript="$PWD/Plugins/GameFeatures/MonsterArena/Tests/Python/validate_monster_statetree.py" -unattended -nosplash -SCCProvider=None -abslog="$PWD/Saved/Logs/MonsterArenaStateTreeNetwork.log"
```

必须带竞技场地图参数。Lyra 前端地图的 `ForceStandaloneNetMode` 会覆盖测试请求的网络模式，造成两个独立单机窗口，不能作为多人测试。脚本等待监听服务器和客户端都出现两个玩家后才开始断言；整体超时 240 秒，结束时正常关闭两个 PIE 会话和编辑器。

检查实际 StateTree 活动状态、追逐位移和客户端复制、客户端不运行 AI、GAS 近战数值／冷却／遮挡、最近存活目标切换、无目标 Idle、目标死亡与复活、怪物死亡停止树及一次击杀结算、复制销毁和重新进入对局。测试设置临时位置和遮挡体，部分条件检查时只冻结移动、保留树运行；不保存这些世界改动。成功标记为 `MONSTER_ARENA_STATETREE_SUCCEEDED`，结构化结果在 `Saved/Logs/MonsterArenaStateTreeResult.json`。这些反射检查不替代真实战斗输入及画面验收。

将入口替换为 `Tests/Python/preview_monster_ai.py` 可启动单人交互预览。该入口只在临时 PIE 世界停止自动补怪，移除已有怪物并在玩家正前方生成一个正常怪物；保留默认生命、伤害、移动和 StateTree，不授予经验、不自动射击、不暂停世界。看到 `MONSTER_ARENA_AI_PREVIEW_READY` 后用真实输入检查移动、追逐、受伤及步枪击杀和经验。预览保持运行，需通过正常编辑器操作退出；仅表示准备就绪，不代表验收通过。

可选 `-MonsterArenaFixedAimPreview` 会在临时世界持续调整相机瞄准该怪物，直到怪物移除或 PIE 结束；仍需实际点击才能开火。它用于隔离检查实弹命中、死亡与经验结算，不能作为正常手动瞄准或鼠标转向验收。此选项只属于测试脚本，正式玩法不包含该功能。

## 运行双人选卡 PIE 回归

使用单独的、有渲染的编辑器进程运行；脚本完成后会退出该进程。先保存当前编辑器中的工作，再运行：

```powershell
& "$UE/Engine/Binaries/Win64/UnrealEditor.exe" "$PWD/LyraStarterGame.uproject" /MonsterArena/Maps/L_MonsterArena -ExecutePythonScript="$PWD/Plugins/GameFeatures/MonsterArena/Tests/Python/validate_reward_multiplayer.py" -unattended -nosplash -SCCProvider=None -abslog="$PWD/Saved/Logs/MonsterArenaRewardNetwork.log"
```

这个脚本通过 `MonsterArenaEditor` 的 `UMonsterArenaRewardTestLibrary` 在同一进程启动监听服务器与客户端两个 PIE World。它使用 Slate post-tick 回调等待网络复制，启动时调用 `unreal.EditorPythonScripting.set_keep_python_script_alive(True)` 保持异步脚本存活；不能将入口替换为同步 `-run=pythonscript` 命令行校验。

所有涉及 RPC 的操作经测试库恢复正常游戏调用规则后执行。不能从 Python 直接调用客户端的选卡／开关界面接口：Python 的编辑器脚本保护会把嵌套的 Server RPC 当作本地调用，形成错误的网络测试结果。测试库仅位于 Editor 模块，不参与游戏打包。

检查包括：升级只累计次数、owner-only 奖励状态、手动打开真实 CommonUI、本地玩家已注册 `IMC_MonsterArena` 且 `U` 映射到打开动作、已获得输入驱动的选卡技能、`N` 映射到波次动作、已获得输入驱动的波次技能、两个绑定均进入玩家可映射设置档案、服务端候选去重、世界时间继续前进、选卡期间正常受伤、关闭重开保留候选、通过客户端 RPC 确认、重复／无效请求、另一玩家状态隔离、死亡自动关闭、复活后手动恢复选择、最后一次领取关闭界面、输入锁清理，以及新对局拒绝旧请求。为了稳定验证网络和菜单状态，脚本暂时停止刷怪，伤害由正常 GAS 路径显式施加；它不是持续刷怪压力测试。

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

追加 `-MonsterArenaAccumulationPreview` 时，仅在临时 PIE 配置中把卡池限制为 CSV 导入的“火力积累”，用于实际键鼠验证叠卡预览和 HUD 收益。不会改动已保存资产。多人测试修改队伍时只允许已注册的 1／2 红蓝队，测试桥接拒绝不存在的队伍，避免非法前置条件触发姓名板空材质。

为了给画面与输入验收保留稳定环境，脚本仅在当前 PIE World 临时禁用刷怪并移除已有怪物，不保存资源，也不赋予无敌。此入口不覆盖选卡期间的战斗危险性、怪物攻击或多人复制；这些行为应由上面的双人回归及正常战斗试玩另行验证。`READY` 只代表预览准备成功，不能作为交互验收通过标记。

## 日志与验收边界

以上内容说明测试入口和预期覆盖，不表示这些检查已经执行或通过。实际结果以 [ACCEPTANCE.md](ACCEPTANCE.md) 中对应版本和运行记录为准。

检查新一轮从启动到退出的完整日志，分别统计 Error/Fatal、断言／Ensure、蓝图异常和脚本警告。`-NullRHI` 中的 Niagara 特效空引用、独立 `-game` 中的实验性编辑器工具初始化问题不能被成功标记掩盖，也不能通过关闭日志消除。无界面测试和反射操作不能替代真实画面、输入及完整网络验收。

## 守点波次验收

`Python/validate_monster_waves.py` 使用有渲染双人 PIE。沿用上面的 `-ExecutePythonScript` 命令，将脚本替换为该路径，输出独立 `MonsterArenaWavesNetwork*.log`。测试临时把每波设为 3 个 Grunt（进攻）＋1 个 Runner（猎手）、同时最多 2 只，以覆盖五波状态转换和同波混合类型；不保存测试配置，不代表默认 152 只敌人的完整节奏／压力验收。

覆盖客户端发起、距离及旧编号拒绝、重复启动、数量上限、销毁补位、同波混合怪物、配置数值生效、五波清场、胜负复制、重置成长、受阻重试、双人死亡期间继续生成、真实 AI 攻击核心与离局重进。Editor 测试桥接位于 `Private/Tests/MonsterArenaWaveTestLibrary`；结果为 `Saved/Logs/MonsterArenaWavesResult.json`，重复运行前保留旧结果。成功标记 `MONSTER_ARENA_WAVES_SUCCEEDED`。

`Python/preview_monster_waves.py` 启动单人预览，把玩家放到核心附近并摆好相机；不启动波次、不自动射击、不授予无敌。真实 N／手柄→ 启动，U 选卡。必须通过实际输入和画面另行验收。

`Python/validate_wave_csv.py` 用 `-run=pythonscript -script=...` 运行，检查四张波次 CSV → DA 的波次、怪物目录、每波类型选择全字段、重复导入、默认组件绑定，以及 30 组非法输入不修改已有资产。成功标记为 `MONSTER_ARENA_WAVES_CSV_VALIDATION_SUCCEEDED`。
