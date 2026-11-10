# S2NE — 验证过程与历史记录

本文件保存从 README 迁出的验证方法、测试证据与历史流程记录。条目按迁移前原文保留，其中阶段性缺口可能已被后续实现替代；当前进度与限制以 [PROGRESS.md](PROGRESS.md) 为准。资源阶段编号不等于连续战斗序号，强化流程不计正常属性通关。

自动化启动测试使用隔离写档目录，避免覆盖用户存档。算法测试、实际游戏流程测试和原版画面对照应分别记录，不互相替代。

## 2026-11-10 画面续作：411587原调色板阶梯／撤离普通移动姿态／到达图元

**有限新增已验收，不是本请求全部收口；传送生产dispatcher、特殊移动遮罩及城镇正常探索链仍未实现。未改PROGRESS。**

### 411587双buffer与淡入的实际证据

重新跟到`474A7C`（asm200601）：它只是指定源／目标buffer的全屏memcpy；`474827`仅选择当前绘图buffer，**两者本身没有淡入帧**。411587确实先2→1，再在1画商品静态页，再1→0后画选中动态层。已有缓存对应这一恢复顺序；本轮新增真正淡入调用`4118CB ->472EF1(palette,0,-4)`，不从buffer名字猜动画。

`472EF1`循环DAC强度0,4,...,60（16次），各强度左移2后传`47F5D2`；后者逐通道`(DAC<<2)*intensity/255`整数除法。参数-4意味着步长4且**不调用delay_**，末尾停留60而非自行补63。本轮生产shop启用presentation timing，16次实际present才解输入锁；无固定毫秒猜测，不交易不吞确认。原DAC通道从渲染值反解后执行该整数乘除；原图、缓存及最终商品状态保留。专项与SDL检查16阶／首帧黑／Escape不能打断／SAVE无改。

仍未声称原OS调色板更新与SDL每刷新一帧完全同频；实际SDL以present作为可验证显示边界，原版没有为负参数规定帧延时。

### completion7移动姿态新增与严格边界

原`42B461`只在action_state==4时查询`440F56`。class0/1/2走`4462B6`原PRG角色bitmap，不是新轨迹；**46234F是移镜头后清present，不让角色沿虚构路线走出地图**。本轮开放该普通移动姿态子集，在撤离绘制保留原movement_frame而非恢复默认idle；仍沿既有镜头逐帧ack、原次序、input/save锁、异常基线回滚。受控stage42原表records14/17加入一名class<3的state4/frame1人物，专项／SDL通过；仍不是未经修改原表的自然剧情入口。

class3=`448B99`，class4=`45CA1D ->45CADA`，class5=`45CAA0 ->45CADA`仍拒绝。已核class4的原共享slots23/24（实际ICONSO index22/23）按全局503464交替，class5从4F70FC方向slots取图，45CADA还必须先448B99重建脚下遮罩并叠原角色AD；不能只画图标或只淡出冒充。尚缺448B99中4F70B0脚印mask与5045B8索引变换的共享background合成，及45CADA mode3/4投影叠加，资源存在，并非资源丢失。没有据此伪称这些分支完成。

### 传送到达十帧：确证与已合并图元

准确原调用是`45DB10 ->454ED5(army,role,194,1)`，**不是455048消失**。454ED5每帧从原AD备份恢复，再424873将非透明原PRG像素向palette194混合，权重100,90,...,10，十帧，每次473192等4百分秒后present，结束恢复AD。本轮`OriginalMap::begin_teleport_arrival/tick_teleport_arrival`新增这个真实PRG角色图元：40ms且每帧需ack，十帧后恢复普通绘制；不改角色坐标／库存／存档。`original_teleport_visual_test`验证原角色图变化、十帧、未ack不前进及39/40ms边界。

**该图元尚未连接battle::settle_teleport_pads生产事务**：现侧末传送同步改坐标并立即reset_side；正确接线还需在45DB10的源隐藏→目标镜头→十帧→恢复action_state／4483CB／4465B4完成前挂起侧切换。未以新增未调用API或算法测试声称生产传送完成。起降45DC37还依赖476707路表与status4／方向／12C/12E状态保存，未实现，未套普通移动替代。

### 旅店城镇探索具体原调用与未实现点

410EEE option1调用`414D5B`（不是恢复），入口`415000(1)`从原worldmap偏移4F6170/6174拷640x480→buffer1，`415084(0)`按原城镇表画current/destination标记；其资源由410C86前置加载到4F6160/4F618C，不能用当前战场或旅店背景代替。循环415415读取输入；4157C6更新原world视口并按4F7CF0计时；点击`414F5D`调用下级再重画；退出恢复音乐4。完整正常探索入口链尚未上线，本轮没有删除其原表以作伪自然fixture。

其他旅店未接入口精确为：option5=`44D439(army,0,0,1)`再441143(2)，option7=`44D439(army,1,0,1)`，option8=`43183B(string51,300,200)`确认后返回11。已有2/3/4/6/9/10保留。以上不是“缺资源”而是原worldmap前置解码／视口／下级及军团管理状态机尚未移植；本轮并未完成用户要求的未经删表正常探索链。

### 验收与发布

`out/visual-{Debug,Release}-build.log`均0，Release最后。九个重新编译专项：`original_{events,equipment,shop_menu,inn,save_bridge,ai_special,death_continuation,evacuation,teleport_visual}-visual{,-build}.log`全通过，events完整执行。实际SDL `visual-sdl-{shop,evacuation,auto,auto-alternate}.log`全0；shop含新16阶输入锁，evacuation含受控state4普通移动姿态。共享CDS打包`visual-package.log`保留源资源；独立无DAT解包`out/visual-package-check`，不带两个txt，`visual-package-{verify,smoke-auto,smoke-battle-alternate}.log`。最终根EXE SHA256=`C2BDC2DB2BE07D4087217D0946D9DDA0061221705C71D6608B64B359C8693804`，ZIP=`189401583`字节，根EXE／DLL同步最终Release。没有扩技能或重跑全线。全部验收命令已退出，停写前检查无本工程自启辅助残留。

## 2026-11-10 本轮收口：411587计时缓存与非空completion7挂起恢复

**新增生产实现及回归，不称六类全完成。以下替代旧章“非空completion7未上线”的边界；不是自然全线通关证明。**

### 411587原计时／光标／面板缓存

`47308B`取本地时钟minute/second/hundredth，忽略小时；生产使用相同的每小时360000百分秒计数。`473111(start,10)`和`473192`实际比较abs(start-now)，不是模差；小时回绕不人为补正。鼠标按住水平左右track按10百分秒重复，释放停止，重复只导航不交易。键盘左右和进入／返回商品页请求`438B8F`的原坐标`224+row*54,356`，SDL从640x480渲染坐标转换窗口坐标后warp；鼠标track重复不warp出track。使用已确证原图147/146，未画几何箭头。

报价上层固定面板按首行、类别、钱和库存缓存；选中行／名字／help／框／拇指仍从缓存恢复后绘制，交易变化使缓存失效。专项验证缓存重画相等、warp、9/10百分秒及释放、小时回绕。**这只收口CPU面板缓存与计时／warp；尚未复刻474A7C/474827原双buffer淡入逐帧顺序，不能称411587完整呈现等价。**

### 非空completion7：真实原表、生产continuation及SDL

原链`41FF6E(7) ->46234F ->4391C8(mode2) ->447FC3 ->42ADC7/42B461 ->42927B/47488B ->41756F ->status33&7F`。逐个原敌军按到另一军团最近距离选首个、后续以上一个坐标为起点，严格小于才替换，保留同距列表顺序。保存4F7CF0（SAVE globals偏移32）、置0，全部返回时恢复。死亡Stack增加pending evacuation协议，不把nullopt误当调用栈结束；只在当前对象实际frame_presented后清present，不瞬清集合。镜头依原42B461半格32/16步进、地形高度目标／边界和可见4步提前退出；每个镜头步亦需frame acknowledgement。死亡入口自身completion仍只是原41C02A的flag写，不擅调41FF6E。

**46234F和42B461均没有455048调用。**447FC3保存／恢复runtime DWORD30及临时status22，是原bitmap背景准备，不是455048透明消失动画；本轮未擅自套455048五帧淡出。41756F只轮询Ctrl+C，不是等键，Escape不能跳。生产输入／保存锁维持到外栈恢复；重复advance／取消不会提前结算。异常恢复完整events基线、币值／flags／记录／军团、镜头与控制delay，清锁且不回调resume。尚拒绝action_state4需要440F56分派448B99／45CA1D／45CAA0等移动姿态的分支；该分支未冒充普通站立绘制完成。

新增`tests/original_evacuation_test.cpp`自动定位**真实stage42 role39死亡record14→flag record17 completion7，29原敌军**。其余原记录受控禁用，因此是原表隔离fixture，不是自然可达全关证明。覆盖逐个顺序、未呈现不移除、Escape／保存锁、延迟恢复、一次resume、破坏运行军团触发异常rollback。`--smoke-evacuation`实际SDL事件／纹理／present通过，170 acknowledged frames；仍明确为上述受控fixture。没有扩技能或重跑全路线。

### 六类剩余边界

1. 资源：82项共享CDS及原archive fallback保留；未删除或改写源CD。PRG额外组用途未被推测成生产切盘规则。
2. 存档／回归：完整events与原档事务回归通过；不能据专项断言所有原脚本handler均已接通。
3. 画面：本轮新增撤离镜头／呈现门控；传送／起降完整原呈现链仍未补，不以淡出代替，其他原画面未做全量逐像素对照。
4. 菜单／关间：此前装备购买／复活／捐献、旅店2/3/4/6/9/10保留；旅店1/5/7/8仍在410EEE分支拒绝，城镇探索及其下级状态机未接；原45DDA6关间恢复不是付费住宿，未造休息页。411587淡入／双buffer时序仍如上未完成。
5. AI：已发布AI6 mode1失败继续40B0BA／40AED5 mode0回退，相关专项继续通过；未据6/7有限目标分支宣称完整技能／全AI决策等价。
6. 死亡／剧情：本轮非空站立军团completion7已上线；action_state4移动呈现分派及其它被preflight拒绝的generic handler仍未完成。stage42受控SDL不等于自然剧情可达验证。

### 最终验收／发布证据

`out/closure-Debug-build.log`、`closure-Release-build.log`均0，Release最后。九个专项均重新编译再运行：`original_{events,equipment,shop_menu,inn,church,save_bridge,ai_special,death_continuation,evacuation}-closure{,-build}.log`；events为完整执行非抽样。实际SDL：`closure-sdl-{shop,evacuation,auto,auto-alternate}.log`全0。`closure-package.log`重建共享CDS及ZIP；独立解包`out/closure-package-release-check`确认无DAT、无PACKAGE.txt/SHA256SUMS.txt，`closure-package-release-{verify,smoke-auto,smoke-battle-alternate}.log`验证。根EXE/DLL由最终Release同步，原SAVE未覆盖；最终根`S2NE.exe` SHA256=`3823F2271FB71FE5D752BD591CD157CE473C68CF4C6298CE3223C70FC7475126`，`S2NE-game-win64.zip`=`189398995`字节。全部命令退出，项目路径内无运行辅助进程。未修改PROGRESS。

## 2026-11-10 聚焦续作：共享箭头初始化、报价刷新、自动装备及AI6回退

**本次有新的生产实现；仍不称411587逐帧全等价或六类系统全部完成。非空completion7未上线。**

### 原共享箭头已定位，不再是未知资源

此前只搜4F72B0直接写引用遗漏了共享数组初始化。真实入口是`4288A6 ->4262EC`（asm66020..66305），不是只看426CEC：先读iconso count=142和menusp count=653；`loc426598`将iconso指针写入`4F7060+i*4`，`loc4265D3`将menusp前57项接到同一数组。4F7060本身保存分配缓冲指针，不是图0；因此`4F72B0`对应全局索引148、实际menusp第6项即**ID147**；4F72AC为ID146，4F72C8为ID153。公式`(address-4F7060)/4-1`。原资源尺寸实读：146=18x26、147=20x29、148=22x6。不能以相邻ID外形猜映射。本轮替换了旧猜测147拇指为确证146，44EE0E每个有变化属性项在560/y+4绘制原147箭头。无几何替代。

`tests/original_equipment_test.cpp`断言共享映射、原箭头上部20行像素重绘幂等；整29行不可幂等断言，因为后续24px间隔属性项会按原顺序覆盖上一个箭头底部。绘制和实际SDL购换/取消/跨页通过，不冒充原EXE逐像素全屏对照。

### 411587刷新和输入具体新增

生产页增加原报价提示100、251库存字串及原坐标、价格小字体居中/钱不足68色，返回下页后按当前钱/库存重新绘制。鼠标移动即更新选中商品（原x196..<628、y307..379），不再必须单击才刷新；水平滚动x196..625/y382..401，左右边界221/600、track比例349、首行count-8及x571右端选row7按asm411B4A..411BE9。钱不足接原104提示、确证共享153底板及等键返回，不扣钱。专项验证悬停回选、track取消不改原档；实际SDL保留原档购买替换/取消和文件往返。**仍未移植原438B8F光标移动与473111/47308B按住重复计时、原buffer拷贝/淡入时序，因此不是411587完整呈现等价完成**。整页重画不宣称等于其原缓存刷新协议。

### 旅店10自动装备生产闭环

`410EEE option10 ->43183B(string6,x200,y200) ->451ABC ->451B21 ->441143(2)`接通正常原确认/取消/提交/旅店返回；main成功后同步events，后续capture不会覆盖装备。不新增自创人物页。精确复用原卸除/装入属性事务：按军团人物顺序排status70/control1；每人物六槽，库存ID顺序扫描，武器1..6另需440C06(kind技能表)，其余type=slot+6；440A89资格；ITEM+25无符号rank严格大于，保留第一个最高候选。旧装合法且rank严格大于候选才保留，同分仍先退库再装；无候选也可卸除不合规旧装；最后武器type4且第五槽非空则卸第五槽。原clamp99、属性不对称规则保留，全军flag最终重算，钱不变。

440C06资源索引依据427BD5：KIND+1E DWORD是kind技能目录，kind数WORD+16，目录4字节/职业；后接23字节记录，首字段等于武器type+1。不是用一般eligible冒充440C06。

旅店专项增加真实原SAVE取消全档相等、提交与生产事务逐字节相等、排除人物不改、结果资格/武器技能、type4第五槽移除、库存加装备数量不凭空增加及capture往返。SDL使用正常事件进入10、取消、再提交、返回及真实文件往返。确认阴影原palette转换修复为原索引背景，未添加替代图形。存读档/覆盖取消既有回归仍通过。

### AI6实际回退增量及死亡边界

原40D2ED先40D351(mode1)，失败才40AEB3→40B0BA；40B0BA的无攻击候选尾部40AED5又会以mode0尝试特殊目标。因此旧测试“6未精确抵达必须完全不动”错误。生产先按原对象3/5或4/6和原路表尝试精确目标，成功才直接移动；失败继续现有40B0BA攻击候选扫描，再40AED5，不再在尾部错误强制mode1。`original_ai_special_test`受控stage15、无对手fixture验证6/7都能原目标移动，未把此有限分支称所有40B0BA技能决策完整。未扩新技能。

非空completion7本次重核`46234F`的4391C8(mode2)逐个选择→447FC3焦点→42ADC7/42B461镜头→42927B/47488B呈现→41756F轮询→清status80，再恢复4F7CF0。现有`OriginalDeathDispatch::Stack::next`同步调用`events.complete`，没有将这个呈现动作挂起并在frame_presented后恢复原死亡栈的协议。未改成瞬清敌军或空集合，仍严格拒绝。此项没有本轮生产完成证据。

### 统一验收／发布

全部日志在`S2NE\\out`：`focus-{Debug,Release}-build.log`均0，Release最后；`equipment-focus{,-build}.log`原箭头/购换/跨页通过；重新编译`original_{events,inn,church,shop_menu,save_bridge,ai_special,death_continuation}-focus{,-build}.log`均0（events完整）。`focus-sdl-{shop,auto,auto-alternate}.log`均0，shop含新增451ABC正常输入与文件往返。最终共享图元索引校正后重跑equipment及SDL shop均0。

`focus-package.log`最新CDS包；独立`out\\focus-package-check`没有DAT及PACKAGE.txt/SHA256SUMS.txt，`focus-package-{verify,smoke-auto,smoke-battle-alternate}.log`均0。根EXE/DLL已同步，EXE SHA256 `26E3CA521F877C6D90421AFD91B7F7C0A6A0A15AA7E73067D0AD110907E60CAC`，ZIP `189394920`字节。源CD归档不变，用户SAVE未覆盖，无自启游戏/专项残留，PROGRESS未改。

## 2026-11-10 同轮续作：对象跨页与旅店存读档／设置

本次是明确有限增量，**六类未收口，411587／箭头、自动装备10、AI6完整fallback及非空completion7仍未实现**；不得将本章归为全量完成。

- `original_shop_menu.hpp`将4125FC对象首行与选中行拆开。原x239..258、y=top+51..230滚动入口；上箭头到top+74、下箭头自top+207；track top+76..205按`(y-top-76)*count/129`、尾页count-5、top+175划分row0/4。上下移动先走可见行再移动首行，不再以`recipient-4`强制贴尾。此次没有实现按住鼠标的47308B/473192重复计时，也没有虚构滚动图元。保留既有便利键，不声称它们是原4125FC键循环。
- `original_inn.hpp`将410EEE选项3/4分别接4371E6(1)/437181(1)原204选择页：读档空表门控、原翻页/选择、写档覆盖确认及取消返回、OriginalSaveStore原槽53备份/原档原子落盘。读档交给main用OriginalSaveBridge恢复原档并退出旧旅店上下文；不把战斗菜单返回塞给旅店。专项在out隔离目录复制原资源、写入原SAVE字节并读取逐字节相等，不覆盖用户SAVE。
- 旅店9接434103(2)原设置页，沿用现有原资源及事件处理；关闭只返回旅店，运行设置/小字与原难度28字节回传，不以mode0覆盖CFG。设置入口、绘制、关闭和不改钱/库存专项通过；SDL正常输入进出设置通过。
- `tests/original_equipment_test.cpp`和SDL增加12个同一原record的**受控**对象跨页，验证底部first7/index11、上移保持first7/index10、track顶部复位及取消全档无变化；不是自然原档有12个角色的证据。原档购换/确认取消/文件往返仍通过。

验证日志均在`S2NE\\out`：`continuation-{Debug,Release}-build.log`均退出0，Release最后；`equipment-continuation{,-build}.log`、`inn-continuation{,-build}.log`均0。完整`original_events_test-continuation.log`及church/shop/shop_menu/save_bridge/ai_special/death_continuation同后缀日志均0（后七项复用已有专项可执行文件，装备/旅店本轮重新编译）。`continuation-sdl-{shop,auto,auto-alternate}.log`均0，shop含新增跨页和设置实际SDL输入。

具体未接入口与障碍：
- 411587仍缺自身刷新/光标及完整上级命中状态，不把整页重画称其全部移植；44EE0E读取4F72B0、4125FC读取4F72AC。426CEC证实批量按ID加载原图，但本次未找到这些global的初始化ID表，未上线猜测147/148，更没有几何替代。
- 旅店1：414D5B→415000/415084/415415/4152FA/414F5D城镇探索脚本/呈现未接；5/7：44D439两种对象装备业务未接；6原名册已接；8原退出确认未接；10：确认6→451ABC→451B21未接。已核自动装备并非逐槽最高普通属性：六槽按ITEM+25严格大于挑选，武器另需440C06遍历kind技能表（4F6F94/98，23字节记录），候选资格440A89；旧装同分也可能退库替换，最后武器type4移除第五槽。这些差异不能用现有购装eligible直接替代。无缺失资源的结论，缺的是其精确算法与生产确认接线。
- AI6不可达40D2ED→40AEB3实际调用40B0BA完整普通决策，不是mode0接近移动；尚未移植其完整决策，继续拒绝而不伪装验收。AI7已有范围不扩。
- 非空completion7：46234F暂存4F7CF0置0→4391C8(mode2最近对象)→447FC3→42ADC7/42B461→42927B/47488B→41756F→逐个清status33的80位，最后恢复global。缺跨死亡call stack的可挂起逐帧呈现/平移/输入轮询协议；未瞬清敌军，未拿空分支代替。上述两项本次只有原证据核查，无新生产实现。

发布：`continuation-package.log`生成最新CDS ZIP；独立`out\\continuation-package-check`无DAT、无PACKAGE.txt/SHA256SUMS.txt，`continuation-package-{verify,smoke-auto,smoke-battle-alternate}.log`均0。根EXE SHA256 `F05F6F93ADD31B9746E56A5F111943F947D9384B49D5537E54D8F8194F1B9B03`；ZIP `189389240`字节。根EXE及运行DLL同步，源CD归档不改，PROGRESS不改。

## 2026-11-10 装备购买生产事务与有限原比较页接线

本章替代旧章“装备全部拒绝”的描述。**已形成可合并生产买入／替换闭环，但不是原比较页面全部图元等价完成，不称菜单类收口。**

- 新增`src/original_equipment.hpp`：买入对象实际是`44159E ->440A89 ->443351`；用户提到的440C06为自动装备相关资格，不把它错当此处唯一门控。实现角色专限（ITEM23/24字节对应role ID）、kind+17 mask、role+118位8豁免、性别与属性mask、特殊64对属性加10。排除status33&70、control+8==1。
- 生产shop前两类按BIN4原town报价进入，原218底板、204“不装备”行、五行35px人物、六槽原图与名字、原44EE0E七类变化项原字串／坐标／颜色，原208报价和96／207两种确认。可只买入库存，或选择人物后买入并替换原槽。鼠标对象选择／确认／取消返回、Tab切换不装备与人物。**上级仍沿现有水平报价页，尚未完整移植411587自身全部绘制差异；44EE0E的4F72B0箭头与4F72AC滚动拇指原资源映射尚未完成，故不能称原比较图全等价**。没有自行绘制几何箭头替代。
- 提交副本原次序：按原价／flags10h八折扣钱→库存clamp99加一→旧槽退库存（最高99）→新槽扣一→原属性重算及441143(2)全军flags。已逐条核44D11B/44CDB7的相邻WORD sar16、SP分支不对称负值零目标，保留原999／110非对称cap，不用通用clamp冒充。军团装备效果31／58分别flags8／4，保留其他能力flags重算。生产成功同步events，下一次capture不覆盖装备和属性。
- `tests/original_equipment_test.cpp`从真实SAVE找原town可负担报价与合资格人物，正常生产scan完成取消、不装备买入、人物比较、取消替换、再次提交；独立断言钱、旧装备退库、新槽、攻击／攻击上限计算、全档delta、capture及真实文件往返；受控满99库存、status10/20/40拒绝、钱不足不变。不是完整原版逐像素比较证明。
- SDL `--smoke-shop`真实SAVE报价**item7、recipient0**，正常SDL事件经过原比较页、确认取消、替换、两级返回，核原事务全档等价并写档往返。原复活／消耗品买入／出售／旅店名册传闻／大小额捐献同次回归通过。无扩新技能或全线重跑。
- 当前明确缺的函数语义／呈现：`411587`装备上级刷新与原面板保存恢复；`44EE0E`箭头共享图元、`4125FC`原滚动拖动与键盘循环精确行为，及原版画面逐像素对照；不能以本次事务通过推论这些完成。旅店剩余业务、AI6的40AEB3、非空46234F仍未新增；本轮先停在装备原呈现未全收口边界，不越序声称后续完成。

### 最终构建／验证／发行

- `out\\equipment-Debug-build.log`、`equipment-Release-build.log`：统一Debug／Release退出0，Release最后发布根EXE和DLL。
- 重编并运行`out\\original_{equipment,church,events,save_bridge,shop_menu,shop,ai_special,death_continuation}-equipment{,-build}.log`八专项退出0；events完整两分支，不是抽样。
- `out\\equipment-sdl-{shop,auto,auto-alternate}.log`全部通过（复活及装备均包含在shop）；`equipment-debug-sdl.log`为之前Debug快照证据。
- `out\\equipment-package.log`；新独立解包`out\\equipment-package-check`无DAT，`equipment-package-{verify,smoke-auto,smoke-battle-alternate}.log`全部通过；包未带PACKAGE.txt／SHA256SUMS.txt，源CD和原SAVE保留。
- 根EXE SHA256 **9259D7AEE5B7CE7127E10C943E9C0A2FD0BB0B11A58A3EAAE3A4E13ECFEC0B04**；ZIP **189383773 bytes**。未改PROGRESS。

## 2026-11-10 教会复活生产接线阶段交付（装备与后续仍未收口）

本章为最新实际结果；下方历史章的“教会option2未接”已被本章替代。**不是六类完成，也没有完成用户要求的装备买入、AI6 fallback和非空completion7。**

### 已实现与验收：41430B(0)复活

- 生产教会option2接`441867`死者对象（status33的20位且非10位），不要求present。原217底板、44D675人物大图、职业、属性、十行对象与技能名字来自CEL/PAL、BIN2、KIND及原PRG站立图，不以战场名册替代。上下与鼠标选对象，原106确认（43183B的EBX=1默认为是）、取消回对象、最后对象成功后返回教会。无对象102等键返回；钱不足104等键后仍在对象页，原档不变。
- `45CFBB(mode0)`原战力费用，难度1除10、2除5、3原值、4乘2、5乘3，最小1。成功仅清该角色status33的20位和扣钱，**不补HP、不改库存或装备**。生产键鼠提交同步events，防下一次原档capture恢复死亡标志。
- `tests/original_church_test.cpp`真实SAVE派生**受控死者／1000000钱**fixture：对象页面绘制、取消不变、独立原combat_value费用、全档精确delta、capture持久化、真实文件往返；空对象／零钱拒绝和等待返回。不是自然死亡角色进城的完整流程证据。
- SDL `--smoke-shop`正常SDL事件→教会页scan→原对象绘制→取消／再次确认／返回通过，核全档HP／库存保真与实际扣钱。当前人物侧图采用原PRG站立帧，**尚未对44D675全部像素逐项原版截图等价验收**；滚动条原4F72AC资源映射及441706第三业务也未宣称完成。

### 装备买入：本轮确证调用链，未交付

- 更正前章“440C06”为买入资格的表述：购买对象实际`4125FC ->44159E ->440A89 ->443351`；440C06是自动装备的相关路径。`440A89`先查物品23/24字节职业专限、kind+17职业mask、role+118位8豁免，再检查443351的性别／属性mask；不能只看库存／职业名放行。
- `411288 ->411587`装备上级不是消耗品411FA1：调用者传原panel给4125FC，后者五行35px对象、456EEA六槽、44EE0E旧新属性比较、两种确认。提交先扣钱／库存加一，选择装备时`44D11B`旧槽退库存→`44D018`新槽／属性→`441143(2)`全军flags重算。44D11B存在按完整DWORD偏移sar16读取相邻WORD的非对称负值钳制，不能以通用stat clamp代替。
- 曾开始装备实现实验，因尚未完整还原上述原比较页及非对称事务而撤回，**没有启用残缺装备UI或半套装备算法**。当前仍只开放消耗品、出售；源资源不缺，障碍为尚未完成的原411587/4125FC/44EE0E呈现与44D11B/441143事务移植，而非资源无法获取。不能称确定不可实现。
- 旅店十项具体入口与业务清单沿用紧随其后的表：已接传闻与名册，其余探索、mode1存读档、装备、特选角色、返回标题、设置、自动装备仍未接；**原旅店没有已证付费住宿恢复**。
- 按用户顺序本轮未越过装备缺口伪做AI6或非空撤离。原40D2ED失败必须40AEB3，当前仍拒绝；46234F必须4391C8逐个选→447FC3/42B461镜头→present→清presence→恢复外栈，当前同步死亡Stack尚未建立此挂起协议，非空仍拒绝。无新技能／全线重跑。

