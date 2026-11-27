# JSON 数据字段说明

JSON 文件位于资源根目录的 `json` 文件夹。阶段文件命名为
`stage-<阶段号>.json`。阶段号由文件名确定，不在文件内容中重复保存。

当前字段说明以导出器和运行时实际读取路径为准。没有经过汇编、原资源和运行时共同确认的
字段不得改名为看似确定的业务概念。

## 阶段文件

| 字段 | 含义 | 来源或约束 |
| --- | --- | --- |
| `map_id` | 战场地图编号 | 原阶段表头；地图格仍从 `SUPERSP.PRG` 读取 |
| `music` | 阶段初始音乐编号 | 原阶段表头；0 表示不改变当前音乐 |
| `units` | 固定敌人、NPC 和剧情角色模板 | `behavior=0` 的存档队伍槽位不放在这里 |
| `player_party` | 我方存档队伍槽位与候选部署点 | 属性、等级、经验、装备和库存来自当前存档 |
| `deployment` | 第1回合可用的部署坐标、方向和角色编号 | `fixed` 只表示固定部署点，不表示强制出战 |
| `deployment_rules` | 部署规则的证据边界 | `mandatory_role_ids` 未被证实时必须为空 |
| `objects` | 已确认的动态对象数据 | 未确认的地图物件不得命名为宝箱等业务对象 |
| `name` | UTF-8 阶段名称 | 导出时由原文本解码 |
| `briefing` | UTF-8 阶段简报 | 导出时由原文本解码 |
| `objectives` | 胜利与失败目标文字 | 当前导出器只保留已读取的文本位置 |
| `events` | 按原事件表顺序排列的事件 | 数组位置就是原事件序号，不再另存 `index` |

阶段文件还可以包含 `route`：

| 字段 | 含义 |
| --- | --- |
| `route.next_stage` | 原阶段表头的默认下一阶段 |
| `route.alternate_stage` | 原阶段表头的另一阶段字段；当前只导出，尚未替代事件旗标路线判定 |
| `route.mode` | 原阶段模式；普通战斗阶段为1，剧情/中间阶段为3 |
| `route.interbattle_town_id` | 普通战斗阶段结算后使用的村庄表编号；它不是阶段编号 |

## 事件字段

所有事件都可以有：

| 字段 | 含义 |
| --- | --- |
| `type` | 原事件类型字节，不是 JSON 自定义枚举 |
| `condition` | 原事件子条件；为 0 时省略 |
| `completion_flag` | 事件完成后写入的原旗标；0 有实际含义，不能统一删除 |
| `lines` | 该事件自己的对白行数组；数组长度就是对白行数 |

已确认的事件专用字段：

| 事件 | 字段 | 含义 |
| --- | --- | --- |
| `type=1, condition=0` | `trigger.kind`, `trigger.turn` | 与当前回合相等时触发的回合事件 |
| `type=3, condition=3` | `victim_role_id` | 被击杀角色 ID |
| `type=3, condition=3` | `killer_role_id` | 要求的击杀者角色 ID；为 0 时不写 |
| `type=3` 其他条件 | `role_id` | 已确认的角色操作数；其余非零未知操作数才进入 `raw_operands` |
| `type=5, condition=0` | `branch_flag` | 选择事件写回的旗标 |
| `type=5, condition=1..4` | `branch_flag`, `branch_value` | 选择分支匹配的旗标和值 |
| `type=9, condition=1..2` | `required_flags` | AND 旗标集合；条件2成功后按原逻辑清除 |
| `type=9, condition=3..4` | `any_flags` | OR 旗标集合 |
| `type=9, condition=6` | `route_stage` | 原路线阶段值 |

未确认的非零事件操作数使用：

```json
{
  "raw_operands": {
    "offset_3": 1,
    "offset_5": 2,
    "offset_7": 0,
    "offset_9": 0
  },
  "raw_operands_semantics": "unconfirmed"
}
```

全为 0 的未确认操作数不输出。`offset_N` 表示原 20 字节事件记录中的字节偏移，
不是已确认的业务语义。

## 对白行

| 字段 | 含义 |
| --- | --- |
| `speaker` | 原角色编号 |
| `name` | UTF-8 说话者名称 |
| `text` | UTF-8 对白文字 |
| `music_before` | 对白前音乐；0 表示不改变，故省略 |
| `music_after` | 对白后音乐；0 表示不改变，故省略 |

## 数据边界

- JSON 不复制地图格、地形属性、寻路规则或地图图像；`map_id` 只引用原 PRG 地图。
- 存档中的队伍属性、事件完成状态、旗标、库存和成长状态不由阶段 JSON 覆盖。
- 当前阶段 JSON 已接入通用事件加载路径：存在且有效时覆盖阶段地图号、音乐号和事件对白，
  事件数量不一致或文本格式错误会直接报错；固定单位模板、动态对象、村庄、世界地图和阶段
  路由仍在迁移中，不能把导出文件存在误认为全部系统已经接入。
- JSON 文件存在但格式错误时必须报错；JSON 缺失时，迁移期才允许明确记录并回退原 BIN。

## `towns.json`

`towns.json` 是共享村庄表。`records` 中的 `id` 是村庄表编号，不是阶段编号；`name` 是
UTF-8 村庄名称，`x`、`y` 是主地图坐标，`services_allowed` 是原村庄服务开关，
`offers.weapons`、`offers.armor` 和 `offers.items` 是商店三个分类的原物品编号。
`rumors` 保存原BIN4的两段UTF-8旅店传闻，顺序对应原随机分支0／1，必须恰好两项。
导出入口为 `tools/export-town-json.ps1`，同时更新资源根副本与工程内副本。
运行时
将这些语义字段重建为现有旅馆、商店和世界地图消费者需要的内部表；文件不再保存原始字节
转储，也不对未由源码确认的字节自行命名。

白志超路线的已证实首个节点为：阶段140选择白志超进入阶段1；阶段1结算进入村庄1，
但村庄1服务关闭；阶段2结算进入村庄2，村庄2服务开启且有14个商品，是该路线第一个商店。
