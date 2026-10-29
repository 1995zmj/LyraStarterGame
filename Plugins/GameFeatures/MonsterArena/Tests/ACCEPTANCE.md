# MonsterArena 验收记录

日期：2026-10-29，时区 Asia/Shanghai。

当前结论：手动选卡功能已实现，编辑器编译、战斗／弹药／死亡复活回归及双人 PIE 的 44 项网络检查通过。默认键已改为 U，但真实 U 键打开、鼠标选卡、手柄导航及完整画面验收尚未完成；完整日志仍有启动错误和出生表现警告，不能标为完整验收通过。编译或单项自动测试通过不等于完整交付验收通过。

## 升级后手动选卡（2026-10-29）

升级只累计待选次数，由玩家手动打开 CommonUI 选卡；世界不暂停。服务端抽取和结算，关闭／死亡后保留候选，复活不自动开窗；永久效果保留，即时治疗和补弹不重复发放。代码、原生界面、卡池及测试均归属 MonsterArena。

| 检查 | 方法、证据及实际结果 |
| --- | --- |
| 编辑器目标编译 | `LyraEditor Win64 Development` 编译成功；U 键及 HUD 提示修改后再次编译成功 |
| 击杀、经验与选卡结算 | `Saved/Logs/MonsterArenaManualRewardsSmokeR2.log`：`MONSTER_ARENA_SMOKE_SUCCEEDED`、`MONSTER_ARENA_REWARD_CHOICES_SUCCEEDED`；覆盖关闭保留、无效／重复请求、卡池过滤与即时奖励去重 |
| 自动射击与弹药 | `Saved/Logs/MonsterArenaManualRewardsAmmo.log`：`MONSTER_ARENA_AMMO_SUCCEEDED`；包含扣弹、松开停火、空弹匣和换弹 |
| 两次真实死亡与复活 | `Saved/Logs/MonsterArenaManualRewardsLifecycleR3.log`：`MONSTER_ARENA_LIFECYCLE_SUCCEEDED`；第一次复活保持测试设定的 20 生命、不重发治疗，第二次正常复活维护后实际／预期均为 120 生命 |
| 双人有渲染 PIE | `Saved/Logs/MonsterArenaManualRewardsNetworkR8.log`：44 项 PASS、0 FAIL，`MONSTER_ARENA_REWARD_NETWORK_SUCCEEDED`；`MonsterArenaRewardNetworkResult.json` 的 `passed=true` |
| 网络覆盖范围 | 同进程监听服务器与客户端，验证 owner-only 状态、服务端发奖、关闭后快速重开、候选不刷新、重复／旧对局请求、选卡受伤、死亡关闭、复活继续领取及输入锁清理；测试暂时停止刷怪，由正常 GAS 路径施加伤害 |
| Runtime／Editor 边界 | UBT JsonExport 输出 `Saved/MonsterArenaRewardsGameModules.json`：游戏目标含 MonsterArenaRuntime，不含 MonsterArenaEditor、UnrealEd；没有执行完整打包 |
| 交互预览 | `MonsterArenaManualRewardsVisualR3.log` 的预览已准备，实际画面看到 U 键提示及三次待选机会，未自动弹窗；下一步真实按键操作被用户物理 Esc 停止，未将其记为选卡交互通过 |

网络验收通过 Editor 测试桥接恢复正常 RPC 调用规则。Python 编辑器执行保护会将直接嵌套的客户端 Server RPC 作为本地调用，因此早期直接调用界面的结果不能用作网络通过证据。测试也避免跨地图旅行持有临时 World 引用；失败轮次日志保留。

本轮修复了快速重开时旧 Slate 析构清理新会话、复活时最大生命效果移除造成血量夹断，以及动态近战 GameplayEffect 的网络引用警告。R8 的 `NetPackageMap`／`SupportsObject` 警告为 0。真实输入检查发现原有 K 键绑定 `DamageSelfDestruct`，默认选卡键改为 U；R8 使用正常界面函数和 RPC 入口，未验证物理 U 键。

完整 R8 日志从启动到关闭仍有 **1 条 Error、11 条 Warning**：

- PixelStreaming2RTC 启动 Error：`Cannot set target window - GEngine is not valid`。
- 音频设备采样率转换 Warning 6 条，MCP 插件许可提示 Warning 1 条。
- `GA_SpawnEffect` 两次出生播放 `AM_MM_Pistol_Spawn` 失败，共 4 条 Warning；此项属于出生表现初始化问题，尚未修复，不能归类为外部服务错误。
- Fatal、Ensure、断言、LogScript 警告、Python Traceback 及蓝图空引用均为 0。

独立 `-NullRHI -nosound -game` 回归还保留实验性工具初始化 Error、Niagara／倒计时音频空引用警告；功能成功标记不代表这些完整日志无报错。没有删除历史日志或修改日志等级。