### 最终验证与发布

- `out\\revival-debug-build.log`、`revival-release-build.log`：Debug／Release退出0，Release最后、已发布根EXE/DLL。
- 重编十专项 `out\\original_{church,events,save_bridge,shop_menu,inn,ai,ai_special,death_special,death_continuation,shared_disc}-revival{,-build}.log`全部通过，events是完整运行非子集；church最后追加空对象／钱不足也重编通过。
- `out\\revival-sdl-shop.log`（含复活、原买卖、旅店、大小额捐献）、`revival-sdl-auto.log`、`revival-sdl-auto-alternate.log`全部通过。没有声称装备／AI6／非空撤离SDL通过。
- `out\\revival-package.log`重生成CDS与ZIP，独立`out\\revival-package-check`无DAT、无PACKAGE.txt／SHA256SUMS.txt，`revival-package-{verify,smoke-auto,smoke-battle-alternate}.log`全部通过。源CD、SAVE保留。
- 根EXE SHA256 **97CEFB81D7221C79E43448D542AEFF3F6794D4ED5BCE781EFC1C1B2DBC8E2410**；`S2NE-game-win64.zip` **189376149 bytes**。末次核查无游戏／专项辅助残留；未改PROGRESS。

## 2026-11-10 同轮菜单／AI续作阶段交付（未全收口）

### 实际新增与修正

- **大额捐献生产接线**：继续读完整`44ACAD`后纠正上一章描述：这里不是随机选择经验奖励；先按难度1..5给基础50/40/30/20/10，再加`(fee-100)/100`、上限150，直接加WORD+113经验、不经过普通战斗经验倍率。每个原军团角色依原status33的E0／10和control+8==1过滤加经验，但全部仍执行999封顶及升级阈值检查。达到阈值调用原`4593CC(mode0)`成长，无升级提示页；复用已核`OriginalProgression::award_native(raw=0)`及原成长／RNG。费用、D6阈值累计、D4 WORD加一、一次捐献flag与成长用副本一次提交，取消不改钱／角色／RNG。生产使用现有Battle共享RNG，经`Prologue::interbattle_random`，无共享上下文则拒绝并给明确诊断；不另造一条随机流。角色同步回events防后续capture覆盖原档成长。
- **大额验收**：`original_church_test`覆盖难度1..5、受控1000钱fee100、取消、无升级不耗RNG、强制原角色阈值50触发level5→6和原成长耗RNG、encode/decode往返。SDL原键序列走确认／取消／再次确认／结果／返回，检查原事务全档和RNG精确等价。属于原档派生受控fixture，**不是自然合资格大额捐献流程**。小额原测试保持通过。
- **旅店名册下级生产页**：option6接`43471C(1)`，按`441460(1)`排除status33&70、control WORD+8==1，只取本军团；复用现有原名册资源、角色选择／空格详情／取消回名册／返回旅店。关间定位按原mode1禁用。专项及SDL覆盖真实SAVE来源名册和详情、完整返回、不改原档。并未制造旅店休息。
- **AI运行时字段错误核正及有限生产实现**：`40AED5`的`mov eax,[role+8];sar eax,10h`读的是**WORD+0Ah**，不是control WORD+8，也不是stage行为／军团阵营。新增`Unit::ai_role10`从原source+20及SAVE+0A恢复／保存，现有阵营保持不变；不再把6/7塞进behavior。`40D351/42D554`按原军团DA选择物件高字节：敌3优先5、友4优先6；逐行扫描、严格距离tie，route marker非0、精确目标优先、否则最近可用原route。kind7可逐边可见移动后结束且不追加接触攻击。kind6的`40D2ED ->40D351(mode1)`只有本回合可精确到达才走此路径；不可达保留拒绝诊断，**尚未接原40AEB3完整fallback**，不能称6全量完成。原stage15受控kind6/7、禁对手fixture验证kind7运动、kind6不能到达时不伪装普通追近。完整events／保存／初始化／低血撤退回归保持通过。

### 原菜单业务清单及具体未接入口

| 原入口 | 当前交付 | 未接部分／下一具体依赖 |
|---|---|---|
| 商店410DC0 option1/2→411288→4125FC | 仍拒绝 | 装备对象页、440C06可装备条件、替换原槽／旧装备退库存及能力重算；不能以消耗品购买页替代 |
| 商店option3→411FA1/41252F | town门控消耗品买入完整页面及原档事务 | 无新增装备开放 |
| 商店option4→412F1D/413F16 | 原出售页面及原档事务保留 | — |
| 旅店option1→414D5B | 仍拒绝 | 原城镇探索、415000/415084/415415/4152FA/414F5D场景与NPC脚本 |
| 旅店option2→414C4A | 原两传闻、等待／返回 | — |
| 旅店option3/4→4371E6/437181(1) | 旅店未接 | mode1关间存档／读档恢复；不得复用仅支持battle上下文读档假返回 |
| 旅店option5→44D439(0)/441143 | 未接 | 角色装备／库存与全军能力flags重算 |
| 旅店option6→43471C(1) | 本次名册／详情／返回接通 | 无关间定位 |
| 旅店option7→44D439(1) | 未接 | 原特定角色选择条件 |
| 旅店option8→确认51／返回标题 | 未接 | 现主循环关间同步未建立exit_to_title恢复，不能以关闭游戏冒充 |
| 旅店option9→434103(2) | 未接 | 原mode2设置子页接回旅店 |
| 旅店option10→确认6／451ABC／441143 | 未接 | 原自动装备6槽候选、440C06资格／价值／全军重算；**并非休息回血** |
| 教会option1→413F9B/44ACAD | 小额及本次大额／成长确认事务 | 共享RNG上下文缺失时拒绝；自然大额fixture仍缺 |
| 教会option2→41430B(0) | 未接 | 441867原死者status20且非10；原217对象页＋44D675人物图／属性／列表，45CFBB价值按难度费用后最小1、确认106，**成功只清status20，不自造满血**；本次已核但未实现UI |
| 教会option3→41430B(1) | 未接 | 441706资格→45954D业务，不能未经证据称净化或复活 |

### 死亡非空撤离：仍未实现，精确纠正原等待含义

`46234F`逐角色选取并聚焦、绘制、present后清presence是真实顺序，仍不能由空敌军测试代替。新增调查证实：`42ADC7/42B461`是可见范围／镜头平移，不是先前报告猜测的地图占位；`41756F`只是poll可中断输入、检查Ctrl+C，**不是等待任意键确认**，不得添加每角色人为确认。`4391C8(mode2)`初次(0,0)调用军团+282中心距离`4392FF`，后续按上一角色坐标严格最近tie，资格含runtimeAD／动画状态／442B08和mode2活体备选。当前死亡Stack只同步返回对白事件，未提供special completion挂起／逐present栅栏恢复；必须新增专用完成状态机并保持外层call stack，不能把present函数内瞬清全军或插入普通消失动画。该项仍严格拒绝；本阶段没有完成此入口。未再跑全路线或新技能。

### 本阶段最终构建／回归／发行

- `out\closure-debug-build.log`、`closure-release-build.log`：Debug/Release原生构建退出0，Release最后同步根EXE／DLL。
- `out\original_{events,save_bridge,initialization,retreat,ai,ai_special,church,inn,shop_menu,death_special,death_continuation,shared_disc}-closure.log`及对应`-closure-build.log`全部退出0；完整events两分支、82共享条目等价、原档往返保留。
- `out\closure-debug-sdl.log`为前一源码快照Debug大额捐献测试；最终Release `closure-sdl-{shop,auto,auto-alternate}.log`通过，包含旅店名册详情、大额捐献受控fixture及原真实town买卖、双线自动回玩家控制。未声称AI6/7或非空撤离已SDL验收。
- `out\closure-package.log`，独立无DAT解包`out\closure-package-check`，`closure-package-{verify,smoke-auto,smoke-battle-alternate}.log`全部通过；源CD保留，包无两个txt。最新ZIP **189368326 bytes**，根EXE SHA256 **1589CECF9BA6F625EA1407C628E56ED2BF21B666E51B9814AF84D93BBE21E872**。末次核查无本工程游戏／专项辅助进程残留。

## 2026-11-10 同轮续作：消耗品买入、旅店传闻、教会有限捐献生产接线

本次不是六类全量完成，也不是旅店／教会全量完成。没有继续新技能或全路线。源码已停止写入，Release最后构建并同步根EXE／DLL，原SAVE和CD归档保留。

- **商店买入已实现**：`411288`读取BIN4城镇14字节记录及三类WORD报价列表，按原`4F7CE8`低word城镇门控，只开放第3类消耗品；前两类装备必须`4125FC`对象装备页，仍拒绝。`411FA1/41252F`价格原价／army flags10h八折，数量只按钱限制，买入后库存上限99（满库存也按原扣全款，不自创库存拒绝）。原portrait44、title198、数量底图258/413及确认488/416。真实SAVE正常页面输入、取消、精确全档delta、文件往返和SDL全序列均通过。
- **旅店已实现有限原业务，不冒充住宿恢复**：新增`OriginalInn`并接hub option1，原DAT13、10项原矩形／字串；option2走`4732ED(2) ->414C4A`，按原town城名后跳三类商品WORD表，随机选择两条原传闻，portrait154、title194、149/42六行16列、行距28，输入等待后返回。真实SAVE两条内容及不改任何存档字节专项通过。其余子页保留明确拒绝。核明option9=`434103(2)`设置页；option10=`451ABC`自动装备而非恢复，不能自创付费住宿。`45DDA6`恢复原本在进入关间`44A4A9`路径，现已有对应story转关重置；本次未宣称旅店恢复新增完成。
- **教会已实现有限原捐献**：新增`OriginalChurch`接hub option3，原DAT15、三项矩形／字串、portrait153/title209及原确认107(200,278)、结果108等待、取消返回。`413F9B/44ACAD`仅支持fee<100确证直接返回分支（当前钱1..99）；扣army+CC、按原阈值增加+D6、WORD+D4加一、globals[27]=1；再次捐献101／零钱104原提示。fee>=100涉及原随机经验收益，明确拒绝，不简化为扣钱。复活／另一对象业务`41430B ->441867/441706`仍拒绝，未伪称全业务。专项为真实SAVE派生受控钱1/9/10/99及100/999/1000/2000拒绝fixture，精确全档delta和encode/decode通过；**不是自然合资格原档捐献证据**。
- **画面紧耦合修复**：保留PCX原索引，下级`41144E`恢复全局PAL显示后再作原确认阴影。初次教会测试暴露非索引像素异常，已修复并重跑；不是近似RGB混色。
- **AI6/7与死亡非空撤离没有完成**：`40D351 ->42D554`已核目标物件属性高字节，armyDA0优先4再6、DA1优先3再5及原严格距离tie；但现`Unit::behavior`还承担hostile阵营判定，直接改成6/7会使敌方退出hostile选择，实验未能验收，已撤回全部AI实验源码／测试而非发布未证实实现。后续需分离原runtime kind8与army+DA、核`40AED5/40D2ED`调度／到达`452AD5+44ABD4+453FFF`。非空completion7仍严格拒绝：`46234F`要求`4391C8(mode2)`逐角色最近选择、`447FC3`镜头、`42ADC7/42B461`地图占位、`42927B/47488B`呈现、`41756F`等待后才清presence，不能瞬清全军；本次未完成该同步呈现状态机。原空敌军completion7、真低血撤退保持通过。

### 本次最终验收和发行

- 原生Debug／Release：`out\services-debug-build.log`、`out\services-release-build.log`，退出0，Release最后发布根运行时。
- 重编专项日志：`out\original_{shop_menu,inn,church,shared_disc,events,retreat,death_special,death_continuation,save_bridge,ai,disappearance}-services.log`及对应`-services-build.log`全部退出0；完整events保留两个原分支并跑至地图1／71，不放宽门控。
- SDL dummy：`out\services-debug-sdl.log`，最终Release `out\services-sdl-{shop,auto,auto-alternate}.log`全部通过。shop覆盖真实town买卖以及真实town旅店传闻；教会为受控99钱fee9夹具。两线auto真实菜单进入、友军行动、敌军行动、回到玩家控制均通过。历史auto卡住确证为驱动首次右键只取消选择、却过早标记菜单已开；只修驱动，未延长timeout或修改AI规避。
- `out\services-package.log`重建CDS并逐条等价：82条→40块，原DAT仍保留。最新ZIP **189364890 bytes**，无DAT、无PACKAGE.txt／SHA256SUMS.txt。独立解包`out\services-package-check`后`--verify`、`--smoke-auto`、`--smoke-battle-alternate`通过，证据`out\services-package-{verify,smoke-auto,smoke-battle-alternate}.log`。
- 根`S2NE.exe` SHA256 **6936C0C340BE2BE820A34D0A0A380CED842DF6EC63CE961CBFF0F21FE384407E**。最后按本工程绝对路径核查无S2NE／专项测试进程残留。

## 2026-11-10 按序系统补全：本轮实际交付与未完成边界

本轮不是六类系统全部完成。没有扩展新技能／剑法，没有重跑两条完整强化路线，没有改写用户存档、原资源或PROGRESS。原32／88／94输入及两结局历史证据保留。以下按资源→存档／回归→画面→菜单／关间→战斗／AI→死亡／剧情记录。

### 1. 资源：共享DAT具体实现及验收完成；PRG固定安装路径不变

- 新 `tools\pack-disc-resources.ps1` 读取两盘原 PAL 索引，以 SHA256 候选分桶后逐字节比较，只对完全相同内容合并；82条映射归并为40个唯一内容块。原条目总计270404582字节，共享载荷180653377字节，节省89751205字节；完整PAL、版本、明确盘号／编号、偏移／大小和每条FNV-1a-64校验和写入 `SUPERSP.CDS`。生成后逐条读回比较，源CD资源不修改、不删除，不采用硬链接压缩归档。
- `OriginalDiscResources::dat_entry` 有明确CDS运行解析器；存在CDS就严格验证，不能以损坏包偷偷回退DAT。校验版本、原PAL全字节、原条目数量与长度、盘号／编号顺序、载荷精确边界和所读内容校验和。FNV是意外损坏检测，不是攻击者防篡改认证。原DAT解析仍可使用；打包 `-OriginalDiscArchives` 切回传统模式。关间和剧情转场预检已改经 `dat_entry(disc,1)`，不再要求CDS包存在物理DAT。
- `original_shared_disc_test.cpp` 独立目录只有PAL/CDS，对两盘82条资源与原DAT逐字节比较通过，覆盖版本、PAL、条目盘号、偏移、大小、载荷翻转及文件截断拒绝。`original_disc_test` 重编通过；测试隔离目录由已删除的build改到out，没有改原资源验证门控。
- `original_prg_layout_test.cpp` 固化实际字节事实：根PRG=CD1；双盘前8组逐字节相同。CD1第9组11086字节与第7组属性完全相同，且恰好到EOF。CD2从100703123到EOF共2778214字节，与第6组完整132地图块逐字节相同，其内部132项索引边界恰好覆盖尾端。因此尾部不是未明填充；但根PAL第9组只声明11086字节，不能把CD2该尾块按根PAL伪称为正常第9组地图入口。
- 原 `425BF9/425F4F` 按10+条目序号还原PRG索引；`473A55/473B26..56` 在DEVICE非零时固定安装PRG，DAT按 `4FB488` 选择两张不同目录（473B5B..BF0）。检视全部静态 `473A55/473CDB` 调用未发现PRG常量组9调用；这仅是静态调用证据，不证明动态路径绝不引用。保留两PRG原样，不切CD2、不删除其尾部；原制作归档为何追加副本没有可证原注释，不编造用途。

### 2. 存档／回归：完整事件旧fixture修正并全量通过

- 修改前直接运行完整 `original_events_test`，复现：branch0、turn4、expected4、campaign_modal1、defeated0、blocker空。原 `Prologue::tick` 在友方banner结束先调用 `dispatch_campaign`，它扫描原 `turn_trigger` 排入原对白；`advance` 消费原对白后返回原调用阶段。这不是第四回合未恢复缺陷，而是fixture误要求剧情前已到玩家输入。
- 仅fixture正常调用 `reveal_text/advance/tick/draw` 消费真实第四／第五回合对白，继续断言阶段、回合不变、无modal且恢复原玩家控制。娜线第1回合有既有原动画／脚本阶段，同样逐帧输入至原目标界面；没有改脚本数据、完成旗标、阵营状态或游戏门控，没有移除branch1。之前hidden的branch1目标fixture错误因此也消除。
- 最新完整 `out\events-full.log` exit0：难度1..5开场、branch0第4／5回合原对白、branch0 stage1 23角色、branch1 stage71 27角色、三张地图均完成。`original_save_store`、`original_save_bridge` 重编通过（原22／42角色逐条往返）；日志 `out\original_save_store-ordered.log` / `out\original_save_bridge-ordered.log`。
- 仍未完成：全部特殊状态／完整嵌套剧情半事务保存；本轮不开放这些边界。

### 3. 画面：已有专项重新验证；原缺口没有虚报补全

- 重编 `original_ui_timing_test`：9个实际呈现banner姿态／frame fence通过；`original_map_frame_test`：indexed B1反应合成及stage1／71共122帧signed offset（正61／负61）通过。
- 最新Release SDL dummy娜线stage71实际玩家移动、可见敌方移动与两轮阵营banner通过；共享CDS独立包重复通过。不以dummy测试代替原版真实逐帧手感对照。
- 仍阻塞／未实现：传送／起降／特殊移动的完整原表现、全部遮挡与特效组合、原淡入淡出。本轮没有以几何图形替代，未将重跑专项记作这些功能完成。

### 4. 菜单／关间：明确生产未接通范围

- 重编 `original_interbattle_test` / `original_menu_test` 通过：PCX17、四个原入口、准确命中矩形、键鼠及callback门控、5难度选择。
- 原入口地址为旅店410EEE／商店410DC0／教会41115D／出发416333。生产 `main.cpp` 的关间adapter仍对前三项返回unsupported，仅option4在原转关事务中放行。测试验证unsupported拒绝，不是证明三级交易已实现。
- 因此旅店／商店／教会下级交易仍是**确证未实现**，人物详情与完整菜单树仍未达到逐项验收。没有假回调、跳页或伪交易；本轮不能声称完整关间完成。

### 5. 战斗／AI：已有算法／可见流程验收，不代表整套补全

- `original_ai_test` / `original_combat_test` 重新编译运行exit0；SDL stage71真实两个阵营周期通过。日志 `out\original_ai-ordered.log` / `out\original_combat-ordered.log`；无输出的算法测试以进程退出码为准。
- 最终追加的 `--smoke-auto` / `--smoke-auto-alternate` 均失败，原诊断仅为 `SDL battle entry timed out`（`out\ordered-sdl-auto.log` / `out\ordered-sdl-auto-alternate.log`）。驱动的smokePhase计数对战场进入后、非computer_actions帧仍累计，缺少此失败点的turn／menu具体状态；该追加失败尚未确证是生产AI还是驱动，不能宣称自动战斗SDL通过。已通过的两轮手动结束回合／可见敌方行动与headless完整事件测试不替代它。本轮不放宽超时或伪造通过。
- 仍未完成：范围效果、连击／反击／装备触发全部生产交互覆盖，状态回合效果、behavior6／7、完整低血撤退／飞行与脚本寻路逐项实战。不改原规则，不将本轮算法测试提升为完整实战认证。

### 6. 死亡／剧情：保留严格拒绝边界

- 重编 `original_death_continuation_test`：原stage6 role104／stage82 role147同步对白、completion13先置、输入／存档锁与只恢复一次通过；stage1／15不支持generic dispatch仍严格拒绝且无旗标泄漏。
- `original_campaign_motion_test`：真实NPC移动→轨道／反馈排空→原始保存capture通过。其强制结果为既有隔离时序测试，非自然击杀／真实关卡胜利。
- 仍阻塞：`OriginalDeathDispatch::continuation` 对type4..16除type9/cond0和已有货币分支仍拒绝，对completion1／3／7仍拒绝；完整通用handler与自然普通击杀正常输入E2E本轮没有新增证明。两路线已有结局证据未重跑，技能94本轮不扩展。

### 同轮继续（2026-11-10 追加，覆盖上面相应旧结论）

- **自动SDL超时已修并通过**：最小复现发现默认选人状态为command，首个正常右键只取消选人，不会打开战场菜单。smoke却立即把autoMenuOpened置true，后续再不发送开菜单右键，卡在idle；不是生产AI超时。驱动现继续正常发送右键，只有真实battleMenu出现后才标记opened，不改AI、时钟／超时阈值或输入门控。Release `--smoke-auto` / `--smoke-auto-alternate` 均exit0，分别1391／1627呈现帧，证明原战场菜单→友军自动→敌军→玩家控制。日志 `out\auto-driver-build.log`、`out\ordered-sdl-auto.log`、`out\ordered-sdl-auto-alternate.log`。
- **画面新增原消失动画呈现栅栏**：原455048每个原索引混合帧调用呈现，先前普通剧情消失轨道只按tick前进，连续tick而未呈现可吞帧。`Disappearance` 新增逐帧present确认，`Prologue::enable_presentation_timing/frame_presented` 接线到map，重开轨道清除旧present确认；headless显式逻辑时钟不变。`original_disappearance_test` 新测每帧20次超长tick不跳帧、present后才进入下帧和重开不复用确认，原stage155 role22真实图源五帧／AD-B1还原校验保持通过。日志 `out\ordered-disappearance.log`。这是新增生产帧门控，不是全部传送／起降表现完成。
- **菜单交易新增核心但尚未UI闭环**：`original_shop.hpp` 按412F1D筛选真实背包中数量非零且ITEMSP+1A价格非零项；4138F7／413F16按army+DE低字节10h决定原价50%或70%，提交army+CC dword货币原32位加法和inventory byte扣除，拒绝无货／超量／零量／非法id，装备／角色记录不动。`original_shop_test` 使用现有SAVE原档副本（不写源档）验证真实库存、准确整个原档差异及encode/decode往返、两种售率、拒绝不变。最初fresh无可售物品，改用真实原档，没有注入物品。日志 `out\ordered-ai-shop.log`。**生产商店界面尚未接线，本项不是用户要求的真实键鼠交易闭环**；不以草稿假UI交付，旅店／商店买入／教会仍未完成。
- **AI新增40CFE3原低血组件权重及生产approach接线**：从原40CFE3逐条恢复45CFBB modes5..8按40／(30或40)／(kind3为25否则15或20)／25加权、dword wrap、有符号除法、HP百分比<=比较。生产以前对所有非urgent低血候选一律unsupported；现真实调用该判定，判为approach者进入既有原select_40D25A，retreat者仍明确拒绝，因为40C425还有原持久behavior转换未接全，不能冒充撤退完整。`original_ai_test` 新边界31/32%、kind3区别、无最大HP／零战力，旧AI测试保持exit0。日志 `out\ordered-ai-shop.log`。behavior6/7仍缺。
- **死亡嵌套新增原type11/condition0..6身份谓词支持**：`OriginalDeathDispatch::Stack` generic调用帧按已有4211A3原谓词实际值决定是否呈对白／消费；false保持休眠，true后继续原嵌套调用，不预展开。capability允许该明确子集，但completion1/3/7仍拒绝。`original_death_predicate_test` 使用原146 condition2/3两支真实9/10句载荷，隔离调用帧验证两军身份查找／友军缺席、只消费true分支、不吞false；这是无死亡触发的146隔离原handler测试，不是自然击杀E2E。`original_death_continuation_test` 重新编译通过，stage6/82、货币／嵌套、rollback及不支持stage1/15拒绝仍保持。日志 `out\ordered-death-predicate.log`。

### 同轮第三阶段：生产商店／撤退／特殊完成（2026-11-10）

- **商店现已接入生产main关间页面**，不再仅独立事务：原hub option2 pending进入410DC0的PCX DAT14，原4EA618四个命中矩形、4EA638字串93/94/95/199；Tab/左右按原顺序切换，出售进入412F1D原可售库存，413061八列选择，4138F7数量默认1、左右限制1..实际库存，Enter提交413F16，Escape／右键逐页返回hub并解除原pending。购买三页继续failclosed，不假装实现；旅店／教会不变。出售直接更新rawSave的army货币和共享背包，后续capture保留metadata，不把army钱错误覆盖为另一组campaign_currencies。
- 原图而非自制界面：DAT14原PCX；41144E CEL235(6,8)、212(352,200)、167号商人头像对的group3 index332(居中80,y37)；412F1D出售底图为 **D3h=211**，不是旧调查误记的213；原库存物图id+492，(204+col*54,313)，价格y358；4EA71C thumb213／数量216，4137C8真正数量位置 **(198,413)**（不是底图(4,236)）。原ITEMSP物名、DAT3 ITEMHLSP帮助、字串194/201/203/205/206/110，原选框color60及原边界，Big5使用已有原换行器，避免拆双字节。尚未做人工像素截图逐点比对，不声称全部商店页面视觉完整。
- `original_shop_menu_test`真实SAVE副本正常页面scan、每页原资源draw、取消不变、确认精确全档delta、encode/decode、正常返回hub通过。生产EXE新增`--smoke-shop`，SDL_PushEvent正常Tab/Enter/Escape事件穿过同一OriginalShopMenu scan/draw，11次呈现，真实物品68售价1330，写`out\shop-sdl-roundtrip.sav`后load精确相同，源SAVE不改。是隔离hub入口的生产页面验收，不是重新通关到hub。日志`out\original_shop_menu-continued.log`和`out\continued-sdl-shop.log`。
- **低血真正生产撤退**：40D171 retreat接40C425/40C659原候选顺序、每格43948D mode0最近敌人、最大距离、HP<90%优先保留configF0 AP；可见原移动执行器逐边播放，撤退不追加接触攻击。结束按40C548 mode0再判40CFE3，写原runtime+0E=5（独立role WORD，不误改Unit.behavior／阵营）；SaveBridge新增该原字段restore/capture。`original_retreat_test`stage1受控HP=1夹具确认真实生产候选移动及role14持久化、capture/restore通过。不是合法自然低血全局平衡验收；behavior6/7及特殊status38完整控制仍未完成。日志`out\original_retreat-continued.log`。
- **死亡特殊完成最小可证分支**：核实41FF6E(7)->46234F逐个撤离敌军，空敌军会立即退出且恢复原4F7CF0。现仅允许无present hostile的completion7，非空仍严格拒绝，不跳撤离动画。使用原stage41 event8 role119／四句对白completion7，隔离其它未支持事件并隐藏敌军后，经Prologue生产同步输入正常返回、保留死亡record、save锁正确；非空相同原表拒绝。日志`out\original_death_special-continued.log`。这只是特殊完成7的空集合分支，不是整军撤离或自然击杀E2E；completion1/3仍拒绝。
- `original_events`完整两分支、旧死亡continuation、AI及SaveBridge两原档22/42人回归重新编译通过，日志`out\original_<名称>-continued.log`。Problems接口无客户端，使用原生构建和专项验证。
- 自动SDL历史超时仍明确为**输入driver根因**：首右键只取消默认command选择，驱动却提前标记菜单opened而不再发第二右键；改为真实battleMenu出现才置位。未改生产AI或放宽超时，双线正常菜单→友军→敌军→玩家验证保留；最终重跑见下。

### 第三阶段最终发布（以此为最新结果）

原生Debug/Release均exit0，根EXE及四DLL同步为Release；日志`out\continued-debug-build.log`、`out\continued-release-build.log`。最终SDL出售11帧、白线auto1396帧、娜线auto1618帧exit0（`out\continued-sdl-shop.log`、`continued-sdl-auto.log`、`continued-sdl-auto-alternate.log`）。最新ZIP **189351107字节**，根Release SHA256 **CF787D1164CD93AF4B1F36ED182FF9EEBFD230692A33E5250DC01BEB17180EE2**。`out\continued-package`为新独立无DAT解包目录，`--verify`、`--smoke-auto`、`--smoke-battle-alternate`全部exit0，日志`out\continued-package-verify.log`／`continued-package-smoke-auto.log`／`continued-package-smoke-battle-alternate.log`，打包日志`out\continued-package.log`。商店原档不随发行包分发，SDL商店在原资源根真实SAVE副本验收，不宣称无SAVE发行包可执行该测试。调查scratch已清，无S2NE或自启辅助进程。

