# CSV 配置

## 统一管理入口

配置统一登记在 `ConfigRegistry.json`。当前包含“成长与奖励”（Levels、RewardCards、RewardActions）和“守点波次”（Waves、WaveSettings），CSV 仍保存在本目录。

编辑器 **Tools → MonsterArena CSV 配置** 提供：列出配置清单、校验全部、导入全部、按组校验／导入、打开 CSV 目录、打开导入报告目录。若当前会话没有该菜单，在 Tools → Execute Python Script 执行 `Content/Python/manage_csv.py`；此脚本会注册菜单并执行一次全量导入。插件 Python 启动脚本在可用的交互编辑器中也尝试注册菜单。

统一工作流：编辑 CSV → 校验全部 → 导入全部 → 重新进入 PIE。PIE 中禁止导入；“校验”不加载或修改玩法资产。旧 `import_progression_csv.py` 和 `import_waves_csv.py` 保留，分别转发到统一管理器的对应配置组。

全部所选组先解析、校验并固定快照，再按依赖顺序写入资产。某组校验失败时，所有组均不写入。检测到校验过程中源文件变化会拒绝导入，防止混用不同版本。资产写入不是跨包事务：若写入／保存失败，先前成功的组可能已更新；报告会区分 imported、failed 和尚未写入的 validated，不宣称全量回滚。

每次操作的 JSON 报告位于项目 `Saved/ConfigImports/MonsterArena/`，包含配置组、源文件 SHA-256、输出资产、执行状态和错误。报告不进入版本控制。

以后增加新配置组：

1. 在 `Data/` 添加 CSV，在 `ConfigRegistry.json` 登记稳定 ID、中文名称、源文件、输出资产与 `depends_on` 依赖。
2. 在 `Content/Python/arena_csv/` 增加转换器，提供 `SOURCES`、纯读取的 `validate(directory)` 和只使用校验快照的 `apply(snapshot)`。
3. 在 `manager.py` 的 `ADAPTERS` 中登记转换器。菜单按注册表生成，不需要再增加独立批量入口。
4. 补充数据关系、非法输入、资产回读和实际玩法测试。新增行为仍需相应 Runtime 实现，CSV 不执行任意脚本。

未登记 CSV、重复来源、未知配置组、循环依赖、来源越界均拒绝处理。完整资源 setup 在改资产前先全量校验，结束后使用统一入口导入。转换器和菜单均为编辑器 Python 工具，Runtime 无新增编辑器依赖。

在 Unreal Python 控制台也可调用：

```python
import runpy
api = runpy.run_path(r"E:\workspace\LyraStarterGame\Plugins\GameFeatures\MonsterArena\Content\Python\manage_csv.py")
api["run"]("list")
api["run"]("validate")
api["run"]("import", ["waves"])
```

统一回归入口为 `Tests/Python/validate_csv_manager.py`；原有两组配置的细节与校验继续保留。

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

## 守点波次 CSV

修改 `Waves.csv` 配置每一波，修改 `WaveSettings.csv` 配置全局节奏。保存为 CSV UTF-8，保留列名与列顺序。全部源数据校验通过后才更新 `/MonsterArena/Data/DA_ArenaWaves`；不要把 DA 作为另一份数值来源。

| Waves.csv 列 | 含义与约束 |
| --- | --- |
| Wave | 从 1 连续递增，不重复、不跳号；新增波次可直接增加一行，最多 1000 波 |
| AssaultCount | 进攻兵数量，非负整数 |
| HunterCount | 猎手数量，非负整数；两类总数为 1～10000 |
| AliveLimit | 同时存活上限，1～200 |
| BatchSize | 每批数量，正整数且不能大于 AliveLimit |
| BatchInterval | 本批最后一只生成后到下批的间隔，秒，0.3～3600 |
| EntranceCount | 当前四入口地图使用的入口数量，1～4 |

`WaveSettings.csv` 仅一行：`WarningDuration` 为开波预警秒数（0～120）；`WithinBatchInterval` 为批内生成间隔秒数（0.1～60）；`PlayerSafeDistance` 为生成位置距玩家的最小距离，UE 单位 cm（0～10000）。这些范围是数据保护限制，不代表推荐难度或性能承诺。

保存并退出 PIE，在编辑器 Tools → Execute Python Script 执行 `Plugins/GameFeatures/MonsterArena/Content/Python/import_waves_csv.py`。看到 `MONSTER_ARENA_WAVES_CSV_IMPORT_SUCCEEDED` 后重新进入 PIE。也可使用前面的命令行导入方式，将 script 路径替换为此文件。修改 CSV 本身不会立即改变运行中的对局。

负数、空值、非整数计数、NaN／Inf、错误列名、断档波次、零怪物和批量超过上限等均拒绝，错误包含文件、行号或字段；非法输入不会修改已有资产。配置与生成的 DA 应一同版本管理。检查入口：`Tests/Python/validate_wave_csv.py`。
