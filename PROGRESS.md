# S2NE — 进度、缺口与验证记录

游戏与项目介绍、开发环境及日常构建入口见 [README](README.md)。本文件集中记录实现进度、已知缺陷、验证边界和历史还原证据。

## 阅读说明

- 以最新实际回归结果为准，算法或资源测试通过不等于对应的完整游戏流程已经恢复。
- 下方保留历史记录；其中“新游戏不能存档”“奖励未接”“转职未提交”等旧限制已被最新结果取代，不应作为当前结论。
- 已验证两路线首战胜利、奖励、关间出征、下一关及自动档重载；另已合法续玩白志超至第4关并完成实际转职与重载。这不代表后续全战役完成。

## 当前主要缺口（2026-11-09）

| 范围 | 缺口或尚未完成的验收 |
|---|---|
| 后续战役 | 全路线逐关通关、第154等中间剧情、全部特殊事件与转场尚未覆盖。 |
| 战斗规则 | 完整技能、范围效果、连击／反击、装备触发及状态回合效果，部分只有独立算法，尚未完整接入实战。 |
| 特殊 AI | 特殊行为6／7、脚本目标移动、完整低血撤退及飞行路线等仍有缺口。 |
| 关间及菜单 | 旅店／商店／教会下级交易、完整菜单树、人物详情与条件撤离尚未逐项闭环。 |
| 画面与动作 | 特殊移动、传送／起降、完整选中合成及部分遮挡仍需恢复；原版画面与时序对照不足。 |
| 换盘 | 本地自动读盘、两盘82项资源及边界专项通过；正常战役推进至真实换盘节点的端到端测试未完成。第71→72关仍为CD1，不算跨盘验证。 |
| 存读档 | 已验证原格式往返、自动档和部分续玩；全部特殊状态与跨盘节点未覆盖。 |
| 范围排除／暂缓 | 按用户2026-11-09要求，双人对战不处理，不列入本轮实现及通关验收范围；保留原入口，不实现替代玩法。音频完整还原继续暂缓。 |

以下详细进度和历史证据须结合上述现状阅读。

## 单人全流程验收推进（进行中）

验收标准是依据原规则自行完成单人战役并到达原终局；首战胜利、进入下一关或独立规则测试均不能替代全通关。双人对战不纳入范围。

已新增连续战役启动测试 `--smoke-campaign-continue`，不在第2关提前退出，每次进入战场核对原自动档。第4关后的154阶段已从原事件确认为模式3剧情中间阶段，不应按普通战斗或再次结算奖励处理；原选择事件指向第5或第8阶段，正在恢复该转场并接入实际运行。此段记录为进行中，不是验收通过声明。

连续 SDL 实跑已完成第1关（第13回合）与第2关（第14回合）的胜利和自动档核对，第3关在第100回合触发原超时败北；结束时主角仍存活，日志中的敌军均已死亡。后续查明并非特殊目标移动：首领77仍隐形存活，AI 用移动前位置评估未来攻击点的可见性，导致漏选目标；已按原 `40B0BA` 修复候选观察者坐标。最新连续 SDL 实跑通过第1关第12回合、第2关第11回合、第3关第11回合、第4关第8回合，并正常经过154剧情进入第5关、第25回合胜利。再次实跑仍通过第1至5关，随后定位到具体错误：第5关胜利报告后尝试存档时仍有原生移动动画状态，触发 `Cannot save during a native movement animation`。该问题已修复：仅在原胜败结果确认、移动和攻击反馈均结束后清理残留显示状态，不移除存档安全检查。后续实际 SDL 连续运行已完成第1至7关及154剧情分支，每关均验证自动档重载；第7关胜利后进入第10阶段时报 `Invalid original event index`，已定位并修复入口：第10阶段没有首回合对话，旧流程将未找到事件的末尾索引交给对话准备函数。现按正常战场初始化、自动档与横幅流程进入，保留第2回合对话的原触发时机。真实资源入口及第2回合触发回归通过，连续 SDL 全链复测进行中。该关第二回合选择、剧情角色加入适配及原特殊胜利条件已补入并通过专项回归。最新实际连续 SDL 已进入第10关并在第27回合消灭敌军，但该关要求原出口条件，不能以敌军清零代替胜利；已修正连续测试对此的提前失败判断。原普通自动战斗没有已证实的自动寻出口策略，因此测试在歼敌后通过正常选人、移动命令与实际AP逐步走向原出口（选择索引1、主角11、出口21/4或20/5或21/6），不改生产AI、不直接设坐标／旗标；连续验收正在运行。不能宣称全通关；不回改回合数规避失败。

## 当前状态

### 首战到第2关闭环（最新结果，取代下方历史缺口描述）

- 新游戏已依据原初始化构造完整原格式状态：实际序章140标志、原设置、库存、全局与角色记录，首回合仅纳入实际到场17／22人；后续增援按原时机、碰撞选位与复用／新建流程原子加入。不是拷贝已有存档或把全部源人物填入。
- 生产已接真实胜利奖励：原WORD经验+30与原成长、1500金钱、247物品两件／248一件、既存角色刑天加入；随后进入原PCX17关间界面，出征按持久队伍顺序进入第2关，保留等级、经验、装备和库存，执行原资源池重置。
- 自动档51／52与覆盖前备份53已接生产；新游戏不再因无基线而拒绝这一已核实路径。`--smoke-campaign` 在SDL dummy驱动完成真实新游戏→第1关自动战斗→原胜利剧情／报告→奖励→鼠标关间出征→第2关剧情→自动槽52重新载入，4375帧、第22回合胜利。测试写档全部隔离，未改写用户原档。
- 两分支、五档难度的新档初始化／原格式往返及原22／42人档回归通过。首战不够原转职门槛，不人为加经验；已完成的转职事务现在可使用真实新游戏基线，但实际SDL转职点击仍需另外验证。
- **范围边界**：闭合的是白志超线首战到第2关，不是全游戏完成；关间旅店／商店／教会下级交易、第71关全部特殊脚本、完整技能与反击等仍有待恢复。下方保留的历史“无基线拒绝”与“奖励未接”记录不再描述该首战路径现状。

### 光盘切换核实（续查）

原 `416333` 以路线、流程进度及全局关卡分别判断：娜塔莉娅线流程进度达到7才选CD2，第二战第72关仍使用CD1；白志超线全局关卡60／61选CD2。已接本地光盘自动解析与两套不同DAT目录，优先兼容的windows-disc提取，其次原disc提取，拒绝混用错误目录或损坏档案；开场／标题固定CD1，地图、战斗及关间按原状态传递，读档恢复保存的盘号。两盘82个真实条目、两盘消费者、第72关地图与实际标题读档通过，不要求用户手动换盘。娜塔莉娅线曾因 NPC 调度与 AI 阵营映射错误而自动败北；修复后最低难度实际 SDL 回归已完成71→胜利奖励→关间出征→72及自动档重载，见下方 NPC 专项。

### 合法续玩与实际转职验证（2026-11-09）

已从真实序章及自建原格式存档连续推进：第1关第8回合胜利、第2关第6回合胜利、第3关第13回合胜利，第4关第12回合以非终结攻击使白志超达到等级11／经验4，职业81。未注入经验、等级、HP或回合；终结攻击后已进入胜利的旧快照未用于战场转职验证。

