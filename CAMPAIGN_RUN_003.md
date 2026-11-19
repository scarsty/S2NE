# 流程003 — 第三线／强度排序与最低难度自动验证

创建：2026-11-18。路线为第三线双主角合流，不加载既有流程001/002存档，不修改资源压缩包或个人存档。

## 路线生成规则

- 201–312 共112个逻辑阶段，按原始底座敌方单位数量与HP计算的可审计分数升序排列；分数为 `敌方数量 × 100 + 敌方HP总和`，同分按原始stage编号排序。
- 每个逻辑阶段的地图、敌军、事件、目标文本均由对应 `json/ref/stage-*.json` 重建；运行时 `original_events`、`original_campaign`、`original_initialization` 使用同一底座映射。
- 每关 JSON 固定12个部署槽。已有原始部署点全部保留，不足部分从首回合原始到场坐标或已有部署坐标按相邻六边形扩展；扩展槽在 `deployment_rules.source` 中标明，不能视为原资源原生槽位。
- 双主角队伍顺序固定为攻略交错后的 `11,1,14,15,13,2,3,4`：白志超线基础进度的 `11,14,15,13` 与娜塔莉雅线四人 `1,2,3,4` 交错拼接，娜塔莉雅 `1` 明确位于第二位。该顺序由 `try_fresh()` 创建存档军表时直接写入，不能只依赖 JSON 的显示顺序。白志超 `11` 与娜塔莉雅 `1` 都标记为 mandatory，并由运行时自动占用前两个部署槽且不可取消。
- 所有阶段目标统一为全灭，失败条件为白志超或娜塔莉雅阵亡；原始事件数组保留，单线流程不保留可选分支跳转。

## 验证边界

- 难度目标：最低难度（difficulty=1）。
- 自动策略：优先使用原自动战斗入口；不可把内置自动战斗结果称为严格语义AI决策通关。
- 每关应记录入场条件、实际胜利回合、队伍状态、奖励、经营、存读和下一关；未实际通过的关卡不得填写虚构结算。
- 截至创建时，已完成 JSON 重建、路由映射修复、双主角强制部署实现、clang-format 校验和 Release 构建；全流程 CLI 通关尚未完成，序幕自动驱动仍需继续核验。

## 当前阻塞

- CLI 序幕在自动协议探针中出现 EOF，尚未得到 stage 201 的可靠实战入口证据；因此不能声称第三线已通关。
- 强度排序是工程内可审计指标，不是外部攻略官方排序；若攻略表与该指标冲突，应以用户提供的攻略表重新生成。
# 第三路线流程003：当前回归结果（未验收）

## 已证实

- 开场 `stage-140` 仍保留三路线选择，第三项进入第三路线。
- `stage-201` 已改为原始 stage 162 的真实战斗数据，当前包含 2 个敌军单位。
- 第三路线 JSON 已移除 `type=5` 剧情选择事件。
- 同阶段加载已增加幂等保护，避免同一事件对象重复重建。
- 部署 picker 的资源加载改为显式 `try_load`，避免部署对象构造阶段的隐式加载。
- 发现 201 的 JSON 部署数组是 12 槽但只有 6 个唯一地图格；自动部署现已跳过重复格，防止重叠单位进入战斗。第三路线关卡名已恢复为各自原始底座关名；目标保持简单的全灭敌军/双主角阵亡条件，不再错误回填原始复杂目标。
- 部署点现已直接固化到两份第三路线 JSON：每关 12 个唯一坐标，新增点从该关 `player_party[].deployment` 原始位置邻近扩展；运行时不再动态生成部署点。201 的前两个槽仍固定为白志超和娜塔莉雅。
- 201 闪退根因已定位为补点使用了逻辑关卡坐标而非 `base_stage=162` 的原始地图坐标；现已按 ref 底座坐标重写两份 JSON，并保留第三路线逻辑角色 ID。PowerShell 7 CLI 回归正常进入 201 战斗阶段，未再在自动配置后退出；仍未完成第一关胜利验收。
- 对真实 201 部署边界 SAV 的直接复测又定位到事件状态数量不匹配：旧存档来自旧 JSON 事件表，恢复时曾被 `try_restore_event_states()` 直接 fatal。现对 JSON 阶段恢复共同前缀并保留新增事件默认状态；真实 SAV 启动 exit 0，未再出现事件数量 fatal。完整试玩点击链仍需用户确认。