未完成：U 键和鼠标的完整选卡流程、按住开火时开关界面、焦点切换、手柄实机导航、完整界面截图验收；独立进程专用服务器、丢包／高延迟、断线重连和正式打包未验证。本次提交不包含 CSV／xlsx 数据导入功能，该部分目前仅讨论了方案。

## 测试目录整理（2026-10-29）

- 三个 C++ 测试实现移至 `Source/MonsterArenaRuntime/Private/Tests/`，测试逻辑、开发测试宏和命令行触发条件不变。`LyraEditor Win64 Development` 重新编译三个源文件并链接成功。
- 编辑器校验脚本移至 `Tests/Python/validate_monster_arena_editor.py`，验收记录移至本目录，新增 `Tests/README.md` 说明目录与运行命令。资源生成脚本保留在 `Content/Python/`。
- 从新路径实际执行 Python 校验，进程退出码 0，日志 `Saved/Logs/MonsterArenaTestsDirectoryValidation.log` 包含 `MONSTER_ARENA_EDITOR_MODULE_VALIDATION_SUCCEEDED`；完整日志 Error/Fatal、Warning、断言和蓝图异常均为 0。
- 实际运行移动后的战斗测试入口，进程退出码 0，日志 `Saved/Logs/MonsterArenaTestsDirectorySmoke.log` 包含 `MONSTER_ARENA_SMOKE_SUCCEEDED`。完整日志仍有 71 条 Error、325 条 Warning，Fatal、断言及 `Blueprint Runtime Error` / `Accessed None` 均为 0；功能检查成功不能视为日志清洁通过。
- 与此前 `MonsterArenaScopeCombat.log` 对比，Error/Warning 的类别及各类别数量一致：AutomationTest Error 13、Python Error 58、EditorDataStorageUI Warning 21、TextLocalizationResource Warning 297、Sockets Warning 3、无渲染特效 Script Warning 4。保留这些既有问题和原始日志，不作日志清洁通过结论。
- 本轮未重复执行弹药、重生、人工画面输入及联网验收。编译前已保存编辑器资产，完成后重新启动原 MonsterArena 地图。

## Editor 模块边界调整（2026-10-29）

`UMonsterArenaContentLibrary` 已从 `MonsterArenaRuntime` 移至 `MonsterArenaEditor`，API 宏改为 `MONSTERARENAEDITOR_API`。插件将新模块声明为 `Type: Editor`，资源脚本显式加载 Editor 模块。Runtime 不包含该工具类，也不反向依赖 Editor。

- `LyraEditor Win64 Development` 编译成功。
- `validate_monster_arena_editor.py` 实际验证类路径为 `/Script/MonsterArenaEditor.MonsterArenaContentLibrary`、旧 Runtime 类路径不存在，并在临时 Experience 对象上调用配置函数。日志 `MonsterArenaEditorModuleValidation.log` 输出 `MONSTER_ARENA_EDITOR_MODULE_VALIDATION_SUCCEEDED`；执行阶段汇总 0 Error、0 Warning。
- 更新后的 `setup_monster_arena.py` 实际配置与保存资源成功，日志 `MonsterArenaEditorSetupValidation.log` 汇总 0 Error、0 Warning。
- UBT 对 `LyraGame Win64 Development` 的 JsonExport 结果保存为 `Saved/MonsterArenaGameModules.json`。581 个目标模块中包含 `MonsterArenaRuntime`，不包含 `MonsterArenaEditor`、`UnrealEd`；Runtime 的直接依赖也无 Editor 模块。这是模块图检查，没有执行完整游戏打包构建。
- 本轮只调整编辑器工具归属，没有重复执行全部战斗或人工输入验收。此前尚未完成的项目不因本次检查而变成已通过。

## 专用资源迁移回归（2026-10-29）

专用资源均位于 `/MonsterArena/`。检查的 8 个资源（播报、两种准星、两种物品、角色子类、PawnData、组件集）均没有其他插件的反向引用。共享 ShooterCore 的三张蓝图中仅移除了此前添加的 4 个 MonsterArena 有效性检查节点，保留其他原有或用户修改的逻辑。共享播报注册从 ShooterCore GameFeatureData 移至共享射击组件集；共享组件集没有 `/MonsterArena/` 引用。

角色使用 `B_MonsterArenaShooterHero`，继承 `B_Hero_ShooterMannequin`，覆盖本地初始物品。迁移中曾使用同原生父类的角色副本，导致共享武器 `GetTypedPawn` 在死亡及卸载时转换失败；该副本已经备份并删除，现用子类保留共享武器类型契约。