独立存档重新载入新的Prologue并正常tick确认仍在战斗，随后SDL验证实际命令4→原职业选择→原鼠标确认→职业81转83→独立写档→重新载入运行态，验证通过（8帧状态呈现）；确认鼠标事件关联真实windowID，未绕过原事务。原格式转职前后存档与各阶段检查点保留在本会话files下的earned-promotion目录，用户原SAVE未改写。此结果取代历史“SDL转职未验证”的限制；不代表第4关之后的中间剧情及全游戏均完成。

### 后续实战发现与修正

继续第2关时发现原胜利条件33／34并非消灭全部敌军，已按原汇编恢复：分别检查是否仍有活跃存活且运行时 `+2F==1`／`+2F!=1` 的敌人。角色选择字段与原档保留随之补齐，回归包含普通敌人尚存时特殊目标已满足的胜利情况。此前超99回合的失败存档不回改回合冒充胜利，从真实战前检查点重打。

### 最低难度与友方 NPC 自动行动续查

已确认旧调度遗漏 `437B4A→4390EC→439119` 的友方 NPC 阶段，现接在玩家行动结束之后、敌方阶段之前，等待移动与攻击反馈结束再切换；不重置 NPC 的行动力。另修复自动单位单次攻击即结束、目标评分沿用旧命中公式、军队 `+DA` 阵营值写反（原 `44367C`：友军0／敌军1），以及误将职业外观类6／7当作行为6／7禁用行动。最低难度测试此前误选第二档，现通过实际难度页鼠标选择第一档并检查索引。

统一 Release／Debug 构建通过。实际 SDL 最低难度自动战役回归：娜塔莉娅第22回合胜利，4240呈现帧，完成原剧情／奖励→关间鼠标出征→第72关→自动档52重载；白志超第11回合胜利，2783呈现帧，同样完成第2关及自动档重载。上述帧数为本机加速测试结果，不是原版固定时长。测试写档隔离，未调整角色属性、经验或胜败条件；尚未还原的特殊 AI 分支仍不得视为完成。

### 自动化测试存档隔离

所有启动参数测试模式的写档均使用独占创建的临时目录，正常游戏仍写原目录；测试退出时只清理本次创建的目录。`--smoke-load` 已验证原菜单读档→隔离槽52写入→重新读取字节一致，不改写用户原槽位。

### 连续MISS与扣血专项续查（2026-11-09）

- 已定位连续MISS主因：接触攻击仍使用旧简式，遗漏 `40A4B3` 原动作／武器命中加值；现与远程共用 `408CB7` 原命中／伤害计算，不提高自定义命中下限。真实第1关前三名人物首次攻击旧命中16／4／10，补全原加值95／90／75后均为99，实际伤害67／41／37。
- 真实资源合法行动回归已重新编译并通过：从第1回合按实际可达路线移动、消耗实际AP攻击、推进敌方并执行共享随机流经验成长，第1关第8回合消灭敌军，第71关第23回合消灭敌军。**此项仅证明物理战斗和成长，不等于剧情事件、胜利结算及下一关全部完成。** 第1关结束队员仅等级1～2，不满足原转职门槛，不能假称一战即可转职。
- SDL反馈验证观察点已移至每帧绘制之后，避免仅在玩家控制阶段检查而漏掉电脑行动的闪色、浮字及HP变化。最新Release的 `--smoke-feedback` 已在SDL dummy驱动通过（1173帧），观测到原轮廓闪色、上浮伤害数字和分步HP下降；不是原版录像对照。独立回归覆盖逐步扣血、致死限幅、MISS不扣HP，以及122个原人物帧受击轮廓的透明0／阴影32／地形遮挡逐像素检查。
- 生产战役已接回合4／5、死亡／击杀者条件与type9标志事件，先排空剧情再判胜败；最新 `--smoke-clear` 实际SDL自动作战在第22回合到达原胜利报告（4538帧，测试时钟加速但不修改HP／AP／经验）。这是第一关战斗及胜利剧情闭环，**不代表奖励落盘、关间后续部署或下一关闭环**。生产关间界面仍要求完整奖励适配；缺少真实基线时明确阻止，不伪造库存／金钱。
- 命令4转职已接生产模态与原档事务，但仅对完整原档基线启用；40次真实资源转职／原档往返事务通过，尚无实际SDL点击转职的验证。新游戏无完整基线时仍禁用。自动档51／52边界已有请求与安全写盘，但新游戏缺原档完整初始化时明确拒绝；测试未覆盖或改写用户原档。
- `453238` 受击双索引位图已接生产：按单位保留AD／B1、原顺序重叠合成、DAT脚底遮罩、map5阴影映射、128宽裁剪与逐绘制交替相位；不是整个人物±4平移。独立索引字节及122帧回归通过；SDL实际攻击专项要求检测生产合成调用、非空位图并与目标／方向对应的预期合成逐字节一致，同时观测闪色、浮字与HP。尚无原版录像逐帧对照，也未证明存在额外格位推移。

### 后续功能续查（转职／远程／技能）

- 结束行动窗口已接原Home／PageUp／End／PageDown朝向映射（原扫描码47／49／4F／51→方向3／2／0／1）及 `453CE8..453D78` 原鼠标四区，确认保留新朝向，取消恢复进入窗口前朝向；面板位置按原CEL197尺寸、人物高度与特殊职业类计算。真实单位回归覆盖四区、确认与取消。方向盘使用原ICONSO28透明合成；原自动朝向仍待补齐。
- 已把原命令3人物资料接入战场独立模态，保留选人／AP，关闭回到当前命令，资料窗口冻结战斗与边缘滚动；新增 `--smoke-detail` 验证真实SDL入口与返回。命令4经反汇编确认是转职，不能当作资料。
- 远程type5现已接玩家攻击子菜单、原射程／隐藏判定、伤害与AP消耗、动作帧以及自动行动；仅执行已证明的contact0／ranged1，其他slot、反击、投射物特效与死亡奖励结算仍缺，详见 [攻击证据](analysis/ai-evidence.txt)。
- 转职已恢复原KINDSP分支、等级／经验／装备条件、原CEL210／231选择确认、训练组与成长表计算；从原EXE补解折叠的rand/srand函数，恢复原LCG及200次重播种包装。**转职提交仍未完成，也未伪造改职业成功**：全军库存、自动装备、职业运行时重建及原档原子同步仍需恢复。选择器与成长测试通过不等于生产转职可用，详见 [转职证据](analysis/promotion-evidence.txt) 与 [随机数证据](analysis/random-evidence.txt)。
- 技能已恢复学会记录菜单、条件／消耗查询与有限效果事务分析；完整目标、奖励／熟练度／经验／升级适配未闭合，生产技能命令仍禁用，详见 [技能证据](analysis/skills-evidence.txt)。
- 修正运行时字段混同：基础战术C9与当前138、回合AP12C与剩余12E、经验门槛111、标志30现分别保留，原22／42单位存档字节级往返保持通过。这些前置恢复并不代表新游戏存档或转职已完成。

### 本轮行动／攻击／自动战斗续查