## 当前阻断

- 短 CLI 回归可从 140 选择第三项并到达 201 briefing，但推进 briefing 后进程未在协议超时内返回。
- stderr 仍出现 140/201 交替加载，表明保存恢复和序幕创建之间仍有重复加载路径。
- 尚未进入可验证的第一回合，尚未验证部署、自动战斗、胜利、奖励和战后村庄。

## 2026-11-19 崩溃定位补充

- 严格新建流程已实际进入 stage 140 第三项选择，并到达 stage 201 briefing；此前控制器在 `briefing_advance` 的同步处理阶段超时，不能据此宣称已进入第一回合。
- 用户提供的崩溃位置为 `src/prologue.hpp:5427`。该行原先按 `story_group_units_` 数量访问 `story_group_->tracks()[i]`；原版路线失败时 `Group::begin()` 会跳过无法建立路径的角色，使轨道数量少于角色数量，形成越界访问。
- 现已按 `min(story_group_units_.size(), tracks.size())` 遍历，并增加单位索引边界保护；Release x64 构建和 clang-format dry-run 通过。
- 修复后严格流程仍在 `briefing_advance` 同步阶段超时，尚未取得自动部署成功响应或第一回合稳定证据。该阻塞与 5427 越界修复分开记录，不能把“构建通过”当作流程验收。
- Debug 卡顿初步确认与 `--stdio` 详细诊断输出及同步资源加载有关；本轮未改变诊断级别，避免隐藏实际加载/保存错误。后续应在不带 stdio 诊断的 VS Debug 启动下单独采样重复资源加载和逐帧输出。
- 2026-11-19 自动战斗已从真实 201 检查点进入第 9 回合；最后完整状态为 `computer_actions=true`、`interaction.mode=advancing`，敌方 51 HP=35、54 HP=224，进程 CPU 持续增长超过一分钟，按规则停止。该次不是崩溃或协议 EOF，而是自动战斗电脑行动未返回边界的真实阻塞，尚未宣称过关。
- 同次证据显示 stdout 大量重复 `Data source towns=JSON shop path=...`；已移除旅馆／商店数据源的无条件 stdout 输出，并将音频设备枚举限制在诊断模式且送 stderr，Release 构建通过。后续需用新构建重跑确认日志量下降，再继续定位第 9 回合 AI 行动。
- 2026-11-19 重新构建后，stage 140 的重复数据源输出已移除，201 入场简报和真实目标均可读取；短时回归在第 12 个请求等待电脑行动超过 10 秒后按硬超时停止，状态为 `stage=201`、`phase=9`、`computer_actions=true`、`ai_skill_pending=false`、无移动／战斗反馈。由此排除本次现场是 AI 技能视觉锁死，但普通自动行动规划／执行仍未在可接受时间内完成。
- 本次短时回归未取得胜利、奖励、存档重载或下一关证据；不能宣称第一关完成。回归证据目录已清理，Release 可执行文件保留。
- 2026-11-19 用户复现补充：第三路线 JSON 的敌方单位曾写入 `max_hp=0`，导致战斗反馈按零上限裁剪伤害；现装载时对零／缺失值回退到当前 `hp`。同批发现原路线 transition 只读 BIN 的 `first_stage/alternate_stage`，而 JSON route 未同步写回，现已补齐 `next_stage/alternate_stage` 路由字段；未支持 transition 改为带 stage/phase/event 的 blocker，避免静默错误推进。Release 重建成功。
- 同次复核确认 stage 140 的第二条原路线实际目标为 `stage=71`（第一条为 1，第三条为 201）；此前 JSON／ref JSON 错写为 0，已同步改为 71。stage 201 的两名敌方零 `max_hp` 不再影响运行时实际最大 HP，Release 产物存在且路由 JSON 装载检查通过。
- 2026-11-19 原路线卡点复核：stage 140 的两个 `type=9` 开场事件缺少 `branch_flag=13`，JSON 覆盖层因此无法触发选择后的路线对白，最终落入未支持 transition；已在实际 JSON 与 ref JSON 同步补齐。另完成异常输出集中化：业务 `Error` 不再携带／打印栈，只有 `main.cpp` 最外层未捕获异常打印一次标准库 `std::stacktrace`；运行时参数、abort、terminate 和 Debug CRT 钩子不再重复打印栈。JSON 成功加载日志已移除；FluidSynth 仅保留错误级别 stderr，ANI/结局仍会按原资源表调用 MIDI 回调，不能视为全曲目都是 MP3。

