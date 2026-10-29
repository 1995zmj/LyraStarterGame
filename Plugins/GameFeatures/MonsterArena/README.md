# MonsterArena

第一阶段原型：步枪击杀怪物 → 获得局内经验 → 升级 → 自动获得强化。

## 运行

编译 `LyraEditor Win64 Development` 后重新打开编辑器。在内容浏览器启用插件内容，打开 `/MonsterArena/Maps/L_MonsterArena` 并运行 PIE。该地图使用 `/MonsterArena/System/Experiences/B_MonsterArena`，自动启用 ShooterCore 和 MonsterArena。也提供玩法入口资源 `/MonsterArena/System/Playlists/DA_MonsterArena`。

每个新角色自动装备步枪。方块怪物从四个 `MonsterSpawn` 标签出生点生成，每 2 秒补充一只，同时最多 6 只。怪物生命 60、移动速度 220，直接追逐玩家，140 单位内每秒近战造成 10 点伤害。直线追逐适用于当前开阔测试地图；复杂场景需要后续接入导航。

每只怪物给予击杀玩家 20 XP。初始 1 级，经验门槛依次为 60、100、140、180、220、260、300、340、380，最高 10 级。经验溢出保留，可连续升级。偶数等级获得最大生命 +20 并恢复 20 生命；奇数等级（从 3 级起）获得基础伤害 +3。步枪初始每发基础伤害 12，首次火力升级后为 15，距离和物理材质仍由原枪械规则处理。

`URunProgressionComponent` 保存在 PlayerState 上，服务端结算，成长状态仅向所属玩家复制。角色换 Pawn 时恢复强化；新对局重建 PlayerState 后重置。`ResetRun()` 可显式清空经验、击杀和奖励。停用玩法时清理怪物、界面和本玩法装备。当前未实现重连恢复或跨局存档。

## 配置与扩展

- `URunProgressionDefinition`：经验表、生命奖励与伤害奖励参数，可创建 DataAsset 并配置到成长组件的蓝图子类。
- `UMonsterArenaComponent`：刷怪间隔、存活上限和怪物类。
- `AArenaMonster`：初始生命、经验奖励和近战伤害；使用独立 ASC，不需要怪物 PlayerState。
- `UMonsterArenaContentLibrary`：位于 `MonsterArenaEditor` 模块的编辑器资源配置接口；仅编辑器使用，不属于运行时玩法。Python 脚本显式加载此 Editor 模块，Runtime 不依赖 Editor。
- `Content/Python/setup_monster_arena.py`：在 Unreal 中生成插件专属 GameFeatureData、Experience、地图和玩法入口。已存在的 Experience 和地图布置保留。

当前奖励采用固定交替发放。后续在升级与发奖之间加入待选奖励队列、卡池和三选一确认流程。

## 验证

测试目录、运行方式及验收记录见 [Tests/README.md](Tests/README.md) 和 [Tests/ACCEPTANCE.md](Tests/ACCEPTANCE.md)。C++ 测试集中在 `Source/MonsterArenaRuntime/Private/Tests/`，编辑器校验脚本位于 `Tests/Python/`。

独立无界面战斗验证（仅开发构建）：

```powershell
& "$UE/Engine/Binaries/Win64/UnrealEditor-Cmd.exe" "$PWD/LyraStarterGame.uproject" /MonsterArena/Maps/L_MonsterArena -game -MonsterArenaSmokeTest -unattended -NullRHI -nosound -SCCProvider=None -abslog="$PWD/Saved/Logs/MonsterArenaSmoke.log"
```

验证 Experience 组件注入、真实步枪 GameplayEffect 伤害、武器射线通道、死亡奖励去重、8 次击杀升到 3 级、奖励后伤害变化、经验溢出、等级上限及重置回收。成功输出 `MONSTER_ARENA_SMOKE_SUCCEEDED` 并以 0 退出。

无界面测试不能代替画面和输入验收。双客户端网络复制、死亡复活全过程和手动射击仍需要 PIE 验证。

### 自动射击与弹药回归验证

MonsterArena 通过 Lyra QuickBar 添加并选择步枪，不再绕过快捷栏直接叠加装备。射击能力、换弹能力与 HUD 因此使用同一个库存物品。对局清理只移除本玩法创建的物品；角色失去控制器时，快捷栏也会清空旧装备引用。

以开发参数 `-MonsterArenaAmmoTest` 替代上述命令中的 `-MonsterArenaSmokeTest`，运行真实输入驱动的弹药测试。验证单武器装备、快捷栏与弹药来源一致、初始 30 发、持续空射扣弹、松开停止扣弹、空弹匣停火和备用弹药换弹。成功输出 `MONSTER_ARENA_AMMO_SUCCEEDED` 并以 0 退出。

使用 `-MonsterArenaLifecycleTest` 可运行两次实际怪物击杀与自动复活测试，验证复活后等级、奖励和武器物品保留；成功输出 `MONSTER_ARENA_LIFECYCLE_SUCCEEDED`。无渲染模式中 Niagara 特效可能返回空组件，日志清洁验收仍应在有渲染的 PIE 中执行。

### 玩家死亡与复活验证

本地的 `B_MonsterArenaEliminationFeedRelay` 在读取姓名、Pawn 和队伍前检查消息双方的 PlayerState；`W_MonsterArenaReticle_Rifle` 和 `W_MonsterArenaReticle_Pistol` 在读取击杀者控制器前检查 LyraPlayerState。非玩家击杀者不进入仅支持玩家的播报和准星击杀动画逻辑，死亡事件与自动复活照常处理。

### 插件资源边界

专用蓝图和配置位于 `/MonsterArena/`：`UI/Notifications` 保存击杀播报，`UI/Weapons` 保存步枪及手枪准星，`Weapons` 保存引用本地准星的物品定义，`Characters/B_MonsterArenaShooterHero` 继承共享射击角色并覆盖本地初始物品，`System/HeroData_MonsterArena` 和 `System/Experiences/LAS_MonsterArena_Components` 将它们接入对局。使用角色子类保留共享武器对射击角色类型的要求。通用枪械能力、模型和基础 HUD 继续复用 ShooterCore。

ShooterCore 原有播报和准星蓝图中的 MonsterArena 专用检查已移除。通用播报由 `LAS_ShooterGame_StandardComponents` 启用，不再由 ShooterCore 的 GameFeatureData 全局注入；MonsterArena 的本地组件集只注入本地播报。共享资源不能引用 `/MonsterArena/`，避免其他玩法加载本玩法专用逻辑。

已在编辑器 PIE 中验证怪物击杀玩家并连续自动复活，未再出现这些蓝图的 `Accessed None` 或击杀播报队伍颜色的 Switch 警告。此前命令行战斗验证只覆盖玩家击杀怪物，不能替代这个反向场景。

项目启用的实验性编辑器工具在独立 `-game` 启动中可能输出原有 Python/Automation 初始化错误；应同时检查本插件测试标记和玩法日志。