- 已修正结束／取消行动后仍保留行走动作的问题：按 `453FFF` 与 `439C77` 恢复动作0、清动作游标；选中命令循环期间的动作1不被误当作永久待机。
- 普通同高接触攻击已接入玩家命令→攻击子菜单→目标确认→扣血／AP→原动作记录序列，使用原动作 `+0A` 与120ms逐帧时序，动画期间拒绝重复输入。攻击子菜单绘制使用实际可执行条目，不再固定绘一张选中图。远程、法术／范围技能、完整效果与反击尚未恢复；目前不把它们冒充普通近战执行。
- 已按 `4372EF→4390C2` 把战场“自动作战”接入我方实际行动阶段；它不是直接跳过我方回合。友方自动阶段与敌方阶段均锁定手动操作，只有敌方完成后增加回合数。实际SDL右键菜单→自动作战确认→我方行动→敌方行动→恢复我方控制回归通过。
- 战场根菜单194、系统菜单200、总检表201及报表206／207按 `474E3E` 原透明参数恢复合成，修正原零游程被涂黑；负操作码字面量仍无条件复制（包括索引0），不把整张图简单色键透明。原非黑底图逐像素参考回归通过；命令图标 `43AD50` 的opaque参数仍保持原值，不混淆两种绘制调用。
- 新游戏存档续查确认：原角色字段、到场筛选／队伍合并相关库存、首战存档边界全局值仍不足以构造兼容的完整运行时记录。第1／71关、五档难度新增拒绝与不修改原状态回归，已有22／42单位原档往返不变。未为绕过限制填零或写入自定义存档；此项仍是明确缺口。

### 敌方行动／动作贴图专项修正

- 已将原有序路线表、普通近战评分与 `40AED5→40D171→40D25A` 追击接入生产战斗，不再仅对相邻单位作接触攻击。读取原职业／动作／装备／难度资源，并修正 `441020` 隐形判定、`42CCFD` 地形判定及敌方side DA语义。第1／71关实际资源回归均验证敌方移动、可见动画及对我方扣血。
- 普通移动长帧不再累积超时连续跳过多个姿态，按 `452167` 每姿重置计时；不是人为增加插值动画。特殊行为6/7、脚本目标移动、低血量完整撤退、法术／范围技能、完整反击预览及飞行路线仍未全还原，不能称为全部战斗规则完成。详见 [AI生产集成与限制](analysis/ai-evidence.txt)。
- 已核实并修正角色帧描述符 `+6` 有符号横向偏移遗漏；原帧左右偏移同时用于单位绘制、地形／景物遮挡采样及裁剪，不再把原图一律居中。对应 `4407E1→4462B6→42B1D2/42F027`，详见 [绘图证据](analysis/render-evidence.txt)。
- 敌方横幅、敌方实际行动、下一轮我方横幅拆为独立运行阶段；敌方行动期间保留当前回合数、锁定我方输入，移动姿态逐帧呈现并同步镜头。回归不再只检查回合数增加，而检查第1／71关敌方坐标实际变化，以及全部我方行动结束后无需F10自动进入敌方行动。原生Release与SDL实跑两轮阵营循环通过：第1关1019帧敌方行动，第71关498帧；这些是本机呈现帧数，不是原动画固定时长。
- 原资源回归覆盖第1／71关122个真实动作帧（正负偏移各61例），含越界裁剪。地图背景、遮挡及原阴影回归保持通过。这是资源与反汇编级检查，尚无原版运行录像的逐像素对照；完整相邻人物遮挡仍缺。

### 上轮实际实现与限制

- 鼠标事件只更新状态，地图纹理在每帧末统一生成，避免同一帧积压多个鼠标事件时反复绘制整张地图；不是改变原操作速度或移动规则。
- 选人按 `439A0E→447E59` 切换动作1，区分运行时 `+2C` 动画游标与 `+2D` 行动状态。普通移动保留原逐格朝向／序列推进与80ms间隔，特殊类3/4/5使用120ms；传送、起降及完整遮挡仍未完成，不能称为原版动画完全一致。
- 结束行动现在有原确认框，No返回命令，Yes记录已结束状态；阵营结束与敌方行动结算后正确标记已行动，再依 `4471D4` 重置AP。此前敌方未标记完成导致后续回合AP不能恢复，现有回归已覆盖。敌方生产逻辑后续已接入普通追击与近战，详见上方专项；完整技能与特殊控制器仍缺，未用自创最近目标算法替代。
- 战场右键接入 `431DE8` 菜单，包含胜败条件、总检表、系统设置、回合结束及原确认返回路径。人物总检／定位、静态1:3地图总检、基础资料与熟练度已有实现；完整人物详情下级及条件撤离未完成。自动作战的最新集成见本页上方专项，不再显示旧未实现占位提示。
- 主菜单与战场系统菜单接入 `4354C0` 原53槽选择页。两个已有原档均可恢复全部22／42单位并字节级往返，主菜单读取已通过SDL回归；载入后的战场可重新存储。覆盖使用Windows原子替换并备份，不先删除旧档。
- **新游戏直接存档仍未完成**：原101字节关卡单位到362字节存档运行时记录的完整初始化、库存历史及部分全局转场状态未恢复；无原档基线时明确拒绝保存，禁止猜测填零生成貌似兼容的文件。分析见 [存档证据](analysis/save-evidence.txt)。因此本轮五项请求尚不能标为全部完成。
- 本节记录上轮交付时的限制；敌方行动与贴图的最新修正以上方专项为准。完整菜单证据见 [战场菜单](analysis/battle-menu-evidence.txt)、[报表](analysis/battle-reports-evidence.txt)、[AI](analysis/ai-evidence.txt)。

- 对话指向图按 `41CA6E` 与原 EXE 的 `4EA890` 表恢复：对话框在人物上方使用 CEL 237，下方使用 CEL 238，修正原来的反向选图。战场浏览按 `4307E8` 的严格十像素边缘及 `42BACD` 的半列／两行步长连续滚动；鼠标不再必须产生移动事件才能继续浏览。窗口失焦、移出画面、对话／阵营提示、命令弹窗及人物移动动画期间不触发边缘滚动。地图测试覆盖四角、边缘阈值、连续滚动和边界停止，战场 SDL 冒烟测试覆盖镜头往返。