**剩余边界不撤销**：出售生产页已不再是仅事务；真正撤退已有生产路径和存档，但特殊状态及6/7仍未全接；特殊完成7只允许空敌军原立即返回分支，非空撤离、completion1/3不支持。买入、旅店、教会、全部死亡演出和其它画面缺口仍未完成，不报告六类全量完成。以下两阶段验证为历史，包大小／哈希及“出售未UI接线／真正撤退未接线”的旧结论以本阶段为准。

### 第二阶段最终验证（历史）

- 所有生产源码停写后，原生MSBuild Debug／Release x64均exit0；日志 `out\ordered-debug-build.log`、`out\ordered-release-build.log`。根EXE和四个DLL已由publish同步，最终为Release。
- 统一重编首次完整事件回归捕获本次AI接线缺陷：新40CFE3传入5..8，但事件combat_value仅接受0..2。现按45CFBB的503468/6C/70/74原组件返回值接5..8；本地重算使用同一当前Unit和原配置，与紧邻mode0计算相同，不引入全局跨单位缓存。修复后完整 `original_events_test` 两分支exit0，日志 `out\original_events-final.log`。未放宽事件门控。
- 重编并通过 `original_disappearance`、`original_death_predicate`、`original_death_continuation`、`original_ai`、`original_shop`、`original_campaign_motion`，各日志 `out\original_<名称>-final.log`；共享82项和PRG布局再次通过 `out\original_shared_disc-final.log`、`out\original_prg_layout-final.log`。新增专项已注册CMake测试清单；本轮执行使用原生cl而非新建CMake构建目录。
- 最终Release正常SDL自动输入双线分别1397／1620帧exit0，日志 `out\ordered-sdl-auto.log`／`out\ordered-sdl-auto-alternate.log`。dummy视频／音频，不宣称人工画面观感或音频验收。
- 最新包 `S2NE-game-win64.zip` **189341196字节**；根Release SHA256 **AE6EB31939E202F2128918068E83E16BFD0C78DBC2FD63F8FB71692D9A600901**。日志 `out\ordered-package-final.log`。保留原CD资源，不含PACKAGE.txt/SHA256SUMS.txt，不含原DAT。
- 新建独立解包目录 `out\ordered-package-final`，确认无DAT，运行`--verify`、`--smoke-auto`、`--smoke-battle-alternate`全部exit0；原PCX/两段intro解码、白线自动完整控制返回、娜线battle71敌方行动及两次完整side banner通过。日志 `out\ordered-package-verify.log`、`out\ordered-package-auto.log`、`out\ordered-package-alternate.log`。结束检查没有S2NE残留进程，没有常驻辅助服务。
- **本轮未达到六类全部补完**：资源共享和历史回归已验；画面原消失栅栏、AI低血非撤退路径及死亡嵌套身份谓词是实际生产新增，但仅覆盖明确子集。菜单只有原档出售事务，未接生产商店正常输入／原图UI，不能报真实交易闭环；背景425092负资源14和原菜单几何还需完整核验。购买／旅店／教会、真正撤退behavior转换／6/7、传送到达原10帧和其它死亡特殊completion仍明确缺失。未新增94／剑法或重跑强化全线；未动PROGRESS。上述缺口不是测试已证明不可实现，而是当前交付尚未实现。

### 第一阶段构建／包／独立验证（历史，以上面本次续补最终验证为准）

- 原生MSBuild VS18 Debug/x64全量通过 `out\ordered-debug-build.log`；随后最终Release/x64通过 `out\ordered-release-build.log`，构建后同步根EXE及四DLL。根EXE与Release SHA256同为 `30404489D7B7A8B680FBC98C517C93566A3D63D926C577CDAE5BBABB11D9865E`。
- `S2NE-game-win64.zip` 最新189338224字节（约180.6MiB），15运行／资源文件+启动器；无PACKAGE.txt／SHA256SUMS.txt，无个人SAVE。保留CD2 PRG，不含两份DAT，以CDS运行。
- 独立解压 `out\shared-package-acceptance`，确认无DAT，运行 `--verify`、`--smoke-opening-right-skip`、`--smoke-battle-alternate` 全exit0；stage71玩家移动／可见敌方移动／两个阵营周期通过。验收目录已移除，所有本轮启动程序正常退出，没有留后台辅助进程。仅本机无DAT独立包验证，不是全新无开发环境机器VC运行库认证，也不是六类全完成。

### 连续流程出口输入验证

`--smoke-campaign-strengthened` 仅用于强化友军、隔离存档的流程验收，不代表原版平衡或正常通关。连续 smoke 对原关卡中单独使用条件31的胜利组，在真实 flag1 就绪后读取原地图 object6 出口，通过既有合法鼠标输入与 BFS 寻路；第10关保留清敌后再寻出口的策略，第16关无需先清敌。若没有合法空闲路径，立即将真实断点写入隔离验收目录并报告阻塞，不传送、不置旗、不强制胜利。

`tests\original_campaign_stage16_test.cpp` 验证出口元数据和输入策略；可传入真实第16关存档路径，额外输出地形、占位与友军阻挡诊断，并验证原 NPC 通道穿越。原汇编 `477DE4–477E6E` 对敌方占位拒绝扩展，对友方占位仅清除落点标记；玩家移动现允许经过友军（包括 behavior1 NPC），但不能停在友军格，逐步扣除真实 AP。第16关真实断点的地形可连通全部五个出口，阻塞源是 `(2,21)` 的 NPC2，并非缺桥或门事件。

第156关剧情条件16面对死亡或未激活角色时，按 `461149–461173` 返回成功但不修改角色；外层 `41BA1E–41BA56` 仍负责对白及消费事件。恢复该语义不复活角色、不创建替身；可用 `tests\original_campaign_stage156_test.cpp`（参数为真实第20关存档）验证活动/死亡/未激活分支与存档桥往返。强化流程实测已通过第156关至23关，后续146关 opcode11/condition2 原生执行器仍未实现，不能据此宣称正常通关。

剧情 opcode11 的 condition0..6 现按 `4211A3` 执行身份/状态谓词，而非选择菜单；condition2 对应 `440D4C(mode0)`，在两军中查找身份，仅排除 `40h` 有而 `80h` 无的角色，不按外观或 HP 推测。condition3 只检查友军身份缺席。满足者显示原对白后消费，不满足者保留记录；剧情收束允许未命中的休眠谓词保留，以执行原关卡 header 后继。第146关实际两支对应身份34存在/友军缺席，分别9/10句；`tests\original_campaign_stage146_test.cpp` 验证两支与状态真值表，不创建原菜单。

第26关原资源是 NPC-only 战斗：开场仅 behavior1 的22号角色活动，32号第2回合到场，其余由 opcode6 插入；持久主角队伍不应自动激活。连续 smoke 的“没有可控角色即失败”检查现允许真实存活友军 NPC。战斗 flag 驱动的单角色动画（opcode12/condition10、16、1）及等待（opcode8/condition7）复用现有可见脚本执行器，结束后返回战斗派发，实际插入原始援军。`tests\original_campaign_stage26_test.cpp` 默认读取真实 `run-72852-stage26-start.sav`，验证脚本、援军与主角保持未激活。opcode12/condition22 现以原始存档事务执行 `461535`：角色原军团/槽位不变，WORD +8=0、+A=1、+C=0，仅清 status33 的40h；死亡或未激活角色按461149成功但不变。condition1 是 `461236` 撤离（40h置位、80h清除），不是控制反转，不修改控制字。测试比较整个原始角色记录及存档桥往返，确保没有额外角色、复活或统计修改。真实强化流程已从第26关通过原结算到40关，再到41关。第41关原资源有两个替代胜利组 `{33}` / `{31}`，真实存档中主角11未活动，22/32可控。`438656`→`4620AA(edx=0)`→`45CDF3` 明确按友军身份字+2找11（`41E6A8`由flag41初始化分支时改找1），不能把22/32猜作出口主角。修正仅限 smoke 输入策略：只有原身份角色可控且存活活动时才尝试鼠标出口路线；可选出口不可用时继续正常自动行动，不激活主角、不修改胜利条件。`tests\original_campaign_stage41_test.cpp` 使用真实 `run-97384-stage41-start.sav` 覆盖此分支；第10/16关原出口策略测试保持通过。真实强化续跑已验证41关T3结算→42关T4结算→150剧情→46关；46关原先剩2敌至T99，T100原回合上限失败。真实资源胜利条件为 `{0}` 歼灭，不是出口；剩敌在 `(19,42)` / `(9,64)`，均 abilities80h 隐形，无同高度近战邻格。原 `441EFD` 要求相邻观察者且高度差 -3..+2，自动控制不会主动探查不可见目标。仅强化 acceptance 增加通用输入：全部剩敌不可见时，经现有合法 BFS/鼠标移动一个已有可控友军至真实揭示格，随后交回原自动控制；不改 AI、隐形、地形或脚本。鼠标落点遮挡时选择同一路径上可拾取的中间格。`original_campaign_stage46_test.exe` 验证真实资源、隐藏目标、揭示路线与自动控制交还；真实 SDL 已46关T27→47关T5→157，原下一边界为157事件3 `Original45E107 scripted movement unavailable`。157真实事件指定角色6从 `(17,29)` 到 `(11,42)`，角色 movement_type=4、movement_class=0。失败来自 `Battle::scripted_route` 误限 movement_type==1，不是缺角色、并行或路线不通。原 `45E107` 调用 `476707(8000h)` 建图、`45E450` 执行原路径；后者明确含类型4表现分支，并没有类型1限制。现允许已有地形步耗支持的1..10类型，仍由原移动类型计算地形路线，保留80ms逐格等待/朝向，无瞬移、AP退款或激活。`tests\original_campaign_stage157_test.cpp` 从真实47入口经持久部署构造157，验证类型4路径每条边、等待、终点、资源不变和原选择续关48；141单人/并行回归保持通过。真实强化 SDL 从47入口验证47→157→48（T15结算）→50；50在T12起显示敌0但至T24仍未结算，已停止验证进程，此边界尚未诊断。日志 `out\stage157-flow.log`，下一真实入口 `out\strengthened-flow-acceptance\run-50492-stage50-start.sav`。日志 `out\stage46-reveal-flow.log`。不代表正常通关。日志 `out\stage41-exit-flow.log`，下一关真实入口 `out\strengthened-flow-acceptance\run-87644-stage46-start.sav`。

## 原程序与分析资料

分析目标为原 Windows x86 程序 `WSUPERSP.EXE`，SHA-256：

```text
58A9808E090DE51F56059F965D313BB274A08B82A3526E3C8991C4D3F978AAD2
```

