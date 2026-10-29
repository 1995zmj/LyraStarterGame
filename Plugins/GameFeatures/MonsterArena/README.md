# MonsterArena

步枪击杀怪物 → 获得局内经验 → 升级积攒选卡次数 → 玩家手动打开三选一 → 确认后获得强化。

## 运行与操作

编译 `LyraEditor Win64 Development` 后重新打开编辑器。在内容浏览器启用插件内容，打开 `/MonsterArena/Maps/L_MonsterArena` 并运行 PIE。该地图使用 `/MonsterArena/System/Experiences/B_MonsterArena`，启用 ShooterCore 和 MonsterArena。玩法入口资源为 `/MonsterArena/System/Playlists/DA_MonsterArena`。

每个新角色自动装备步枪。方块怪物从四个 `MonsterSpawn` 标签出生点生成，每 2 秒补充一只，同时最多 6 只。怪物生命 60、移动速度 220，直接追逐玩家，140 单位内每秒近战造成 10 点伤害。直线追逐适用于当前开阔地图；复杂场景需要接入导航。

每只怪物给予击杀玩家 20 XP。初始 1 级，经验门槛依次为 60、100、140、180、220、260、300、340、380，最高 10 级。经验溢出保留，可连续升级；每升一级增加一次选卡机会。达到等级上限不会清除尚未使用的机会。

- 升级只更新 HUD 的“可选强化 ×N”提示，不自动打开界面，也不自动增加属性。
- 默认按 `U` 或手柄上方向键手动打开强化界面。输入动作的稳定映射名为 `MonsterArena.OpenRewards`，提示读取实际映射按键。默认使用 `U` 避开项目中 `K` 的自毁调试快捷键。
- 点击一张卡，再点击“确认强化”；键盘 `1` / `2` / `3` 可选中卡牌，`Enter` 确认。手柄可通过界面焦点导航选择和确认。
- `Esc`、手柄返回键、键盘打开界面的映射按键或“返回战斗”按钮均可关闭。手柄方向键在界面内用于导航。未选次数和同一组候选保留，反复开关不能刷新卡牌。
- 有剩余次数时，确认一张后在同一个窗口展示下一组；全部选完自动关闭。没有自动领取倒计时。
- 单人和多人都不暂停世界。选卡玩家仍会受到正常伤害，界面不提供无敌；打开时停止自身射击并切换为菜单输入，关闭后恢复操作，开火需重新按下。

## 默认卡池与配置

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

- `MonsterArenaRuntime`：成长状态、卡池、奖励效果、怪物、HUD，以及 `UMonsterArenaRewardScreen` CommonUI 界面。HUD 实现在 `Private/MonsterArenaHUD.cpp`，选卡界面实现在 `Private/MonsterArenaRewardScreen.cpp`。原生默认布局可以直接运行，`RewardScreenClass` 提供界面子类入口。
- `MonsterArenaEditor`：`UMonsterArenaContentLibrary` 资源配置接口，以及 `Private/Tests/` 下的编辑器测试桥接。Runtime 不依赖 Editor；Editor 模块不进入游戏运行时目标。
- `Content/Python/setup_monster_arena.py`：创建和配置插件专属 GameFeatureData、Experience、地图和玩法入口；已有 Experience 和地图布置保留。
- `UMonsterArenaComponent`：刷怪间隔、存活上限和怪物类。`AArenaMonster`：初始生命、经验奖励和近战伤害，使用独立 ASC，无需怪物 PlayerState。

专用蓝图和配置位于 `/MonsterArena/`：`UI/Notifications` 保存击杀播报，`UI/Weapons` 保存准星，`Weapons` 保存物品定义，`Characters/B_MonsterArenaShooterHero` 继承共享射击角色并覆盖本地初始物品。`System/HeroData_MonsterArena` 和 `System/Experiences/LAS_MonsterArena_Components` 将它们接入对局。通用枪械能力、模型、动画和基础 HUD 继续复用 ShooterCore。

本地播报和准星在读取击杀消息中的玩家状态前进行有效性检查。共享播报由 `LAS_ShooterGame_StandardComponents` 启用，MonsterArena 的组件集只注入本地播报。共享资源不应反向引用 `/MonsterArena/`。

## 验证

运行命令与测试覆盖见 [Tests/README.md](Tests/README.md)，已执行结果及未完成项见 [Tests/ACCEPTANCE.md](Tests/ACCEPTANCE.md)。Runtime C++ 测试位于 `Source/MonsterArenaRuntime/Private/Tests/`，Editor 测试桥接位于 `Source/MonsterArenaEditor/Private/Tests/`，Python 校验脚本位于 `Tests/Python/`。

无界面断言和反射调用不能替代画面、真实鼠标键盘／手柄输入及网络验收。每次检查完整运行日志：成功标记只代表对应断言完成，不代表整个会话没有 Error、Warning 或蓝图异常。独立 `-game` 和 `-NullRHI` 测试可能出现引擎实验性工具初始化错误或特效空引用警告，需在验收记录中明确区分。
