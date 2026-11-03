# MonsterArena

CSV 配置统一通过编辑器 **Tools → MonsterArena CSV 配置** 管理，支持全量／分组校验和导入。注册清单为 `Data/ConfigRegistry.json`，当前接管成长奖励与守点波次共 5 张表；新增配置组的接入方法见 [配置说明](Data/README.md)。

守点大本营血量由独立子控件 `W_MonsterArenaCoreHealth` 显示，HUD 蓝图在设计器中通过命名槽 `CoreHealthSlot` 持有该子控件；关系与 `W_TopDownArenaHUDLayout` 持有其子控件一致，不使用运行时 `CreateWidget`。子控件显示“大本营血量：当前 / 上限”和血条；核心引用尚未同步时显示“等待同步”并将血条置零。当前血量与上限均由服务端复制，受伤、摧毁和重置后使用权威值刷新。HUD 的 `Status` 只保留等级、波次和提示信息，不再拼接血量文本。

界面开发遵循仓库 [界面制作规范](../../../AGENTS.md#界面制作规范)，涵盖目录归属、C++／蓝图职责、子控件组合、数据事件、资源生成迁移与运行验收。以下为本玩法的具体绑定约定。

HUD 的可编辑界面位于 `/MonsterArena/UI/W_MonsterArenaHUD`，父类为 `UMonsterArenaHUD`。在 Widget Blueprint Designer 调整布局、字体和颜色；保留 `Status`（TextBlock）、`ExperienceBar`（ProgressBar）、`Reward`（TextBlock）、`CoreHealthSlot`（NamedSlot）及其内部 `CoreHealthDisplay`（`W_MonsterArenaCoreHealth`）实例。`CoreHealthDisplay` 由设计器 Add Widget 放入命名槽，C++ 只通过 `BindWidget` 绑定并更新数据，不创建或重新挂载子控件。`Public/UI/MonsterArenaHUD.h` 声明 HUD，`Private/UI/MonsterArenaHUD.cpp` 更新成长与血量快照，`Private/UI/MonsterArenaHUDInput.cpp` 管理选卡输入、弹窗与输入锁。

子控件 `/MonsterArena/UI/W_MonsterArenaCoreHealth` 的父类为 `UMonsterArenaCoreHealthWidget`，保留 `HealthText`（TextBlock）和 `HealthBar`（ProgressBar）供 `BindWidget` 绑定；它只接收 HUD 传入的权威血量快照，不自行查找玩家或业务数据。`Public/UI/MonsterArenaCoreHealthWidget.h` 与 `Private/UI/MonsterArenaCoreHealthWidget.cpp` 负责格式化和血条比例。刷怪组件通过 `HUDClass` 引用 HUD 蓝图类，默认使用该界面。

资源生成入口为 `Content/Python/setup_monster_arena_hud.py`，它会先调用 `setup_monster_arena_core_health.py` 创建血量子控件，再创建 HUD；两个脚本都只在资源不存在时创建布局，已有设计器调整不会被重置。对于已存在的 HUD，生成器只补建缺失的 `CoreHealthSlot` 和其中的 `CoreHealthDisplay` 实例，保留其余布局。创建和校验代码位于 MonsterArenaEditor 的 `Public/UI`、`Private/UI` 下的 `MonsterArenaHUDLibrary` 与 `MonsterArenaCoreHealthWidgetLibrary`。完整资源初始化脚本也会调用该入口。UI 源码统一放在各模块的 `Public/UI` 和 `Private/UI`，UI 资源放在 `Content/UI`；测试继续放在模块 `Private/Tests` 与插件 `Tests`。

选卡界面使用三级组合：`W_MonsterArenaRewardScreen` → `W_MonsterArenaRewardCard` → `W_MonsterArenaRewardStat`。页面保留 `Card1..3` 三个独立卡牌实例，以及 `PendingText`、`FeedbackText`、`ConfirmButton`、`CloseButton`；单卡保留 `SelectButton`、`CardBorder`、`ChoiceNumber`、`CardTitle`、`CardDescription`、`CardStack`、`EffectsBox`；效果行保留 `EffectName`、`EffectValue`、`EffectPreview`。这些名字和类型供 native BindWidget 绑定，布局、字体和样式由各自蓝图设计器控制。

页面通过 Card.Configure 传入候选快照，卡牌为每种行为生成效果行，同类行为合计；Card.OnSelected 仅通知页面选择索引，服务端确认仍由页面的原有流程处理。卡牌／效果行不发放奖励，也不持有旧对局的成长组件引用。选中／未选中底色位于卡牌蓝图 Class Defaults 的 MonsterArena → Style。生成入口 `Content/Python/setup_monster_arena_reward_screen.py` 依次创建效果行、卡牌与页面，保留已有子蓝图编辑；对于旧页面，仅替换旧的三张内嵌卡牌并保留页面其他布局。生成和迁移代码仅位于 Editor/UI。

步枪击杀怪物 → 获得局内经验 → 升级积攒选卡次数 → 玩家手动打开三选一 → 确认后获得强化。

## 运行与操作

编译 `LyraEditor Win64 Development` 后重新打开编辑器。在内容浏览器启用插件内容，打开 `/MonsterArena/Maps/L_MonsterArena` 并运行 PIE。该地图使用 `/MonsterArena/System/Experiences/B_MonsterArena`，启用 ShooterCore 和 MonsterArena。玩法入口资源为 `/MonsterArena/System/Playlists/DA_MonsterArena`。

每个新角色自动装备步枪。怪物由独立波次导演从四个 `MonsterSpawn` 标签出生点分批生成。靠近圆柱核心（450 单位内），按 `N` 或手柄右方向键开始；预警 3 秒后出怪。每波清场后进入整备，手动启动下一波，第五波清场胜利。核心 1000 生命，被摧毁即失败并清理待生成队列和活怪。胜负后存活玩家可按同一按键重置对局；复活期间可等复活后操作。怪物生命 60、移动速度 220，140 单位内每秒近战造成 10 点伤害。AI 由 `/MonsterArena/AI/ST_ArenaMonster` 驱动，怪物 Actor Tick 已禁用。波次怪物沿导航路径移动；进攻兵优先核心、猎手优先玩家，无存活玩家时都进攻核心。

## 怪物 StateTree

`AArenaMonster` 的 `StateTreeComponent` 仅在服务端完成生命组件初始化后启动，客户端只接收角色移动和生命复制。`StateTreeAsset` 可配置使用的树，默认指向 `ST_ArenaMonster`；感知半径、近战距离、攻击间隔分别由 `DetectionRadius`、`MeleeRange`、`AttackInterval` 配置。

树中包含三个状态，条件和转换可在 StateTree 编辑器中调整：

| 状态 | 进入条件与执行 | 转换 |
| --- | --- | --- |
| `Melee` | 优先选择，目标存活且进入近战范围；调用受冷却和视线遮挡校验的 GAS 伤害接口 | 目标无效或离开近战范围时回到 Root 重选 |
| `Chase` | 存在有效目标；波次怪物沿导航路径移动，孤立旧测试怪保持直线追逐 | 目标无效或进入近战范围时回到 Root 重选 |
| `Idle` | 没有可选择的战斗状态；停止残留移动 | 获得有效目标后回到 Root 重选 |

全局 `Find nearest living player` Evaluator 默认每 0.2 秒寻找感知范围内最近的存活玩家，再应用波次怪物的核心／玩家优先级及目标保持时限；目标死亡或离开感知范围时提前重新搜索。状态选择由资产中的 Enter Conditions 和 Transitions 决定，不再由怪物本体的 Update／Tick 判断。StateTree 自身仍会更新条件和任务；追逐任务的 Tick 负责执行移动，近战任务调用伤害接口，不包含整套 AI 状态分支。攻击冷却在切换目标或反复进出近战范围时不会重置。怪物死亡及 EndPlay 停止树并清理目标，原有经验／击杀结算不变。

节点代码位于 Runtime 的 `Public/AI`、`Private/AI/MonsterArenaStateTreeNodes`。Editor 专用 `MonsterArenaStateTreeLibrary` 和 `Content/Python/setup_monster_arena_ai.py` 创建并编译资产；完整初始化脚本也会调用。已存在的树保留设计器修改，生成器不重建覆盖。运行生成脚本前结束其他进程对该资产的使用，避免 Windows 文件占用导致保存失败。

每只怪物给予击杀玩家 20 XP。初始 1 级，经验门槛依次为 60、100、140、180、220、260、300、340、380，最高 10 级。经验溢出保留，可连续升级；每升一级增加一次选卡机会。达到等级上限不会清除尚未使用的机会。

- 升级只更新 HUD 的“可选强化 ×N”提示，不自动打开界面，也不自动增加属性。
- 默认按 `U` 或手柄上方向键手动打开强化界面。输入动作的稳定映射名为 `MonsterArena.OpenRewards`，提示读取实际映射按键。默认使用 `U` 避开项目中 `K` 的自毁调试快捷键。
- 点击一张卡，再点击“确认强化”；键盘 `1` / `2` / `3` 可选中卡牌，`Enter` 确认。手柄可通过界面焦点导航选择和确认。
- `Esc`、手柄返回键、键盘打开界面的映射按键或“返回战斗”按钮均可关闭。手柄方向键在界面内用于导航。未选次数和同一组候选保留，反复开关不能刷新卡牌。
- 有剩余次数时，确认一张后在同一个窗口展示下一组；全部选完自动关闭。没有自动领取倒计时。
- 单人和多人都不暂停世界。选卡玩家仍会受到正常伤害，界面不提供无敌；打开时停止自身射击并切换为菜单输入，关闭后恢复操作，开火需重新按下。

## 默认卡池与配置

当前玩法已绑定由 CSV 导入的配置资产。修改 `Data/Levels.csv` 和 `Data/RewardCards.csv` 后运行导入器；字段与操作步骤见 [CSV 配置说明](Data/README.md)。

| 卡牌 ID | 名称 | 效果 |
| --- | --- | --- |
| `Firepower` | 火力强化 | 本局基础伤害 +3 |
| `Vitality` | 体魄强化 | 本局最大生命 +20，立即恢复 20 生命 |
| `Balance` | 均衡成长 | 本局基础伤害 +1、最大生命 +10，立即恢复 10 生命 |
| `FieldMedicine` | 战地医疗 | 立即恢复最大生命值的 40% |
| `AmmoSupply` | 弹药补给 | 本玩法步枪备用弹药 +60，仍需正常换弹 |

默认卡池等权抽取，同轮不重复，跨轮可再次选择。满血时纯治疗卡不进入候选；没有本玩法步枪时不提供补弹卡。永久强化随本局保留，即时治疗和弹药只在首次确认时发放。

`URunProgressionDefinition` 配置经验表和可选的 `RewardPool`。创建 `URunRewardPoolDefinition` DataAsset 可替换原生默认池，其 `Cards` 数组每项包含：

- `Option`：稳定 `CardId`、标题、说明、最大生命与伤害加成、固定／比例治疗、备用弹药数量。
- `Weight`：抽取权重，0 表示不参与抽取。
- `MaxStacks`：本局最多可选择次数，0 表示不限次数。
- `MinimumLevel`：这次升级奖励对应的最低等级；积攒次数后再打开不会把早期奖励变成高等级奖励。

将该卡池赋给成长定义的 `RewardPool`，再把成长定义配置给 `URunProgressionComponent::Definition`；为空时使用原生五张卡。候选不足三张时显示实际数量；卡池为空、无有效卡或全部达到上限时，提供可重复选择的基础火力强化，避免已获得的次数无法领取。卡牌说明应与配置的效果数值保持一致。

## 状态与生命周期

`URunProgressionComponent` 位于 PlayerState 上。服务端负责经验、抽取候选、资格校验和发奖，等级、待选次数、候选及已选记录作为同一份状态仅复制给所属玩家。客户端提交对局编号、候选编号和卡牌 ID，不提交奖励数值。重复、伪造、旧候选或上一局的选择请求不会重复发奖；失败保留选卡机会。

死亡时关闭界面并释放选卡输入状态，保留已选记录、待选次数和当前候选。重生只从已选记录恢复永久属性，不再次发放治疗或弹药，也不自动重开界面。新对局重建 PlayerState 后重置；`ResetRun()` 可显式清空经验、击杀、候选和奖励，并更新对局编号。当前未实现断线重连恢复或跨局存档。

MonsterArena 通过 Lyra QuickBar 添加并选择步枪，射击、换弹和 HUD 使用同一个库存物品。停用玩法时清理怪物、界面、输入绑定和本玩法创建的装备；共享库存物品按原生命周期管理。

## 代码与资源归属

- `MonsterArenaRuntime`：成长状态、卡池、奖励效果、怪物，以及 `Public/UI`、`Private/UI` 下的 HUD 和 CommonUI 选卡界面。HUD 使用 `W_MonsterArenaHUD` 的设计器布局；选卡界面实现在 `Private/UI/MonsterArenaRewardScreen.cpp`，`RewardScreenClass` 提供界面子类入口。独立的 `MonsterArenaRewardInputAction.h/.cpp` 定义选卡快捷键 Action。
- `MonsterArenaEditor`：`UMonsterArenaContentLibrary` 资源配置接口，以及 `Private/Tests/` 下的编辑器测试桥接。Runtime 不依赖 Editor；Editor 模块不进入游戏运行时目标。
- `Content/Python/setup_monster_arena.py`：创建和配置插件专属 GameFeatureData、Experience、地图和玩法入口；已有 Experience 和地图布置保留。
- `UMonsterArenaComponent`：装备、HUD 生命周期与波次导演接入；`WaveDefinition` 引用波次配置。旧 `MaxAliveMonsters <= 0` 仅保留为测试暂停开关，正数不覆盖波次上限；旧 SpawnInterval 已废弃。`AArenaMonster`：初始生命、经验奖励和近战伤害，使用独立 ASC，无需怪物 PlayerState；AI 使用本插件的 StateTree 和独立节点代码。

专用蓝图和配置位于 `/MonsterArena/`：`UI/Notifications` 保存击杀播报，`UI/Weapons` 保存准星，`Weapons` 保存物品定义，`Characters/B_MonsterArenaShooterHero` 继承共享射击角色并覆盖本地初始物品。`System/HeroData_MonsterArena` 和 `System/Experiences/LAS_MonsterArena_Components` 将它们接入对局。通用枪械能力、模型、动画和基础 HUD 继续复用 ShooterCore。

本地播报和准星在读取击杀消息中的玩家状态前进行有效性检查。共享播报由 `LAS_ShooterGame_StandardComponents` 启用，MonsterArena 的组件集只注入本地播报。共享资源不应反向引用 `/MonsterArena/`。

## 验证

运行命令与测试覆盖见 [Tests/README.md](Tests/README.md)，已执行结果及未完成项见 [Tests/ACCEPTANCE.md](Tests/ACCEPTANCE.md)。Runtime C++ 测试位于 `Source/MonsterArenaRuntime/Private/Tests/`，Editor 测试桥接位于 `Source/MonsterArenaEditor/Private/Tests/`，Python 校验脚本位于 `Tests/Python/`。

无界面断言和反射调用不能替代画面、真实鼠标键盘／手柄输入及网络验收。每次检查完整运行日志：成功标记只代表对应断言完成，不代表整个会话没有 Error、Warning 或蓝图异常。独立 `-game` 和 `-NullRHI` 测试可能出现引擎实验性工具初始化错误或特效空引用警告，需在验收记录中明确区分。

## 守点波次配置

波次数值来源为 `Data/Waves.csv` 与 `Data/WaveSettings.csv`，编辑后运行 `Content/Python/import_waves_csv.py`，生成 `/MonsterArena/Data/DA_ArenaWaves`。运行时读取 DataAsset，不读取磁盘 CSV；直接修改 DataAsset 的数值会在下一次导入时覆盖。`Content/Python/setup_monster_arena_waves.py` 创建缺失的核心和导航边界，保留已有位置，重建导航并调用 CSV 导入。完整 setup 也调用此入口。

| 波次 | 进攻兵 | 猎手 | 存活上限 | 每批数量 | 批次间隔 | 入口数 |
| --- | --- | --- | --- | --- | --- | --- |
| 1 | 12 | 0 | 6 | 2 | 4 秒 | 1 |
| 2 | 16 | 4 | 8 | 3 | 4 秒 | 2 |
| 3 | 22 | 8 | 12 | 4 | 4 秒 | 2 |
| 4 | 28 | 12 | 16 | 5 | 3.5 秒 | 3 |
| 5 | 34 | 16 | 20 | 6 | 3 秒 | 4 |

每批内部间隔 0.3 秒。批次间隔从本批最后一只成功生成起计算；达到存活上限等待空位，不追赶积压时间。猎手均匀穿插，入口轮转。生成前校验胶囊空间、导航连通性及距玩家至少 350 单位；失败保留配额并换位置／入口重试，连续受阻 10 秒显示提示并记录一次 Warning，恢复后自动清除提示。不能将生成失败当作清场。

正常死亡在一次性死亡结算时扣减活怪；未死亡即异常销毁的怪物补回配额，不发击杀奖励。追逐中连续 15 秒几乎未位移会回收并补位。新对局与退出先清队列并解绑，再销毁旧怪，避免触发补位。胜负、波次、计数与核心生命由服务端决定并复制；下一波请求带对局／波次编号，经玩家所属组件 RPC 校验，客户端不能指定数量或收益。重置会清局内成长，不代表恢复玩家生命与库存至新开局装载；完整重新入场可退出后重进。

核心为最小灰盒目标，HUD 复用现有 Status 文本显示核心生命、波次、存活／待生成与操作提示；尚未制作独立血条、入口特效、命中特效或完整结算页。原有 U 选卡及世界不暂停规则保留。正常孤立生成的旧测试怪物不绑定核心，继续使用原有 StateTree 追人逻辑。
