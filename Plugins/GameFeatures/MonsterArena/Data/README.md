# CSV 配置

用 Excel 编辑并保存为 **CSV UTF-8**。保留第一行英文列名及列顺序，文本中的逗号或换行使用 Excel 自动生成的双引号转义。

- `Levels.csv`：Level 从 1 连续递增；ExperienceToNextLevel 为正整数，表示本级升到下一级所需经验。9 行对应最高 10 级，19 行对应最高 20 级。
- `RewardCards.csv`：只保存 CardId、Title、Description、Weight、MinimumLevel、MaxStacks。CardId 为稳定英文／数字／下划线标识，忽略大小写不得重复；Weight=0 不参与抽取；MinimumLevel 最低为 2；MaxStacks=0 不限制本局次数。
- `RewardActions.csv`：每行一个 `CardId,Behavior,Value`，同一 CardId 可以有多行。每张卡至少一个行为，行为不能引用不存在的卡。至少有一张 2 级可用的正权重永久强化。

例如“均衡成长”对应三条行为，增加组合效果只需要增加行：

```csv
CardId,Behavior,Value
Balance,MaxHealthBonus,10
Balance,DamageBonus,1
Balance,HealAmount,10
```

| Behavior | Value 含义 | 生命周期 |
| --- | --- | --- |
| MaxHealthBonus | 增加最大生命值 | 本局持续，复活保留 |
| DamageBonus | 增加基础伤害 | 本局持续，复活保留 |
| HealAmount | 恢复固定生命值 | 领取时一次 |
| HealFraction | 恢复最大生命值的比例，0.4 表示 40% | 领取时一次 |
| SpareAmmo | 增加步枪备用弹药，必须为整数 | 领取时一次，不自动装填 |
| DamagePerKill | 每击杀一个怪物或敌方玩家增加的基础伤害 | 领取后生效，累积值及行为在本局复活后保留 |

Value 必须为有限正数；无效果时删除该行，不填写 0。相同行为合计结算：HealFraction 总和不超过 1；其他行为的单值与合计不超过 16777216（float 能连续精确表示整数的上限）。先应用所有持续加成，再按新生命上限执行治疗，最后补充弹药；CSV 行顺序不改变结算阶段。治疗不会超过生命上限。复活／定时维护仅恢复持续效果，不重放即时行为。

运行时 `FRunRewardOption.Actions` 保存行为列表，服务端候选快照、网络复制、已领取记录和界面预览共用它。蓝图中也可编辑数组，无须增加卡牌属性。

`FirepowerAccumulation,DamagePerKill,1` 是“火力积累”。领取前的击杀不追溯；同类行为／重复领取相加，两张每次击杀 +2。服务端只奖励最终击杀者，怪物与敌方玩家都计入，队友、自杀、助攻不计；玩家必须有不同的有效队伍 ID 才算敌方。按死亡 Pawn 去重，同一玩家复活后可再次计入。玩家击杀不额外发放怪物经验。新对局清除行为和已累积伤害；伤害总值以 16777216 为数值安全上限。HUD 显示每次击杀收益和本局已累积伤害。

新增卡牌或组合已有行为只改 CSV。新增一种此前不存在的玩法行为（如移速、吸血）仍须在 C++ 中增加 `ERunRewardBehavior` 项、数值校验与结算逻辑，更新导入器的行为映射，并补充适用条件、生命周期、预览和测试；两个 CSV 的列结构都不用改。当前行为不是可执行脚本，也不会根据字符串任意调用函数。

先保存并退出 PIE，在 Unreal 编辑器的 Tools → Execute Python Script 执行：

`Plugins/GameFeatures/MonsterArena/Content/Python/import_progression_csv.py`

也可关闭编辑器后从项目根目录执行（`$UE` 为 UE 5.8 安装目录）：

```powershell
& "$UE/Engine/Binaries/Win64/UnrealEditor-Cmd.exe" "$PWD/LyraStarterGame.uproject" -run=pythonscript -script="$PWD/Plugins/GameFeatures/MonsterArena/Content/Python/import_progression_csv.py" -unattended -NullRHI -nosound
```

看到 `MONSTER_ARENA_CSV_IMPORT_SUCCEEDED` 才表示成功。导入器先校验全部 CSV，再更新 `/MonsterArena/Data/` 下的 `DA_MonsterArenaProgression`、`DA_MonsterArenaRewardPool` 和 `B_MonsterArenaProgression`，并绑定 `B_MonsterArena` 的 PlayerState 成长组件。仅修改 CSV 不会立即影响游戏，需要重新导入并重新进入对局。

这是 CSV → DataAsset 流程，不使用 DataTable。运行时读取生成的资产，不解析磁盘 CSV；配置随资产引用参与打包。CSV 为数值来源，直接修改生成资产中的数值会在下一次导入时被覆盖。版本管理需要同时提交 CSV、生成资产及 Experience 绑定变更。

`setup_monster_arena.py` 在配置 Experience 后也会执行导入，避免资源初始化把组件恢复为未绑定配置的原生类。

测试入口：`Tests/Python/validate_csv_configuration.py`，使用相同命令行方式执行，成功标记为 `MONSTER_ARENA_CSV_VALIDATION_SUCCEEDED`。