本地 IDA 数据库及完整汇编位于 `analysis\ida\`。运行需要用户本地已有的原游戏资源，本项目介绍不包含原游戏程序或资源的分发授权。

## 2026-11-10 original58 ->158 late-flow verification

- Unified VS18 `S2NE.sln` Release/x64 build PASS (`out\stage158-build.log`), using D:\project\vcpkg. Existing warnings remain.
- Fixed story-entry omission: 410478+24 calls44A4A9(group0,mode0), just as40FEE9 calls it with mode1. Both entry paths now remove only original temporary class1 records (death20 or absent40), preserve retained original-slot correspondence, renumber, and clear the first bytes of11 group triples. Story mode0 applies45DDA6 pool/status/presence reset before427870 arrivals. No army truncation or arbitrary dead-role removal.
- Genuine isolated run-86412-stage58-start.sav regression:31 allied slots ->14 retained ->35 after original158 arrivals; retained growth bytes40..9F, identity/order, input state, and original save roundtrip checked.
- Original158 grouped movement has header flag13 but terminator flag0. 422074 ends at event_count when no matching terminator; the decoder now permits that and completion is bounds-safe. 4619AB drops only actors whose461EF8 route failed, not the entire parallel operation; regression checks failed actors do not teleport. Inert no-trigger group terminators do not block exhausted-story header routing.
- Actual SDL dummy strengthened run (`out\stage158-flow.log`):58 **內鬨** victory turn4 ->158 **休戰儀式** story executed ->59 **奚族聖地** original autosave reload and battle victory turn3 ->60 **返鄉的遊子** original autosave reload, then original defeat turn100 (16 enemies still alive).61 **超時空傳說** NOT reached or claimed complete. No forced victory, enemy removal, tactic experiment, or normal difficulty-rule change.
- Strengthened stats are NON-ORIGINAL flow-only evidence, not balance/legitimate clear evidence. All saves isolated under `out\strengthened-flow-acceptance`; user SAVE untouched. Replay checkpoint: `run-32276-stage60-start.sav`.
- Focused freshly rebuilt native regressions PASS: original_campaign_stage158_test (cleanup, bounded group scan, per-actor failed-route preservation, inert terminator route); original_initialization_test; original_campaign_victory_route_test (both12->15/14 raw entries); original_scripted_movement_test (141 single and both parallel groups); original_campaign_story_test (4->154 choices5/8). The older story test expected temporary actors to survive; updated to the original44A4A9 retained identity/growth contract. Changed-source editor diagnostics clear. SDL helper exited after genuine turn100 defeat; no helper/writer left running by this task.

## 2026-11-10: stage60 teleport chain -> genuine stage61 entry (strengthened only)

- Inspected the cancelled agent's actual `tests\original_campaign_stage60_test.cpp` / exe: it was a resource/visibility dump, not a passing quest regression. No SDL helper was running on entry. README/PROGRESS untouched.
- Original trace: `4296BF` caches first 20 category35 map cells in row order; `40DB58` / `40DC07` invoke `4608B2` at allied/enemy side end; case35 calls `42FFA2`, rejecting occupied pads in BOTH armies, selects with native bounded RNG, then `45DB10` relocates while preserving actor action state. Implemented production pad loading/side-end settlement; does not alter AP/HP/learned bits/flags or remove enemies. No destinations are hardcoded. Original teleport effect194 animation remains unimplemented (coordinate/state execution only).
- Acceptance-only manual mouse/BFS pad policy is restricted to stages60/61 and `--smoke-campaign-strengthened`. It routes to a real reachable pad only while no living enemy can be approached in the component, exits the side normally on the pad, and returns to existing combat once enemies are approachable. An initial probe unnecessarily stayed on pads even with reachable enemies; corrected before the successful second probe. No normal AI/stat changes.
- Actual SDL dummy command, from game root: `S2NE\out\x64\Release\envoy-opening.exe --smoke-campaign-strengthened . S2NE\out\strengthened-flow-acceptance\run-32276-stage60-start.sav` with SDL_VIDEODRIVER/SDL_AUDIODRIVER=dummy. `out\stage60-pad-flow2.log`: stage60 original scripts/victory report T50 with8 remaining enemies -> stage61 **超時空傳說** -> original autosave reload verified. Persistent entry: `out\strengthened-flow-acceptance\run-69588-stage61-start.sav`. This is NOT a stage61 ending or normal-balance clear. Stage60 is NOT final. Existing script routing, not forced victory or enemy deletion, selected61.
- Final-policy SDL bounded probe from that61 entry: `out\stage61-pad-flow.log`, manual input onto (9,81), later real pad relocations and normal combat; stopped at T43 with21 enemies. Do not repeat blind auto as a final-quest solution. All three owned SDL helper trees explicitly stopped using their own shellIds; subsequent process query found no envoy-opening.exe.
- Exact next missing coherent executor: category12 map item pickup -> persisted inventory -> item USE -> native learned-skill transaction -> hostile targeted spell effect34 -> boss equipment removal. Original resource item301 **不明礦石**: type12 (+14), consumable1 (+15), operand3 (+17), operand12 (+19), effect65 (+25). `450550` case65 at450FE1 calls `451DDA`; do not treat pickup as learned magic or set flags. Spell32 **高溫氣化**, group2/bit11, effect34 (+19), native record raw `03-0C-B0-AA-B7-C5-AE-F0-A4-C6-00-00-00-28-00-40-01-04-01-22-00-00-00-46-00-00-00-00-00-00-00-02-00-01-00-00-00`. Existing OriginalSkills executor deliberately only supports self-heal88, so32 is NOT usable yet. Stage61 resource boss role20 at(14,45), equipment14/125/183/165/199/237; item237 **邪神護身符** special25 (+25). Walkthrough locates301 at(3,65); category12 pickup native455324 ->42D200 clears the collected chest and its four candidate items; this has NOT been wired. Stage61 victory33, death dialogue group completion13 -> original flags42/43 and ending flags73/75; no synthetic flags added.
- Verified fresh MSVC tests: `cl /nologo /std:c++20 /EHsc /utf-8 /bigobj /I S2NE\src S2NE\tests\original_campaign_stage60_test.cpp /Fo:S2NE\out\original_campaign_stage60_test.obj /Fe:S2NE\out\original_campaign_stage60_test.exe`, then exe >`out\stage60-pad-test.log`: PASS real60/61 pad lists, occupied-source exclusion, both-army/side guards, all-full/dead/inactive rejection, AP/HP/learning/flags preservation, original save bridge coordinate roundtrip, actual60 entry manual-pad request. Fixture checks do not claim campaign victory. `battle_test.cpp` freshly compiled with same flags: PASS existing movement, combat, AI, first-route tests. Native `MSBuild S2NE\S2NE.sln /p:Configuration=Release /p:Platform=x64 /v:quiet /nologo` and Debug equivalent: exit0 (existing C4996 warnings); logs `out\stage60-release-build.log` / `out\stage60-debug-build.log`. Environment: VS18 Community vcvars64.bat, D:\project\vcpkg. Changed production editor diagnostics clear.

## 2026-11-10: stage61 mineral pickup -> USE -> learned32, bounded real SDL proof (partial quest)

- Implemented production category12 pickup455324 ->42D200: original map marker/chain, four-way native RNG, valid item1..302, locked-marker ability117 bit20, allied/NPC army-metadata guard, inventory cap99, clear all four candidates, remove both rendered object and runtime terrain object. OriginalSaveBridge now restores/captures actual raw dynamic chains (4F6FBC/4F705C), preventing collected mineral respawn. No quest flags changed.
- Implemented real command2 inventory learning-item list (original item names), keyboard navigation, USE confirmation and cancellation; category12/effect65 dispatch follows450550 ->451DDA. Item301 **不明礦石** grants only group3 ordinal12 (**高溫氣化**, skill32), raises proficiency to threshold320*10 only when newly learned, consumes exactly one inventory entry; already-known skill skips proficiency rewriting. Existing native item class/attribute/identity/battle-use masks retained. Modal pauses battle ticks; item commits cannot be undone by movement cancellation. Unsupported item effects remain disabled rather than pretending to execute.
- Acceptance-only stage61 policy prioritizes reachable (3,65), otherwise legal pad traversal. Real normal mouse movement and command cycling/Enter confirmation are used, not inventory/spell/flag injection. Stats remain boosted ONLY in explicit --smoke-campaign-strengthened. First probe found pickup ordering skipped before acceptance end-side; fixed pickup before synthetic next input. Second probe exposed cached command availability after pickup; fixed refresh. Earlier probes stopped by their specific shellIds stage61-mineral and stage61-mineral2; no name-based killing.
- FINAL real SDL command from game root (SDL_VIDEODRIVER=dummy, SDL_AUDIODRIVER=dummy): `S2NE\out\x64\Release\envoy-opening.exe --smoke-campaign-strengthened . S2NE\out\strengthened-flow-acceptance\run-69588-stage61-start.sav` -> **exit0**, 525 frames, T4 category12 pickup301 by11 -> normal USE301 -> learned32, original encode/load/restore verified. Log `out\stage61-mineral-input-final.log`. Persistent 15852-byte checkpoint `out\strengthened-flow-acceptance\run-84592-stage61-mineral-learned.sav`. Earlier successful T2/T11 checkpoints exist too. This command deliberately stops at the proven partial boundary, explicitly logs NOT an ending, and does not mark clearVerified or mutate completion flags.
- Native Release AND Debug: `MSBuild S2NE\S2NE.sln /p:Configuration=Release /p:Platform=x64 /v:quiet /nologo` and Debug equivalent -> exit0. Fresh `cl /nologo /std:c++20 /EHsc /utf-8 /bigobj /I S2NE\src S2NE\tests\original_campaign_stage60_test.cpp /Fo:S2NE\out\original_campaign_stage60_test.obj /Fe:S2NE\out\original_campaign_stage60_test.exe` -> exe PASS. Tests cover real category12 pickup fixture, rendered/runtime clearing, no duplicate/reload respawn, raw inventory/chains/flags, exact learned bit/proficiency floor/already-known behavior, non-learning rejection, learned native skill menu rendering, MP rejection, existing pad regression. Fresh original_save_bridge_test -> PASS real-record subset5 and full22/42-unit fixture roundtrips. Combined log `out\final-quest-final-validation.log`; changed-file editor diagnostics clean. No README/PROGRESS/user SAVE changes.
- EXACT REMAINING BLOCKER: production hostile targeted skill32 executor and its normal magic-target input. OriginalSkills still only executes self-heal88: skill32 is learned/renderable and MP-gated but cannot yet target/cast; no guaranteed amulet deletion substituted. Native effect34:46D874 chooses one nonzero equipment slot1..5 (skip weapon0), records slot+1 in context600; resistance uses (caster ST[128] - chosen item signed defense[21])/2, then46D496/407E2D chance path, native percent stream and target-state gates. Successful467E3A ->44D11B(ecx1) consumes that selected equipment with original stat compensation. Must trace complete hit/payment/range/award settlement before wiring, rather than equating effect34 formula46E858 to deletion or guaranteeing237 specifically. Boss20 still present and equipment237 **邪神護身符** not removed. Stage61 death event completion13 -> flags42/43 -> ending73/75 remains UNVERIFIED. Stage60 is not final; stage61 **超時空傳說** ending has NOT been reached. Teleport effect194 presentation also pending. Natalia and broad magic/sword animation work not started.
- All final SDL runs exited naturally; no owned helper/writer remains running. Initial source writes were cleared by parent; no nested delegation was launched.

### Resource-backed terminal presentation subset —41 pages actually displayed, NOT complete native ending

- Added `original_ending.hpp` and terminal modal wiring, resume from genuine native75 terminal save, original text BIN3 decoding (473A55 selector0/id3), CD2 PCX40 intro/PCX26 report backgrounds (425092 selector3 bias26), native large portraits, variant-index3 role-specific original text and relationship pages. Original13-role priority11/1/22/2/3/4/14/15/18/32/16/17/42 then remaining1..36 retained. Keyboard/mouse advances actual SDL pages; normal battle input remains modal-blocked. One-based flags remain **42=0,43=1,73=0,75=1**. No invented finish flag; serialized original75 is the persistent result, not proof every original scene completed.
- Fresh focused regression rendered all41 subset pages and native75 variant; native Release AND Debug builds pass. Exact commands remain the `cl /std:c++20 /EHsc /utf-8 /bigobj /I S2NE\src ...original_campaign_stage60_test.cpp` runner and `MSBuild S2NE\S2NE.sln /p:Configuration=Release|Debug /p:Platform=x64 /v:quiet /nologo`; output `out\ending-final-validation.log`.
- Actual SDL command `envoy-opening.exe --smoke-campaign-strengthened . .\S2NE\out\strengthened-flow-acceptance\run-79908-stage61-terminal-pending.sav` displayed41 pages through normal Return inputs, encoded/reloaded original result unchanged, saved `run-60956-white-ending-viewed.sav`, then deliberately **exit1**, log `out\white-ending-final-display.log`. Earlier exploratory42-page run94908 included a duplicate summary and premature success/return; corrected before final validation, NOT accepted as complete ending evidence.
- Precise outstanding native presentation:41F011 role illustrations are tall PCX scrolls (height-479,3-row steps/10ms), not implemented by the current640x480 decoder;41EDD4 full campaign summary and41F167 class/stat panels are not fully reproduced;41ED8A calls45EDAB movie3 tail after report cleanup. Those are known local resources/executors needing expansion, not missing data. Current host explicitly fails at this boundary instead of inventing a return-to-title or declaring completion. Full spell compositor remains separate. No Natalia work. Source/tests/VALIDATION only; all owned SDL helpers exited, writes stopped.

### Native terminal trace follow-up — original75 settled, final presentation still blocked

- Corrected an earlier diagnostic off-by-one: native flags are one-based. Actual flags are **42=0,43=1,73=0,75=1**, not both42/43=1. Pristine61 event3 is opcode9/condition1, trigger13 AND operand42, completion73; event4 is trigger13 AND43, completion75. Assembly420A3B case1 checks every valid flag operand;41C527 sets record+11 completion only after processing. Added that missing predicate executor, without synthetic flag assignment. Fresh actual replay save verifies75 only;73 correctly remains0.
- Native terminal proof:40DDD5 returns17h after successful battle and4F7CE0==0;40D6D6 dispatches41E9CF directly. Removed erroneous unconditional hub construction at terminal rewards. Until the actual ending presenter exists, it now stops explicitly at that native boundary instead of fabricating an ending or trying deploy_next(0).
-41E9CF calls41F6E3 (473A55 resource3), displays original resource3 strings, selects ending roster text by flags72..79 priority (this run75 => index3), calls41EDD4/41F167 report/role pages and41F5DD/41F746 lifecycle. **Remaining exact executor is this resource3 ending roster/report renderer, original page input/timing and persistent completion result after presentation.** No placeholder scene/title or synthetic success was added. Native41E6A8 is a separate menu/new-game reset path, not proof for this ending.
- Commands: fresh `cl /nologo /std:c++20 /EHsc /utf-8 /bigobj /I S2NE\src S2NE\tests\original_campaign_stage60_test.cpp /Fo:S2NE\out\original_campaign_stage60_test.obj /Fe:S2NE\out\original_campaign_stage60_test.exe` then exe PASS; `MSBuild S2NE\S2NE.sln /p:Configuration=Release /p:Platform=x64 /v:quiet /nologo` and Debug both PASS. Combined `out\terminal-final-validation.log`. Actual SDL `envoy-opening.exe --smoke-campaign-strengthened . .\S2NE\out\strengthened-flow-acceptance\run-88852-stage61-hostile32.sav` reached genuine death/rewardT14 and native terminal boundary, persisted `run-79908-stage61-terminal-pending.sav` with75=1, exit1; log `out\stage61-terminal-final.log`. **Ending presentation NOT reached.** No Natalia work. Owned helpers exited; writes stopped. README/PROGRESS untouched.

### Explicit strengthened MP follow-up — actual cast, amulet removal and boss death

- User explicitly authorized MP strengthening for flow validation. Only `--smoke-campaign-strengthened` now floors living controllable/allied-NPC maxMP to400 and replenishes currentMP; log declares this NON-ORIGINAL. Normal MP20/maxMP20 and original spell40 payment/rejection remain unchanged. No learned/equipment/flags injection, RNG reset or source-save overwrite. Focused normal-versus-boost eligibility regressions pass in `out\hostile32-mp-validation.log`; Release/Debug builds pass (`hostile32-mp-build.log`, `hostile32-mp-debug.log`).
- Actual SDL normal selection/skill navigation/target confirmation cast32 at boss20: `run-91196-stage61-hostile32.sav`, hit chance65/roll63 removed165. Following same native RNG stream within the repeated-cast session removed183 then **237邪神護身符** (chance74/roll11), saved/reloaded `out\strengthened-flow-acceptance\run-88852-stage61-hostile32.sav`. Log `out\stage61-repeat-cast-input.log`. No selected-slot guarantee was used. Legal approach policy now recovers from blocked connected components and insufficient repeat-cast AP; it preserves native turn recovery instead of injecting AP.
- Continued actual amulet-removed save using legal approach and normal physical attack submenu/target input. Boss20 HP4000→0, original death script and victory report executed at T14. Persisted `out\strengthened-flow-acceptance\run-24456-stage61-boss-outcome.sav`; log `out\stage61-boss-outcome-final.log`. Original runtime flags42=1,43=1;73=0,75=0; selected next route=0. This is strengthened-flow evidence, NOT legitimate balance/MP progression or a verified ending.
- **Precise remaining production branch:** `Prologue::advance()` campaign_rewards always creates `OriginalInterbattle` and enters hub, even for terminal selected_route=0. Hub option4 reaches `OriginalCampaignPersistence::deploy_next()` which rejects target0 with `Original intermediate campaign stage requires dedicated dispatcher`. Original terminal/ending dispatch after native61 death flags42/43 must be traced and implemented; setting73/75 or treating route0 as an ending without that trace is deliberately NOT done. Full original spell effect compositor (beyond installed class11 caster poses) remains unverified; no placeholder effect added.
- All owned SDL helpers stopped by exact shell IDs or exited naturally; source writes stopped. README/PROGRESS untouched.

### Hostile32 implementation follow-up — verified partial, not final ending

- Wired attack submenu magic entry without replacing physical attacks; original learned spell list, keyboard/mouse selection, range markers, target selection and confirmation now reach the production hostile32 transaction. Casting pose uses installed class action type11 / field10; effects commit only after its queued poses drain. Full original spell-effect194 compositor remains pending, not claimed reproduced.
- Native effect34 uses random occupied equipment slots1..5, caster ST minus selected signed item defense, original chance scaling/shuffled percent stream, hit-before-status-clear and immunity gate, equipment removal/stat compensation, native proficiency/XP bridge, then AP/carry/MP payment. No HP damage, guaranteed237 selection, spell unlock, ending flag mutation or enemy deletion was added. Full runtime3D/3E/3F status persists even without the optional action-status snapshot.
- Fresh focused quest test passes hit/miss, noMP recheck, noAP, out-of-range, stale learned selection, award rollback, immunity, equipment-only effect, raw status roundtrip and earlier mineral/pad regressions. Native Release and Debug builds pass. Logs: `out\hostile32-final-validation.log`, `out\hostile32-final-build.log`; existing compiler warnings remain.
- **Actual requested input save cannot currently pay the spell:** unmodified `run-84592-stage61-mineral-learned.sav` protagonist has MP20/maxMP20, ST187 and learned32; spell32 costs40 MP (20 only with native ability0x20). Actual SDL run reached the normal spell list and rejected with reason1 (MP); no forced MP/ability was added. Random pad routes vary; final bounded run stopped at T31 with boss20 HP4000/accessory237 intact and saved `out\strengthened-flow-acceptance\run-49056-stage61-quest-route-blocked.sav`, exit1, log `out\stage61-hostile32-final-input.log`. Acceptance now refuses blind auto when the unresolved amulet quest has no legal route or exceeds the bounded approach budget.
- Next required gameplay step is legitimate caster MP capacity/cost reduction (or another eligible learned caster), followed by a legal boss approach and actual cast; full effect animation and real ending dispatcher remain unverified. No successful SDL cast,237 consumption,boss death or ending is claimed. Source/tests/VALIDATION only; README/PROGRESS untouched. Owned SDL processes stopped by their exact tool shell IDs; no running helper remains.

### White ending executor follow-up — native CD2 long images, reports and movie tail

- Implemented `41F011` variable-height original PCX bottom-to-top scrolling, three rows per timed step and final hold; priority roster images DAT27..39 precede their original role report, remaining roles follow, then `41EDD4` statistics/text and movie3. Added the previously omitted movie4/movie5 transitions between BIN3 intro pages. No invented finish flag or title transition.
- `41F167` now includes original PRG class image at the assembly center/baseline (225,180), portrait, profession/level, cached runtime132 combat value and original WORD1F/21 counters. Added native small-font role paragraph geometry (120,230; 25 columns;20px rows) and relationship heading. `41EDD4` reads saved progress/difficulty/currencies/groupD6 and summed role counters rather than invented values. Original save fields are exposed read-only for presentation, not rewritten.
- Critical resource regression: restored terminal stage-global0 was selecting CD1 dummy two-frame ending movies. `OriginalEnding` now derives CD2 from actual stage61 regardless of stale incoming disc. Full local CD2 movie3=1142 frames, movie4=136, movie5=95. FLIC first-frame offset80 and rewind now agree; native COPY occasionally declares payload length two bytes short, and native45F28D copies width*height bytes regardless. Decoder accepts only this precise variant with archive bounds checks. Opening21/22 remain frame-identical to independently recovered originals.
- Native post-return proof:41ED8A movie3 returns through41EDCB;40D6D6 jumps40D7CB;WinMain428F26 then428F2E cleanup and returns0. Host now exits through normal SDL cleanup after the actual tail; it never manufactures a return-to-title. Acceptance postcondition is marked only after timed presentation returns, not on terminal pending or text pages.
- Fresh commands (VS18 vcvars64.bat): `cl /nologo /std:c++20 /EHsc /utf-8 /bigobj /I S2NE\src S2NE\tests\original_campaign_stage60_test.cpp /Fo:S2NE\out\original_campaign_stage60_test.obj /Fe:S2NE\out\original_campaign_stage60_test.exe` then exe PASS (`out\ending-tail-tests.log`); `disc_intro_test.cpp` same compile/execute PASS (`out\ending-movie-regressions.log`), including all1142/136/95 full CD2 frames and rewind. `MSBuild S2NE\S2NE.sln /p:Configuration=Release /p:Platform=x64 /v:quiet /nologo` and Debug PASS (`out\ending-tail-release.log`, `out\ending-tail-debug.log`). Existing warnings only. Tests ensure Return cannot skip timed phases, incoming CD1 cannot select dummy movies, flags/raw input bytes unchanged, all panels render and timer sequence settles.
- Actual SDL intermediate run90684 from genuine79908 played all57 phases/full CD2 movie3, normal Return input only at reports, saved/reloaded `run-90684-white-ending-viewed.sav`, normal exit0 and14009 presented frames (`out\white-ending-complete-sdl.log`). This run used raw FLIC delay; final timing was subsequently corrected to native45F032 AF12 integer `delay*10/48` centisecond wait (`47308B` is DOS hundredths), and is being reverified separately. Earlier103332 used CD1 dummy movies and ended with a smoke-postcondition error: rejected as full ending evidence.
- Exact original one-based flags remain42=0,43=1,73=0,75=1. Explicit strengthened MP helper and normal MP40 cost/rejection remain unchanged. No Natalia or spell-compositor work.
- Fidelity limits: original ANI BIN7 frame-specific music/effect scheduling, palette fade envelopes and original device-dependent input skip masks are not fully ported; visual movie frames and timed long images are implemented, but this does not certify bit-exact audiovisual parity. BIN7 movie3 script has17 sound events (first frame music32, later effects52/190/179/etc.); movies4/5 each start music14. Existing host audio is not an executor for those native events. Full hostile32 effect compositor remains a separate outstanding task.
- FINAL native-timing actual SDL command: `set SDL_VIDEODRIVER=dummy`, `set SDL_AUDIODRIVER=dummy`, `S2NE\out\x64\Release\envoy-opening.exe --smoke-campaign-strengthened . S2NE\out\strengthened-flow-acceptance\run-79908-stage61-terminal-pending.sav`. PASS normal exit0,25555 rendered/presented frames. Log `out\white-ending-final-native-sdl.log`: movie4=136frames/380ms, movie5=95frames/380ms, all priority long-scroll and19 role reports, native campaign summary, full movie3=1142frames/130ms, original cleanup/exit branch. Result checkpoint `out\strengthened-flow-acceptance\run-49056-white-ending-viewed.sav` written and byte-stable reload verified. Source79908 SHA256 remains `1D64E38324595851F57857A8BED88D696DBB033F9834F9D97234D3F22EBD0874`; flags42=0,43=1,73=0,75=1. This proves full visual ending sequence/return, not the outstanding native sound/fade parity. All owned SDL helpers exited naturally; probe source/object/exe removed; writes stopped. README/PROGRESS untouched.

### Natalia fresh strengthened batch 1 — actual 71 → 72 → 73 → 161 blocker (2026-11-10)

- Added explicit `--smoke-campaign-strengthened-alternate` (optional argv3 remains an original save). Default white option unchanged. Fresh invocation uses normal title/setup, easiest mouse choice (native difficulty1), stage140 Natalia choice; native initialization supplies her own party/inventory/learned state. Only existing explicit HP/ATK/DEF/MP strengthening is applied; no injected flags/items/learned skills or forced victories. Smoke SAVE roots now live under project `out\isolated-saves`, isolated from player SAVE and cleaned on exit.
- Final VS18 native S2NE.sln Release and Debug builds PASS (`out\natalia-strengthened-release.log`, `out\natalia-strengthened-debug.log`). Native easiest Natalia opening/relationship/persistent72 regression PASS (`out\natalia-route-tests.log`); its synthetic eliminated-enemy subsection is only an isolated settlement test, not gameplay evidence. Disc boundary/native DAT82-entry regression PASS (`out\natalia-disc-tests.log`). Editor diagnostics: none in changed source/test.
- Actual fresh command: dummy SDL video/audio, `S2NE\out\x64\Release\envoy-opening.exe --smoke-campaign-strengthened-alternate .`. Log `out\natalia-strengthened-batch1-sdl.log`, run100460. Actual victories: **71 惡兆 turn11**, **72 青色草原 turn12**, **73 神秘的氣息 turn10**, including original dialogue, rewards, hub and next-stage deployment. Persistent start checkpoints `out\strengthened-flow-acceptance\run-100460-stage{71,72,73}-start.sav` were byte-stable reload verified. All are route1, progress1/2/3 respectively, **CD1**, actual native DAT5 read169978 bytes. Stage72 is explicitly NOT CD2 proof.
- Actual substantial boundary/blocker: **161 返回奚族**, original story event1 opcode12 condition22 lacks native executor. Fresh run exits1, not a pass/full-route claim. No invented opcode mutation/scene skip applied without original executor evidence. Reproduction from the genuine stage73 checkpoint using the same alternate option reaches identical blocker; log `out\natalia-stage161-blocker-sdl.log`, exit1. Durable full original checkpoint `out\strengthened-flow-acceptance\run-93476-stage161-blocked.sav` written/reloaded byte-stably. SHA256 `C65C9EBF4C4459902FD1EA4B04A5CC5275FB94A13A4DF4BFEEE227302C308502`.
- Added per-stage route/progress/disc/resource-read evidence and durable blocked-state capture. Alternate first batch stops successfully only upon entering its first actual CD2 battle after persistence/DAT reads; this boundary was **not reached**. Actual automatic CD2 transition and Natalia ending resources remain unverified until opcode12/condition22 is implemented from assembly and continued naturally. No full Natalia/ending claim. White ending renderer/manual UI untouched; audio/fade and spell-learning audit remain deferred.
- Initial exploratory run102152 stopped before source rebuild; its isolated owned SAVE directory removed. Final runs100460/93476 exited; no owned SDL writer/helper remains. README/PROGRESS untouched.

### Natalia batch 2 — shared461535 story executor and actual automatic CD2 transition (2026-11-10)

- Root cause: battle dispatcher already invoked the production scripted-role transaction for opcode12/condition22; mode3 `dispatch_story` had no equivalent branch, and its condition1 withdrawal duplicated only runtime status mutation. Both dispatchers now use one `apply_scripted_control` transaction (capture → existing OriginalInitialization::insert_scripted_roles/mutate_scripted_control → restore → deferred raw commit). Existing461535 exact control WORD8=0/WORD10=1/WORD12=0/status40-clear semantics, fixed army membership and inactive/dead successful no-op are reused unchanged. Story completion/dialogue uses original event records; no activation, army transfer or replacement statistics.
- A second tightly coupled blocker appeared on actual saved-story resume: restore_battle always started the mode3 saved node as a battle, yielding a false stage161 exit-route failure after conversion. Saved mode3 now dispatches story on the first tick after adapters are attached. Normal campaign entry/resume explicitly retains original briefing and disables that saved-resume pending dispatch. Rejected exploratory log `out\natalia161-continue-sdl.log` is NOT stage161 victory evidence.
- Regression `tests\original_campaign_stage161_test.cpp` loads genuine `run-93476-stage161-blocked.sav`, checks exact raw conversion bytes, byte-identical dead/inactive no-op, production adapter dispatch, no battlefield automation, completed event1 and actual native route74. PASS `out\natalia161-tests.log`. Existing genuine stage26 NPC entry/scripts/control/withdrawal/raw regression also PASS `out\natalia161-stage26-regression.log`. No scripted synthetic victory is used in161 test.
- Native VS18 S2NE.sln Release and Debug PASS (`out\natalia161-release.log`, `out\natalia161-debug.log`), changed source/test editor diagnostics empty. README/PROGRESS untouched; ending visuals/manual UI not changed.
- Actual command: dummy SDL video/audio, `S2NE\out\x64\Release\envoy-opening.exe --smoke-campaign-strengthened-alternate . S2NE\out\strengthened-flow-acceptance\run-93476-stage161-blocked.sav`. Run83796 `out\natalia161-batch2-sdl.log`: **161 返回奚族** story naturally completes → **74 邊城提那 victory turn5** → **75 驅離守衛 victory turn4** → **77 撤退保護戰 entered**. Original rewards/hub/deployment, not forced flags, items or skipped scenes. Story node counts toward native progression; actual saved route1/progress5 stage74 and progress6 stage75 are CD1, then **route1/progress7 stage77 automatically selects CD2**, loads native resources, enters battle and reload-verifies the original autosave. Each checkpoint also performs an actual selected-disc DAT5 read169978 bytes. Earlier72 remains CD1, not switch proof.
- Run ends normally exit0 after2753 presented frames at configured first-CD2 substantial batch boundary, NOT a victory at77 or Natalia ending. Persistent genuine checkpoints: `run-83796-stage74-start.sav`, `run-83796-stage75-start.sav`, **`run-83796-stage77-start.sav`**, in `out\strengthened-flow-acceptance`. Final77 SHA256 `0FBB8644F82AC3AFA9AF4ABEDA39AA98C8060DA4CD1CDB4631EC03DE791D84BA`. Byte-stable checkpoint reload verified by host. Continue from77 for later-route/ending validation; these remain unverified. Existing alternate option intentionally terminates at CD2 entry, so a subsequent batch needs an explicit later boundary rather than silently claiming completion.
- All owned SDL runs exited naturally and no envoy-opening process remains. Isolated SAVE cleaned on exit. No writer/helper left running. Audio/fade and post-both-routes spell-learning audit remain deferred.

### Natalia batch 3 — explicit post-CD2 continuation, genuine77 occupancy boundary (2026-11-10)

- Added `--smoke-campaign-strengthened-alternate-continue`: explicitly disables ONLY the earlier first-CD2 batch success stop. Alternate identity, optional original argv3 resume, independent story flags, per-stage CD selector/native DAT read, byte-stable save reload and all original outcome checks remain enabled. Existing white option and earlier alternate bounded option unchanged. No flags/items/learning injections, scene skips or forced outcomes.
- VS18 native S2NE.sln Release/Debug PASS (`out\natalia77-release.log`, `out\natalia77-debug.log`). Existing genuine161 story regression PASS (`out\natalia77-story-regression.log`); native disc selector/all82 DAT entries regression PASS (`out\natalia77-disc-regression.log`). Changed main editor diagnostics empty.
- Actual command with dummy SDL video/audio: `S2NE\out\x64\Release\envoy-opening.exe --smoke-campaign-strengthened-alternate-continue . S2NE\out\strengthened-flow-acceptance\run-83796-stage77-start.sav`. Run103152, `out\natalia77-continue-sdl.log`, progressed normal legal input/movement in **77 撤退保護戰**, native difficulty1, route1/progress7/CD2, beyond previous CD2 success boundary. Natalia moved from17,27 through13,28 and9,33 to9,36. At turn4 acceptance stopped with `No legal route to original stage77 exit`, exit1; NOT victory, not later-stage/ending proof.
- Durable genuine original checkpoint `out\strengthened-flow-acceptance\stage77-route-blocked-103152.sav`, SHA256 `D03DAC46C2BFCB4485513CF329AC8F917D4BB6E2F4B7A44ED399A82908322F02`. This is a concrete current-roster input-policy obstruction, **not established production terrain/executor defect**. New `tests\original_campaign_stage77_test.cpp` loads it and independently replays real Battle step-cost/passability: native protagonist exit cells0,48 /0,49 /1,50 /0,51 /1,51 all empty; ignoring occupancy terrain path EXISTS, actual live-occupancy path does NOT. There are16 living hostiles and15 living friendlies. No actors/flags/terrain changed to make it pass. Regression PASS `out\natalia77-route-tests.log` establishes occupancy-only blocker rather than incorrectly treating CD2 terrain as broken.
- Per requested bounded substantive batch, stopped at this proof-backed routing boundary rather than starting tactical search, removing collision rules or forced-clearing enemies. Further77 play requires normal combat/moving blocking units; no evidence justifies production terrain changes. Full Natalia route/ending remains outstanding, white ending unchanged; audio/fade deferred.
- Actual SDL helper exited1 naturally; regression helpers exited0. No envoy-opening process or owned writer remains; isolated SAVE directory empty. README/PROGRESS untouched.

### Natalia batch 4 — temporary occupancy is recoverable; actual77→89 victories (2026-11-10)

- Investigated exact native77 objective data: victory group0=[31] (protagonist exit), group1 empty. There is NO eliminate-all alternative. Acceptance prematurely attempted exit whenever flag1 was set; enemy occupancy could temporarily disconnect the valid path. Native477DE4/477E50 collision is already correct: same-side units allow transit, all occupied destinations disallow landing, hostile occupants block transit. No production collision or side metadata changed.
- Explicit Natalia continuation now persists the occupied-route checkpoint, cancels selection, invokes normal automatic battle actions while living hostiles remain, then retries the original objective on return to player control. It does not turn a temporary occupied route into an irrecoverable error, does not mark victory, remove units, alter objectives or bypass collision. Truly unresolved routes without hostiles retain diagnostic failure. Existing per-stage600s bound prevents endless retries. White and first-CD2 bounded modes unchanged.
- Genuine fixture regression `original_campaign_stage77_test.cpp` now asserts exact native objective groups, saved independent Natalia difficulty1/CD2 and terrain-valid/currently-occupied route, PASS `out\natalia77-retry-tests.log`. VS18 S2NE.sln Release/Debug PASS (`out\natalia77-retry-release.log`, `out\natalia77-retry-debug.log`), changed main/test editor diagnostics empty.
- Actual SDL command: dummy drivers, `S2NE\out\x64\Release\envoy-opening.exe --smoke-campaign-strengthened-alternate-continue . S2NE\out\strengthened-flow-acceptance\stage77-route-blocked-103152.sav`. Run97452 log `out\natalia77-retry-sdl.log`. Normal combat resolves temporary77 occupancy at turn4, then original **77 撤退保護戰 victory turn7**. Same recovery works at78 turn6 with10 hostiles; **78 突圍的選擇 victory turn10 → 79 暹族部落 victory turn4 → 162 罕泉 story completes → 81 暹族的伏擊 victory turn3 → 83 會面 victory turn4 → 84 丑子軍團 victory turn6 → 89 孤零的寶兒 original victory report turn10**. No tactical parameter sweeps, force flags/items/learning or scripted wins.
- New substantial executor boundary AFTER89 victory: `Ambiguous original story route`, actual process exit1. Source `OriginalCampaign::completed_story_route` currently throws if multiple consumed opcode9/condition6 records with active trigger flags disagree. This is a distinct route-selection/persistence reconstruction issue, not another dynamic-occupancy blocker; not bypassed by choosing an arbitrary route without native evidence. Full Natalia/ending remains unverified.
- Durable byte-stable reload-verified native start checkpoints `out\strengthened-flow-acceptance\run-97452-stage{77,78,79,81,83,84,89}-start.sav`. All actual nodes useCD2. Last89 native route1/progress14/CD2; checkpoint SHA256 `9E8BB4C59BF5C34C50D95F48C17CCA3002D6D53755663E3A9E10F9C48284B324`. This is a START checkpoint;89 victory is proven by the SDL log, not falsely claimed as post-victory persisted state. Resume89 to reproduce/fix exact ambiguous route using original script ordering/branch semantics.
- Owned SDL run ended naturally; no envoy-opening process, isolated SAVE leftovers, or helper writers remain. README/PROGRESS untouched. White ending/manual UI preserved; audio/fade and spell-learning audit remain deferred.

### Natalia batch 5 — assembly-proven ordered89 route override, actual continuation to100 (2026-11-10)

- Exact original-resource names for prior batch: **77 撤退保護戰**, **78 突圍的選擇**, **79 暹族部落**, **162 罕泉**, **81 暹族的伏擊**, **83 會面**, **84 丑子軍團**, **89 孤零的寶兒**. Prior actual outcomes remain77 turn7,78 turn10,79 turn4,162 story complete,81 turn3,83 turn4,84 turn6,89 victory report turn10. Names are decoded from native stage resources, not invented translations.
- Diagnosed89 actual resource records/header: header_next90; event16 opcode9/condition6 flag14→92, event17 flag15→90, event18 opcode9/condition1 requires flag15 AND66 and completes flag16, event19 opcode9/condition6 flag16→108. Thus90 and108 are sequential assignments in the same flag chain, NOT ambiguous mutually-exclusive story choices or arbitrary historical candidates.
- Verified original `analysis\ida\opening.idb.asm`:41C397 initializes event index0,41C3A0 increments through records;41C4E9 consumes record opcode,41C527 publishes completion flag and calls41FF6E,41C55E processes further story dispatch;41FF30 repeats41C33E until quiescent. Crucially420DC3 writes WORD5 directly to4F7CE0 for EVERY successful condition6, with no first-assignment guard/ambiguity rejection. Existing host dispatch already consumes the triggered records in this order; reconstruction incorrectly threw when a later native assignment replaced an earlier one. Removed only that invented ambiguity error, preserving consumed-record and active-trigger filtering and normal header fallback. This is native sequential override, not an arbitrary first/last tie-break or stage89 hardcode.
- New `tests\original_campaign_stage89_test.cpp` uses genuine97452 stage89 start, dumps original competing records, checks isolated consumed90 followed by flag16/108 override, inactive historical flag ignored, and original save capture/restore preserving108. PASS `out\natalia89-tests.log`; this synthetic subsection is dispatcher reconstruction evidence, not claimed gameplay. Rebuilt existing White stage12 both-choice reward/hub/deployment/raw-retention regression PASS `out\natalia89-white-route-tests.log`. Native VS18 S2NE.sln Release/Debug PASS (`out\natalia89-release.log`, `out\natalia89-debug.log`); changed source/test editor diagnostics empty.
- Actual independent Natalia command with dummy drivers: `S2NE\out\x64\Release\envoy-opening.exe --smoke-campaign-strengthened-alternate-continue . S2NE\out\strengthened-flow-acceptance\run-97452-stage89-start.sav`. Run106452, `out\natalia89-continue-sdl.log`: **89 孤零的寶兒 victory turn8 →108 殺之宴 victory turn6 →91 領主府內 victory turn10 →96 義助刑天 victory turn3 →97 賢者歐西德 victory turn9 →165 賢者歐西德 story complete →100 草原逐惡 entry**. Native89 chain actually selects108, matching resource+assembly test.108 temporary occupied route turn2/12 hostiles resolved through existing normal combat retry; no forced flags/items/learning/collision changes.
- Next substantial production boundary:100 before battle, `Original scripted movement destination unreachable`, exit1. Durable full checkpoint `out\strengthened-flow-acceptance\run-106452-stage100-blocked.sav`, byte-stable reload verified; SHA256 `8FC3DBDE782EE9C5894E3D8A8784919904E56559BD5CAC282643C36124F1DE4A`. Genuine start checkpoints run106452-stage89/108/91/96/97-start.sav also persist; native route1/progress14..18/CD2/DAT5 read verified.100 victory and Natalia ending NOT reached/claimed. Follow-up must inspect original scripted movement semantics/occupancy, not teleport a role or skip the event.
- All owned SDL/test helpers exited, no envoy-opening process or isolated SAVE leftovers. README/PROGRESS untouched. White ending visuals/manual UI preserved; audio/fade deferred.

### Natalia100 — original45E107 failure return, shared animated single-role executor (2026-11-10)

- Genuine106452-stage100-blocked fixture identifies event2 opcode7/condition0, role40 origin13,44 → destination10,49. Existing `Track::begin` native scripted graph reports no route. The battle flag dispatcher instead used a different occupied-cell BFS, teleported successful actors, and manufactured a fatal blocker on failure. Replaced this duplicated path with the existing original_scripted_movement::Track executor: successful movement is animated and returns to prior battle dispatch; failed movement leaves coordinates intact and consumes the matched original record according to native return semantics. Story opcode7 uses the same matched-role failure completion rule. No movement graph/collision/deployment/army data changed.
- Exact original assembly evidence `analysis\ida\opening.idb.asm`:420592 calls45DEE9 for first army and only tries second if return!=1;45DF24 clears the matched opcode before switch;45DF39(case0)/45DF5D(case1) call45E107 but IGNORE its result, then45E0F5 unconditionally assigns return1.45E21A failed4773A3 route clears movement state and exits path extraction through45E2A2, not a campaign failure. Thus successful matched-record consumption despite failed movement is original behavior, NOT skipping an unsupported event. Unlike4619AB parallel-list failure, this is proven independently for single-role45DEE9/45E107. Missing role is not consumed. No teleport/revival or invented route permitted.
- New genuine `tests\original_campaign_stage100_test.cpp` reproduces role40 failed native route, executes production dispatcher, asserts original event2 consumed and exact actor coordinates unchanged. PASS `out\natalia100-tests.log`; it allows the distinct next unsupported opcode but rejects any movement blocker. Requested White movement regressions freshly compiled/executed PASS:141 original story/dialogue/group/checkpoint/route12 (`out\natalia100-white141-tests.log`),157 type4/class0 exact route6→11,42 (`out\natalia100-white157-tests.log`),158 original parallel failed-actor/cleanup/arrival (`out\natalia100-white158-tests.log`). VS18 native S2NE.sln Release/Debug PASS (`out\natalia100-release.log`, `out\natalia100-debug.log`); changed source/test editor diagnostics empty.
- Actual dummy SDL resume command: `S2NE\out\x64\Release\envoy-opening.exe --smoke-campaign-strengthened-alternate-continue . S2NE\out\strengthened-flow-acceptance\run-106452-stage100-blocked.sav`. Run104788 log `out\natalia100-continue-sdl.log`: **100 草原逐惡** passes original failed-movement record, then reaches new actual event3 **opcode12/condition19 requires native executor**. Exit1, no100 victory/later-stage/ending claimed. Durable byte-stable reload-verified checkpoint `out\strengthened-flow-acceptance\run-104788-stage100-blocked.sav`, SHA256 `ECC0D80901AD46917ECBCC23B70F5EF1980488136646F80B5937FBA1BFFD0701`.
- Follow-up evidence located but not partially implemented:4614AF(case19) clears raw DWORD30, sets WORD8=1, clears WORD2 identity, then FALLS THROUGH4614CB(case20)→452DF3. Must audit452DF3 and inactive/dead461149 semantics and reuse exact raw reset adapter before implementing19; treating it as runtime hide or skipping it would be wrong. Full Natalia ending pending.
- Actual SDL and all regression/build helpers exited; no owned writer or envoy-opening process remains, isolated SAVE empty. README/PROGRESS untouched. White ending/manual UI preserved, audio/fade deferred.

### Natalia batch 7 — generic native19/20 removal and actual100→130 (2026-11-10)

- Assembly4614AF opcode12/19 resets DWORD30=0, WORD8=1, WORD2=0 then falls through4614CB/20→452DF3.452DF3 calls447FC3, refreshes map object state, invokes455048 disappearance, presents, then clears status80.461149 returns successful no-op for condition>=10 on dead/inactive roles. Implemented generic19/20 through existing fixed-army raw reset adapter (condition0 unchanged), shared production disappearance and deferred capture/reset/restore transaction;19 clears identity,20 preserves identity, both clear presence only for live active roles. No stage100 hardcode or item/flag/learning injection. Both story and battle dispatch use this executor. Restore clears pending removal state.
- Genuine104788 fixture `tests\original_campaign_removal_test.cpp` verifies exact encoded raw19 reset, fixed army retention, byte-identical dead/inactive no-op, actual animated production completion/removal rather than runtime-only hide. PASS `out\natalia100-removal-tests.log`. Coupled White26 raw-control/withdrawal and158 cleanup/group regressions freshly built PASS (`out\natalia-removal-white26-tests.log`, `out\natalia-removal-white158-tests.log`). Native VS18 Release final PASS `out\natalia100-removal-release-final.log`, Debug PASS `out\natalia100-removal-debug.log`; editor diagnostics empty. Intermediate final Release attempt hit LNK1168 because actual SDL exe was still running; rerun after natural exit succeeded. Actual SDL used initial successful Release with identical removal semantics (subsequent restore-pending reset is defensive).
- Actual dummy SDL command: `S2NE\out\x64\Release\envoy-opening.exe --smoke-campaign-strengthened-alternate-continue . S2NE\out\strengthened-flow-acceptance\run-104788-stage100-blocked.sav`. Run79984 log `out\natalia100-removal-sdl.log`. Original native victories, named resource stages and actual turns: **100 草原逐惡 t8 →103 雙龍之城 t8 →104 千里追劫 t3 →107 反撲 t5 →109 高科技戰士 t3 →113 城內混戰 t9 →114 軍械室 t6 →115 小丑奪權 t2 →116 水門任務 t24 →117 突擊提那城 t4 →118 遲滯敵軍 t15 →120 順流而下 t19 →124 力量與魔法 t6 →126 烽火議和 t1 →121 落魄的元帥 t5 →168 休戰儀式 story completes →129 奚族聖地 t3 →130 返鄉的遊子 entered**. Original rewards/hub/story chains and per-stage CD2 resource reads/autosave reloads retained. No forced victory or skipped scene.
- Substantial actual boundary: **130 返鄉的遊子 original defeat at turn100**, difficulty1, process exit1, not missing-opcode stop. No tactical parameter sweeps or force-win applied. Final full route/ending remains pending. Durable genuine native start checkpoint `out\strengthened-flow-acceptance\run-79984-stage130-start.sav`, native route1/progress37/CD2, SHA256 `8C947EF53CD79B108E1EADA8E190FA9FF0926CCA96539B3E927FC7EA1959C299`. This is START state, not falsely claimed post-defeat save. Earlier run79984 stage-start checkpoints persist and are byte-stable reload verified. Next investigation should inspect130 native objective/arrival/victory conditions against actual input policy rather than weakening original turn deadline.
- All actual SDL/test/build helpers exited; no envoy-opening process or isolated SAVE leftovers. README/PROGRESS untouched. White ending/manual UI preserved; audio/fade deferred.

### Natalia batch8 — shared native teleporter policy130 victory, actual131 reached (2026-11-10)

- Found acceptance-only hardcode `campaign_pad_input`: restricted to stage60/61 despite using generic native object-category35/component BFS. Removed that stage whitelist, reusing identical legal mouse selection/route/ordinary turn-end teleporter activation wherever protagonist's current component has no adjacent living hostile and a real native pad. No map/collision/AI/story/flags altered. This is a shared flow policy, not tactical sweeps or injected objectives.
- Native130 resource: both victory groups=[35], header0, defeat=[11,1]. Existing assembly evidence4384FE deliberately does NOT interpret35 as kill-all;130 depends on original scripted event chains. Native131 resource: victory=[33] (absence of runtime-role-selector1 enemies), header0, defeat=[11,1]. Preserved both; no forced-win replacement.131 original records: turn1/turn3, victory event2 completion13;9/1 requires13 AND63 completes79;9/3 depends13 and64/65 completes77. Natalia ending is therefore not automatically the White61 executor and is not claimed implemented/viewed.
- New genuine `original_campaign_stage130_test.cpp` PASS `out\natalia130-tests.log`, shows original130/131 groups and actual disconnected130 legal pad destination12,82, actor1. Native VS18 Release/Debug PASS `out\natalia130-release.log`, `out\natalia130-debug.log`; freshly built White60 native ending/movement regression completed exit0 `out\natalia130-white60-tests.log`.
- Actual command dummy drivers: `S2NE\out\x64\Release\envoy-opening.exe --smoke-campaign-strengthened-alternate-continue . S2NE\out\strengthened-flow-acceptance\run-79984-stage130-start.sav`. Run84628 log `out\natalia130-continue-sdl.log`: **130 返鄉的遊子 actual native victory turn64 →131 超時空傳說 entry**, route1/progress38/CD2, byte-stable autosave reload/native DAT5 read verified. Normal teleporter commands, combat, original reward/hub transition; no injected items/learning/flags.
- Actual131 still reaches **original defeat turn100**, exit1, no ending proof. Legal pad input observed at19,81 and9,81, but original runtime-selector1 enemies not eliminated. This is a further concrete native final-stage/component/input-policy investigation, not license to disable turn limit or kill boss synthetically. Did not alter enemy visibility or AI to make it pass. Full Natalia route/ending remains pending.
- Durable genuine `out\strengthened-flow-acceptance\run-84628-stage131-start.sav`, SHA256 `092F8155A40910BBAAD2F2F236FD57A6ABFC87FEECBAD3EE842D080FFC22B288`;130 start also persists. These are START checkpoints, not post-defeat saves. All actual helpers exited, no envoy-opening process or isolated SAVE leftovers. README/PROGRESS untouched. White ending preserved; audio/fade and later independent skills audit deferred.

### Natalia131 completed — genuine pickup/USE32/random-slot dispel, native44-phase ending exit0 (2026-11-10)

- Audited genuine84628 stage131: **131 超時空傳說**, header0/victory33/runtime-role-selector1 boss20; actual equipment14/125/183/165/199/**237**. Original map category12 at3,65 references chain1=[301,301,301,301], no invented pickup. Original victory event2 completes13; flag-chain13+63 completes79, alternative13/64/65 completes77. Existing White-only input policy restricted mineral approach/USE tostage61 and protagonist11. Extended only resource-confirmed final131 to same legal original pickup/USE/skill32/boss20 commands, selecting native protagonist1 via branch metadata. Production item/learning/combat/RNG/payment code untouched. Explicit strengthening still only HP/ATK/DEF/MP; no ST/skill/proficiency/flags/items injected.
- Actual first SDL log `out\natalia131-quest-sdl.log` run90072 from genuine84628 start: walks3,65, original category12 pickup301, normal item commands commit USE learning item301 forunit1 atturn17, skill32 learned and original save reload verified. `run-90072-stage61-mineral-learned.sav` is genuinely **stage131** despite legacy diagnostic basename. Initial old White turn30 quest cutoff blocked continuation; explicit Natalia continue now respects native99-turn limit instead, not a new victory rule. Later run43140 naturally hit slot165/125 but failed originalturn100; no RNG tampering or false success claim.
- Actual earned continuation from90072 turn17 learned save, same unchanged native RNG/commands, log `out\natalia131-earned-continue-sdl.log` run77072: normal40MP skill32 casts, original6% chance and random equipment slot selection; real hits removed199,165,125 then**237**, followed by normal physical boss action/death/settlement. **131 超時空傳說 original victory turn78**. No forced flags or forced-hit/slot selection. Genuine earned `run-77072-stage61-hostile32.sav` records final accessory-removed state (legacy61 basename, actual131). Native terminal state saved after replay as `run-94920-natalia-terminal.sav`.
- Original terminal dispatch40DDCF returns23 when next-stage0;40D6D6 calls common41E9CF then cleanup40D7CB. Enabled original131 terminal authorization/CD2 for common executor, retaining native victory flag13+selected route0 gating. Native41EC06→41ECDF flag variant selection uses actual79 for this run; no White flags copied/manufactured. Final artifact uses Natalia-specific filename. Original full sequence is44 phases for this actual retained14-role roster.
- Found/fixed real ending class-sprite bounds bug: frame base in PRG may be0 (actual retained appearances1). Existing class_image used unsigned(first-1), underflowing into4294995609; it also selected previous frame for other classes. Native41F167 calls4585B9; use frame base+first idle frame (one-based1) minus1, equivalently `first*7`, as existing map unit_height does. No art substituted/skipped. Added explicit native index bounds. Genuine `tests\original_natalia_ending_test.cpp` renders every actual terminal page/full timed movies to completion PASS `out\natalia-ending-tests.log`.
- FINAL actual SDL full ending command: dummy drivers, `S2NE\out\x64\Release\envoy-opening.exe --smoke-campaign-strengthened-alternate-continue . S2NE\out\strengthened-flow-acceptance\run-94920-natalia-terminal.sav`. **PASS normal exit0,25003 presented frames**, log `out\natalia131-ending-complete-sdl.log`: all44 phases including movie4 full136frames/380ms, movie5 full95frames/380ms, retained original long images/class/stat/relationships, campaign summary, movie3 full1142frames/130ms, normal cleanup. Durable byte-stable save reload result **`out\strengthened-flow-acceptance\run-61460-natalia-ending-viewed.sav`**, SHA256 `A89CF281CC67FB481AFEEC2FE2D499A5A1D645ED09132A1D8AC485ECC799041F`. Real final flag42=0,43=0,73=0,75=0; no invented completion flag. Both independent storylines now have actual full visual ending/normal-exit proof, NOT original balance certification.
- Final VS18 S2NE.sln Release/Debug PASS (`out\natalia131-release.log`, `out\natalia131-debug.log`). Genuine131 map/chain/equipment probe PASS `out\natalia131-tests.log`; original skills32/MP/payment/random-slot regression PASS `out\natalia131-skills-tests.log`; rebuilt White60/native ending regression PASS `out\natalia131-white60-tests.log`. Actual final full SDL additionally verifies131 genuine state. Earlier renderer-failure logs are rejected as full completion. Defensive terminal-save guard subsequently reduced repeated page0 writes to one, final both builds PASS; no presentation semantics changed.
- All SDL/build/test helpers exited, no envoy-opening process or isolated SAVE leftovers. README/PROGRESS untouched. Full native sound-event/fade fidelity remains deferred; independent skill-learning/effect-animation audit is next separate task, not falsely included in this visual full-route proof.

## 2026-11-10 技能专项：第一轮磁盘全目录差距审计（非全技能完成）

### 范围与重新取证

直接重读 `analysis\ida\opening.idb.asm`、磁盘 `SUPERSP.BIN/PAL/CEL`、`ITEMSP.BIN/KINDSP.BIN` 与当前源码/tests，不沿用 skills-evidence 中旧的生产接线断言。原 BIN2 实际偏移625302，头 `02 0B 08 03 00 00 00 73 2A`，11组、前8后3、115条37字节记录、42个附加字符串。目录 first/count 是原表索引，不推算累计值。新增 `tests\original_skill_audit_test.cpp` 对全部115条逐字节核对、ID唯一性、全部组阈值边界/条件位与45C8CA真值表进行检查。测试中合成 learned/proficiency 只作为隔离算法输入，不修改强化模式、原存档或运行时队伍。

原技能共115条：96条非合作，19条 `record+20h & 7` 合作（魔合8、合击11，尚未实现/本轮未展开）；共60个不同 effect 字节，13条额外条件 `+20h & 80h`。后者ID为63、68..74、81..82、104、114..115；不是所有阈值满足就应习得，不能用统一阈值逻辑强解锁。音频与双人对战不在本轮实现范围；单人战役合作技能仍是完整实现的待补缺口，不属于用户授权排除。

### 类别/数量与逐类差距矩阵

组号以下均零基；效果列是实际记录+13h字节集合，不是自行命名的效果。学习“部分”指通用原阈值/原转职/已有物品路径可复用，不表示该组已有完整合法实机获得、施放、动画证明。

|组/原资源名称|ID/数量|原效果编号|习得/使用/效果/动画的当前磁盘结论|
|---|---|---|---|
|0 神術|1..10 /10|1,9,18,19,20,44,61,65|习得部分；列表/费用检查已实现；无生产施放及专用动画。治療ID1不能借用ID88自疗事务。|
|1 巫術|11..20 /10|1,16,42,56,67,69|习得部分；列表/费用检查已实现；效果与专用动画缺失。|
|2 魔術|21..32 /12|1,2,3,6,10,34,46|习得部分；ID32有物品301→USE65→单条学习/熟练下限/存档、正常目标输入与敌方装备移除事务回归；其余11条效果缺失。ID32仅角色施法动作，非完整原法术特效。|
|3 妖術|33..42 /10|1,17,40,41,43,55,58,62,63|习得部分；列表/费用检查已实现；效果与专用动画缺失。|
|4 幻術|43..52 /10|1,4,15,33,35,45,47,70,72|习得部分；列表/费用检查已实现；效果与专用动画缺失。|
|5 法術|53..63 /11|1,5,8,12,13,37,46,74,76|习得部分；ID63特殊条件记录被普通阈值学习正确拒绝，但其他合法获得途径尚未逐条证明；效果/动画缺失。|
|6 喚術|64..74 /11|21..31|习得部分，本轮修正原45C8CA联动条件；68..73可走45C9B1六学派前提，74不能走此谓词；激活需要44CB0E世界/军团状态，当前fail-closed，召唤效果与动画缺失。|
|7 魔合|75..82 /8|1,88,90,91|8条均合作；目录与条件位已审计，合作施放/效果/动画尚未实现、本轮未展开，不据此开通。|
|8 武術|83..93 /11|1,10,18,84,85,86,87|习得部分；仅88有精确资源尾校验的自疗事务/费用/RNG/奖励适配契约单测，当前main无调用，无生产菜单接线；其余效果缺失，88也无完整动画。|
|9 劍術|94..104 /11|1,43|习得部分；104有额外条件位，通用学习不会无条件送出；列表/费用检查已实现，剑术效果与动画缺失。|
|10 合擊|105..115 /11|1|11条均合作；目录与条件位已审计，合作施放/效果/动画尚未实现、本轮未展开。|

因此非合作96条中：94条无技能专用执行器；32仅有限生产执行，88仅有限模块事务。不能把共用effect1、物理攻击实现、原角色动作、原资源存在或菜单可选择记为这些技能已完成。也不能把“已实现通用习得算法”记为全部技能都能在当前可玩流程合法获得。

### 学习、使用、效果与视觉原调用链

- 学习数据：角色原 learned WORD +FB+2*g、熟练 WORD +E3+2*g；初始/载入来自原记录，`original_initialization.hpp`/`original_save_bridge.hpp` 保留原值，不补技能。原转职45B5C4训练增加300熟练、扫描目录门槛并排除+20h:80，最后45C8CA关系更新；现 `OriginalPromotionTransaction::train/learn_group/learn_relations` 已有实现。门槛是记录signed WORD +0Fh，比较unsigned熟练/10，不是等级。
- 行动熟练：4654A6→4659BD/4659F6，+D7 WORD索引0..5武器、6..16技能组；加法先WORD回绕再上限15000，已封顶不再扫描。465A47..465B74 扫阈值、已有位、额外条件；45C9B1仅一基group7/ordinal5..10，要求原 +F1+2*ordinal WORD（即 learned[ordinal-5]）非零。现 `award_proficiency` 有算法与生产32奖励桥；不代表未接线技能能实际练习。13条条件记录中仅68..73满足该函数范围，其余不能由此自行开放。
- 物品：450550 effect65→451DDA 指定组/起止ordinal学习；当前 `use_learning_item` 仅资源effect65单ordinal场景，301→32已实测。它不等于全套物品效果/通用范围学习UI。反复使用原已学条目仍消费物品，未学时才提高熟练下限；相关原测试保持通过。
- 使用入口：439C77 AC→464C0C→464767（前8），AE→464C83→464900（后3）→4650B4→469F04列表→465342目标→46B34E表现→4654A6效果/奖励→464CFA结算。当前 main command6实际只以已学32且首分区可用为入口，不是全技能菜单已接通；后分区88没有生产执行调用。
- 已实现查询边界：46AC80按记录signed WORD +0Dh费用/+21h flags检查MP、ST、HP严格大于费用、AP；能力20h仅MP减半。入口检查status3D:40、分区抑制及AP configEF/F3。合作+20h低3位返回partner_unresolved；effect21..31返回world_unresolved，尚未执行44CB0E；选中请求绝不是效果执行。完整合作搜索46A924→46AAEB尚未实现、本轮未展开。通用目标范围/地形/视线/敌我/状态/多目标465342及401146/402315/40418D未全移植，不可只靠费用通过就放行。
- 效果原分派4654A6：effect1→466082；18或90→465C14；21..31→465E04；其他→466544→4668F0（原跳表467438）。命中/限制46D496、威力46E858及其公式分派、治疗/伤害468BAC、经验469101、支付464CFA必须逐支核对。当前仅32/88窄路径有原资源证据，不能把它们的公式、支付或奖励推广到其他ID。
- 视觉原46B34E→46C078准备、46C45B特殊分支、46B7C9→46B803/46B9A9/46B97F前后处理。原BIN10每技能时序/支付表与场景资源共同驱动，记录末尾字节不能直接当已复原动画。当前 `Battle::present_skill` 仅找原action_records type11的角色动作，固定120ms帧播放；没有完整原法术场景/投射/范围/命中特效或原时序验证。`OriginalSkills::draw` 是原CEL菜单条/图标/组名，不是施法动画；原帮助469B39、费用/效果页脚及完整鼠标/UI反馈仍缺。没有自行设计新特效。

### 本轮唯一确证源码修复

`src\original_promotion_transaction.hpp::learn_relations` 原写成 source learned[6] 位 **已置位才传播**，与实际45C917→45C971的 `test ...; jz loc_45C973` 相反。现改为 **对应位未置位** 且门槛达成、对应学校已有技能才OR目标i+4位；仍要求原+107h非零。原函数目录索引first+4+2*i（双自增）、原动态循环位读取、目标位保持，不改熟练、物品、旗标或动画，不把此规律推广到45C9B1。此修复紧耦合原转职训练后的学习缺陷；旧练习测试只有阈值/通用组比较，没检查此真值表，故曾漏过。

新增测试覆盖全部115条真实raw记录、11组每条门槛前一单位/边界/边界后/15000、额外条件允许/拒绝、六学派关系更新的source位/学校有无/原喚术非零/门槛状态。明确保留有些循环目标位会影响后续source位的原顺序，不重写为静态快照集合。现有技能58条/7场景、自疗事务、40个真实转职/22物品分支/6退款、特殊角色行动与32物品/命中失败/免疫/事务拒绝/存档回归均PASS。

### 构建与复现

在 `S2NE` 执行原生（均退出0；既有C4267/C4996等警告保留）：

```powershell
& 'C:\Program Files\Microsoft Visual Studio\18\Community\MSBuild\Current\Bin\MSBuild.exe' S2NE.sln /m /p:Configuration=Release /p:Platform=x64 /v:quiet /nologo
& 'C:\Program Files\Microsoft Visual Studio\18\Community\MSBuild\Current\Bin\MSBuild.exe' S2NE.sln /m /p:Configuration=Debug /p:Platform=x64 /v:quiet /nologo
```

测试使用同VS18 vcvars64环境，在S2NE按每个名称编译：

```text
cl /nologo /std:c++20 /EHsc /utf-8 /W4 /I src tests\NAME.cpp /Fe:out\NAME.exe /Fo:out\NAME.obj
```

NAME为 `original_skill_audit_test`、`original_skills_test`、`original_promotion_transaction_test`、`original_special_action_test`、`original_campaign_stage60_test`（最后项实用/W3）。从资源根目录逐个运行 `S2NE\out\NAME.exe`，本轮五个全部退出0。日志 `out\skill-audit-release-build.log`、`out\skill-audit-debug-build.log`、`out\skill-audit-regression.log`；stage60本轮另实际执行成功，其输出不冒充完整技能覆盖。编辑器检查两个改动C++文件无errors。无新依赖、无CMake迁移；测试未自动注册到原单工程执行目标，需上面显式命令。

后续有界范围：先逐条溯源63/74/104非阈值合法获得入口与451DDA范围学习调用者；再任选一个非合作原ID，独立完成465342目标限制→原效果分支→支付/奖励→原46B34E视觉时序并加实资源事务/画面回归，完成前保持fail-closed。尤其先补全已接线32视觉与封顶熟练奖励桥语义，再接88生产自疗，不批量开放共用effect编号。当前尚未证明32所有地形/世界/状态/支付时序等价，保留“部分”，不宣称96条可用或动画完成。

未写README/PROGRESS、未启动辅助服务/游戏进程、未修改两份结局证据。读取指纹White `1D64E38324595851F57857A8BED88D696DBB033F9834F9D97234D3F22EBD0874`，Natalia `A89CF281CC67FB481AFEEC2FE2D499A5A1D645ED09132A1D8AC485ECC799041F`。本轮到目录全覆盖审计、上述确证修复及构建/最小回归即止，不以算法fixture冒充新实机流程验收。

## 2026-11-10 技能专项后续：合法习得与32前置原角色动画链

范围纠正：用户排除的是**双人对战**，不是单人战役中的合作技能。前一节19条合作技能的“排除实现”表述已更正为“尚未实现/本轮未展开”；魔合8条、合击11条及46A924/46AAEB搭档查找仍在完整技能缺口内。本轮不扩展实现这19条，也不以此作为已获授权排除。

### 63/74/104真实合法入口（扫描全部168个原关卡）

新增 `tests\original_skill_followup_test.cpp` 从磁盘遍历**168关**全部opcode12/condition27、28与角色初始技能，而非只扫现有两条通关流程。结果：

- **63 生命力量**：原125关event2，opcode12/condition28，角色1，group6（一本基），ordinal11。原46172E调用451DDA(group6,1..11)，因此不仅63，还有该组1..10；不是熟练阈值自然获得。
- **74 龍**：原135关event1，opcode12/condition27，角色4，group7，ordinal11。原461700调用451DDA(group7,11..11)，只学74，不自动补68..73。
- **104 霸邪劍法**：原物品**226霸王劍譜**，effect65/group10/ordinal11，450FE1→451DDA单条入口；原62、133关的敌角色157也有初始104，但不能把敌初始记录当玩家获得途径。物品226合法USE入口已证明；本轮没有声称已通过玩家实机拾取/购买226。
- 其他真实范围入口：54关event12/13，角色16/17，condition28，group11/ordinal10（合擊1..10）。虽不展开合作施放，本轮保留其合法学习，未把合作技能当双人对战内容删掉。
- 全部9个真实学习物品：effect65为226(g10,n11)、294(g8,n8)、299(g11,n10)、301(g3,n12)；effect66为227(g3,n3)、241(g5,n3)、295(g1,n3)、297(g2,n3)、298(g4,n3)。450FE1传first=last=n；451032传first=1,last=n。原451DDA跳过已学条目的熟练下限写入，只对新条目置位及提高signed门槛*10熟练，保持WORD写入。

### 确证缺陷与实现

1. `OriginalPromotionTransaction::learning_item/use_learning_item` 原只识别effect65，真实effect66范围学习物品无法进入正常USE列表。新增共享 `learn_range`，严格一基组/包含末端的ordinal范围校验，并接effect66；仍要求真实库存、原职业/属性/人物/类别门控，成功才消费一个物品。九件真实资源配原兼容角色回归通过，无额外物品或熟练注入。
2. `OriginalInitialization::learn_scripted_range` 接原opcode12 condition27/28，事务中只修改原固定角色记录的+FB技能与+E3熟练；`Prologue`剧情/战斗旗标派发均复用既有角色事务。保留原旗标触发、外层对白/消费时机；461149死亡20h或未活动80h返回不变，角色缺失按461110失败，不创建角色。测试使用125/135/54真实事件、原始记录逐字节比较、死亡/未活动分支及原序列化布局；这是隔离事务fixture，不是从此前断点实机重走125/135的证明。学习弹框451DDA原文本逐条250ms表现尚未复原；已有事件对白不代替该弹框。
3. main的32奖励桥原将 `award_proficiency` 封顶返回false当作整个技能失败，撤销命中/支付/XP。原4659BD比较15000后跳465B79，仍调用458B03/459309。现只忽略这个合法熟练no-op，不吞异常，继续原XP桥与支付。新测试熟练15000仍能施放32、保留封顶、不退款、不撤销RNG，消耗40MP及AP。未批量启用其他技能。

### 32原视觉：完成范围与明确未完成

原428658加载BIN10四张115字节表，32条目分别：**visual=32、payment=0、paletteSource=140、paletteTransform=45**（后两者使用处有signed/unsigned差别，不能当时长）。新回归钉住原磁盘值。46B34E先46C078角色准备，然后46C45B分支及32+1的SPELLSP表现；32并非凭一个角色动作即已完成法术动画。

本轮完成并生产接线的真实子链为 **46C078→45CE57→454A96→440677/440618/4407E1角色准备**：

- 仅已接线32调用新 `present_skill32_preparation`，不提供通用“所有技能播放同一动作”。寻找原action type11，读取signed动作记录+Ah×4；没有该条时遵守原default offset8；范围无效或无原PRG序列则阻止施放准备，不编造动画。
- 按45CE57原奇偶行坐标比较朝向目标（同格保留朝向），播放原SUPERSP.PRG已存在的对应方向、动作帧序列；不另画几何，不替换为固定帧数。原特殊status33:10动态角色缓冲未支持，明确fail-closed。
- 时钟不是猜测：454E43以0Ch调用473192，47308B的DOS时钟单位是百分秒，所以每个实际呈现帧间隔**12百分秒=120ms**。数值恰与旧硬编码120相同，但此次已溯源到确切原等待调用；帧序列终止/方向/动作来自资源，不用BIN10值32当32ms或播放次数。沿用逐呈现帧重置时钟、不跨帧跳过策略。
- 真实SDL专项使用已有84592原存档恢复、原地图/原角色渲染，隔离摆位验证目标朝向，119ms不前进、再1ms推进；创建真实SDL window/renderer/texture，dummy视频驱动实际RenderPresent，原4个角色姿态全部呈现（本次24次present），正常销毁退出。不是原版截图像素等价证明，也不是完整正常鼠标施放流程的新录像。

**未完成**：463EA6读取SPELLSP条目、其16字节头/帧序/8字节帧目录、46B803的46B5DE调色映射（424873→463516）、46B9A9的目标位置/透明合成/原帧时长、46B97F恢复，尚未移植到生产32。故本次仅完成46C078原角色准备子链，**未完成32完整BIN10/SPELLSP法术合成动画**。音频仍暂缓；不以原帧存在、120ms前置动作或SDL成功冒充全动画完成。合作19条及其他技能原效果/目标/动画缺口保留。

### 验证、失败与复现证据

- 原生VS18 `S2NE.sln` **Release/x64、Debug/x64均退出0**，日志 `out\skill-followup-release.log`、`out\skill-followup-debug.log`。编译参数与前一节相同，既有警告保留。六个编辑C++文件Problems无errors。
- 新专项退出0，`out\skill-followup-tests.log`：168关真实学习入口扫描、九件65/66物品、条件63/74/104直授范围、四条真实脚本事件事务/死亡未活动无改动、15000封顶32支付/XP、原46C078角色动作SDL呈现。测试读取原存档，不写结局或SAVE。
- 重新编译并执行PASS（各 `out\NAME-followup.log`）：`original_skill_audit`、`original_skills`、`original_promotion_transaction`、`original_special_action`、`original_campaign_stage60`、`original_campaign_stage26`、`original_initialization`。
- **额外探测失败未隐瞒**：`original_campaign_stage156_test` 默认97960-stage20-start及另87976-stage20-start均在 `Changed original story identity` 失败，尚未进入条件16检查（日志 `out\original_campaign_stage156-followup.log`、`out\original_campaign_stage156-alternate-followup.log`）。没有修无关剧情身份校验，也未宣称此项PASS或已证明为既有问题。与本轮27/28最接近的26原角色事务/原initialization回归及四个新脚本事件专项PASS；156仍是独立未归因验证缺口。

在S2NE目录、同进程vcvars64环境编译新专项：

```text
cl /nologo /std:c++20 /EHsc /utf-8 /W3 /I src /I D:\project\vcpkg\installed\x64-windows\include tests\original_skill_followup_test.cpp /Fe:out\original_skill_followup_test.exe /Fo:out\original_skill_followup_test.obj /link /LIBPATH:D:\project\vcpkg\installed\x64-windows\lib SDL3.lib
```

资源根目录执行：

```powershell
$env:PATH='D:\project\vcpkg\installed\x64-windows\bin;'+$env:PATH
$env:SDL_VIDEODRIVER='dummy'
& .\S2NE\out\original_skill_followup_test.exe --sdl
```

本轮未修改PROGRESS/README、未增加强化注入、没有写两份结局证据；最终重新读取SHA256仍为White `1D64E38324595851F57857A8BED88D696DBB033F9834F9D97234D3F22EBD0874`、Natalia `A89CF281CC67FB481AFEEC2FE2D499A5A1D645ED09132A1D8AC485ECC799041F`。没有启动后台服务，所有本轮同步SDL/编译/测试进程已退出。下一有界工作应独立解析32+1真实SPELLSP条目/调色映射及46B9A9帧目录，先补一条真正法术合成链；同时单独定位156 fixture身份校验，不无限扩游戏。

## 技能专项续作：156身份归因与真实SPELLSP33合成

### 156失败已归因，不放宽生产身份门控

上节“未归因”是上一轮历史结果，本节取代其当前状态。旧fixture执行 `target=source.events; target.load_stage(156)`，丢失20关存档的实际持久队伍；156是原mode3剧情，不能用156资源默认角色数组替换原队伍。`OriginalCampaignPersistence::deploy_next` 已有mode3 `next.units=previous.units`；`OriginalInitialization::transition_stage` 原槽位WORD0对应raw +2身份/+4外观严格核对，本轮没有改这段生产检查。git差异也显示此前学习补丁只加学习调用及角色事务，未变更此身份门控。

只修 `tests\original_campaign_stage156_test.cpp` 的部署fixture：走真实20→156的 `deploy_next`，逐槽核对id/appearance，再执行原transition。新增两类拒绝证据：同一实际档旧 `load_stage` 路径仍稳定抛 `Changed original story identity`；在正确部署上伪造id也必须被生产拒绝。两实际档97960/87976均PASS，包含原活动对白/移动、死亡未激活条件16不改角色、事件消费、route23与save bridge。故本次是fixture丢失队伍，不是本轮技能学习改动引入；没有用合成角色或宽松身份判断绕过。

### 本轮实际接线

- 原 `473CDB(eax=1,edx=1)` 是 **CEL1**，不是DAT1。PAL DWORD808减56得CEL基址28；CEL1包含133项。32的BIN10 visual32经 `46B34E +1` 选择第33项：相对offset7568321、length200966。
- 新 `original_skill_visual.hpp` 按 `463D6D/463EA6` 读取8字节[offset,size]目录、16字节头。真实8个WORD为 **23,0,0,63,272,0,0,5**：23帧、mode2 anchor0/0、63×272、无颜色混合表、无顺序/重复表、5 DOS百分秒。逐帧目录均63×272，按424F70原signed RLE消费整条：0透明一个，正数重复下一颜色，负数低7位literal，-128空literal；逐段边界、总像素、目录连续性及尾数据严格检查。只接受这一确证布局；其它头/有序/重复/调色表布局拒绝，不批量放开其它技能。
- `46B5DE→424873→463516/463165`：保留真正indexed地图与单位像素，不从RGBA反查（重复RGB会丢原索引）；非零颜色向**unsigned140**以45%原DAC/G-R-B近邻树映射，0不改。随后按原路径重画caster及选中enemy；不把整场RGBA随意变色。
- `46B9A9 mode2`：每帧从固定调色背景恢复，真实RLE解码后 `42AFBA`近景地形遮挡、`4468DF(arg0=1,arg4=0)`两军近处AD轮廓清零，再 `470D41/470B3D`零透明原色合成；原目标脚点按header宽/2、高及高差定位。无几何代替、无随机图形、无自定帧时长。
- `Prologue`提供有界32视觉状态，生产main在角色准备结束后、效果/奖励/支付事务前启动；未呈现帧不跳帧，5cs后最多前进一步，结束即恢复正常地图/UI，视觉期间隐藏技能菜单/目标辅助标记。资源无效拒绝启动且不提交effect/payment。视觉本身不改技能、熟练、库存、旗标或装备；原效果事务仍只有原32门控，至多一次提交。

### 本轮验证与复现

原生MSBuild Release/x64和Debug/x64均退出0，日志 `out\skill33-release.log`、`out\skill33-debug.log`；未换CMake或新增依赖。10个专项重新编译：`original_skill_audit`、`original_skills`、`original_promotion_transaction`、`original_special_action`、`original_campaign_stage60`、`original_campaign_stage26`、`original_initialization`、`original_campaign_stage156`、`original_skill_followup`、`original_skill_visual`；执行全部PASS，156另跑第二实际档亦PASS。前8项日志 `out\NAME-visual.log`；156最终 `out\skill156-97960-final.log`、`out\skill156-87976-final.log`；后两项 `out\skill-followup-final.log`、`out\skill33-sdl.log`。没有未归因失败留作“完整验证”。编辑器相关源码Problems无errors。

在S2NE中、同一vcvars64环境编译（NAME为上面名称，带 `_test`）：

```powershell
cl /nologo /std:c++20 /EHsc /utf-8 /W3 /I src /I D:\project\vcpkg\installed\x64-windows\include tests\NAME.cpp /Fe:out\NAME.exe /Fo:out\NAME.obj /link /LIBPATH:D:\project\vcpkg\installed\x64-windows\lib SDL3.lib
& 'C:\Program Files\Microsoft Visual Studio\18\Community\MSBuild\Current\Bin\MSBuild.exe' S2NE.sln /p:Configuration=Release /p:Platform=x64 /m /v:minimal /nologo
& 'C:\Program Files\Microsoft Visual Studio\18\Community\MSBuild\Current\Bin\MSBuild.exe' S2NE.sln /p:Configuration=Debug /p:Platform=x64 /m /v:minimal /nologo
```

从资源根运行（本轮实际Windows SDL driver，不是dummy）：

```powershell
$env:PATH='D:\project\vcpkg\installed\x64-windows\bin;'+$env:PATH
Remove-Item Env:SDL_VIDEODRIVER -ErrorAction SilentlyContinue
& .\S2NE\out\original_skill_visual_test.exe
& .\S2NE\out\original_skill_followup_test.exe --sdl
& .\S2NE\out\original_campaign_stage156_test.exe S2NE\out\strengthened-flow-acceptance\run-97960-stage20-start.sav
& .\S2NE\out\original_campaign_stage156_test.exe S2NE\out\strengthened-flow-acceptance\run-87976-stage20-start.sav
```

SDL33实际呈现 **23帧、21个不同完整画面**，原资源首尾对称、50ms门限49/50检查、未present不跳帧、透明0、调色端点及所有255个非零映射、损坏头/目录/截断RLE拒绝、结束画面逐字节恢复均PASS。证据 `out\skill33-sdl-frame4.bmp`（另PNG便于查看）为真实原地图上第5原帧；Windows原生SDL窗口/renderer/texture全程实际present。这里验证共用Prologue视觉链，不冒充一次经鼠标选择并提交效果的完整新端到端游戏施放录制；原32效果/封顶奖励/支付另由skills/followup/stage60专项覆盖。

### 保留边界

本轮只接32→SPELLSP33这一条确证mode2动画，不声称全技能/全动画完成。其它SPELLSP模式、带palette混合表/有序/重复布局、其它技能执行器及原32以外特殊视觉仍未展开；单人19条合作施放仍是**未实现/待展开**，不属于用户排除。原32复杂特殊状态/完整全场表现仍应另取实际可达档逐项验证；音频暂缓，仅双人对战排除。后续可执行范围：正常输入完整32施放录制、更多原状态/遮挡黄金像素比对，之后再逐类原执行器，不扩展任意技能授权。

本轮未写README/PROGRESS；两结局原档SHA256复核与上节完全相同。所有本轮同步编译/测试/SDL辅助窗口均已退出，没有后台服务。

## 32正常输入端到端验收（本轮）

新增原生参数 `--smoke-skill32-input ROOT SAVE`，仅向实际SDL队列发送鼠标/方向键/Enter/Escape，不直接调用effect、选人、摆位、补MP/AP或改技能/熟练/旗标/物品，不筛RNG。固定使用原有娜131合法习得部署档 `run-43140-stage131-start.sav`（SHA256 `1A8D73F6E8C84BBBEF63EED9F7BC36E4601EF992B59104F8825E50B41275940B`），其原现场角色1在10,51、敌20在10,52，MP406/AP71。本轮无强化模式；该输入档来自此前强化流程，既有强化数值如实保留，因此这是输入/执行接线证据，不是原始平衡或无强化通关证明。合法技能来源仍为原事件，不另注入。

84592 learned原档MP20/AP21不足，娜90072 learned原档尚未靠近，多个post-cast原档AP26不足：只读试跑明确拒绝，没有把它们改成可施放。曾试blocked-route档MP失败输入，但角色无法正常选中，超时退出；该试验不是PASS、已移除实验分支。最终固定上述43140档完成，不用重试挑选命中结果。

### 实际正常输入链与发现的紧耦合缺陷

- SDL鼠标选择角色1 → Enter打开原攻击子菜单 → Right选择子项2法术 → 原技能列表导航到32 → Enter → 原目标20 → Escape取消；前后完整encode存档字节一致。
- 重开原菜单/32/目标20 → Enter确认 → 原46C078准备 → 原SPELLSP33帧0..22全部实际Windows SDL present → 原32效果及奖励/支付 → 正常画面恢复 → 静置500ms。驱动绝不调用内部effect；候选只读probe仅检查菜单/范围，不抽随机数，不修改实际角色。
- 修复实际暴露的输入集成缺陷：main左右键此前只路由command/finish_confirm，attack_menu被当镜头平移，无法键盘切换法术；现将attack_menu接既有 `cycle_command`，不改技能门控或攻击算法。
- 修复时钟集成：首次同步解析/合成耗时被下一tick重复算作帧0已等待，导致frame0/1同毫秒呈现；视觉成功启动后重取主循环时钟作为lastUiTick基准，最终frame0=11644ms、frame1=11694ms，之后按原5cs推进。没有更改原资源时长。

最终一次固定存档运行：`applied=1 hit=0 immune=0 chance=6 roll=66 removed=0 failure=0`。这是**正常随机未命中**，并非执行失败，不挑RNG重跑到命中；因此敌装备不变。完整合法施放事务仍完成：MP **406→366**，AP **71→26**，熟练 **3932→4054**，XP **64→0**且等级 **18→19**（原升级结算，非丢XP）。共呈现493界面帧，含23个原合成帧。未命中也仅支付一次；恢复后500ms存档与cast-after逐字节相同。

### 持久证据及回归

`out\skill32-input-run.log`含正常输入、23帧时间戳、效果随机结果、支付与恢复日志。`out\skill32-input\sequence.csv`记录实际present编号/时钟/输入阶段/准备活动标记/法术frame/真实画面FNV hash；`frame-0.bmp`至`frame-22.bmp`为同次原生生产draw画面。`cancel-before.sav`、`cancel-after.sav`、`cast-before.sav`、`cast-after.sav`、`settled-after.sav`均实际capture、encode、磁盘load再encode核对。不改原输入档及两结局档。

新增 `tests\original_skill_input_evidence_test.cpp`读取上述真实E2E证据，核对全部帧/取消字节一致/原40MP与45AP/升级熟练/恢复无重复支付；另用隔离副本检验32原MP不足选择拒绝、失效范围执行拒绝不支付不消费RNG。隔离失败测试不冒充正常输入E2E。取消与原随机miss已有真正输入证据；未声称所有失败状态已E2E遍历。

复现（资源根，真实Windows SDL）：

```powershell
$env:PATH='D:\project\vcpkg\installed\x64-windows\bin;'+$env:PATH
Remove-Item Env:SDL_VIDEODRIVER -ErrorAction SilentlyContinue
& .\S2NE\out\x64\Release\envoy-opening.exe --smoke-skill32-input . S2NE\out\strengthened-flow-acceptance\run-43140-stage131-start.sav
& .\S2NE\out\original_skill_input_evidence_test.exe
```

新专项同前述vcvars64+C++20/W3/I src方式编译。最终Release/x64、Debug/x64均exit0（`out\skill32-input-release.log`、`out\skill32-input-debug.log`）；E2E及新证据专项均exit0（`out\skill32-input-test.log`）。相关最小skills/visual/followup --sdl均exit0，日志 `out\skill32-input-skills.log`、`out\skill32-input-visual.log`、`out\skill32-input-followup.log`。音频设备不可用仅保留提示，不属于本轮音频验收。两结局SHA256仍与前节一致。未扩其它94技能，19合作缺口继续保留；未写PROGRESS/README。只读probe源/可执行/obj已清理，所有本轮SDL/编译/测试进程已退出，已停写。



## 88療傷心法：有界生产接线与原资源视觉（本轮）

### 原证与范围

原BIN2第88条Big5名 `C0 F8 B6 CB A4 DF AA 6B` = **療傷心法**，后分区武术group8 bit5，学习门槛signed WORD+15=90，即熟练900；原60/61/130/131/133/134/135关资源角色2已学、熟练3200。只读扫描168关及当前strengthened-flow-acceptance、真实SAVE文件，未找到同时可部署/可操作、AP足够且ST≥25的88实际档；没有把技能/熟练加进真实档。资源初始角色不等于一条已玩到的存档。未扩大其它effect18/formula41或其它94技能。

88原record尾24字节是25,0,90,0,0,1,18,41,120,0,100,0,0,0,0,0,0,0,0,0,66,0,0,0。范围0/目标mode1只接受实际施术者引用；支付ST25而非MP40，后分区status34 mask8而非32的mask4，原AP基值/重复附加、HP上限、装备19、status3c完整DWORD抑制、随机恢复/XP/熟练走原模块。生产已桥接458B03/459309及熟练奖励事务，失败不提交RNG/支付。

原439C77→43A200→464C83是后分区入口（图标AE，与前分区AC不同）。现攻击子菜单index3请求7，按原Context.second=true打开原技能分区，仅88具备执行器。目标固定自己，可Enter或点击自身确认；Tab不跳敌方。原46C078按分区选action type12（32是type11），原PRG动作/原12cs准备，之后真实合成，再执行自疗事务与原行动提交。没有用32效果代替88。

### 原动画不是32

BIN10第88 visual78/payment0/tint233,50 → SPELLSP **第79项**，CEL绝对offset21036570、length43308。原463EA6解析16字节头 **35,89,124,175,124,6,0,5**，35帧，mode3 anchor89/124，175×124，六颜色混合项，无ordered/repeat，5cs。

头后六source颜色95..100；后六百分比90/85/80/70/60/45；继之35×8字节offset/size目录与原RLE。新original_skill88_visual.hpp只接受这一确证布局，目录/数据越界、非连续、像素尺寸/总数及尾数据异常拒绝。复用的仅是原424F70字节解码，不复用32图像或mode2地形合成。

原46B5DE背景非零索引向233混合50%，保留真实indexed值再重画caster；46B9A9 mode3→47065B/470A3B：目标screen+32-anchor定位，不做32 mode2遮挡。透明0跳过；六色按470A3B以当前背景色为source、帧色为target、独立百分比调用463516/463165，非六色原色写入。indexed背景独立保留，避免重复RGB反查失真。每原帧从固定背景恢复，present后50ms至多前进一步，结束恢复正常画面。音频仍暂缓。

### 验证与证据（明确fixture，不冒充E2E）

新增tests\original_skill88_test.cpp：真实60关角色2已学条件、原门槛90、ST24拒绝、实际自引用限制、失败reward bridge无支付/RNG、后分区mask8、原progression桥、真实攻击子菜单→请求7测试。后者使用资源fixture并设置隔离AP/ST/伤血/激活状态，不写任何真实验收档，不能称正常存档E2E。SDL实际Windows driver呈现35帧、35不同完整画面、结束逐字节恢复；图片out\skill88-frame17.bmp。第一次相机未对准导致distinct1，已归因并修测试镜头，现强制distinct>20防止离屏假PASS。

Release/x64、Debug/x64均exit0：out\skill88-release.log、out\skill88-debug.log；88专项exit0：out\skill88-compile.log、out\skill88-test.log。原skills与32 visual专项exit0：out\skill88-skills.log、out\skill88-visual32.log。32原固定43140档正常输入取消/23帧/支付/恢复在最终诊断版及repeat/check1/check2/check3均exit0，持久证据专项exit0：out\skill88-regression32*.log。

**未隐瞒的验证限制**：最初一次32 E2E曾报旧的笼统payment/frame count mismatch；该版未记录失败分支具体MP/AP/计数，无法严谨确认是帧统计还是支付。已将错误扩为记录这些实际值，之后固定同档五次通过（未筛RNG，均自然miss），但不能据此证明最初偶发失败根因已解决；本轮不宣称无任何未归因失败的完整验证。下一有界工作应保留诊断并复现该偶发分支，同时取得真实可操作88档再做正常输入完整E2E。本轮只证明生产接线/资源fixture/真实SDL视觉，不声称88存档E2E完成，也不把19合作缺口改成排除。

复现88专项（资源根，先按前节vcvars64/C++20/SDL3命令编译original_skill88_test）：

```powershell
$env:PATH='D:\project\vcpkg\installed\x64-windows\bin;'+$env:PATH
Remove-Item Env:SDL_VIDEODRIVER -ErrorAction SilentlyContinue
& .\S2NE\out\original_skill88_test.exe
& .\S2NE\out\x64\Release\envoy-opening.exe --smoke-skill32-input . S2NE\out\strengthened-flow-acceptance\run-43140-stage131-start.sav
& .\S2NE\out\original_skill_input_evidence_test.exe
```

本轮未写PROGRESS/README、未改两结局或输入原档。只读scan辅助源/exe/obj已删除，所有本轮编译/测试/SDL进程退出，已停写。

## 技能32/88后续：确定性误断言归因与真实UJ02输入（本轮）

### 32：已证实的断言缺陷，而非以重复成功代替归因

旧smoke以`after.mp == before.mp-40`判断支付，遗漏458B03/459309升级经427919/427A9B增长**同时增加当前与最大MP**。新增`tests\original_skill_schedule_test.cpp`从固定run-43140-stage131-start原档调用原事务，在隔离测试中完整执行seed1..64（不按结果选择E2E随机）；34个案例稳定复现旧断言错误：before406/growth1/after367，实际仍支付40。生产事务未改成长或RNG；诊断仅在成功提交后发布nativeMPGrowth，持久证据测试用maxMP增长核算准确40，而非放宽任意净差。

历史首次失败的笼统日志已被旧重跑覆盖，无法反推那个时刻的具体MP/AP/frames；因此证明的是**同一旧断言可确定性误报的根因**，不声称恢复了丢失的历史现场，也没有证据声称生产tick/present存在竞态。独立调度验证32的0/49/50/250/1000ms边界与23次最终呈现；88固定0/1/49/50/51/250/1000ms，重复present仍只推进一帧，35帧不跳过。请求关闭后再次execute不消耗RNG或支付。真实SDL32的`--schedule-stress`仅在原动画期间使用固定延迟数组，不挑RNG、不改存档；最终23帧、MP406→366/AP71→26、熟练3932→4054、18→19级，静置500ms字节无二次变化。

### 88：修正调查范围，取得真实可操作存档路径

递归真实SAVE/out发现角色2沿途最高武术熟练560、learned8=31，尚无88；部分131档角色2武术0/ST0。不能拿资源60中原已学3200的角色覆盖真实持久队伍。扩大只读检查至**所有队员**后，发现未经本轮改写的`SAVE\UJ02.SAV`角色3原已学88：熟练3635、ST70、HP878/878，但turn6/AP5/action2不可立即施放。新增`--smoke-skill88-input`经正常SDL F10进入生产NPC/敌方回合与原呈现时钟，合法恢复AP93；未注入技能、熟练、HP、ST、身份、旗标，未启用强化。随后正常镜头按键/鼠标选人→攻击后分区→88→自身→取消→重开确认→action12准备→SPELLSP79原35帧→事务→恢复。恢复时原自动选择产生command状态，driver先正常Escape退出再移动镜头，未直接调用选择/效果/摆位内部接口。

真实流程暴露确证生产缺陷：88误拒`difficulty>3`，而原byte_4F7CEC初始化明确为5（opening.idb.asm:27102），现有原档桥合法范围也是1..5，原熟练公式`(6+成功标记-difficulty)*5+10`支持5。仅修88为1..5，0/6继续failclosed；不批量开放其他effect。新增1/4/5合法进入奖励桥及0/6拒绝的隔离回归。

最终真实Windows SDL：角色3 ST70→45、AP93→48、武术3635→3650、MP28不变、35帧全部实际呈现并保存BMP；取消前后及恢复静置500ms档逐字节相等。**该真实角色原为满HP，原规则疗伤0/XP0；不能声称真实档已验证受伤恢复正HP/正XP。**正疗伤/奖励仍由资源fixture专项验证，与真实输入证据严格区分。本轮未为获得受伤样本改HP或筛RNG，正疗伤存档E2E是后续明确边界。

### 复现与结果

资源根运行（PATH含`D:\project\vcpkg\installed\x64-windows\bin`）：

```powershell
.\S2NE\out\x64\Release\envoy-opening.exe --smoke-skill32-input . S2NE\out\strengthened-flow-acceptance\run-43140-stage131-start.sav --schedule-stress
.\S2NE\out\x64\Release\envoy-opening.exe --smoke-skill88-input . SAVE\UJ02.SAV
.\S2NE\out\original_skill_schedule_test.exe
.\S2NE\out\original_skill_input_evidence_test.exe
.\S2NE\out\original_skill88_input_evidence_test.exe
.\S2NE\out\original_skill88_test.exe
.\S2NE\out\original_skills_test.exe
.\S2NE\out\original_skill_visual_test.exe
```

上述均exit0；原生MSBuild S2NE.sln `/p:Configuration=Release`及Debug `/p:Platform=x64 /m /nologo /v:minimal`均成功。六个专项均从当前源码重新cl编译；真实SDL driver=windows，非dummy。日志`out\skill32-growth-{release,debug}.log`、`out\skill32-schedule-input.log`、`out\skill88-authentic-input.log`及`out\original_skill*-final.log`。证据目录`out\skill32-input`与`out\skill88-input`各含sequence.csv、原帧BMP及cancel-before/after、cast-before/after、settled-after.sav；88持久专项还核对源UJ02与施放前学得/熟练/ST/HP/职业一致且真实回合推进、AP恢复，不将资源fixture混为原档E2E。

未扩其它94技能；19单人合作技能依旧未实现，**不是用户排除**；双人对战排除，音频暂缓。原32对difficulty4/5的独立hostile gate仍未在本轮放开，后续须单独核原公式/配置边界。两条结局证据SHA256保持：白线1D64E38324595851F57857A8BED88D696DBB033F9834F9D97234D3F22EBD0874；娜线A89CF281CC67FB481AFEEC2FE2D499A5A1D8AC485ECC799041F。未写PROGRESS/README；调查scanner退出并清理源码/二进制/obj，保留只读调查日志。

## 技能有界收尾：32独立难度核对、88原档正疗伤E2E

### 32 difficulty4/5独立原证据

本轮不是从88推断：原46D496→46D874→407E2D→4080E7。`opening.idb.asm:13172..13255`中407E2D先原随机因子/10，再调用4080E7，最后限1..99；`:13510..13565`中4080E7对原军队/模式门控后，取`dword_4F7CE8+1`最高字节（即byte4F7CEC），作为配置偏移`+F3h+difficulty`（`[edx+F0h] sar18h`等价读取+F3h），乘(100+配置)/100，没有difficulty≤3限制。46D496:46D6F1等实际调用407E2D的原hostile路径也无此限制。原byte4F7CEC初始化5（:27102），原存档桥合法范围1..5。因此只将32错误`>3`门控修为`<1 || >5`；没有改原hostile公式/随机/效果/资源或其它技能。

`original_skill_schedule_test`新增逐个difficulty0..6：1..5实际事务均进入原奖励、MP40/AP45、XP5、group2，原熟练公式`(40+6-difficulty)*5/2+10`精确断言；0/6拒绝且MP/AP/目标装备/RNG不变。原64成长案例/4调度与88七种present调度仍全PASS。

### 88正疗伤：真实接敌而非改HP fixture

先递归只读检索SAVE与out中角色3原已学88且受伤存档，初始扫描结果为空。于是从真实`SAVE\UJ02.SAV`（角色3 HP878/878、ST70、武术3635）继续正常SDL输入：F10合法恢复，选人/移动命令，固定路径(23,30)→(20,35)，各后F10原AI；第三点(17,38)被实际盟友角色1占用，生产拒绝，保存`contact-blocked.sav`，未强行摆位。读取实际阻挡档后仅改为同一目标附近空位(16,41)，从阻挡档继续移动接敌，原AI回合使角色3 HP878→814（ST仍70、熟练仍3635），没有改HP/ST/RNG、seed筛选或新战术参数扫描。首次接敌续跑的driver在准备下一点时先记录“路线耗尽”阻挡，但该保存已含原AI实际伤害；随后直接从该真实受伤档施放，不重抽伤害。此原档副本持久保存在`out\skill88-positive-input\input-source.sav`，明确是UJ02正常输入沿途档，不是原始UJ02本体、不是fixture。

真实Windows SDL正疗伤：正常鼠标角色3→攻击后分区→88→自身→取消（前后save字节相等）→重新确认→action12准备→SPELLSP79完整35帧→疗伤64，HP814→878、ST70→45、AP88→43、武术3635→3655，原奖励rawXP10经倍率实际XP0→12，等级5不变。恢复静置500ms后档与cast-after逐字节一致，无二次支付。未启用强化，也未修改原UJ02或两条结局证据。

### 复现/验证与限制

资源根PATH含SDL3，最短正疗伤复现（无需重走接敌、不筛seed）：

```powershell
.\S2NE\out\x64\Release\envoy-opening.exe --smoke-skill88-input . S2NE\out\skill88-positive-input\input-source.sav --positive-heal
.\S2NE\out\original_skill88_positive_evidence_test.exe
.\S2NE\out\original_skill_schedule_test.exe
```

`out\skill88-positive-adjacent.log`为实际正疗伤日志；接敌链失败/改点证据保留`skill88-positive-input.log`、`skill88-positive-continuation.log`。正疗伤目录含35张真实帧BMP、sequence.csv、cancel-before/after、cast-before/after、settled-after及input-source.sav。正疗伤专项直接与原UJ02核对学习/熟练/ST/职业未注入、HP确有降低且回合推进，再检查正HP/正XP/准确ST25/AP45/熟练20、原准备与35帧、取消/静置字节一致及失败奖励桥不支付。已有满血88/32证据保留，不以正疗伤覆盖。

Release/Debug原生MSBuild均成功（`out\skill-final-release.log`/`skill-final-debug.log`）。七项当前源码专项均exit0：original_skill_schedule_test、original_skill88_positive_evidence_test、original_skill88_input_evidence_test、original_skill_input_evidence_test、original_skill88_test、original_skills_test、original_skill_visual_test；日志`out\*-final.log`。资源fixture真实SDL仍35不同帧恢复、32真实SDL专项23帧恢复。最初调查辅助工具均已退出并删除cpp/exe/obj；仅留证据日志。本轮未改PROGRESS/README。

边界：正疗伤不承诺每次随机命中/每次必有正HP；`--positive-heal`若原随机失败即报失败退出，不重抽。固定接敌route仅本次UJ02局面、阻挡会failclosed，不作为通用战术自动器。其它技能与19条单人合作仍未实现/未展开，双人对战排除，音频仍暂缓（环境恢复音频设备输出不等于已验收原技能音频）。

## 下一条剑术94「虛空劍法」：确证独立原视觉子链（非整技能完成）

### 选择与真实已学证据

优先剑术，选ID94「虛空劍法」，不是重做32/88，也未按effect1批量开放。原BIN2 group9/bit0（剑术94..104共11条），37字节记录：ST成本5、学习阈值WORD15=0（剑术熟练/10>=0，经45C8CA/learn_group；不是所有角色自动有剑术）、range3、targetmode1、effect1、formula44、power50、flags194=0xC2。进入原后分区需原learned bit、AP/状态门控；资源不足仍原ST gate；目标与特殊效果最终限制须沿原施放链继续核实，不能把32敌对range检查当作94全部目标合法性。

只读检查三份固定真实档：原UJ02队员未找到已学94；`run-49056-stage61-start.sav`角色11/16/17/32已学，`run-43140-stage131-start.sav`角色32原learned9=255/proficiency1800/ST142/AP70，位置12,52（其他11/17/16也已学）。本轮未修改任何真实档。该131沿途档来自既有明确强化流程，不重新宣称无强化平衡证明。

### 实际完成范围

新`src\original_skill94_visual.hpp`严格独立读取BIN10第94项：visual84→CEL1 SPELLSP85、payment0、tint233/35。真实CEL1 relative23130084、size19483，原16字节头`15,0,0,107,132,0,0,5`，mode2、15*8目录、107×132真实RLE图、透明0、5cs即50ms。按46B9A9（asm:184661起）anchor0/0分支确认为mode2；复用的只是已验证mode2原地形/近景单位AD遮挡算法，不使用32图像、尺寸、色表或伪几何。背景着色参数改为可显式传原BIN10值，默认140/45保留32行为，94明确传233/35。严格头/目录连续范围/尾数据及RLE异常failclosed。

新`tests\original_skill94_visual_test.cpp`加载真实131档验证原已学记录，但**在隔离视觉测试把显示目标取为已学角色自身，仅为让实际原15帧出现在视口；这不是94合法施放目标证据，不是游戏输入E2E**。真实Windows SDL实际present15帧、15不同画面、按原时钟、恢复前后画面一致，截图`out\skill94-frame7.bmp`。没有调用effect、没有扣费、没有改技能/熟练/HP/ST/RNG。

### 未完成/为什么不开放生产

94与32/88不是相同效果事务。原46D496→46D874的effect1分支还包含原熟练与+128h属性对抗（asm:187941..188084），后续伤害formula44及46E058→46E858/46E2D6含军队/难度/单位+116h/技能flags状态条件（asm:188751、189025起）。本轮尚未完成formula44伤害/防御/目标状态/奖励/支付所有语义核实，不能把32拆装备/88自疗套用。因此只交付确证资源解析→原mode2合成→真实SDL呈现→恢复子链；**未接生产94菜单请求、目标确认、伤害与结算，也未取得94正常输入E2E**，生产可执行白名单仍只有32/88，没有开放全部effect1。下一范围可从上述原地址精确追formula44并为94单条事务加隔离回归，再接实际已学131角色32正常目标路径。

### 验证与数字口径

原生MSBuild Release/Debug均成功：`out\skill94-Release-build.log`、`skill94-Debug-build.log`。资源根运行（PATH含SDL3）：

```powershell
.\S2NE\out\original_skill94_visual_test.exe
.\S2NE\out\original_skill_visual_test.exe
.\S2NE\out\original_skill88_test.exe
.\S2NE\out\original_skill_schedule_test.exe
.\S2NE\out\original_skill_input_evidence_test.exe
.\S2NE\out\original_skill88_input_evidence_test.exe
.\S2NE\out\original_skill88_positive_evidence_test.exe
```

均exit0；修改的地图头由94/32/88视觉专项当前源码重新编译覆盖，真实SDL driver=windows。结果`out\*-94-regression.log`，94编译日志`out\skill94-compile.log`。32/88原证据和正疗伤档保留。

总数115=96非合作+19单人合作。完成生产且有正常输入E2E仍为2/96（32、88）；剩余94/96未完成，其中本轮94仅视觉子链部分覆盖，不能记为第三条完成；19合作仍未实现，非用户排除。双人对战排除、音频暂缓。未写PROGRESS/README；只读probe工具退出并删除cpp/exe/obj。结局两档hash维持既定原值，收尾停止辅助和写入。

## 94后续：原命中基础/原formula44算术子链（未开放事务）

本轮新增`src\original_skill94_formula.hpp`与`tests\original_skill94_formula_test.cpp`，仅提供无状态算术，不改生产菜单白名单，不扣资源、不造成伤害。确证并实现：

- 46D874 effect1（asm187941..188084）：**两次熟练读取都是施法者**同一原+D7数组索引，不是双方熟练；各/10且最少10。`[unit+128h] sar16`实际读取+12Ah SP，不是+128h ST，也不是fencing。双方SP限10..99，差值`((p+500)*actorSP-(p+500)*targetSP)/500`，C++有符号除法向0截断。46DBEF..46DC95分段转命中率，再乘原power50/100。覆盖-120/-80/-40/0/10/42等17个边界，保留原-80分段不连续，不自行平滑。**这是407CD0随机/难度修正前基础值，不是最终命中率。**
- 46E858中94 flags0xC2只进入bit2属性分支，不进入bit1的装备special18加成分支；`[unit+C5h] sar16`实际+C7h fencing，双方取max1；熟练为actor group9/10，无合作平均。case44确定位于46F89E（asm191092起），不是其它剑术公式。
- 46F89E→4704AC（asm192102..192149）寄存器/栈顺序：`((proficiency/10+1000)*max1(actor.fencing))/1000 - max1(target.fencing)*config[98h]/config[99h] - target.defense + power50`。配置从原BIN10后表直接读有符号byte，本资源10/10，除数非正failclosed。**允许原始结果负数，不在此提前加最小伤害。**原131角色32/敌20实际算术探针baseHit15/rawDamage-200，无状态变更、未筛RNG。

继续追查但尚未实现的明确边界：46D496依据46D874返回mode2进入407CD0，再4080E7原军队/模式/难度修正及最终命中；46E058先46E858后46E2D6，后者调用440F56、处理难度1+目标116h bit20、技能flags21h的70h分组、目标3Dh/职业元素抗性和后续修正；46E058还单独处理目标3Dh bit8。不能把pre-modifier算术称为实际最终伤害，更不能在这些未核完时接奖励支付。目标合法性、HP状态写入、原XP/熟练/ST5/AP及生产输入仍未接94，本轮**没有94正常输入E2E**。原asm边界189025..189673与46E058分发表应作为下一次有界追踪，不扩大其它effect1。

验证：formula测试当前源码cl C++20重新编译exit0（`out\skill94-formula-compile.log`/`skill94-formula.log`）；原生Release/Debug均成功（`out\skill94-formula-Release.log`/Debug）。真实Windows SDL94视觉15帧仍PASS，32 schedule64/4、32持久输入、88正疗伤持久输入均PASS：`out\*-formula94.log`。无新增辅助进程。没有写PROGRESS/README，没有改真实验收档或结局证据。数字仍2/96完整生产覆盖、94/96未完成（94现为视觉+基础命中+raw formula部分）；19单人合作待实现而非排除。

## 94继续溯源：重要命中入口纠正及后修正算术子链

**修正此前计划依据，非生产回归：**94真实record flags33=0xC2。46D496在flags21h bit1分支才调用46D874；94 bit1=0、bit2=1，因此实际命中入口是`46D748→46DD11`，不是46D874。上一轮base_hit函数保留为已核实46D874算术，但**它不是94实际命中入口**，不能拿其结果当94最终命中率。本轮未把错误前提接生产。

已追46DD11（asm188425..188749）：先两次42C88F取地图/朝向数据，原非合作actor/target各读group9熟练；双方+C5 DWORD高半是fencing，目标还将+136 DWORD高半（+138 tactics）与fencing平均。随后将双方熟练+1000、fencing/目标平均值、42C88F所返回的signed byte参数参与对抗，分段曲线后乘power50。此**地图/朝向参数42C88F尚未完成映射**，因此完整命中仍阻塞在具体原地址42C88F→46DD11，不能以双方SP旧base_hit替代。94 flags80再启用46D7D5原地势差429F34，difficulty1跳过该差异。

新增`original_skill94_resolution.hpp`与对应真实配置隔离回归，严格只提供无状态算术，无target授权、无HP写入/支付接口：

- DD11调用后的46D748/407CD0会对**原累加器0**消耗一次命中随机，乘/10、4080E7后限为1，再加DD11单独结果；不是对DD11结果本身随机缩放。随后原地势差clamp±5，每差一级20%，difficulty1不修，最终限1..99。函数明确要求外部DD11结果，未自称完整最终命中。配置原不对齐DWORD读取分别是config6/7（命中）与config4/5（伤害），不能直接照抄别类10/11。
- 46E2D6对94已精确裁剪：raw29=0，故元素族/440F56族加成标志全部不触发；flags0xC2没有70h，**原军队地形/目标116h20分支整个跳过**，不得替94虚构这些修正。职业抗性/弱点WORD与原技能mask64相交时分别/10、*3；special13或status3F bit4使*2/3，status39 bit10再*80/100、bit20再*70/100；94 group byte10不触发仅2/4/3/5/6的特判，也非合作，不触发双方special2加成。
- 然后407D88原随机→4080E7军队/模式/难度→至少1；目标3C bit40护盾≤180变1，否则减180。46E058再处理目标3D bit8清0，**清0发生于随机消耗之后**。专项覆盖顺序、抗性、负raw最低值、1..5与0/6 failclosed，所有随机运行仅隔离fixture，不改真实档或筛随机。

**明确尚未完成：**42C88F地图朝向结果/46DD11完整命中及真正命中roll调用点；46D431/其调用者伤害写入与死亡/原XP熟练及ST/AP提交顺序尚未追完；因此没有生产94事务，也没有声称失败原子性或正常输入94 E2E已完成。没有为输出看似完整而使用近似hit、伪死亡或32/88奖励桥套装。本轮仅交付上述确证子链，下一步应先完成42C88F输出布局以及原46D431调用者的写入奖励路径。

验证：新resolution专项当前源码C++20编译exit0（`out\skill94-resolution-compile.log`/`skill94-resolution.log`）。Release/Debug均成功（`out\skill94-resolution-Release.log`/Debug）；resolution/formula/94实际Windows SDL15帧、32 schedule与持久输入、88正疗伤持久证据六项exit0，日志`out\*-resolution94.log`。未写PROGRESS/README，未修改真实验收档/结局档，无辅助进程。完整覆盖仍2/96，94仍部分，19合作未实现非排除。

## 94本轮：42C88F/DD11及实际effect1 roll子链（仍非生产事务）

纠正上节“地图/朝向”措辞：`42C88F→42C8D1`按单位x/y与地图宽度查地面/物件属性，输出三个byte `{movementCost,hitPercent,defensePercent}`，不是面向。复用Battle既有地形映射，仅新增只读`original_42c88f_bytes`入口，不改变private区段或移动规则。DD11读取actor byte1与target byte2，均有符号；双方group9熟练分别参与，目标用(fencing+tactics)/2。DWORD乘法/差值显式模拟原wrap，分段后乘50/100。只读真实`run-43140-stage131-start.sav`角色32/敌20：地形10/0，DD11=45；未执行该档随机或改写档。94原record flags0xC2、mask64、raw29=0等已钉死，D874仍仅无关辅助。

**新增调用链纠正非常重要：**上一定位的4668F0并非94 effect1入口，不能将其case1当94。`4654A6+CE`检测record+13h(effect)==1后直接`466082→46618D→471F51→46D431`。46D431先46D496命中、非零再46E058伤害（含伤害随机）。471F51随后按目标当前HP封顶伤害，伤害0再提升为1，最后在`471FB9`调用47328B，roll<=chance命中，miss输出0。因此46E058的3D bit8清0不等于最后可直接宣称免伤：471F51还提升为1。未把未执行flags或其它effect分支计入94。

删除不完整的chance→percent `resolve_hit`封装，改为94算术packet `resolve`：chance随机→damage随机→HPcap/zero1→percent。只在成功后发布暂存RNG，不写HP/熟练/XP/ST/AP。64个固定fixture seed穷举校验同一原随机流顺序及清0后提升1；非法上下文不推进RNG。它不等于完整施放事务，不声称真实档命中或失败无支付已完成。

**已定位但未完成接线/核全：**46618D在471F51后调用468BAC反馈/HP更新，再依HP0走special25/28复活、452FB1死亡处理、468FDF击杀奖励；存活则special29可能追加percent反疗，随后469101奖励。原HP字段+124h，468BAC还保存到+132h。上述死亡特殊技/反馈副作用、468FDF/469101全部奖励与4654A6尾部ST/AP提交仍需逐分支核实，不能用32/88桥或简化死亡替代。本轮到确证命中/伤害packet子链为止：**未接生产94菜单、未实现完整94事务/失败无支付、没有94正常输入E2E**。后续有界目标正是这些地址，不再追4668F0无关case1。

最终验证（改动后重新执行）：原生`MSBuild S2NE.sln /p:Configuration=Release /p:Platform=x64`及Debug均exit0，日志`out\skill94-hit-Release.log`/Debug。resolution用cl C++20重编译exit0，`out\skill94-hit-compile.log`。从资源根执行`S2NE\out\original_skill94_resolution_test.exe`、formula、visual、schedule、input_evidence、88_positive_evidence六专项均exit0，逐项日志`out\original_skill*-hit94.log`；其中Windows SDL94为15个不同原帧、50ms时钟及恢复，仍仅视觉隔离链，不冒称正常输入E2E。32 schedule和持久输入、88正疗伤证据保留通过。两结局SHA256保持原值：white `1D64E38324595851F57857A8BED88D696DBB033F9834F9D97234D3F22EBD0874`；natalia `A89CF281CC67FB481AFEEC2FE2D499A5A1D645ED09132A1D8AC485ECC799041F`。

本轮未写PROGRESS/README，未注入真实档、未筛真实随机，无辅助进程留存。完整生产覆盖仍2/96，94是部分，另94/96未完成；19单人合作仍未实现，不属于用户排除。

## 94本轮事务边界：HP反馈、常驻复活、存活反疗与原支付（非完整生产交付）

逐分支追到以下原证据：468BAC先保存目标HP到+132h（MISS也保存），然后effect1按伤害递减；负值治疗封顶maxHP。46618D在HP0时优先441088 special25常驻装备复活，负maxHP调用468BAC故+132重新为0；其次special28同样复活后451D28→44D11B消耗首个匹配装备并重算属性；否则452FB1增加loss，执行41C02A死亡事件分派（可变身/移除/清激活），然后468FDF→469101及战利品缓冲累加，最后增加actor击杀数。存活路径special29另抽47328B，严格roll<30才以原damage*2反疗；MISS也会抽这个roll，反疗再次更新+132，不能始终用最初HP作为奖励快照。

469101使用45CFBB selector=1的**+132 HP快照、其它当前属性**（不是新造战力缓存公式），94非合作second分区取config E6倍率、E1除数：damage*E6*targetValue/(targetValue+actorValue)/E1，末尾至少1；真正死亡另加config E2*targetValue/actorValue+3。466082熟练为(成功目标数+本次击杀数增量+6-difficulty)*10/3+10。4654A6完成熟练学习再458B03/459309，**AP/ST实际在其调用者随后464CFA**，不是4654A6尾内：second mask8、config EF基本AP、已有mask8再扣F3、carry下限同步；505C1C状态2仅免资源，仍扣AP；94 flagsC2只扣ST5，无MP/HP支付。

新增`original_skill94_transaction.hpp`为**菜单请求所有权约束的部分事务模块**，不是已开放生产执行：仅94、真实learned bit、second、合法range3、ST/AP/运行态门控；暂存actor/target/RNG后执行现有原resolution→HP/+132→常驻special25或存活special29→469101存活公式→外接已验证学习/XP桥→464CFA支付，成功消费请求。回调拒绝、取消、重复提交、未知runtime不发布。special28消耗/重算缺完整army inventory桥，以及普通死亡41C02A/452FB1/468FDF战利品/脚本副作用**仍明确原子failclosed**，不是简化死亡；对应失败不扣费不写HP/RNG。没有连接94生产菜单执行授权或动画触发，避免真实施放遇到未覆盖死亡副作用被冒称完整。

紧耦合修复：OriginalEvents::combat_value(mode1)现在严格要求已有+132快照，并复用原战力算术；OriginalSaveBridge写回可选+132，以前只读不写会丢失本次反馈。没有改变mode0/2或身份校验。测试只修改隔离fixture副本，原131档只读，绝不称正常输入E2E。

专项`original_skill94_transaction_test`固定64 seed全部执行，不筛选：命中27/未命中37、常驻复活27、反疗roll触发27；覆盖HP快照、原额外随机顺序、原熟练、ST/AP、奖励回调失败rollback、死亡/消耗复活failclosed、取消/重复提交/未知runtime及+132持久往返。测试award为隔离断言adapter，不能冒称真实成长E2E。测试开发初期反复构造大资源导致长运行，终止对应工具进程后将不可变rules/formula移出64循环；最终单次执行全部通过，没有将旧中断算PASS。

最终源码重新cl C++20编译七专项：transaction94、resolution94、formula94、visual94、schedule32/88、32持久输入、88正疗伤持久输入均exit0。命令从工程编译`cl /std:c++20 /EHsc /utf-8 /I src /I D:\project\vcpkg\installed\x64-windows\include tests\original_skill94_transaction_test.cpp ... SDL3.lib`，从资源根运行`S2NE\out\original_skill94_transaction_test.exe`；其它同名专项同理。日志`out\*-transaction94-compile.log`与`out\*-transaction94.log`。Release/Debug原生MSBuild均exit0，`out\skill94-transaction-Release.log`/Debug。真实Windows SDL仍15个不同原帧/50ms/恢复通过，**仅视觉隔离链，不是正常输入E2E**。修改文件Problems无诊断。最后追加取消/runtime测试后重编译重跑transaction exit0。

**未完成且阻塞可用94生产路径的准确范围：**special28的451D28→44D11B装备消费/重算；452FB1→41C02A完整死亡脚本/状态和468FDF战利品提交；因此没有完整死亡奖励事务，也没有94真实131角色32正常输入施放E2E。下一轮应围绕这三项收口及生产接线，不重新做已核命中。完整覆盖仍2/96、94仍部分；19单人合作未实现而非排除。未写PROGRESS/README，未修改真实验收档；两结局hash核验仍原值。所有本轮辅助进程已结束，停写交付。

## 94收口续轮：special28与普通死亡/掉落持久化（仍未生产E2E）

本轮只推进前轮三项，不重做命中。新增451D28首匹配槽查找：按slot0..5、item special28、item+15类型0/1取首个，复用已还原44D11B的`OriginalPromotionTransaction::remove_equipment(...,consume=true)`同步移除装备/属性，不退库存。原441088激活判定仍只接受类型0；找首匹配与是否激活不是同一条件。事务现在可接调用方的完整army inventory vector及equipment资源桥，库存长度必须精确item_count，副本完成后才发布；未接main真实rawSave库存，不能声称真实army生产联通。

普通死亡仅授权原41C02A无匹配type3/condition2/roleID事件、且非status33 bit40移除/bit10召唤队伍槽的分支：452FB1 loss WORD+1、普通corpse bit20、保留原激活bit；actor kill WORD+1。匹配死亡脚本/移除/召唤分支仍failclosed，不调用物攻的简化队列冒充41C02A。该函数有即时脚本返回11/12/13及变换/移除/队伍副作用，现queue_original_death不能完整替代；**未实现这些完整死亡脚本，不开放94生产施放**。死亡消失视觉455048也未接本事务。

重要奖励纠正：前轮记录“死亡另加config E2*targetValue/actorValue+3”只是4693E0通用尾分支，**不能用于94真正击杀**。468FDF先调用469101时arg4=0、arg8=1，HP0立即在46913D给XP accumulator+1并返回，跳过后面所有战力/击杀加成算术。新94普通死亡rawXP严格1；熟练成功数与kill增量均计入466082。

468FDF mode0/friendly army读取目标+11B四个WORD（现Unit.initial_status），不是装备；按ID累加byte loot buffer（重复ID叠加），458B03仅非零loot项并入我军inventory并cap99，无loot的项不改。未触碰敌军装备/其它inventory。事务对库存/装备/目标/actor/RNG同一暂存，奖励回调失败全部回滚，成功消费菜单请求。OriginalSaveBridge紧耦合新增写回原+1F kills/+21 losses（原先只读），不改变单位身份/顺序/分组校验。

隔离94固定64 seed全运行：hit27/miss37；新增双special28装备仅消费首槽且不退库、奖励失败不泄露移除；普通无脚本死亡/重复掉落/99封顶/无掉落项保留/击杀及loss；死亡奖励失败不泄露库存/HP/kill/loss/RNG。新增`out\skill94-death-fixture.sav`为**明确隔离fixture**，使用原Save encode/load/bridge完整往返核对corpse/我军库存/actor kill/loss；该fixture把目标ID60000副本在编码前恢复原roleID，不属于真实验收档或正常输入E2E。没有修改原131或两结局证据。

最终验证：Release/Debug原生MSBuild exit0（`out\skill94-close-Release.log`/Debug）；九项均以当前源码cl C++20重新编译并执行exit0：94 transaction、94 resolution、94 visual、32/88 schedule、32 input evidence、88 positive evidence、save_bridge、promotion_transaction、combat。编译/运行日志`out\*-close94-compile.log`/`out\*-close94.log`；末次补充special28失败rollback与kill持久断言后transaction重编译重跑exit0。复现从资源根`S2NE\out\original_skill94_transaction_test.exe`，其它同名专项同理。Windows SDL仍15不同原帧、50ms、恢复PASS，仍仅视觉子链。Problems四个修改代码文件无错误。两结局SHA256保持原值，未写PROGRESS/README，无本轮运行辅助进程。

**准确剩余：**41C02A即时死亡脚本/返回值→452FB1移除/变换/召唤槽清理与持久化尚未完成；main真实army inventory及94生产菜单/准备/15帧/目标集成未接；真实131正常输入E2E未执行。本轮比上轮新增特殊28与普通死亡loot可提交的隔离事务，但仍不是完整94技能交付。完整生产覆盖仍2/96、94部分，19单人合作仍未实现非排除。停写交付，不将已通过隔离fixture包装成可用生产路径。

## 94最后阻塞续核：41C02A有序扫描与原返回值（本轮未完成生产）

本轮直接重读opening.idb.asm 47195..47510（41C02A）、47757..47838（41C57C/41C5EA）、48301..48830（41CA6E）、53886..53930（41FF30）、144363..144620（452FB1/4531AB）、147686..147835（454ED5），与现prologue的queue_original_death/dispatch_campaign及Save.Group.status对照。未将现异步queue改名伪装同步原routine。

新增`OriginalDeathDispatch`只做原扫描计划，不伪称完整脚本执行：按原记录顺序，只匹配type3/condition2/role WORD；第一个匹配的line count按signed WORD解释，<=0在41C0FE直接退出整个扫描，返回0，不继续后续同ID记录、不完成事件、不置completion flag。正行数在41C148置合法completion flag，41C182将返回值设为**十六进制12h（十进制18）**，随后41C57C准备、41CA6E(mode1)同步呈现、41C5EA释放、41FF30同步扫描触发链后才回452FB1。新计划将该类标为synchronous_script且仍精确拒绝，事务不发布临时HP/支付/库存/RNG。此前“任何匹配type3/cond2均拒绝”的门控过严：现精确允许首个匹配空记录退出这一无脚本分支，不开放任何正行数记录。

重要控制流澄清：41C161/41C1A3里condition1/3/4的11h/12h分支在此函数type3/cond2过滤后不可达；41FF30循环41C33E直至0，41FF5B显式返回0，所以不能把11h/13h当此死亡入口实际产出，更不能为测试随意造一个返回11/13适配器。452FB1自身仍有防御性11h移除/13h特效分支，尚未实现其生产副作用；**不是已完成全死亡脚本**。454ED5的FE是十轮原位图调色/重绘、末尾恢复AD缓存，不是将roleID或appearance设FE；本轮未修改身份序列化。status33 bit40或11h调用4531AB后清active，13h清active并置dead；普通分支保留active并置dead。status33 bit10按appearance在army+286的11个三字节槽找首匹配置首字节0，再441143重算。原Save.Group.status恰保存这33字节，但事务现尚未接army status与441143，因此0x10/0x40仍拒绝，不“只改单位旗标”冒充完整持久化。

测试增加：有序扫描略过cond3/其它role，首空匹配阻断后续非空，signed FFFF按空退出，正行数返回12h且拒绝/no match；固定64事务增加两种0x90/0xC0死亡分支原子拒绝，检查HP/ST/status/inventory/RNG(draw count与seed)未泄露。真实131只读审计：角色32原已学94，xy12,52、ST142、AP70；range3活敌仅1个。role20 xy10,52本身距离4，不在range3，虽无死亡事件也不能拿它假称合法施放目标；部署中活敌正行数死亡脚本计数0。前后OriginalSave capture/encode逐字节相同，未执行移动或RNG。此审计只证明脚本扫描边界，未证明真实生产施放；未注入/筛随机/更改实际档。

统一验证exit0：Release/Debug原生MSBuild（`out\skill94-dispatch-Release.log`/Debug）；九项当前源码cl C++20专项：original_skill94_transaction_test、original_skill94_resolution_test、original_skill94_visual_test、original_skill_schedule_test、original_skill_input_evidence_test、original_skill88_positive_evidence_test、original_save_bridge_test、original_promotion_transaction_test、original_combat_test。日志`out\<name>-dispatch94-compile.log`/`out\<name>-dispatch94.log`。复现资源根运行`S2NE\out\original_skill94_transaction_test.exe`；输出固定64 hit27/miss37，真实131只读range/death诊断。Windows SDL=windows独立原SPELLSP85仍15个不同帧/50ms/恢复PASS，仅视觉子链，不是正常输入94 E2E。新增只读诊断后transaction重新编译重跑exit0。

**本轮未达用户期望的生产收口，明确未完成：**41CA6E即时对话/41FF30触发链的通用事务同步复用，452FB1移除/召唤槽清理与441143提交，94 main真实army库存/菜单/准备/15帧/range3/正常输入结算尚未接。现生产仍只32/88（2/96），94部分，19单人合作仍缺失非排除。不可因本轮scan/fixture/视觉通过称94技能可用。后续可执行范围：先把原同步死亡脚本所需flags/event states/单位与army33字节同事务呈现提交，再将该支持能力而非脚本queue接入94；或严格预检无脚本普通目标后接生产，但仍需补全真实运行态39/职业mask来源、原反馈时序和保存提交，不能默认零。未写PROGRESS/README，停写前复核两结局hash及进程退出。

## 94有限生产路径与真实SDL输入（131角色11；角色32原为reserve）

本轮不再以完整41CA6E作为全部目标前置。main接入独立94事务、原army groups[0].inventory、原菜单后分区、46C078准备、独立SPELLSP85 15帧/tint233/35/50ms、原范围3/公式/支付奖励、OriginalSave capture保存。未批量开放effect1。OriginalSkills新增显式94 record gate；后分区可操作性只加原已学group9 bit0。target_supported在准备前及事务随机前拒绝status33 10/40、首匹配正行数同步死亡脚本或未知39运行态；空首匹配仍按41C0FE退出，不改脚本旗标。不支持目标不会消费随机/HP/ST/AP/inventory。actor新增active/dead gate，避免此前隔离fixture漏验真实reserve资格。复杂死亡仍未实现。

运行态来源：OriginalSaveBridge从每个原unit record+39保存optional byte，不依赖仅bit400才存在的action_status_runtime；若runtime snapshot存在，生产读live snapshot+39/+3C/+3D/+3F，否则读原解码缓存。不以未知零冒充已知。KINDSP+18 DWORD职业表、(appearance-1)*37，+16/+18 WORD按46E489/46E4DB给mask64抗性/弱点；main调用OriginalEvents::skill_class_masks，不复用32/88公式。army来自rawSave完整group0 inventory，事务同单位/RNG暂存并在成功后捕获持久化。39缓存写回同原byte；保存身份校验未放宽。实际验证mask0/256、runtime39=0，是真实读取而非默认猜零。

纠正前轮源档可达断言：run-43140-stage131-start.sav角色32虽原已学94、ST142/AP70，但status33=0，是reserve，原occupant返回-1，不能正常选中。已有五个131原档角色32均inactive。原日志曾把其range3唯一敌当可达施放，遗漏active前提，不能再沿此断言。本轮保留原旗标，未激活角色32。只读查实际沿途存档：stage40/46有active32但初始无range3目标；有界正常接近/回合尝试未形成施放，未用此掩盖完成。探索性stage46单waypoint输入已删除，未加入生产战术参数。

**实际成功路径：同一未改131原档另有角色11原已学94且active。**正常输入驱动严格过滤active/dead，选原角色11、原现地敌role0（原资源普通敌合法ID，非boss20），未注入技能/熟练/位置/HP/ST/旗标，未设置seed或按命中重跑。固定这次结果MISS：chance50/roll53、damage0；原ST219→214/AP71→26/熟练2800→2826/XP2→3，MP401不变，无升级。生产正常选人→攻击后分区→94→原目标→取消无支付→重开确认→原准备→SPELLSP85帧0..14→事务→画面恢复，500ms idle无重复支付。Windows SDL实际Presented312，15帧逐present序列/图像见out\skill94-input\sequence.csv及frame-*.bmp；before/after/cancel/settled原档在同目录。成功日志out\skill94-production-input.log（原首写文件名input-blocked.log来自预期reserve阻断探测，但结果实际为角色11成功，已复制为准确success日志；不把角色11冒称角色32）。没有为获取hit而重跑成功施放；首次达到施放即MISS验收。

新增original_skill94_input_evidence_test核对源131角色11已学/active、before与源学习位/装备/位置/ST一致，after原ST5/AP45/熟练26/XP1、库存未变，cancel及settled逐字节相同。复现资源根S2NE\out\original_skill94_input_evidence_test.exe。正常输入命令：S2NE\out\x64\Release\envoy-opening.exe --smoke-skill94-input . S2NE\out\strengthened-flow-acceptance\run-43140-stage131-start.sav。会重新运行真实随机施放，不应要求仍roll53；持久证据测试不重新掷随机。

统一Release/Debug MSBuild exit0：out\skill94-production-Release.log/Debug；当前源码十专项重新cl并执行exit0：94 source/transaction64/resolution/visual、32/88 schedule、32 input evidence、88 positive evidence、save bridge、promotion transaction、physical combat；另新增94 input evidence重新编译执行exit0。日志out\*-production94-compile.log/*-production94.log及skill94-input-evidence.log。原15帧独立Windows SDL仍PASS；真实输入另有上述E2E，不再仅视觉。调度32仍64案例34旧误报、无双支付。真实输入驱动修正鼠标像素落在原picker内部，禁用smoke期间外部真实鼠标edge scroll干扰，未改生产玩家点击/地图或单位位置。

准确覆盖：94现在有限生产接线并有真实角色11 MISS完整输入及存档证据，不是全目标/全死亡脚本完成。32/88保持；96非合作中2条既有完整路径+94受限路径，其余93未有此种完整生产验收；19单人合作仍未实现而非排除。41CA6E同步死亡、0x10召唤/0x40移除、死亡特殊呈现/数字反馈进一步原时序仍缺，不声称本轮完成这些。原角色32的131正常输入不可达因reserve，未帮改旗标；下一范围可独立扩原部署或同步脚本，不能把本轮角色11成功推广全94目标。未写README/PROGRESS，结局档hash不变，结束停止辅助并停写。

## 94命中数字反馈：原468BAC新生产分支与续档HIT证据

重读468BAC asm180011..180415，确定原数字循环与物攻不同：直接step0，不走物攻452E7F silhouette/20ms flash；damage0显示MISS，正damage用3C/56，MISS68/24；x=32-字数*8，y=-64-step*8（仅前8步更新y）；每步扣min(remaining,1<<step)，至少8次present，剩余伤害未扣完则继续；前0..2步30ms、step3为20ms、step4..5为10ms，6以后无时钟等待但仍须实际present；文字仅step<7。新增Battle::begin_skill94_feedback单94专用入口，复用原Feedback文字投影/字体与presented HP chunks，明确绕开settle_physical（不得二次物攻XP/死亡回调）。present gating保证卡顿不会跳过原数字帧；presentation_units用反馈HP临时显示，不将暂存计算当提前live写入。

main在原15帧结束计算94完整暂存副本（unit/inventory/menu/RNG及奖励），仅接受后挂起在数字显示；数字完成callback一次性发布actor/target/ST/AP/熟练XP/inventory/RNG并capture原save。此期间live值未提交、游戏输入由combat_feedback_active锁定；不会用物攻routine再结算94。不是异步死亡脚本队列：**41CA6E/41FF30仍未接，不支持同步事件的目标仍在选择和随机之前拒绝**。本轮新增可用分支是普通目标命中伤害数字显示+延后提交，不是全死亡/脚本完成。原特殊25/28/29事务算术仍在，但其独立多次数字恢复与死亡淡出完整呈现尚未全覆盖，不能将一次普通hit外推特殊动画已完成。

真实HIT并非从同起点为命中重跑：从上一轮真实MISS的持久结果`out\skill94-input\cast-after.sav`继续，仅执行一次正常输入；未设置seed/筛roll。此时原角色11已支付后AP不足，正常驱动按已有合法门控选下一位原active且原已学94角色16（非reserve32），原位置22,81、原现地普通敌role0。取消后重新确认是原既有cancel验收，不是重掷；最终HIT chance50/roll32、damage1，目标实际HP减少1。actor16 ST230→225/AP72→27、熟练2800→2830、XP60→61、MP400不变、level60不变。数字完成后保存，500ms idle无重复。SDL=windows，Presented359，独立15帧后数字恢复完成日志`out\skill94-feedback-input.log`；证据`out\skill94-feedback-input\cast-before.sav`/cast-after.sav/cancel-before.sav/cancel-after.sav/settled-after.sav/sequence.csv/frame-*.bmp。旧MISS证据目录保留未覆盖。**MISS是原角色11，HIT是续档原角色16，两者不是同一次结果，不冒称角色32或击杀。**

新增`original_skill94_feedback_test`：固定300 damage验证step0、暂停20 tick未present不前进、不发布live HP，至少8帧、无物攻flash、commit仅一次；持久HIT证据核实角色16与原MISS after存档学习位/熟练/HP/ST/位置一致，后ST5/AP45/熟练30，恰一个敌HP减1、cancel/settled逐字节一致。复现资源根`S2NE\out\original_skill94_feedback_test.exe`，不重新掷随机。真实输入复现命令`S2NE\out\x64\Release\envoy-opening.exe --smoke-skill94-input . S2NE\out\skill94-input\cast-after.sav skill94-feedback-input-new`，新运行不保证hit（不能用重跑筛hit），指定新目录保留既有证据。

统一Release/Debug MSBuild exit0，日志out\skill94-feedback-Release.log/Debug；十二专项当前源码编译运行exit0：94 feedback/transaction64/input-evidence/resolution/visual、32/88 schedule、32 input evidence、88 positive evidence、save bridge、promotion transaction、physical combat、physical combat visual。日志out\*-feedback94-compile.log/*-feedback94.log。Windows独立15帧仍PASS；真实SDL命中数字新链已执行。最后HP presentation与source continuity断言变更后两配置重建、feedback专项重编译exit0；没有再次从原起点重复施放求hit。

剩余精确边界：未实现41CA6E同步对话及41FF30触发链的共享dispatcher挂起/恢复，未开放正行数死亡事件/0x10召唤槽/0x40移除。普通HP0死亡数字后还缺完整455048死亡呈现，故本轮不声称真实击杀E2E或完整死亡动画。同步脚本事务后续须在原completion flag→即时对话→41FF30整条链结束后再交452FB1，不能拿queue_original_death替代。本轮无脚本能力新增；交付确证普通HIT+数字延后提交这一新生产分支。reserve32纠正保持，19单人合作仍未实现非排除；仍94受限覆盖、其余93非合作未有此类完整生产验收。未写PROGRESS/README，未改两结局证据，结束无辅助进程并停写。

## 94普通HP0：数字→原455048(3C,0)死亡模式→延后单次发布

重读452FB1与455048 asm147835..148110发现不能复用现故事消失的五帧模式：普通死亡loc4530C5传EBX=3Ch/ECX=0，455195先424873把原masked source用3C、80%映射，再4551B7选择初始weight10、10帧、step10、间隔3 DOS百分秒即30ms；无色参数故事消失才是初始40、5帧、20ms。本轮OriginalMap::begin_disappearance增加显式death模式，复用原AD/B1、脚影、地形与遮挡mask，单独原tint3C/80及10..100十次原palette mix；旧五帧故事模式默认未改，不拿几何/RGBA淡出替代。

Prologue新增单94 death continuation：数字完成且暂存finalHP==0时先begin_skill94_death，不立刻公布corpse/loot/payments。原live目标保持可绘制，Battle专用death lock继续阻挡选择、取消、移动、物攻、脚本扫描；每帧须实际present并30ms才推进，末帧结束解除锁并只调用一次原先暂存publish。main最终actor/target/inventory/RNG/reward/ST/AP及save capture全部在该点发布。普通目标不再数字结束即突然变尸体，亦不调用物攻settle_physical/queue重复奖励。特殊同步死亡脚本/0x10/0x40预检仍拒绝。

新增tests\original_skill94_death_visual_test.cpp实际Windows SDL：原资源普通敌indexed十帧distinct10、weight10..100/30ms；29ms不提前帧。再用真实资源Prologue隔离lethal数字→死亡→单次fixture发布，20次无present tick不公布HP，23次实际present完成整个numbers/death锁链，commit计数1，额外idle不重复。此整链为明确隔离fixture：末回调发布transaction64已生成的skill94-death-fixture.sav事件副本，**不是自然击杀E2E，也不是在此测试里重新调用完整main菜单事务**；对应必杀/loot/库存99/kill-loss/一次支付/RNG原子与原save往返由original_skill94_transaction_test固定64隔离回归覆盖。本轮没有正常输入自然击杀，不把上轮damage1非致死HIT当击杀，未修改任何真实验收档造HP0。

Release/Debug原生MSBuild exit0（out\skill94-death-Release.log/Debug）；十一当前源码专项编译执行exit0：94 death visual/feedback/transaction64/input evidence/visual、32-88 schedule、32 input evidence、88 positive evidence、save bridge、physical combat、physical combat visual。日志out\*-death94-compile.log/*-death94.log。集成death测试补充后重新cl与真实SDL执行exit0；复现资源根S2NE\out\original_skill94_transaction_test.exe后S2NE\out\original_skill94_death_visual_test.exe。原Windows SDL截图链没有生成新的自然kill存档。

本轮确证新增生产能力为普通无脚本HP0的专属原死亡十帧与数字后挂起/提交，非全94死亡事件。41CA6E→41FF30同步dispatcher仍未接，本轮没有开放任何正行数事件：completion flag必须在即时对话前、41FF30整条触发链必须在452FB1继续前完成；现queue不是等价，不以“设计”名义开放。未知脚本/移除/召唤持续精确拒绝。特殊复活/反疗多段反馈仍缺独立完整呈现。原reserve32资格纠正保留；旧角色11 MISS及角色16 HIT证据未改，两结局hash未改。未写PROGRESS/README，结束无本轮辅助进程并停写。

## 94同步死亡：真实原对白挂起/恢复（严格受限，不等于完整dispatcher）

本轮接通最小确证链：数字结束→41C02A completion flag先置位→Prologue原prepare/分页/逐字/输入advance对白→41FF30可证明无操作扫描→返回452FB1原死亡十帧→原暂存奖励/支付/库存/RNG一次发布。没有使用script_queue，也没有跳过对白输入。death record按原保留type3；重复begin拒绝、tick/cancel不能跳过、尾resume仅一次。正行数死亡目标的选择/提交能力预检复用同一OriginalDeathDispatch::continuation。

严格纠正预检：41C33E每个扫描位置还调用41B9A3；41B5DF接受type4..16，不能只凭+3旗标未激活就认定无副作用。因此本轮仅接受整张当前event表无type4..16的正行数死亡，任何此类记录均保守拒绝（含type9/cond0，尚未实现其嵌套扫描顺序）。41FF6E对completion1/3有原显示、7有原例程，均拒绝；其它completion在该routine无操作。无脚本/首空仍保持原早退，不擅自执行41FF30。此前候选stage2/15含其它dispatcher指令，已改为拒绝，未以其假称安全。

新增original_death_continuation_test真实资源fixture：stage6 role104首匹配一行、completion0；stage82 role147一行、completion13。后者验证13在对白开始后且恢复前已置1、死亡record未消费、链尾仅回一次。draw/advance使用原Prologue，不是正常存档自然击杀E2E。stage1 role90与stage15 role202拒绝且flags/锁/return计数不变；重复begin、无present多tick/取消不跳过覆盖。原资源扫描器tests\original_death_event_scan.cpp及out\death-event-scan.log提供选链证据（扫描器仅诊断，不改变资源）。

中途原sav策略为禁止写半事务，不序列化continuation：original_save_boundary_ready在数字/同步对白/死亡呈现期间false；main原保存/读取提交与autosave均门控，SDL quit期间拒绝跳过；完成后恢复保存。prepare任意后续行解析异常会恢复events baseline/flags、清pending callback及锁并抛出，不继续奖励支付。外部强制终止不写新半事务存档。尚未建立复杂脚本中途可恢复sav协议。

验证：原生MSBuild S2NE.sln /p:Configuration=Release及Debug /p:Platform=x64 /m /nologo /v:minimal均exit0，日志out\skill94-sync-Release.log、Debug.log。当前源码cl C++20编译、资源根运行以下11项均exit0：original_death_continuation_test、original_skill94_transaction_test、original_skill94_feedback_test、original_skill94_death_visual_test、original_skill94_input_evidence_test、original_skill_schedule_test、original_skill_input_evidence_test、original_skill88_positive_evidence_test、original_save_bridge_test、original_combat_test、original_combat_visual_test；日志out\*-sync94-compile.log及*-sync94.log。原事件专项original_events_test --finish-facing-only exit0。

完整original_events_test本轮执行exit10：Automatic battle cycle branch0 turn4 expected4 campaign_modal1，不能列作全绿。已用out中的隔离prologue头副本禁用本轮prepare异常包装与advance同步分支重新编译原同一测试，仍同处exit10（out\original_events_baseline.log）；确认此失败不依赖本轮同步执行路径，未放宽测试/生产门控。隔离头已清理，保留复现日志。未声称完整原事件套件通过。

未完成：41C33E type9cond0嵌套扫描及41B9A3 type4..16有副作用链、41FF6E 1/3/7原呈现；下一最小范围为独立实现41C33E每记录后41B9A3调用栈continuation，而不是预展平queue。未取得正常输入自然击杀E2E，既有角色11 MISS/角色16 HIT与两结局档均未改。94仍仅受限生产覆盖；其它93非合作技能与19单人合作原缺口保留，不称全94目标。未写README/PROGRESS。

## 94同步dispatcher：41C33E→41B9A3逐记录调用栈（本轮有界扩展）

OriginalDeathDispatch::Stack现在保存death/41FF30-repeat/41C33E-flags/41B9A3-generic原调用帧、逐记录index/返回PC/本次change返回值。每次next只执行到下一条实际对白，原输入结束后从原PC恢复；不是预展平对白队列。41C33E每个记录（含不匹配）后进入41B9A3；41B9A3成功type9/cond0处理后进入嵌套41C33E，再返回原generic索引，最后返回外层scan。依据asm47512..47750，41C33E只有合法completion置位才返回change=1，不把仅消费记录且completion0当作1。41FF30按原返回值重扫至0。依据420A3B cond0的确定分支，generic在合法trigger且flag>0时返回true；其余type4..16或非cond0 type9继续整表保守拒绝，不按未激活旗标猜安全。41FF6E completion1/3/7继续拒绝，其它无操作分支可用。

共享能力预检用副本完整运行同一Stack并验证原line资源，只保留entry索引，不保存预展开执行顺序；实际Prologue另建live stack，completion仍在death对白前、flag对白后complete。原prepare/advance分页与输入锁继续复用。advance新异常边界同时保护stack.next中的后续complete/资源异常，恢复完整events baseline（含原event bytes/flags）、清callback/stack并解锁；不返回452FB1、不发布支付。原save/input/quit门控沿用。

真实资源fixture：stage11 role147 event1 completion13、event2 type9cond0/trigger13/空行/completion11、event5 type9cond0/trigger13/三行/completion14。**整张真实stage11另有currency等未支持handler，生产仍拒绝**；隔离测试仅把其它event的type状态设0，原保留三条目录/对白/条件/旗标字节均来自原资源，不是正常E2E档。精确验证death event1先显示且flag13=1；外层到event0后的generic扫描消费空event2并置11；event5在深度5的repeat→flags→generic→flags→generic调用帧暂停（event2成功后先嵌套扫描，再进入下一generic），type仍9且14尚为0；三行完成才type0/14=1；death record仍3，整栈仅return一次。Prologue原draw/advance完成同链、取消无跳过/save锁验证；在嵌套暂停后人为破坏运行stage的隔离故障测试触发原index异常，确认flags/event状态恢复、return0且解锁。未修改任何真实验收sav或资源。

资源扫描stage1..156当前初始flags没有找到无需隔离就有多对白nested链的安全死亡目标；不可把隔离stage11当真实自然击杀。已支持原stage6/82无副作用死亡对白仍PASS；复杂stage1/15和完整stage11预检无旗标泄漏。新增能力为实际嵌套type9cond0调用栈，不是开放全部死亡脚本，普通自然击杀E2E仍未验。

验证原生命令：MSBuild S2NE.sln /p:Configuration=Release（及Debug） /p:Platform=x64 /m /nologo /v:minimal，均exit0；out\skill94-stack-Release.log、Debug.log。当前源码C++20 cl编译及资源根执行12专项exit0：original_death_continuation_test、original_skill94_transaction_test、original_skill94_feedback_test、original_skill94_death_visual_test、original_skill94_input_evidence_test、original_skill_schedule_test、original_skill_input_evidence_test、original_skill88_positive_evidence_test、original_save_bridge_test、original_combat_test、original_combat_visual_test、original_events_test --finish-facing-only；out\*-stack94-compile.log/*-stack94.log。完整events既有对照失败边界不变，本轮未重复跑全套、不声称全套通过。未写PROGRESS/README，未扩其它技能。

## 2026-11-10 本地检查副本与精简打包验收

新增 tools/publish-runtime.ps1，原生vcxproj AfterTargets=Build同步EXE及四个实际运行DLL；Debug与Release均构建exit0，最后Release覆盖根目录，SHA256与out/x64/Release一致：9EEA4EF3EDFF2187907B9439C5C4E06500149DE19285463D528AD65577B19F2E。备用CMake POST_BUILD亦配置同步，但本轮未运行CMake，不能称该入口已验证。

新增 tools/package-game.ps1，用显式运行清单而非整目录复制：EXE、SDL3/SDL3_mixer/fluidsynth/mpg123四DLL、PAL/BIN/CEL/PRG/CFG/ITEMSP/KINDSP七文件、当前main实际读取的MP3/SUPERSP_A02.mp3，以及运行时首选windows-disc1和windows-disc2各一个DAT；加run-game.cmd、PACKAGE.txt、SHA256SUMS.txt，总18个ZIP项。无个人存档、旧EXE、重复光盘、调试符号、源码/测试/分析产物、攻略或修改器。不可裁剪原索引归档内部内容，避免破坏偏移。目标Windows x64，仍需Microsoft VC++ x64系统运行库。

打包命令：powershell -NoProfile -ExecutionPolicy Bypass -File S2NE\tools\package-game.ps1。最终S2NE-game-win64.zip为220993658字节。曾发现遗漏当前引用MP3的初版包，已重打替代，最终独立解压后全部15项SHA256核对通过；包内--verify原标题与双开场解码exit0，--smoke-opening-right-skip实际SDL dummy呈现105帧exit0，混音322773个非零样本。临时解压目录已清理，未触碰原SAVE或结局证据。DLL直接依赖经dumpbin核查，包内运行未依赖工程目录。该验证不冒称完整战役重新跑通或全新机器VC运行库安装测试。

## 2026-11-10 交付更正：S2NE.exe及不同CD2 PRG

用户要求EXE与sln同名：原生TargetName改为ProjectName、备用CMake OUTPUT_NAME=S2NE，同步publish/package/run-opening/README。Debug与Release实际MSBuild均exit0，最终资源根S2NE.exe与Release SHA256同为999D1E9650A7056572B1FA2AF150A521CBF299363DD543264A07BC0AD1DC4183。历史envoy-opening文件不再发布且不入新包，未批量改写历史验证命令。

确认根SUPERSP.PRG与windows-disc1相同，SHA256 F3B3EF5587536991062879782209C7E104A19BD4DC568169C1ADAD7A4A69498A；windows-disc2不同，SHA256 C6E36E77DEC30C3FEC80E156096B8F6DFBB2DF35A97CC07FF5061815C0E9A62F。上包只带根PRG，确有遗漏，现追加cd/windows-disc2/SUPERSP/SUPERSP.PRG，CD1不再重复放cd子目录。最终ZIP248254008字节，16项运行资源加三辅助项；独立临时解压全部哈希通过，包内S2NE.exe --verify和--smoke-opening-right-skip退出0，103帧，当前音乐非零样本322773，临时目录清理。

源码核实仍有独立运行缺口：OriginalMap构造及其它PRG消费者直接读根SUPERSP.PRG，set_disc仅改disc字段和reaction缓存，不重载PRG。此次修正没有未经原证据盲改所有PRG索引，包含双PRG不代表生产已正确换PRG；原双路线记录也不能当作该项验收。后续必须独立核原PAL与CD2 PRG索引/加载调用并补回归。

## 2026-11-10 PRG用途核实及上一结论纠正

直接核原473A55 case2（473B26..473B56）：4F7D68非零用安装目录supersp.prg，为0才4737AD解析光盘路径。原42800C在69066附近清默认值、随后fscanf DEVICE首值写4F7D68；本地SUPERSP.CFG实际DEVICE=1,0,F。因此上一节将“固定读取安装PRG”定性为换盘缺陷证据不足，现明确撤回，不应盲目替换CD2 PRG。

按同一原PAL目录比较：第1组136角色记录、第2组4417动作帧、第3组4284地面图、第4组896物件图、第5组360场景图、第6组132地图、第7组属性表、第8组50背景图，共8组全部SHA256一致。第9组（原PAL offset100703123/length11086）不同，CD2余下尾部也不同；第9组用途与CD2内置布局尚未完整核实，不能称全文件只有尾部填充差异。两文件头SuperSp_P，均9组；CD1长度100714209、CD2长度103481337。PRG是原图形/地图资源归档，不是待运行的游戏EXE，也不是DAT影片/换盘资源的同义替代。当前恢复所用前8组在双盘实字节相同，故现证据不支持必须按关卡切换PRG。

## 2026-11-10 包内容精简

按用户要求，package-game.ps1不再生成或打入PACKAGE.txt、SHA256SUMS.txt，已重建资源根S2NE-game-win64.zip并检查ZIP目录：16个运行/资源文件加run-game.cmd，共17项，无上述两个TXT。其余资源与程序清单不变；上文含说明/哈希清单的包记录为历史。

## 2026-11-10 构建目录清理

检查无活跃MSBuild/cl/link/游戏进程后，仅清理可再生及明显重复产物：旧CMake build整个目录（当前原生sln不依赖它）、out/obj中间目录、out根82个obj，以及out/x64两配置旧envoy-opening产物、重复旧测试EXE/PDB和ILK。共535文件，回收554.12MiB。保留当前S2NE.exe/PDB与四运行DLL、out根专项测试EXE、全部日志/截图/序列CSV/存档及战役/技能证据；未删SAVE、源码、原资源或游戏包。

清理后资源根S2NE.exe --verify退出0，原标题与两开场解码成功。Release与根EXE哈希仍一致，Debug/Release及两结局、94实际输入与88正疗伤关键存档存在核对通过。下次构建需重建obj；旧build如需备用CMake可重新配置生成，未改IDE配置或工程源码。

## 2026-11-10 18:02 暂停补全：main职责重构／格式／无模态诊断

main拆为app_options.cpp/hpp（参数与资源根）、app_campaign.cpp/hpp（原档盘号、关间adapter和转职）、app_smoke.cpp/hpp（驱动状态与前后输入），原生vcxproj/filters与CMake同步。相同格式基线4250行→2478行，约42%；压缩旧源1467行与格式化新行数不可直接比较。agent词法对照smoke/转职提取保持等价。所有9个本次C++文件clang-format --dry-run --Werror通过，父已复核；新增format-code.ps1及VS Code C/C++保存格式设置，沿原_clang-format，不批量改无关代码。

runtime_diagnostics.hpp在main首行调用：CRT debug报告file/stderr，SetErrorMode抑制系统错误窗、abort/参数/terminate/purecall/SEH给stderr并致命退出。tests/runtime_diagnostics_test.cpp Debug独立编译，crt report退出0，assert/abort/parameter退出1，未处理C++/SEH非零，均无等待窗口且有stderr。一次Start-Process初始化abort进程返回0xC0000142无日志，直接运行与后续子进程通过；未隐瞒该系统初始化异常。诊断日志out/runtime-diagnostics-*.log；不承诺能拦截外部调试器或第三方显式MessageBox。

Release/Debug MSBuild exit0，日志out/refactor-{release,debug}-build.log。Release verify/menu/new-game/battle/shop/promotion通过；Debug verify/menu通过。Debug battle/shop触发MSVC span下标越界stderr断言；旧main独立对照同点复现（out/refactor-baseline-smoke-{battle,shop}.err与新debug对应.err），不是重构引入证据，但仍是未修缺陷，不能列全绿。断言现在终端退出，不忽略继续执行。未重跑完整战役或技能专项。

最终根Release SHA256=9DED3DCC606E3FA740AA9392EFD8C1B98AF193C7C3BC3ABB08B9E4A419BFC916，与out/x64/Release/S2NE.exe一致；父重打共享资源ZIP189423232字节，独立临时解包--verify及--smoke-opening-right-skip通过，104帧，非零混音361650；临时目录清理。包不附两个TXT。补全任务暂停，原档与旧证据未撤销。CDS仍为82映射40块去重内容，无须用户手工执行。