| 检查 | 结果与证据 |
| --- | --- |
| 最终 C++ 编译 | LyraEditor Win64 Development 成功 |
| 资源配置脚本 | MonsterArenaScopeSetup.log：MONSTER_ARENA_SETUP_SUCCEEDED；命令行汇总 0 Error、0 Warning |
| 实际使用本地资源 | MonsterArenaScopeCombat.log：本地角色、共享角色兼容性、唯一的本地播报组件检查通过；原共享播报未注入 MonsterArena |
| 战斗与成长 | MonsterArenaScopeCombat.log：MONSTER_ARENA_SMOKE_SUCCEEDED，退出码 0 |
| 本地物品与弹药 | MonsterArenaScopeAmmo.log：MONSTER_ARENA_AMMO_SUCCEEDED，退出码 0，额外验证本地物品定义 |
| 两次怪物击杀与自动复活 | MonsterArenaScopeLifecycle.log：MONSTER_ARENA_LIFECYCLE_SUCCEEDED，退出码 0；等级、生命与伤害奖励和武器物品均保留 |
| 有渲染 MonsterArena PIE | 10:46:54 至 10:47:38，进入、运行、退出：新增 Error/Fatal 0、LogScript Warning 0、断言/Ensure 0、蓝图异常 0 |
| 原 ShooterCore 玩法 | L_Expanse 的 B_ShooterGame_Elimination 已进入 PIE，GameState 有原 B_EliminationFeedRelay_C，没有本地播报；启动至退出新增 Error/Fatal 0、脚本警告 0、断言 0 |
| 工作区恢复 | 已停止测试，重新打开 L_MonsterArena 编辑状态 |

无渲染命令行测试中，原有死亡/出生 GameplayCue 无法创建 Niagara 组件，仍有特效空引用 Warning；其功能检查结果不能充当日志清洁验收。上表中的日志清洁结论来自随后有渲染的两轮 PIE，并未修改或屏蔽日志。独立 `-game` 初始化还保留实验性编辑器工具的 Python 初始化错误。

迁移过程中的失败日志也保留，包括角色副本类型错误、配置脚本初次失败，以及在 PIE 中调用仅支持编辑态的资源依赖查询。资源依赖检查已停止 PIE 后重新执行；最终 PIE 回归不再调用这些编辑态查询。最终结论仅覆盖上表明确的运行区间，不能称整个编辑器历史日志没有错误。

下面保留迁移前的验收记录，供追踪。

| 项目 | 方法与证据 | 结果 |
| --- | --- | --- |
| C++ 编辑器目标编译 | 最近一次 LyraEditor Win64 Development 构建成功 | 通过 |
| 弹药、停火、换弹 | 原生测试按实际 FireAuto、Reload 输入标签驱动能力，包含 8 项检查；Saved/Logs/MonsterArenaAmmo.log 中 MONSTER_ARENA_AMMO_SUCCEEDED，进程退出码 0 | 通过；独立无界面对局 |
| 击杀、经验、升级奖励 | Saved/Logs/MonsterArenaSmoke.log 中 MONSTER_ARENA_SMOKE_SUCCEEDED，进程退出码 0 | 通过；直接验证实际枪械伤害效果和成长逻辑 |
| PIE 进入 MonsterArena | 10:02:43 开始，正确加载 B_MonsterArena 及对应 Game Feature | 通过 |
| 怪物击杀玩家、自动复活 | 运行时检查观察到角色实例从 C_1 更换为 C_6；复活后仍有步枪开火、换弹能力，未观察到重复步枪能力 | 通过上述运行时状态检查 |
| PIE 退出清理 | 本轮已正常结束，日志包含 BeginTearingDown | 通过 |
| 本轮新增 Error/Fatal、断言、蓝图异常 | 从本轮 Creating play world package 起检查到退出：Error/Fatal 0、断言/Ensure 0、Blueprint Runtime Error/Accessed None 0 | 通过本轮统计范围 |
| 实际画面、鼠标持续射击、HUD 弹药同步、键盘换弹 | 电脑操作工具返回 Computer Use was not approved to use unrealeditor，未取得实际界面验收证据 | 未完成 |
| 双客户端同步 | 本轮未运行 | 未验证 |

## 仍需区分和处理的日志

- 当前编辑器会话启动阶段仍留有 PixelStreaming2RTC 的窗口初始化 Error；它在本轮 PIE 开始之前。关闭 PIE 自动串流解决了每次进入 PIE 时触发的串流问题，不代表编辑器启动阶段的该条错误也已经消失。
- EOS／编辑器遥测通过本地代理连接时存在 TLS Warning；音频设备存在采样率转换 Warning。没有通过修改日志等级隐藏这些记录。
- 本轮运行时工具读取 QuickBar 私有字段失败，产生一条 GetObjectProperties 的 LogScript Warning。该查询未成功，不把它记为快捷栏字段验收通过。复活后的能力状态来自成功的 GAS 检查。
- MCP 会话重新初始化曾在本轮 PIE 开始前留下 Unknown session id Error。它属于工具连接记录，不能宣称整个编辑器会话 Error 为零。

只有补足实际画面与输入验收，并明确处理仍未解决的问题后，才可更新完整验收结论。上述记录不替代玩家对实际体验的验收。