本文件不记录自动通关结论；流程003仍未验收。

- 2026-11-19 重复加载修复：确认 OriginalEvents 的三个实例分别承担可变的 flags/units/事件状态，不能直接共用同一状态对象；其基础资源（BIN/PAL/KINDS/ITEMS）本已由共享资源缓存复用。此前删除成功日志只是隐藏证据，已改为 stage JSON 进程级静态缓存加互斥锁，并仅在首次实际解析时输出 JSON stage parsed；后续实例命中同一规范化路径不会重新解析。路由判定 OriginalCampaign::selected_route 与 Prologue 初始化改为复制现有 OriginalEvents 后重载 stage，避免额外构造基础资源上下文。clang-format dry-run 与 Release x64 构建通过。

- 2026-11-19 CLI 路线回归：使用 Release --stdio --headless-speed，通过受限 skip_opening 仅跳过 53+658 帧真实开场动画，然后执行 new_game、setup_confirm、原对白推进和实际 story_choose。未修复前第一项在 stage=140 phase=2 event=2、第二项在 stage=140 phase=2 event=3 进入 campaign_blocked，stderr 为 Unsupported original prologue event transition。根因是 stage 140 的分支对白完成后清除了 route flag，通用 phase=branch 路径没有继续调度两个原版 type=9 收尾事件。现已按 stage 140 原事件顺序补充分支后的 route dialogue 调度；回归结果第一项进入 stage=1、第二项进入 stage=71，均处于下一关 prologue，未进入 blocked。第三项路径未改动。

- 2026-11-19 第三路线第一关胜利条件修复：stage 201 JSON 明确为 mode=1、敌军为 id=51/54，玩家队伍随后追加在同一 units 容器。原 dispatch_campaign 用遇到第一个 behavior=2/3/8 后把后续所有单位归入 enemies，导致玩家单位被混入敌军列表；即使两个敌人死亡，mode=1 的 absent(enemies,0) 仍为 false，任务不结束。现改为逐单位调用 OriginalEvents::Unit::hostile() 分类，保持行为1盟友与行为2/3/8/9/10敌军的真实判定。Release x64 构建与 clang-format dry-run 通过。
- 调用栈说明：此前每个 Result<Error> 创建时捕获 std::stacktrace，目的原本是定位错误调用来源，但会让普通可恢复的协议校验、资源失败、状态拒绝都付出栈捕获成本并打印完整栈，造成大量噪声和 Debug 卡顿；本轮没有恢复这种逐业务错误栈。当前保留 main.cpp 顶层未捕获异常的一次标准库栈输出，运行时 abort/purecall/terminate/CRT 参数错误保留一行明确错误，真正未捕获异常仍可定位。

- 2026-11-19 队伍顺序修复：原 try_fresh() 先批量追加白志超线 11,14,15,13，再批量追加娜塔莉雅线 1,2,3,4，导致运行时存档军表实际为 11,14,15,13,1,2,3,4；第三路线 JSON 中的 player_party 顺序不能覆盖该军表顺序。现按攻略交错顺序直接创建为 11,1,14,15,13,2,3,4，因此娜塔莉雅在第二位；Release x64 构建与 clang-format dry-run 通过。尚未据此宣称第三路线第一关完整实战通关。
- 2026-11-19 本轮修复：第三线战斗结算未设置 `OriginalCampaign::Runtime.mode`，导致 JSON 的 `mode=1` 全灭规则没有执行，现由当前 stage 的 JSON/BIN 模式写入 runtime。另修复自定义 stage 转场仍按原版 BIN 阶段范围校验的问题，并将 stage-140 的娜塔莉雅路线 `alternate_stage` 恢复为 71。Release x64 构建与 clang-format dry-run 通过；尚未冒称完整实战通关。