- 从现有 Windows 版 IDA 数据库导出全部 **3727 个已识别函数**，逐个生成可编译的 C++ 桩。
- 覆盖游戏函数、已识别运行库函数和数据库中的跳板；不表示已经发现二进制中所有潜在函数。尚未覆盖 DOS 版 SUPERSP.EXE，也没有逐函数恢复类型或业务逻辑。
- 使用地址命名 `s2ne::original::fn_00401010`，原始名称、地址范围和 IDA flags 保存在 [函数清单](analysis/wsupersp-functions.tsv) 与函数注册表中。后续确认功能后可引入语义名称，地址仍作为稳定关联键。
- 桩统一接受 `X86Context&`。这是占位分析接口，**不是已还原的原始签名或 ABI**，不允许直接从原程序调用。调用时抛出带原始地址的 `UnimplementedFunction`，不会静默假装成功。
- 已替换此前无关的 9 帧 CEL 预览，当前顺序播放 CD1 DAT 中原程序指定的两段开场：640×360 / 53 帧 / 71ms，640×480 / 658 帧 / 85ms。原生 C++ FLIC 解码支持调色板、BRUN、SS2 差分等块；播放结束显示原菜单入口引用的第 19 号 PCX 背景。现已使用 BIN 第 5 项字符串表、第 11 项字形索引、第 13 项 24×24 点阵字体绘制原五项菜单：重新開始、載入進度、雙人對戰、系統設定、離開遊戲。位置 (278,286)、行距 30、选中/普通色索引 2/15 来自 0x40DEAF；上下键不循环，Enter/空格确认，鼠标命中范围 x=266..386、y=283..432。“重新開始”已接入下述新游戏流程；其余四项仅记录原处理函数地址，尚未实现。
- SDL3_mixer 继续使用现有 `MP3\SUPERSP_A02.mp3`，不提取光盘音轨；原版开场的准确音轨映射仍待核实。此前自行设计的原型菜单已删除。播放器暂未接入业务桩。
- 复刻约束：菜单内容、布局、资源编号、选中状态、输入和动作必须以原程序反汇编及原版运行结果为依据，禁止自行设计替代菜单；未确认的行为保持未实现。
- 原版开场播放器 `0x45EDAB` 请求资源类型 3、ID 21/22。`0x473CDB` 已确认该类型从 `supersp.dat` 读取；该资源已优先从用户提供的 `北方密使Windows版CD镜像.rar` 的 MDF 镜像重新提取至游戏目录的 `cd\windows-disc1\SUPERSP\` 和 `cd\windows-disc2\SUPERSP\`，两张光盘的 DAT、PRG 及 DISK.ID 均已保留，未覆盖安装版资源。SHA-256 校验确认两张 Windows 光盘的 DAT 分别与此前 DOS 光盘提取件完全一致，Windows CD1 的 PRG 与安装目录的 PRG 完全一致。后续优先使用 Windows 提取件；来源及校验记录位于 `cd\windows-manifest.csv`。现已按 `0x460348` 的头部修复逻辑恢复两段 FLIC 并接入 SDL3。运行时已直接从 `cd\windows-disc1\SUPERSP\SUPERSP.DAT` 读取并修复头部，不再依赖预先生成的 FLC。仓库外的 `cd\recovered-intro` 保留作独立参考，可通过 `tools\recover-intro.ps1` 重建；新增测试逐帧对比直接读取与参考文件的全部 711 帧，像素与时序必须一致。运行时不需要 FFmpeg。原版 16 位头字段交换及魔数/尺寸偏移已依据汇编实现，711 帧均通过本机解码测试；FFmpeg 也独立解码两段文件成功。仍需核实原程序是否覆盖头部时序，以及音轨切换、跳过和淡入淡出行为。

## 待完成列表与已知缺陷

以下是实际缺口，不代表已实现原版完整战斗。范围包含**原版每个选项下面的全部具体内容与操作流程**，不以顶层菜单出现或图标可点击作为完成标准：

- [ ] 逐项从反汇编建立完整菜单树：每个入口、子菜单、弹窗、标签、说明、数值、可用条件、确认／取消及返回路径；尚未查明的项目标注“未核实”，不得自行补造。
- [ ] 主菜单各入口的完整下级内容：重新开始、载入进度、双人对战、系统设定、离开游戏；继续核实各自所有子页与确认窗口。
- [ ] 难度页完整像素对齐：已按 `sub_40E53C` 补入四组原参数、BIN 12/15 小字库及原矩形选中色块；五档、出发及返回已有实现。完整阴影、原运行画面对照仍未核实。
- [ ] 战斗每项命令的全部下级内容：按原程序逐项核实攻击方式／目标、移动、技能、物品、人物状态、结束行动等入口实际归属及子页；未核实名称不视为既定菜单结构。
- [ ] 战场全局菜单与快捷入口：按原程序核实任务目标、系统设置、存读档、回合结束等实际入口、限制和全部下级流程。
- [ ] 为每个已核实选项记录原函数地址、资源／字符串编号、实现状态和验证方式，并对每条可达菜单路径逐项验收。首轮逐项证据与未核实分支见 [菜单审计](analysis/menu-audit.txt)，清单存在不代表下级流程已实现。

- [x] 回合切换基础流程：每轮我方／敌方使用各自原 CEL 横幅，敌方横幅结束后进入可见的逐单位行动阶段，行动全部完成才增加回合数、播放新我方横幅；横幅与敌方行动期间锁定操作，目标窗口仅首回合显示。
- [ ] 回合切换像素／时序完整对齐：原索引调色板暗带、字体双颜色效果已接，原版运行画面对照尚缺；已移除人为 700ms 停留，逐帧呈现原九个位姿；长帧不跳过入口位姿，最终帧呈现后才切换目标窗口／阵营。
- [ ] 选人：已按 `42A2BD→42A3D2` 替换生产地图的几何菱形扫描，恢复整数鼠标坐标、镜头奇偶、四三角及高度20至0的逐级命中／提前拒绝；另按设置字节 `4F7D07==1` 接入 `42A72F` 的跳过遮挡拒绝分支；完整人物像素命中、景物遮挡、不可操作单位反馈及自动选择规则仍缺。
- [ ] 命令菜单：完整原可用条件、技能／物品／状态命令、说明与禁用效果。
- [x] 移除自画黄／红／蓝菱形及白色路径，普通可达区域改用 DAT 第5项 ICONSO 原第35号 64×32 索引图，并在人物之前绘制；修复人物脚点未扣除地形高度及增援显示固定首回合的问题。
- [ ] 移动绘图完整恢复：原三类范围 DAT36／37／38、三类可达鼠标与不可达鼠标、普通逐格移动、逐格即时镜头已接；人物与地形／景物的分层遮挡、特殊移动动作及原完整选中合成仍缺。不添加自行绘制的路径线；478484 的玩家分支直接复制地面缓冲，不能将敌方／调试分支的范围脉冲无条件套到玩家。
- [ ] 战斗结束行动：原朝向选择、确认、自动耗尽处理；F10 属开发快捷键。已独立恢复其依赖 `478AF3` 的1至79环格距离（无命中返回80，含同格情况）与 `45DA52` 的装备可用动作最大射程查询；攻击结算现按 `45CE57` 的原朝向函数更新攻击者方向；这些查询尚未接入完整结束行动窗口，不代表结束流程已完成。
- [ ] 伤害与技能：武器射程、职业／状态／装备修正、连续攻击概率与执行、反击、原随机数与难度修正。普通等高接触攻击已接入命中／伤害基础及408455朝向子集；不代表完整武器、职业、状态、技能、远程或反击流程已恢复。44132F武器门控、44196C武器／动作射程以及406E7C／407053射线／环形目标格枚举已独立还原并回归（原边界、特殊点、高差截断及顺序），441020／441EFD隐形可见性、446E53目标筛选、43AE24攻击动作槽与行动力条件也已组合为独立查询并测试，仍未接生产攻击菜单与远程实战，不等于这些功能已恢复。408CB7的已解码输入命中／物理伤害算术、40A42D条件修正、40A4B3命中装备修正、407B83／407C35随机倍率与4080E7敌侧难度顺序已独立实现并测试；40A622修正算术也已测试；生产普通接触攻击现按408455的朝向分类加入命中与伤害偏移，并按45CE57先更新攻击者朝向（相克、动作倍率、分段压缩、综合值与状态顺序）；其中45CFBB输入已纠正为综合值而非经验，并独立恢复四分量、原400常数、能力位排除、装备18h及种别倍率的算术，已提供原运行态字节偏移解码、4094C4行动经验算术及43D93B动作状态尾部11分支的地址／参数查询回归（442CD5限时状态、442E96双属性状态、443093单属性状态及442828属性重算已独立恢复字节写入、仅首次修改属性、延长／永久哨兵及按活动标志从基值重算；442DC5按原PE两张掩码表仅移除首项并重算也已独立回归；46044D状态倒计时前段及动作尾部→原字节状态写入→到期恢复组合也已回归，尚未接生产；其他状态管理、回合恢复／伤害与此前装备／能力proc尚未实现），45D52F运行态百分比调整（13处基值／当前值、原限幅与负值分支）也已独立回归，尚未接原缓存生命周期、完整运行态数据及实战；408CB7中的429F34已纠正为地形高度而非随机数，不能把算术回归视为完整战斗恢复。409BBA正向／反向预估已与实算分离：保留先除1000再乘100／除125、反向伤害35%、反向命中45%、命中额外十分一、固定伤害与装备30三倍及免疫归零顺序；仅独立回归，尚未接确认窗口。4097EB已纠正为连击概率而非次数／AP，409A2D反击候选与408455方向模式、42C8D1地形三字节修正亦仅独立实现，完整执行尚缺。409060反向实算另有负方向修正、独立命中／伤害随机衰减、固定伤害和装备30三倍顺序，已独立测试；没有将预估35%／45%常数误用于实算，仍未接反击动画与结算。43E160／43E1FC的武器效果覆盖、原效果资源与43E36B整数斜率初始方向已有独立解码回归，43E36B源偏移、偏移后二次轴线分类、横竖32／斜16像素逐帧路径也已独立测试；未接生产弹道背景恢复与合成。另从原EXE折叠的47FD8E确认CRT乘数为41C64E6Dh，独立Random已恢复4731D8转置／600次交换与200抽重播种、percent查表；生产Battle近战已接原Random及百分表／余数分流，并校正最小伤害条件抽样，120次交替伤害回归跨越200抽重播种边界；仍未恢复跨模式共享生命周期及完整全局调用序列。
- [ ] 敌方 AI：目标评分、主动移动、技能选择；目前只有接敌普通攻击子集。
- [ ] 事件与进度：援军、胜败触发、战后剧情、下一关、存读档。
- [ ] 显示：地形与单位前后遮挡、阴影、背景层、原完整文字阴影与分页。人物已接42AFBA→42F027→42F2A7的12轮前景地面像素裁剪，并用真实第1关轮廓交集回归；另接42F2A7物件类型／事件标志门控及42F4D7景物深度／特殊点4000h抑制，修正物件裁剪纵向16像素偏差和42E01E景物底图高度；原资源回归覆盖15个事件控制物件案例与353034个景物交集像素（42AFBA仅在人物所在地面编号>4时进入景物裁剪）；42E89C静态物件随事件flags重绘且不改变镜头，475BED的索引32/33已按原表6/4混合底图而非画实体色块，42EA09景物亦使用475BED，42EAAF末行外景物补绘亦已接入，42EB5E景物后物件工作副本裁剪／再合成已接入，九张真实地图独立索引重放覆盖157575/22429个阴影像素、178次非零深度再合成并比较顶部／底部视窗；零深度4000h门控已按汇编实现，但这些真实地图尚未触发该分支。42FD9F地图背景／前景group8原图已按header有符号偏移及不透明／透明顺序接入，含raw/RLE与literal零语义，九图六镜头独立重放通过。邻人及移动偏移中的完整遮挡仍缺，不能视为完整合成。普通对话已接41D01C百分秒逐字、D/RD延时、当前页快显和原CEL236分页提示；选择prompt及目标正文已改423AC5原28像素格换行，4239AD/423AC5及普通对话已接原小字体分支：固定字格保持原24/16，wrapped格改20，普通正文进距16/8；普通正文41D1DD固定style2，因此W第二色不应额外产生阴影；仍缺完整输入队列冲刷、其余style0/1窗口的全部字体／阴影分支。
- [ ] 难度界面逐像素核对：四项参数及选中色块已依汇编补入；40E31E 使用独立 DAT20 底图，40E8DB显示时切换全局调色板；透明操作码／重复零保持底图，literal 零按原写入。五档背景保留及悬停不改选择有回归覆盖；原调色板过渡及用户所见变暗效果尚需原版运行对照，不把静态资源切换认作暗化完全恢复。
- [ ] 音频：按用户前述要求暂缓，不将设备播放推进作为实际出声证据。

## 构建与验证

日常开发直接打开原生 [S2NE.sln](S2NE.sln)，方案内**只有一个 C++ 工程** [S2NE.vcxproj](S2NE.vcxproj)，不需要 CMake 配置或生成。支持 Debug／Release、Windows x64、C++20，使用 Visual Studio 2026 的 v145 工具集。主程序和生成的函数桩直接编入同一工程，没有额外的静态库、ALL_BUILD、ZERO_CHECK 或测试工程。

SDL3／SDL3_mixer 通过原生 vcpkg MSBuild 集成读取 `D:\project\vcpkg` 的现有 x64-windows 依赖，并在构建后复制所需 DLL；可用 MSBuild 属性 `VcpkgRoot` 覆盖路径。F5 工作目录设为游戏资源目录（工程上一级）。源码、头文件、生成代码和测试源文件分别分组，测试文件只作浏览，不编入游戏。

在 Visual Studio Developer PowerShell 的工程目录执行：

```powershell
msbuild .\S2NE.sln /p:Configuration=Release /p:Platform=x64 /m
.\run-opening.cmd --verify
```

原生方案输出至 `out\x64\Release`（Debug 对应 `out\x64\Debug`），中间文件独立保存在 `out\obj`。旧 `build` 目录和 [CMakeLists.txt](CMakeLists.txt) 保留作为既有回归测试的兼容入口，不是日常 VS 工程入口；不删除现有测试或旧构建产物，也不将它们加入新方案。

测试会将清单与全部桩逐一核对，检查地址顺序、原始名称、范围、flags，以及每个桩的异常行为；另运行原始 CEL 资源解码回归测试。

2026-11-08 本轮复核：从磁盘现状继续，未重放旧差异或清理会话数据。CMake Tools 在当前父级工作区报告“无法配置项目”，无诊断；改用现有 `build/S2NE.slnx` 的 MSBuild Release/x64 构建，以及直接执行对应的七项测试程序，均通过。另用 SDL dummy 音频驱动跑通 `--smoke-new-game`、`--smoke-battle`、`--smoke-battle-alternate`；两条战斗入口均覆盖选人→命令→移动及两轮阵营切换。此验证不等于原版画面或战斗规则已完整一致，也不验证实际出声。

可以直接启动 `out\x64\Release\envoy-opening.exe`；未指定资源路径且当前目录无资源时，将向可执行文件的上级目录寻找原资源。也可运行 [run-opening.cmd](run-opening.cmd)：动画结束进入原菜单，Esc 可提前进入菜单（开发阶段便捷入口，未宣称与原跳过逻辑一致）；上下键选择、Enter/空格确认，或鼠标悬停后左键确认。关闭窗口退出。确认结果及尚未实现的原函数地址输出到控制台。

从游戏资源目录运行 `S2NE\out\x64\Release\envoy-opening.exe --smoke` 可测试真实音频设备播放和菜单键盘选中/确认；成功时输出音频播放位置已推进。此检查不能代替人工确认扬声器实际出声。

## 重新生成

[tools/export_functions.idc](tools/export_functions.idc) 可通过 IDA 原生 IDC 导出当前数据库中的函数，无需 IDAPython。脚本会在数据库旁生成 `.functions.tsv`，人工审阅后更新仓库的函数清单。

```powershell
.\tools\generate-stubs.ps1
```

[生成代码](src/generated/) 会整体覆盖，暂不在这里直接实现业务逻辑；确定签名和功能后应调整生成策略或移出对应实现，避免覆盖人工修改。

## 来源

目标：WSUPERSP.EXE，x86 PE，IDA 自动分析识别 Watcom 运行库。
SHA-256：`58A9808E090DE51F56059F965D313BB274A08B82A3526E3C8991C4D3F978AAD2`。
原分析数据库及汇编导出已迁移到 `analysis\ida\`，其中数据库为 `opening.idb`。该目录仅作本地分析缓存，由 Git 忽略；仓库不包含游戏二进制、资源、IDA 授权文件或反编译代码。旧工程目录已移除。

## 声音排查

`test-audio.cmd` 会打印 SDL 音频后端、系统播放设备及实际非静音混音样本数；不再仅凭播放时间推进判断声音正常。当前验证环境的 WASAPI 唯一设备为“远程音频”，非静音样本检测通过，但不能证明远程客户端扬声器已出声。远程桌面应设置“远程音频播放：在此计算机上播放”，并检查客户端及 Windows 音量混合器中程序的静音/音量设置；程序不会擅自修改系统音量。

可设置环境变量 `S2NE_AUDIO_DEVICE` 为目标设备名的一部分以指定输出。不存在的设备会明确报错，不会悄悄使用其他输出。默认继续跟随系统默认播放设备，轨道与混音器增益显式设为 1.0。
## 新游戏进度

“重新開始”已进入原难度选择页，使用 DAT 第 7 项内的第 4/3 张原始图形，以及原字库、字符串 215..221。五档难度默认第三档，支持上下键、鼠标选择，Esc/鼠标右键返回，Enter/空格或点击“出發”确认，保存难度值 1..5，对应原 `byte_4F7CEC`。现按原入口补绘底部四组说明：ＡＩ強度（5/25/50/80/100%）、簡單規則（首档是，其余否）、敵人強度（-20/-10/0/+10/+20%）、經驗點數（300/200/100/100/100%）。使用原 BIN 12 的 16×16 汉字与 BIN 15 的 8×15 ASCII，而非缩放大字库；选中色块取原 `sub_475970` 的 x=249..266、y=266+28×档位..290+28×档位、索引60。阴影与原运行画面对照尚缺，不宣称像素级完整恢复。

确认后读取原 `STAGESP` 阶段表和原 `SUPERSP.BIN` 事件表：第 140 阶段的 9 行对话 → 原两项选择 → 对应旗标分支对话 → 第 1 或第 71 阶段的关卡说明 → 首回合对话 → 战场初始画面。选择支持上下键及鼠标（先选中，再点击确认），Enter/空格推进。修复难度鼠标事件落入主菜单处理而覆盖难度页的问题，以及事件数据长度漏计 4 字节、部分头像采用未压缩格式的问题。

地图、地形、景物及人物静止帧直接解码原 PRG 资源。**战斗操作已改为原汇编支持的模态命令流程，但仍非完整战斗还原**：左键选择我方单位 → 原横向图标命令栏 → 移动命令 → 悬停预览可达路径、左键确认目的地 → 返回命令栏 → 攻击子菜单 → 普通近战目标选择/确认。命令栏之外点击不再直接移动或攻击。右键/Esc 退出当前目标/移动模式；命令栏下可撤销尚未攻击的移动并恢复行动点。左右键选命令，Enter/空格确认，Tab 切换攻击目标；非命令栏模式方向键滚屏。Enter 不再跳过整个我方回合。F10 是明确保留的开发用结束阵营快捷键，并非原版键位声明。窗口标题显示当前阶段和操作结果。尚未实现的原命令不执行替代动作；完整命令可用性条件、转向/情报/特殊命令、结束行动确认框、完整敌方 AI 和后续事件仍待恢复。

`--smoke-new-game` 检查第二行对话；`--smoke-battle` 和 `--smoke-battle-alternate` 通过 SDL 事件走完两条流程，同时回归难度页鼠标选择及“出發”按钮。资源测试覆盖五档难度、两个分支、三个地图和战场渲染/滚动。实现位于 [original_events.hpp](src/original_events.hpp)、[prologue.hpp](src/prologue.hpp) 和 [original_map.hpp](src/original_map.hpp)。音频仍按要求暂缓处理。

没有默认音频设备时会记录警告并继续无声运行，不再阻断新游戏界面。
### Battle interaction and original evidence

Left click opens the selected behavior-0 unit's horizontal original CEL command bar. Select Move, preview an AP-budgeted route, and click a reachable destination. Select Attack, select the ordinary attack submenu, then confirm an eligible target with left click or Enter/Space (Tab cycles targets). Esc/right click returns to the command bar; cancelling there restores saved position/AP if movement has not been committed by an attack. Direct empty-cell/enemy shortcuts are removed. Enter/Space confirms the current modal operation; **F10 is a development-only side-finish shortcut**. Original archives are never modified.

- `0x439C77`: modal unit-action loop; command `0xA0` enters `0x452069`, `0x9E` opens the attack submenu, `0xA6` calls the original action-finish dialog. `0x439E6D` restores saved coordinates/AP when rolling back movement.
- `0x43AE24` copies availability masks from `0x4EAC10` (`00 00 00 80`, `00 00 00 40`, `00 00 00 20`, `00 00 00 10`, `00 00 00 01`, `00 00 00 08`), then packs enabled entries; icon IDs from `0x4EAC28` are `158/160/162/164/188/166`, with slot order preserved. `0x43AA52` places horizontal 32-pixel slots and clamps the bar on screen. `0x4262EC` joins the 142 DAT icons to CEL menu sprites; rendering now omits disabled slots and uses the selected CEL variant (+1), not invented labels/buttons.
- `0x452069`, `0x452B64`, `0x476940`: movement is a separate reachable-destination/path selection mode with cancellation, not unrestricted adjacent click teleporting. Dijkstra routing currently uses the existing restored terrain/AP costs; reachability and path outlines are diagnostic overlays rather than original pixel-identical effects.
- `0x43B788`: attack target is selected separately, with left-click/Enter/Space confirmation, right-click/Esc cancellation, and Tab target cycling before `0x43BF9F` resolves the attack.
- `0x42A2BD/0x42A3D2`, `0x42C61A`: mouse surface resolution accounts for elevated terrain. The current diamond-surface picker follows visible row order; the complete original sprite-mask/occlusion picker is still pending.

- `sub_4443B3`: signed 101-byte unit records; HP +27, SP +33, strikeback +35, attack +37, defense +39, fencing +41, tactics +43; action-point override +47, otherwise KINDSP class +21. `sub_4274B5` separates enemy behavior 2/3/8; behavior 1 is allied AI.
- `sub_448CD6`, `sub_42A1B2`, `sub_477BEB`: staggered neighbors, interior boundaries, ordinary climb <=3/descent <=4, and terrain cost plus config EBh times ascent. Special-cell flag 8000h blocks movement.
- `sub_42DD41`, `sub_42C8D1`: PRG entry 7 supplies ground/object attributes; disc-1 DAT entry 6 supplies per-kind signed terrain costs. This replaces the former invented ground-ID cutoff, blanket object blocking and equal-height movement restriction. KINDSP movement categories are read from the original table. Flying/special modes fail closed pending their full trace.
- `sub_476707`: movement reserves config E8h and caps search allowance at 110. `sub_439C77` ordinary attacks spend config EEh. `sub_4471D4`, called between sides by `sub_40D7D4`, restores base action points plus limited carry and clears the current per-action flag byte (the full runtime2Dh gate and all four status bytes remain incomplete). `439C77` contact attacks now charge EEh, subtract four for ability118h:20h, add F3h on repeated attacks unless ability116h:80h suppresses it, set status34h:02h and retain the 16-bit write. Production command re-entry no longer imposes the synthetic one-attack limit when enough AP remains. `43B096` reads EBh/F0h dwords then shifts by 24, so attack-menu AP thresholds use the actual EEh/F3h bytes too. `440CC6` movement-command availability is gated by AP/class, not by whether an unoccupied destination was found; surrounded actors retain that AP-qualified command.
- `sub_408CB7`, `sub_40A4B3`: base hit input is clamped to 1..100 before multiplying by 12/10. `sub_40A8A7` supplies the random divisor; `sub_40A622` supplies piecewise damage and minimum-damage distribution. Equal-height base melee is implemented, not all final modifiers.

**Remaining concrete gaps:** the original availability-bit construction and all command submenus are not fully ported: all six original top-level icons are displayed, but direction/information/special commands fail explicitly without mutating gameplay. Only ordinary equal-height adjacent melee is currently executable; weapon-specific ranges, attack variants, finish/facing dialog, automatic no-AP termination and inter-unit state flags need further restoration.  `sub_40B0BA` scores movement destinations and targets, with `sub_40C152` handling additional AI actions. That scored movement/skill AI is not reproduced: enemies currently respond only at equal-height adjacent contact, in record order. ZOC search-state handling, flying/special traversal, facing/class/status/equipment/skill modifiers, counterattacks, full difficulty adjustments, reinforcements, battle events and victory/campaign continuation remain pending. This is not full restored combat.

`battle-test` / `original-battle-interaction` strictly replays real spawn-to-contact paths for **both stages 1 and 71**, spending movement budgets across turns, verifies enemy HP loss, player HP loss from enemy contact response, and action-point reset. 生产近战已切换原 CRT 递推、转置／600次交换百分表与每200抽时间重播种，区分 `47328B` 百分表和 `4732ED` 普通余数抽样；最小伤害仅在需要时抽样。固定时间种子使回归可重复。当前随机对象仍属于战斗实例，尚未恢复跨菜单／剧情／战役共享生命周期及全部动作消费顺序。 Both SDL battle smoke branches inject unit selection, original Move-command clicks, destination confirmation and F10, checking modal state, final coordinates and turn advancement. Synthetic UI regressions cover command barriers, multi-step path preview/commit, invalid destinations, rollback, submenu/target cancellation, confirmation and elevated hit testing. Existing synthetic picking/occupancy/damage/death regressions remain.

`4443B3 → 45894C` 的已学能力熟练值初始化现从原 PRG 第2项的分组目录和37字节能力记录读取，保留 `FBh` 位表与 `E3h` 熟练值的区别；`44CDB7` 装备增量累计至 `13Ah/13Ch/13Eh/140h` 的运行态字段也已保存，两条分支均有原资源回归。`40DDF0 → 45D41C/45D4C2` 的战力差百分比计算已还原并接入简报结束、首次阵营横幅之前的生产入口，包括步数为0时必须在难度项之前退出；仅调整原敌方上下文的有效单位，保留原有百分比截断、16位回绕和非对称上下限。战力读取由当前单位字段映射至原 `45CFBB` 的 mode0/2；mode1所需 `132h` 结算快照与缓存模式仍明确拒绝，不能将此等同于完整难度或战斗恢复。`45878C` 条件性动作熟练值初始化已接入当前新游戏单人载入：按 `4274B5` 分界而非敌对判断选择上下文，关卡类型3跳过，按原配置／难度／职业系数／等级初始化六项武器与特殊动作熟练下限；双人模式的全局退出条件尚无生产入口可接。

Resources additionally needed: original `KINDSP.BIN` and disc-1 `SUPERSP.DAT` under `cd\windows-disc1\SUPERSP` (fallback `cd\disc1\SUPERSP`). No replacement data is generated.

### 原版战斗开场与任务目标

回合入口遵循 `sub_40D7D4`：我方横幅 → 仅首次显示任务目标 → 玩家操作 → 敌方横幅 → 基础敌方结算 → 回合数增加 → 新我方横幅。`sub_438719` 使用合并图标表 150（我方）／149（敌方）、152／153，在原坐标绘制阵营标志、左右条带和实际 TURN 数字；修正第8帧提前截断使标志未落定的问题。入口位移采用原百分之一秒步长，移除原先人为增加的 700ms 停留；修复按墙钟直接跳过位姿／最终帧未呈现的问题。每次重绘最多推进一个位姿，九个位姿全部呈现后退出，按4389A9→473111的EDX=1每位姿至少等待10ms，九位姿最低呈现预算90ms，低帧率下总时长随绘制开销增长（不是原版实际运行时长声明）；动画期间不接受战斗操作，也不允许 F10 重入结算。随后使用 `sub_43260D` 的原 CEL207 面板、原字符串标题、关卡名称及胜败条件文字显示任务目标，确认后才进入操作。胜败条件按 `sub_427016` 的实际段地址读取（失败条件前包含两组索引数组），不生成替代目标。

当前复原的是首次进入战斗的提示与主要目标文本。每次阵营切换的提示及背景调色板映射已接；原窗口的部队统计、完整文字特效及逐帧视觉核对仍待进一步复原；不能宣称已恢复全部原版细节。两条路线的测试检查动画变化、动画输入锁、目标数据非空与确认后进入战斗。

### Original terrain transparency correction

`sub_42E89C -> sub_4753A4` passes arg0=0: palette index 0 is transparent. Ground blits preserve existing pixels instead of opaque diamond padding erasing previously drawn rows. No sprites, maps or resources were replaced. CTest `original-map-transparency` covers maps 1/62/71.

## 本轮画面反馈复核（2026-11-08）

- 难度背景进一步追到40E8DB：40DEAF加载DAT19／20，但难度呈现经472EF1切回SUPERSP.PAL全局调色板，并经47F5D2原步长淡入到60。现按此显示DAT20、说明和面板，不沿用DAT19的PCX调色板，也不使用任意RGB压暗。完整原运行画面对照仍未完成。证据见 [difficulty-evidence.txt](analysis/difficulty-evidence.txt)。
- 原 ICONSO 26 的 64×32 轮廓已用于人物选择反馈；不是自行绘制几何框。完整人物像素命中、调色板脉冲及遮挡仍缺。
- 战斗不再仅输入时重绘；待机帧按原 PRG 表及 190ms 时钟推进。但外观 1..135 的原待机表只有一帧，仅外观136有两帧，不能虚构全员呼吸动画。普通移动已接逐格80ms方向帧呈现，完整动作、特殊移动与遮挡仍缺。证据见 [unit-animation-evidence.txt](analysis/unit-animation-evidence.txt)。
- 按 43AA52 恢复 CEL143 上／下沿面板，并区分命令图标的原不透明零与普通透明绘图。图标自身的黑底存在于原索引资源中，不能擅自换色；本轮按4571EB/4573A1/457B08补入原头像、姓名、职业等级、经验、HP/MP/ST/AP上下限、缺量条和六项装备展开数值；完整文字阴影及特殊经理缩放仍未恢复。投影和原版动态画面对照仍缺，不认定菜单黑底问题已完全解决。
- 本轮停写后 Release/x64 构建、七项测试程序及第1／71关 SDL 冒烟均通过。CMake Tools 仍无法配置父级工作区，使用现有 VS 解决方案构建和直接执行测试程序。音频恢复继续暂缓。

本轮对话核实：按 444509 从原人物记录 +32h 读取方向低两位，并按 444BCF 的零终止序列选择方向；普通对话按 41CA6E 的屏幕 Y 分支放置上下框和原箭头，选项框仍固定 (6,125)。头像不擅自镜像。原 449011 镜头调整完整行为仍未移植，故不宣称最终位置逐像素一致。证据见 [dialogue-evidence.txt](analysis/dialogue-evidence.txt)。
已更正零基指针与资源序号的对应：人物前／后光标为 DAT27/28（64×137），移动范围三类别为 DAT36/37/38（64×32）。范围按4780C2的剩余AP、状态附加消耗及443235的已学能力字表分类，键鼠入口均同步上下文。472D78的三组调色板循环已接入对话／战斗持续重绘；循环色3..31已核对不存在重复RGB。普通逐格行走已接，特殊移动已按452450区分每节点120ms（普通452167为80ms）及class5不回写目的AP，但特殊绘图／攻击动画、景物完整遮挡、物件交互仍未恢复。426EFC的四图链解析、4296BF的type12坐标登记、42F8B7四图有效性检查及42FBC6当前物件初始化合成已接；真实map1物件803的非零／透明像素、无效链和map71无type12格均有回归。无效四图链已按42F8B7清运行态物件（不改资源），重载恢复原ID并有回归；42F8B7合法链的64×96索引背景备份及42D200不透明回填／清格原语已实现，map1物件803移除后逐像素恢复底图、重复移除拒绝均通过回归；事件flags重绘保持已移除格；初入Battle同步42F8B7清格，避免渲染已清但寻路仍被旧物件阻挡。455324随机取物／原弹窗／背包及移动后触发入口仍缺。42FE12已查明是46243A清单中的随机物件名称选择且跳过type12，并非流水定时器，不据此编造物件帧率。证据见 [selection-evidence.txt](analysis/selection-evidence.txt) 和 [terrain-evidence.txt](analysis/terrain-evidence.txt)。

本轮集成事实（未完成项仍以待完成列表为准）：
- 438719横幅暗带改用4637DA/463165原索引最近色表5/6，TURN与数字双颜色；9个位姿每姿至少10ms（4389A9→473111，非移动4523CC的80ms），操作锁和首回合目标顺序已回归。
- 425FE5将全局palette索引0清零，修正地图无瓦片区域错误使用磁盘青绿色。原资源不改。
- 449011/42ADC7/42B461对话人物有效标志、左上投影、上下框箭头及有条件镜头目标已接；对话持续待机／palette动态重绘已接。
- 4571EB/4573A1/457B08人物小状态加入原小头像、姓名、职业等级、经验、HP/MP/ST/AP上下限、缺量条和六项装备展开数值。战斗AP消费和阵营恢复同步12Eh显示字段；经理E2特殊缩放与完整文字阴影仍缺。
- 普通移动逐格80ms呈现与面向已接，期间不显示命令栏、不接受确认/取消/结束阵营。仍缺完整动作序列、特殊移动分支、完整遮挡，不能称行走动画已全部复刻。
- 主菜单系统设定与退出原确认页已接；完整存读档、技能、远程、反击、敌方AI、胜败事件和后续战役仍未完成。
本次续查：普通命令图标已接43AA52→424D47原table6双条阴影；普通移动逐格镜头按452167→42B37A(mode1)立即定位，AP12Eh显示同步及奇偶行面向已加入回归。476AFE/478484的范围合成存在玩家/敌方条件，不能用无条件颜色脉冲代替原逻辑；完整预览合成与遮挡仍未完成。本次测试不构成原版视觉一致性结论，也不代表完整游戏已实现。
续查更正：4389A9传EDX=1给473111，横幅每姿最小等待是10ms；80ms仅为普通移动4523CC，先前混用已修正。430516的idle/movement人物简易状态已接；43AA52面板上下切换按格左上Y及184/272滞回阈值保留原侧。本轮Release统一构建、11项独立测试、资源verify和三个SDL入口回归通过；这些证据不代替原版逐帧视觉对照。全部技能、远程/反击、评分AI、胜败事件后续、存读档及完整菜单仍未实现。
移动目标鼠标已按452BFD/476A23/438DF9/4753A4接原ICONSO资源：不可达slot15、三类别slot17/19/18，真实鼠标位置零透明绘制，仅此模态隐藏系统指针；测试逐像素验证类别0。退出确认也补入431880原阴影并有边界回归。原完整菜单树、特殊移动/遮挡、技能远程反击、完整AI及后续战役仍未完成。
本轮继续核实（非完整交付）：小状态六数值已重新对照 4446CA/457B08，原字段顺序正确；补原小字垫边、经验标签阴影、难度选中字阴影及大字垫边。对话头像按实际宽居中，正文恢复 ASCII16／Big5 24 的字进距；镜头恢复四次可见检查提前结束，区分前置mode2与循环mode0。人物绘制补33h的20/80隐藏标志。生产地图选格改为42A2BD/42A3D2整数三角与高度探测，不再用最后覆盖的几何菱形。完整遮挡、逐字时序、其他命令与战役缺口仍以上表为准。

续查：设置页 Big5 原垫边为 Y+0，ASCII 为 Y+1，已区分修正并加入标题首字逐像素回归；43183B确认窗改用原大小字垫边。41D230/41D2B2正文分页按绘字后判定，恢复非空行的 `*` 换行控制，不再直接丢弃。42A72F设置分支已传到实际SDL选格路径；暂未将该字节冒充完整地形遮挡开关，其余遮挡缓冲重建仍未恢复。
