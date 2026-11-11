# S2NE — 进度、缺口与验证记录

游戏介绍、开发环境及日常入口见 [README](README.md)。本文件是实现进度、验证方法、未完成项与历史证据的统一入口；原验证文档已合并删除，不再另设验证文档。

<a id="current-status"></a>
## 最新状态与证据口径（2026-11-11 文档合并）

### 直接正面计时整族（2026-11-11，本轮有界收尾）

**完成35/115，未完成80/115；本轮新增6项，不重复既有29项。** 保留035 BIN effect40/side3纠正；旧effect53误判不恢复。唯一写者本工程；未广播、未新md、未操作DOSBox。

先按BIN115逐记录及原466544 side0/1→4668F0反向`repne scasb`分派穷举，不将case号当effect：直接442CD5正面计时候选effect14/32/37/50/51/52/54/57/62/70/72/76/85/87，实际七ID。完成六项如下；**055飄浮術**同effect37→case12→466DF1/mask2，但原CEL header[6]=65534（signed -2专用序列），未沿用普通帧目录硬放行，明确保留。

| ID 原名 | group/bit；阈值；支付 | BIN→ASM oracle；mask/timer；CEL |
|---|---|---|
|037 報復蠱|3/4；560；MP18|effect62→case21→466FFD；8/88h=3；dispatch37/color245/tint50/24帧|
|044 幻術攻擊|4/1；100；MP18|effect70→case24→467085；80h/8Ch=3；44/59/25/15帧|
|046 心靈護盾|4/3；320；MP15|effect72→case25→4670A2；100h/8Dh=5；46/138/50/36帧|
|058 再生|5/5；900；MP30|effect76→case27→4670FE；200h/8Eh；duration125→470505=max(1,actor ST/25)，ST200得8；60/233/20/43帧|
|086 輕功|8/3；320；ST12|effect85→case29→46713F；400h/8Fh=4；76/233/45/53帧|
|092 無想轉生|8/9；2800；ST50+HP50|effect87→case31→467180；800h/90h=2；82/137/60/50帧；flags70不能误作只扣ST|

- 四项side1友方、两项side0沿用原收集；同公共`collect_targets→execute_pipeline→feedback→publish→capture`，没有复制支付。定值power/一次percent，原side1 hit100钳99；友方/自身支持不误套敌方4000/3E10免疫，不凭名字加入攻击、防御、伤害。状态消费者沿用既有机制，不声称本批重新认证所有后续战斗消费。原sprite零run未改。
- `original_timed_buffs_test.cpp`每ID独立原Big5/学习±1/cost/effect/mask/timer/frame oracle；穷举七候选与055排除；混合new/extend/permanent三目标唯一支付、半价/state2、免疫、取消、晚目标失败、奖励拒绝/RNG全回滚、计时到期/永久、capture→encode→decode→restore全部PASS。058仅原计时分支认证，不把本批称为全回合再生结算重验。模块日志`out\timed-buffs-test.log`。
- 最终独立包`out\timed-buffs-certified`，`timed-buffs-certified-verify.log`及37/44/46/58/86/92六SDL exit0；046使用异方位友军（非敌人）目标+timer255；其它为自身目标，086/092走第二技能专用分区，092实际ST200→150及HP937→887，不误称无支付扣血。全部目标取消无支付/RNG、原CEL、反馈暂不发布、唯一提交、save/decode/restore及500ms settled均PASS。`timed-buffs-certified-rollback.log`058拒绝奖励后目标/支付/奖励/RNG/save不变exit0。
- **CONTROLLED只作smoke**：显式注入学习/ST/半价能力/status/RNG；个人UJ02.SAV只读，不称自然习得或自然战斗。SDL dummy非桌面观感。首轮使用不支持的controlled字符串导致清空控制入口并等待，已停止该自有运行；最终用mixed。初轮独立包cwd错误导致BMP路径失败，改独立资源cwd；092初轮断言漏HP支付已改为原flags70双支付；均不拿失败日志充验收。
- 小回归重新编译`buff-reg-cast_pipeline.log`、`buff-reg-special_status.log`、`buff-reg-hostile_stat.log`全部PASS；C++23/main-only catch，clang-format，最终Debug→Release根同步/15项无SAVE精简包/独立verify完成。未完成子族仍保留：022護甲/023嗜血/028迅捷術能力增量，047隱身含4465B4/4462B6专用分支，055特殊CEL，057鼓勵，076自然祝福复合状态/能力，079時間妖魔行动重置，以及其它未逐ID分派；不扩伤害/召唤。094脚本死亡独立待验不阻塞。
- **持久SHA256**：根/Release/独立EXE一致`3ED42487EB87BDC2583C2CAE3B20FE50FFF4EFEFB3D94FD075CE5DFB9070FA9D`；ZIP `BB3BCFE305C8D36D749CB4D8F9E7C2B5B2A302DAE2FD24BABC0E1D209E2A23E4`。
- 日志均在原址`out`，最终认证日志hash如下（早期`final`/`independent`不替代`certified`）：
  - `timed-buffs-test.log`：`647CD12F971172CA853AB55A1BD3FA5479320EF3E84BBD2D3F9E9468BAE8E5D2`
  - `buff-reg-cast_pipeline.log`：`E7C1935B68523011AA08F43B139C9AE6D1629AA9D07A7074770B296ED2E60FCD`
  - `buff-reg-special_status.log`：`00A6BCCFE10AA042C364DC8BD3B547F46EC52F7C835704686EC58C7C61BE8483`
  - `buff-reg-hostile_stat.log`：`1FB27908F7C97B510603A679CB00CDD2AF8519EB7B65F1B1E6750CA7D5E09C5A`
  - `timed-buffs-certified-verify.log`：`1136ED7BA62331599D894A4F31B780DE107D4BF40F693659BA3A251682FED98D`
  - `timed-buffs-certified37.log`：`B642F98129097FF52748D9C5813DBB746AF6F0A28BAE60B1AAE9D5A0D3AF4111`
  - `timed-buffs-certified44.log`：`1137427DD3EAFB0DB3CA28B081D705698916BB68865F0E983CBD54FEFB37C886`
  - `timed-buffs-certified46.log`：`DA574FCC0521772696A4E44D717FAF06C2636050FE95815A42A6A5249A89AAA3`
  - `timed-buffs-certified58.log`：`E8F867952D0A5F43F1A62D62F4C3BA1867E7A1A5CA6A5C0A8E339A0BD4E42CE3`
  - `timed-buffs-certified86.log`：`5A267396B1C4CE5CB1DE3A5E18DE424A64B312AC232996F8079DE4CED7F4BD5D`
  - `timed-buffs-certified92.log`：`B6877E561783C3AABC2EDB71A4343C2390E62CA889E50E3C2E829A2E0716B3F1`
  - `timed-buffs-certified-rollback.log`：`78F01FE79E67EDAD72B9EC2E00F03681C3906E59CCD166C6059DE723CDFA2061`

本六项有界族完成停写；055不计完成。

### 特殊状态与清除整族（前轮七项，最终验收完成）

**完成29/115，未完成86/115；本七项7/7，既有22项不重复计数。**

本轮按BIN115记录穷举effect5/20/40/47/56/58/74，边界恰为 **008祈求、012邪惡祈求、035狂亂之蠱、039瘴氣、048解除魔法、056平靜、060石化術**。不把不同effect硬套计时处理，不按固定三项停点；专用`NativeSpecialStatus`共用`collect_targets→execute_pipeline→feedback→publish→capture`与唯一支付/奖励/RNG事务。**纠正此前035的effect53记录：BIN实际为十进制40（0x28），side3双方目标；不是十进制53。** 原`466838→468181→46844C→443093`，与`467FA2`分开。原sprite零游程未变，094脚本死亡与伤害/召唤/剧情独立留后。

| ID 原名 | group/bit；学习阈值；MP | 原效果与边界 |
|---|---|---|
|008 祈求|0/7；1700；20|effect20/side1，反向repne表真实case9→466D3A，WORD38 &=BB74、DWORD3C &=FEF00FFF再442828重建；不是case20→466FC7。友方不额外唤醒，命中仍走ST/RNG，不固定99。|
|012 邪惡祈求|1/1；100；13|effect56/side2，467625+90：先免疫再五次独立percent，依次毒400/timer68、咒10000/95、封咒4000/93、定身8000/94、眩光1000/91；duration210→470505按max(1,ST差/10)。全部已存在可延时但不冒报新状态成功；只有成功后唤醒。|
|035 狂亂之蠱|3/2；200；20|effect40/side3，46844C→443093，mask2/timer4B，WORD21=0，不凭名称创造伤害/增攻；角色1排除，命中后先唤醒再能力免疫。|
|039 瘴氣|3/6；1280；25|effect58/side3，4684C0：当前攻击减runtime C1×20/100，mask8/timer51=3/delta52；WORD21=20同时进入命中，不误用0或额外power RNG。重施只延时，不叠扣，255永久保持；允许双方。|
|048 解除魔法|4/5；900；30|effect47/side3，46847F：同方与008相同负面掩码清除；异方442DC5按3974/6F00FFF优先只清一个正面。先唤醒，绕过4000/3E10状态免疫，不把敌方当全清。|
|056 平靜|5/3；320；20|effect74/side1，4670BF：38/3C两个DWORD全清并重建，和008选择性清除区分；命中仍需ST/RNG。|
|060 石化術|5/7；1800；30|effect5/side2，467B3F→442FE2→442E96，mask200/timer63=0；203攻→102、157防→79。0不倒数、不自动到期（255也永久），清除可恢复；不把0改成1。|

- **每ID独立oracle**：`original_special_status_test.cpp`固定原Big5名字、group/bit、阈值±1、cost/effect、目标范围/收集、命中/免疫/角色、RNG顺序、半价/payment2、晚目标/奖励拒绝回滚、真实capture→encode→decode→restore、CEL帧；012独立五组件、039独立WORD21命中及短/永久重施、060零计时持久与恢复。公共机制不为七项复制。小回归`special-pipeline.log`、`special-stat.log`、`special-timed.log`重新编译PASS，旧13项状态与旧管线不退化；编辑器无错误。
- **原CEL**：008 dispatch8/color176/tint50/37帧；012 12/233/25/32帧；035 35/139/0/22帧；039 39/233/50/30帧；048 49/245/65/36帧；056 58/233/35/34帧；060 62/107/35/33帧。精确原header门禁沿用原CEL解码，不改SPRITE零run。
- **生产边界**：CONTROLLED仅smoke注入学习/ST/能力/状态/既有单位部署/RNG；个人SAVE只读，不称自然学习/自然布阵。SDL dummy仅认证原事件/绘制/反馈状态机，不等于桌面观感。修正side3选自身导致actor ST被fixture改写的初轮失败；最终选异方且048实测同一事务多目标友方清负面/敌方只清正面，反馈未发布阶段save不变，提交与settled真实save联合。初轮旧二进制日志不替代最终独立包认证。首次加强面积fixture错误地要求008窄范围容纳两名敌人，导致smoke部署失败；已限定048双阵营多目标严格面积fixture，其余窄范围只作单目标代表，不扩大原技能面积。
- **最终构建**：最后源码改动后Debug→Release根同步，`special-family-package.log`精简15项无SAVE，独立`out\special-family-independent`的`--verify`通过。一次早期Debug链接LNK1168是未结束smoke占用exe，待原批退出后重建通过，未杀个人进程。强化混合fixture后重新Release并重打包，以最终hash为准。
- **剩余**：本七项有界族不剩技能ID；友方增强等其它状态子族仍待逐ID，不能称所有状态已完成。伤害/召唤/剧情与094独立待验。不新md、不广播、不操作DOSBox。

- **最终独立SDL验收**：`special-independent8/12/35/39/48/56/60.log`七项全部exit0；原名册定位→第一技能→目标取消无支付/RNG→再确认→原CEL→反馈暂不发布→唯一提交→全部fixture目标save/decode/restore→500ms settled。半价MP分别152→142/146/142/140/137/142/137，AP106→61，XP135→141；熟练奖励使用各自group独立断言（此前错误日志打印旧group已修正）。048同事务友方清负面/敌方选择性清除/免疫绕过PASS；`special-independent-rollback.log`048拒绝奖励后全部目标/支付/熟练/XP/RNG/save不变exit0，预期错误栈不是崩溃。
- **最终持久交付SHA256**：根/Release/独立EXE一致 **37DC4ED264073AB1D8BFDF4B67E52508465479031DC54E28E04883F9D6389AAF**；`S2NE-game-win64.zip` **C0B1A263CDF771FF9D8E7FB152B9FC671FDE84FACEAAA1A19D62D60AE375A327**。最终独立verify通过，源码/模块/SDL/根/精简包口径一致，本七项有界族完成停写。
- **日志SHA256**：
  - special-family-test.log：`00A6BCCFE10AA042C364DC8BD3B547F46EC52F7C835704686EC58C7C61BE8483`
  - special-pipeline.log：`E7C1935B68523011AA08F43B139C9AE6D1629AA9D07A7074770B296ED2E60FCD`
  - special-stat.log：`1FB27908F7C97B510603A679CB00CDD2AF8519EB7B65F1B1E6750CA7D5E09C5A`
  - special-timed.log：`178ECB8ED93A8D511A62AE45E43E8D6FC36A5ADA932F27CBABB2C9BE6F5B586D`
  - special-independent-verify.log：`1136ED7BA62331599D894A4F31B780DE107D4BF40F693659BA3A251682FED98D`
  - special-independent8.log：`B51C8E1A0993C864125723728C910011F879C1638D94C19E0D7D427AE9CE9CB7`
  - special-independent12.log：`B2636A01606F0CAF3798DC12046B74CC14B55DCB7EC7028246D5F50B0AB947E4`
  - special-independent35.log：`303507F05B0AD93DB149187BDF696985B7C75349EEFE04FDEB366EB334178E1E`
  - special-independent39.log：`1FF2AA65A719F6CB36C6A1F7E74C5B5EA637F03A1F6A1E4394D871FB660E896C`
  - special-independent48.log：`7304D93AE66432A3354DB5D02EB6326208451650AD845E9FEC872B2940882B39`
  - special-independent56.log：`1B8421EFA0FCFC38A3C7289BB47924051D7BE17BAE41379EA66D1300A1E48D8B`
  - special-independent60.log：`37FDED425886876342A8BBA165FF2E3B384300C86733D6DC9E5D46D0D2A06BA5`
  - special-independent-rollback.log：`EC189946638FF9A21FDC62EFA8041D28375AA63AD1367C83F06D5270485D2F6E`

### 敌方能力减量/毒/迟缓整族收尾（2026-11-11，历史批次）

**完成22/115，未完成93/115；本族4/4，既有直接敌方定时9/9不重复计数。** 先读顶部18/115，再按原`467625→46803D`比较树及BIN115项穷举effect7/15/16/46敌方记录，恰为 **017毒擊、025遲緩術、052衰退打擊、061牽絆術**；effect7无技能记录。035狂亂之蠱effect40（旧记53已由本轮BIN纠正）/双方阵营、060石化術effect5→442FE2且duration0，独立留后，不硬套；094脚本死亡及其它伤害族未动。

- **原机制逐项而非状态位冒充**：017 group1 bit6/熟练1279/1280/1281，MP25，effect16→467E03→442F74→442E96，stat mask400/timer68=5，当前攻击/防御各减1/5，原signed取整，重复仅延时、不重扣，永久255不延。025 group2 bit4/499/500/501，MP25，effect46→467E9D，stat mask80/timer5D=3，AP上限/当前/携带分别减10/减10/钳当前；061 group5 bit8/2299/2300/2301，MP40，同处理但减12，raw18=7为**镜头可视区403717**，不是半径7或全地图；原列奇偶9/10格、36行，公共收集按行列顺序过滤合法地形/出场/敌方。052 group4 bit9/2799/2800/2801，MP45，effect15→467CFD，stat mask4000/timer7C=4，AP上限减40，攻击/防御各减1/10，当前AP不扣。原`46E858`formula16/29虽计算候选，但`46E058→46E1D0`最终返回WORD21，**不可误套随机power**；原命中46DBA9用ST差+WORD21-100再原随机/难度，非WORD23。017额外能力1000/4000在命中前排除；命中后先唤醒再状态/能力免疫。025允许角色/飞行，061 raw29=129排除movement_class!=0，017/052排除角色1。
- **共用事务**：仅扩展`NativeHostileStatus`参数及stat分支，维持`collect_targets→execute_pipeline→feedback→publish→capture`，唯一奖励/支付/RNG暂存及失败回滚。倒数复用`tick_status_durations→recalculate_status_stats`恢复原基值，补同步Unit AP上限/status39；存取档保留38..9F全状态减量/计时，非只保存mask。未复制事务，C++23/Result/main-only catch/clang-format保持。
- **独立原CEL**：017 dispatch17/color233/tint0/header31,0,0,139,221,17,0,5→31帧；025 dispatch25/223/35/header12,0,0,126,158,0,28,5→28呈现帧；052 dispatch54/233/65/header63,119,265,237,463,1,0,5→63帧；061 dispatch63/233/60/header32,65535,65535,640,480,0,0,5→32帧。面积与viewport均使用公共原动画/反馈发布，不伪造HP伤害。
- **公共模块+逐ID oracle**：`tests/original_hostile_stat_test.cpp`/CTest登记`original-hostile_stat`；`stat-family-test.log`四ID学习/固定原参数/命中RNG/免疫/角色/movement/短延长/255不重扣/到期能力恢复/晚目标与奖励拒绝回滚/cancel/半价/真实capture-decode-restore/CEL均PASS；025/061多目标混合、原收集顺序及payment2唯一AP，061独立镜头奇偶边界通过。小回归`stat-pipeline.log`及`stat-timed.log`PASS，既有9项不退化；编辑器诊断无错误。使用现有VS工程MSBuild及直接cl测试，不冒称运行CTest。
- **独立生产SDL最终证据**：`stat-independent{17,25,52,61}.log`均exit0：原名册定位→第一技能→目标→取消→再确认→原CEL→反馈未发布→一次提交→save/decode/restore全部目标→500ms settled。017/025/052/061半价MP分别152→140/140/130/132，熟练0→75、6053→6128、4077→4202、0→112；AP106→61，XP135→141。025混合新减量/能力免疫/可命中角色，061 viewport混合新减量/能力免疫/飞行排除；017/052单目标代表不同减量处理。`stat-independent-rollback.log`061奖励拒绝全部目标/支付/奖励/RNG/sav不变exit0，内部预期错误栈非崩溃。
- **批末交付**：`stat-family-debug.log`→`stat-family-release.log`，一次最终Release根同步，`stat-family-package.log`精简15项无SAVE，独立`out/stat-family-independent` verify与标题音频PASS。独立初次用错cwd导致截图相对路径不存在的失败不计认证，已在独立包cwd重跑成功；025旧邻居排除断言也已改为其原可命中语义。根/Release/独立EXE SHA256 **1564C654C5C83E259FFD4F65F27C0CE6595159B454E73FFC7C9A93E23090341C**；ZIP **BB45655F20D25A488AE49BB58EAE2FBC9A358740C1AF32674FFE5D5162853241**。
- **日志SHA256**：模块`1FB27908F7C97B510603A679CB00CDD2AF8519EB7B65F1B1E6750CA7D5E09C5A`；017`DAA4C1C802D020CDCCAA7AE61A7016A80B336F96B36914E10A86CB983400BEA1`；025`1FA85A7BEDC583B6E31218C1053652AF3EDC2E1E7AC7C552B80DE2F5E6E7A076`；052`6569CB135E63A96EBDC8DFEB73E504AC61383F32BB211D7F9A3455F527EC4307`；061`C43F242E6A7A9A9604BFD42D5D76BE40606628E0D03E68F08FA95833C3122C7C`；回滚`96C93EAE36FBE5BABC8FBFF23EB12F8E83B031BF7103775E58816884E993430F`。
- **限制与停点**：所有新技能生产证据显式CONTROLLED，只在smoke入口注入学习/ST/能力/状态/固定基值/RNG/既有敌人部署，个人SAVE仅加载，不称自然学习/自然布阵；SDL dummy验证实际事件/绘制状态机，不等于人工桌面观感。viewport只覆盖原mode7、未扩大其它技能模式。未新建md、未广播/联系agents、未操作DOSBox；本有界整族完成停写。

### 敌方直接定时状态整族收尾（2026-11-11，历史批次）

**完成18/115，未完成97/115；本族9/9。** 从磁盘保留19:44未交付改动，先补完中断Debug（`next-hostile-debug.log`只有编译警告、无成功结尾，不视作完成），未撤销旧工作。按`466544→467625→46803D`原比较树与BIN115记录穷举，直接`442CD5`零power敌方计时族恰为 **002催眠術、013衰弱術、015恐懼、033詛咒術、034定身術、038封咒、053眩光術、054冰凍術、096破魂劍法**。本批新增034/038/054/096四项，不把既有五项重计；不是按三项停点或凑满预设数量。

- **原数据/适配逐ID**：034 group3 bit1，熟练99/100/101，effect43→467E7F，mask8000/timer94=3，排除角色1，MP13（半价6），CEL34 header27/89/70/177/99/0/36/5→36呈现帧。038 group3 bit5，899/900/901，effect17→467E1C，mask4000/timer93=4，排除角色1，MP20（半价10），CEL38 header30/60/250/120/289/0/0/5。054 group5 bit1，99/100/101，effect13→467C81，mask2000/timer92=4，排除角色1/2，MP20（半价10），dispatch56/header26/0/0/125/125/0/0/4；原467C81首次后的4465B4/4462B6为图层重绘/呈现而非额外数值伤害，原CEL和原子反馈发布保持。096 group9 bit2，199/200/201，第二技能菜单，effect43同034但**不排除角色**、**支付ST15而非MP**，dispatch89/header30/0/0/111/249/0/0/5、233/35；不按剑法名称归伤害。
- **共同管线而非新事务**：`NativeHostileStatus`参数化effect/排除位/duration；002与033生产路径也迁入，033仍按原ST差/10取duration，002计时2。`collect_targets→execute_pipeline→feedback→publish→capture`与唯一支付/奖励保留；修复紧耦合的第一/第二菜单可达bit、遗留034/038/054 smoke角色group、支付/帧数及真实熟练日志，未扩无关功能。
- **明确特殊排除**：012邪惡祈求effect56不在上述直接计时比较分支，不当作55；017毒擊effect16→442F74→442E96（毒的两项能力减量及mask400），060石化術effect5（专用处理且WORD25=0），025遲緩術/061牽絆術effect46及052衰退打擊effect15含独立power/能力字段，035狂亂之蠱effect53但阵营字段不是敌方2；它们不冒充本直接计时族完成。007/011/014等独立伤害、召唤、事件及094脚本死亡继续未验，不阻塞9项直接族。
- **一次公共机制、独立数据oracle**：`timed-family-test.log`先独立穷举BIN得到九ID，再逐ID学习/原hit与RNG/唤醒先于免疫/角色排除/延长与255/到期/晚目标和奖励拒绝回滚/取消/半价及payment2/唯一支付/真实capture-decode-restore/CEL。七条独立固定数据，已完成002/033分别由`timed-sleep.log`、`timed-curse.log`独立期望回归；`timed-pipeline.log`、`timed-restoration.log`均PASS，不重跑所有旧项长SDL。
- **最终独立生产SDL**：`timed-independent-{34,38,54,96,53}.log`均exit0。真实名册定位→命令→第一/第二技能→目标→取消→再确认→原CEL→反馈未发布→原子发布→存档→500ms settled。034/038/054半价MP分别152→146/142/142，熟练0→45/62/62；096 ST1000→985、MP152不变、熟练0→50；统一AP106→61、XP135→141。053复用面积代表三个目标new/能力免疫/角色MISS，逐条反馈后统一发布；`timed-independent-rollback.log`为最终096生产奖励拒绝sav/全部目标/支付/奖励/RNG回滚exit0，内部预期错误栈不冒充运行崩溃。初次034旧帧数断言失败日志仅诊断，最终独立成功日志才是认证依据。
- **边界**：上述新项全部显式 **CONTROLLED**：只在smoke入口注入actor1学习bit及ST/能力/角色/状态/RNG，搬动既有敌人，battle-only恢复跳过故事续跑。仅写out，不改个人SAVE，不宣称自然学习或自然布阵；SDL dummy验证真实事件/绘制状态机，不等于人工桌面观感。未新建md、未联系旧agent、未操作DOSBox；本有界整族结束停写。
- **一次批末交付**：clang-format；`timed-family-debug.log`→`timed-family-release.log`成功根同步；`timed-family-package.log`精简15项无SAVE，独立`out/timed-family-independent` verify及标题音频smoke通过，编辑器诊断无错误。CMake Tools因未配置工作区无法配置，诊断为空，采用现有VS工程MSBuild验证，不把它记录为CTest成功。根/Release/独立EXE SHA256 **9F552E5ACFD35B0FBDF6E75F14F4FA81AFADE4DB62FCEBCED795F56E29F3027C**；ZIP **B3FE9599F9EEA6B01BF3ED4980256AB059BEE1589AA334054698D484C925FFD3**。
- **最终日志SHA256**：模块 `178ECB8ED93A8D511A62AE45E43E8D6FC36A5ADA932F27CBABB2C9BE6F5B586D`；独立034 `C1F4B7743549206E0E1ACB5872D7DC544F3FAF64750A232DFFE92478C1A5D038`；038 `A91F8DB7B09FEBEF64FE5867ADB4C3CC0EFB718BADE38EC93C4A16BAED5AFEDD`；054 `0098F8F5CBC020FDD6A8B841F70418EDD28EED14AC58028CE76DFD1E576853B8`；096 `F684703B6D0CF9CD3E8B81A568A83D3D99F358698E8C20F4ABD4D3FEB997287C`；面积053 `A0E2045C30D3EA351922AA2FBD5A3121145D1E3311F6E5083070A6E147C71812`；生产回滚 `AC1D368082E83C814A88DACCA257189E9507E4D3C4DA7977E0A88D971E1979DB`。

### 013/015/053敌方定时状态族（2026-11-11，历史批次）

**完成14/115，未完成101/115。** 新增013《衰弱術》、015《恐懼》、053《眩光術》；094脚本死亡联合独立缺口未动、不阻塞本族。先查原表：007《聖言》/011《邪惡之箭》是effect1伤害（formula5/7），008《祈求》effect20另有清除语义，012《邪惡祈求》effect56并非55，不能按名字或IDA case号凑族。本批只选同`466544→467625`零power敌方定时状态原处理器。

- **原地址与专用边界**：`46803D`直接比较原record19字节，69→`46801A` mask40000/timer97=4，67→`467FFC` mask80000/timer98=4，8→`467C63` mask1000/timer91=3；均调用`442CD5`原首次/延长/永久规则。`46D874→46DBA9`按ST差-100分段、`407E2D`随机/难度修正、`46D50F/46D545`按record29角色排除：013无、015角色1、053角色1/2。命中先`467804→4685CD`唤醒，再`467A42`状态/能力4000免疫；MISS不唤醒。没有伤害、治疗或伪造数值power抽数。
- **公共代码**：新`native_hostile_status.hpp`只提供上述原处理器的参数化packet，不复制支付/奖励事务。继续`collect_targets→execute_pipeline→begin_status_area_feedback→publish→capture`，原收集顺序/地形/出场/阵营、一次MP/AP、原生奖励与RNG暂存回滚共用。001等已完成11项不重复计数；formula4/6正确18h/19h与1Eh/1Fh保持。`original_sprite`原零游程规则完全未改。
- **每ID独立oracle**：`tests/original_hostile_status_test.cpp`与CTest `original-hostile_status`；学习原group1 bit2/4、group5 bit0，WORD15=20/56/0，测试199/200/201、559/560/561及0/1；053门槛为零无合法负熟练前值。各自cost15/30/15、effect69/67/8、timer97/98/91、duration4/4/3、CEL帧25/38/27独立固定期望。覆盖固定ST差独立命中/RNG、角色排除、两种免疫/先唤醒、重复延时/255、倒数到期、晚目标拒绝/奖励拒绝全回滚、cancel/settled、半价7/15/7、payment2与实际capture/decode/restore全计时/学习。日志`out/hostile-family-test.log`全PASS；小回归`hostile-regression-{pipeline,restoration}.log`全PASS。
- **真实CEL与生产SDL**：013 dispatch13/header25×69×354，015 dispatch15/header38×165×384，053 dispatch55（不是53）/header27×640×480；原调色233及tint25/60/50、锚点/逐帧呈现复用。最终独立包`hostile-independent-{13,15,53}.log`全exit0，真实右键名册定位→命令→技能→目标→取消→再确认→原CEL→反馈→一次发布→落盘/500ms settled。013 MP152→145/AP106→61/熟练0→50；015 MP152→137/熟练0→87；053三目标mixed new/能力免疫/角色MISS（空格保留），3条反馈后原子发布，MP152→145/熟练0→50；均XP135→141。`hostile53-rollback.log`生产拒绝奖励全部sav/RNG回滚exit0。最终日志的proficiency已指向真实group1/5，不采用早期错误group2显示。
- **严格受控，不冒充自然**：个人SAVE只读；UJ02是原story-stage115，原角色1未学此族。生产fixture明确注入角色1学习bit、ST/能力/status/roles/RNG，并部署/搬动两个既有敌人（053在九格内、013/015在单格外），只写out。`restore_battle(..., resume_story=false)`仅受控新族入口跳过story续跑，默认true保持普通加载行为；无stage/地图字段篡改。早期未经此控制的输入进入旅店/原worldmap断言、无自然053施法者以及错误跨stage捕获均失败，不列成功证据；本批未修改无关worldmap。最终明确CONTROLLED，SDL dummy证明事件/绘制状态机，不宣称自然战斗或桌面人工观感。
- **一次批末交付**：clang-format；`hostile-family-debug.log`→`hostile-family-release.log`成功根同步；`hostile-family-package.log`精简15项无SAVE，解压`out/hostile-family-independent`独立verify/标题音频smoke及上述三技能实际输入全通过。诊断无错误。根/Release/独立EXE SHA256 `3B782AF8705F59900CA38030CFBE885BE1C577D5A2E4364EB98A25258B84FCDA`；ZIP `F43D96AA14BF1C0BE28DA06AD59DDEA8E48B28887A44FB1DA0948D20004F5100`。
- **日志SHA256**：模块 `58D5928EBE7F6B2EB54553229027CF8CCE2477DF8AB3FF1144CD1C1F76CC9C28`；独立013 `A79C7C5132C764040BB1A936DBEA997C4333195A42976B1E27B6F9D8EF9B89CE`；015 `056E9F5568E7AC8336C23358BB224F8D53597E73D1F258D42DD3657E16EE08C5`；053 `2F82F0E07237EAF1F0CF3E48D8835E55ED7DBCE017D6B72F08D5B5F836271A92`；生产回滚 `28B38B44464F8F002EC5C1A330D6ADD946D977D6B659417B0F0BA553A007D70B`。未新md、未广播siblings、未操作DOSBox；本批完成停写。

### 006/009/010公共族收尾（2026-11-11，历史批次）

**完成11/115，未完成104/115。** 本批完成006《集體治療》、009《新生術》、010《神之干涉》；原八项共同管线不另建、不重跑八项SDL全套。094脚本死亡联合仍按下节待验，不扩011或其它技能。

- **学习门槛实核**：原`465A47→465A67→465AEE`取熟练WORD/10，与37字节record的signed WORD15比较，门槛大于熟练才跳过；三项group0 bits5/8/9、WORD15=85/214/260，因此849/850/851、2139/2140/2141、2599/2600/2601逐项验证未学/已学/已学。UJ02原角色1三项已学，只读；门槛测试是明确受控副本，不伪称自然习得过程。
- **公式修正，不沿用旧结果充数**：006原`465C14→46EBDB→470427` formula4读取signed config18h/19h，009原`46EC72` formula6读取1Eh/1Fh；发现上一实现分子/分母均早一字节，本批修正。独立测试直接按原偏移计算ST装备18加1/5、目标装备19翻倍、原随机乘数、缺血钳制及XP/熟练/RNG；默认双目标006期望260/286、XP90/熟练25，009期望469/469、XP120/熟练25。全HP只power抽数且无反馈；禁疗只percent、零治疗反馈；混合目标与缺血1均通过。
- **010不是治疗**：`467068→442CD5`置mask40/timer8B=2，首次反馈固定180但HP不变；重复静默、短计时延2、永久255不缩、倒数到期通过。`40A823..40A83E`实际受伤减180、最低1独立验证，不当作防御加180。原99命中范围没有虚构随机MISS；失败验收为MP不足、晚目标非法、奖励拒绝及全事务/RNG回滚。
- **共用边界保持**：继续复用上批`collect_targets→execute_pipeline→feedback→publish`，无新ID支付/奖励事务副本；原`464CFA`半价32/2=16、40/2=20、55/2向零=27，payment2免MP仍扣AP45；取消不动sav/RNG、反馈未发布、settled不重复支付、restore保留全部目标/学习/status/timer。模块`out/family-close-original_restoration_test.log`及旧八项共同事务`family-close-original_cast_pipeline_test.log`、相邻`family-close45.log`全PASS；原八项完成数不因重构再加。
- **生产SDL核心补缺**：`out/family-close{6,9,10}-branches.log`及最终独立`family-close-independent-{6,9,10}.log`全exit0；006五目标全HP/禁疗/其余正治疗联合，009四目标同类联合，010永久255静默/HP不变；分别38/33/59个原CEL帧，半价16/20/27。`family-close9-rollback.log`实际生产奖励拒绝后sav/RNG全回滚。首次010状态置位及普通MP/CEL采用上批已有证据复用；不重复所有无关自然流程。所有branches显式CONTROLLED，仅内存能力/HP/status/RNG副本，不改学习/坐标/个人SAVE；SDL dummy为真实事件/绘制状态机证据，不宣称人工桌面可见。
- **统一交付**：统一clang-format，`family-close-debug.log`→`family-close-release.log`成功、根同步；一次`family-close-package.log`精简15项无SAVE，解压`out/family-close-independent`，独立verify/smoke均exit0。首次从工程cwd运行独立SDL的截图路径失败，改用独立包cwd后全通过，最终日志已覆盖为成功结果。诊断检查无错误。
- **SHA256**：根/Release/独立EXE `C5B6B8E7D934C155BC7ABF59E69400686C32C12AAA1C79781522856132339684`；ZIP `F98CBE10D321D6EE9F1248E3DFEAA026E8DA4A5ED7E2584E637045BFC8362656`；模块日志 `AFCE49FA0E8BF454E57113A0B3555F77C1AF92F1F51F6C692388BF5C525E471D`；共同事务日志 `E7C1935B68523011AA08F43B139C9AE6D1629AA9D07A7074770B296ED2E60FCD`；独立006 `267CD492E0FED48B120BCCCEC6798AF353C23A01835BA714D007255A265ED9C2`、009 `BEDEC14450B2D478A80373A8FFD1B033ADA4DBF7B81D25FD0AD9F2061F9EAC78`、010 `7CFDD8FAD1B51201BF9FFC8FC7D52BE50B91366DDAF6CBDC933515FE531EFDA7`。本批未新md、未广播siblings、未操作DOSBox，完成停写。

### 原处理器公共管线迁移批（2026-11-11，历史口径）

**完成仍8/115，未完成107/115；本批重构不增加完成数。** 001治療、002催眠術、003祝福術、004魔力強化、005天神護體、032高溫氣化、033詛咒術、088療傷心法全部进入共同生产管线；006集體治療、009新生術、010神之干涉已接入并有受控生产结果，但尚未完成各自全部学习阈值/参数分支认证，仍不计完成。094脚本死亡联合保持下节具体待验，不扩范围。

| 迁移ID | 原处理器/专用适配 | 共用范围 |
|---|---|---|
| 001、088（及006、009） | `4654A6→465C14`；001 formula1 `46EB27`，088 formula41 `46F781`；006 formula4 `46EBDB`、009 formula6 `46EC72`，各自config/装备/禁疗/全HP抽数保留 | 原顺序收集、逐目标结果、XP/成功数熟练、事务/RNG/支付/发布 |
| 003、004、005（及010） | `466544→4668F0`；003 `466FAA` mask4000000/timer9F；004 `466AB5` SP增量/57/58；005 `466E0E` mask20/8A；010 `467068` mask40/8B | 同一状态循环，保留首次数字/重复静默、永久/短计时与独立power抽数 |
| 002、033 | `466544→467625`，`NativeSleep2`/`NativeEffect55`保留原命中/角色排除/唤醒/免疫/计时差异 | 与其它技能同一事务及反馈发布，非旧ID事务套壳 |
| 032 | `466544→467625` effect34装备slot/防御/消除适配 | 同一目标/事务/奖励/MP/AP/RNG/反馈/保存路径，不借物攻 |

- **已删除重复流程**：`OriginalSkills::execute_pipeline`按原Handler运行唯一staged actor/targets/RNG、奖励拒绝、`pay_staged`及一次提交；旧execute/execute_healing1/execute_sleep2/execute_blessing3/execute_strengthening_status/execute_hostile/execute_curse33仅兼容调用签名，旧计算/奖励/支付事务体已删除。生产`app_runtime`中001、088、区域状态、032/033四套独立复制/反馈/发布回调被一个`collect_targets→execute_pipeline→begin_status_area_feedback→publish→capture`取代。CEL仍是独立精确原资源描述，不强行统一公式。原`402701`mode1/2/3按1/3/5行交错顺序，逐有效地形/出场/阵营收集。`original_save_boundary_ready`同时锁技能视觉与反馈；反馈完成才发布全部目标/原生升级奖励/MP/ST/AP/RNG/存档。
- **八项等价验证**：新`tests/original_cast_pipeline_test.cpp`读取本批八项实际sav，逐ID检查共同拒绝奖励→全目标/actor/RNG不变、取消字节一致及settled无二次支付；`out/pipeline-common.log`全PASS。重编`pipeline-original_{skills,skill88,skill1,skill2,skill3,skill45,skill33_trace,skill_followup,restoration}_test.log`全PASS；额外32/88持久输入、94反馈/事务、sprite回归全PASS。032首次误用UJ02未升级源触发旧证据测试预期；已重跑原131历史强化源并恢复升级18→19证据，未修改测试断言。
- **生产SDL八项均exit0**：`out/pipeline{1,2,3,4,5,32,33,88}-sdl.log`，帧26/40/27/24/22/23/21/35。001/088读原UJ02合法移动/AI受伤续档`out/skill88-positive-input/input-source.sav`；002/004使用明确mixed fixture，003/005为原学习/坐标；032最终为只读131历史强化档；033原UJ02自然MISS。全部取消→再确认→真实绘制/反馈→支付/持久sav→500ms无重复。另`pipeline2-rollback-sdl.log`、`pipeline4-rollback-sdl.log`生产奖励拒绝、sav及RNG回滚均exit0；错误诊断栈为预期拒绝日志，不是崩溃。
- **新族受控证据，不冒充自然或完整认证**：独立包`pipeline6-independent.log`五友军正治疗、38帧/MP32；`pipeline9-independent.log`四友军正治疗、33帧/MP40；`pipeline10-independent.log`单友军mask40/timer8B=2/固定180无HP改变、59帧/MP55。仅内存HP/status/RNG受控，学习/坐标/源SAVE未注入。模块`original_restoration_test`查精确BIN尾/原CEL/two-target支付/奖励失败RNG/保存及payment2。尚缺新三项学习阈值/完整各公式独立期望值和全部分支，因此完成数不变。
- **最终一次交付**：`out/pipeline-debug.log`→`pipeline-release.log`成功且根同步；`pipeline-package.log`单次精简包15项无SAVE，解压`out/pipeline-independent`，`pipeline-independent-{verify,smoke}.log`均exit0。独立包001/004/032/088代表分别26/24/23/35帧，004四目标、088 HP814→878/ST70→45、032升级18→19；另上列新三项均通过。SDL dummy证明生产事件/绘制状态机，非人工可见桌面。
- **Hash**：根/Release/独立EXE均SHA256 `DCF1987E1FC548F5459C0CDE0BA97580EE6F21B2B8CBDF489E87ABAAA52C4BAC`；ZIP `F780542790B24ED5B18416AA5226871058AE7CA8309A583F734EA215C0E6C15C`；`pipeline-common.log` `E7C1935B68523011AA08F43B139C9AE6D1629AA9D07A7074770B296ED2E60FCD`；独立004 `1C7325941FB72DE80B79EA4F1EDEFDD39B694283F82B1562FBCAEE0CE53E3C71`；独立088 `59B827CCD1D75F5919BD3D705369C632D167FC795B87FAF3A7B10F66E33666A1`。
- **磁盘基线修复/限制**：首次Debug发现既有`inject_synchronous_death_failure_for_test`引用未声明成员，改调用既有rollback入口以消除编译阻碍，不宣称补齐094脚本SDL。生产C++已统一clang-format；任务工具拒绝子目录workspace，改原生MSBuild，无新增任务文件。未写个人SAVE、未新md、未广播siblings、未操作DOSBox；以下旧“当前最终”均为历史。

### 094《虛空劍法》收尾批（2026-11-11，历史口径）

**完成8/115；未完成107/115。** 完成001、002、003、004、005、032、033、088；**094未计完成**。本批独占源码/测试/文档；未扩006/其他技能，未写个人SAVE，未操作DOSBox，未广播siblings，收尾后停写。以下004/005为前批记录。

- **实核原ASM，不沿旧“已实现”推断**：`46618D→4662DE/468BAC`先伤害；`46630E..46635C`装备25再一次负数468BAC全复活；`46635C..4663C1`装备28同样反馈后`451D28`消耗；存活`4664D0..466508`装备29额外percent<30，再468BAC(-2×damage)；死亡`466425→452FB1→41C02A`同步事件返回后`455048(3C)`，`468FDF/469024`收initial_status六WORD、`458B03`入库cap99，奖励`469101`。本次修复原来25/28/29只有静默最终HP、缺第二段生产反馈：现两段present-gated反馈完成后HP/装备/奖励/ST/AP/inventory/RNG一次发布，复用共享sprite/原468BAC，不借物攻结算。
- **生产联合已补齐**：独立包`close94-independent-{kill,revive25,consume28,reverse29,rollback,natural}.log`全exit0。源只读`out\strengthened-flow-acceptance\run-43140-stage131-start.sav`（既有合法续档、历史强化HP30000，非原始平衡）。自然角色11已学/已出场，原坐标经SDL右键总检名册定位→命令第二技能→目标取消/再确认→SPELLSP85 15帧→HIT1→HP反馈→ST219→214/AP71→26/熟练2800→2830/XP2→3→实际sav decode/restore→500ms不重复；非自然死亡声明。
- **受控数据明确标记**：kill/25/28/29/rollback不改学习/坐标/原档，只改内存HP、ST、熟练、防御、装备、状态、固定RNG（12生成原percent表，接受前受控seed0，无live重试）。kill HP1→0、原455048十帧死亡、kills/losses各+1、initial_status物品1入库；25原物品237 HP1→5000且保留；28原物品238 HP1→5000、装备消耗且不返库存；29原物品239 HP2500→1699→3301、第二段负数1602。每项15技能帧、ST1000→995/AP71→26，一次XP。熟练60000已封顶，**不伪称这些fixture熟练增加**；自然+30另证。
- **持久与回滚**：新`tests\original_skill94_branch_evidence_test.cpp`重编通过，登记CTest `original-skill94-branch-evidence`（先生成上述独立包证据）。读取真实产物查全部六分支HP/装备/死亡计数/掉落库存/支付/XP/学习未注入、cancel字节相等、settled字节相等；反馈未发布比对**仅归一化施法者朝向/动画pose/action_state**，其余sav原字节相等，不称整个运行时画面不变。rollback生产奖励适配器拒绝，单位/库存/支付/奖励原字节及RNG seed/calls回滚；输入锁由现有反馈/death锁保持。`close94-branch-evidence.log` SHA256 `FEE8D7D9EE7A1447869B6C2E7D875C94543A1DF3B8D8EDAB0D0C729794CD5C41`。
- **脚本死亡：恢复同步入口/返回/回滚，仍有明确验收阻碍**。保留原`41C02A→41FF30→41C33E→41B9A3`调用栈及return12h；预检在副本运行，仅对可达flag/currency/predicate分支验证，遇独立未知type4..16谓词仍拒绝，**未无脑删校验**。生产入口可接收暂存HP0/status20供脚本谓词；真实脚本失败恢复事件/单位/镜头/输入锁、不调用支付回调。修复restore_battle中断同步链未清锁/回调；死亡展示以独立pose副本清20，不改逻辑尸体。`close94-original_death_continuation_test.log`验证原stage6角色104、stage82角色147完整表对话/flag13先置/只返回一次，以及受控HP0可见与恢复边界；SHA256 `677A90D6A63A169731609444262A99EBEB8CCD18ABD045692C6FC8BB32A6483C`。**这是原完整表调用回归，不是094生产SDL脚本击杀联合验收**。
- **未完成核心与安全下一步（不泛称待验）**：现只读131续档活动敌方正行数death script=0，无法在该档合法触发094脚本死亡。原stage1/15等含尚未实现独立generic谓词，按原`41B5DF`不能把WORD3当统一flag放行，继续fail-closed。下一步应取得含已学094、已部署施法者且可合法抵达有正行数死亡事件敌方的只读原档/合法续档，先用现有continuation预检输出具体event/type/condition，再沿**同一生产SDL**补事件→返回→死亡→奖励/支付→sav与脚本失败回滚；若只有未知谓词，先逐地址实现该可达谓词，不改事件表/身份/坐标冒充自然。目前不把缺此证据的094计完成。
- **相关回归与交付**：重编transaction64、resolution、feedback、visual15帧、death_visual10帧、death_continuation、death_predicate、共享88反馈全通过（`out\close94-original_*.log`）；生产C++23/Result/main-only catch，修改C++ clang-format。Debug→Release→根同步、`close94-{debug,release,package}.log`；包解到`out\close94-independent`，verify/smoke及上述六SDL分支独立运行全exit0。SDL dummy为生产事件/绘制验收，不冒称人工桌面。包原15项（含MP3子目录）不含个人SAVE；验收后生成的S2NE/out证据不是发行zip内容。
- **最终SHA256**：根/Release/独立EXE均`AA3C05EF578489FAF73DB9E2CF392F93A875E9AE2FE4D55226BD47A3993D9657`；ZIP `C8BC0EE07C92B248B1863EFDB72D37CE560A868B2E8D4B6395137AE6256CF380`。独立kill日志`80238AEEEE6CB84492492ACA9E7F8155528AD60C16658CA4D476CD3C6EF3BDE6`；25 `B304A23DFB0D7C3222AA3B9C2BDA1D1622F98828CC2D7CE50F1500129B2D8B03`；28 `91594AC635A80F66F0F90C06D0C2186D0BECBB7709D1C8E742BF234AB0E4698D`；29 `DC5E5920CB8683B3D4B6B39E764866B5CC56A25F8F0954F2C5AF1A588335969E`；rollback `4006ACE7087C05376C9F12B521608DA8DC5237D494AEEC0686F386837E16161F`。

复现：独立目录运行`S2NE.exe --smoke-skill94-input <独立ROOT> <只读131续档> accept94-kill kill`；最后参数可选revive25/consume28/reverse29/rollback，自然不传最后参数。证据目录由第四参数指定，不向SAVE写入。

### 004／005生产联合验收（2026-11-11，前批口径）

**完成8/115；未完成107/115。** 完成：001《治療》、002《催眠術》、003《祝福術》、**004《魔力強化》、005《天神護體》**、032《高溫氣化》、033《詛咒術》、088《療傷心法》。094仍已实现待验，其他106项未逐项验收。下方旧批计数仅历史；本批仅004/005，唯一源码/测试/文档写者；不改个人SAVE、不新md、不广播siblings，完成停写。

- **原BIN/ASM，不按技能相似性猜效果**：004原Big5 `C55DA44FB16AA4C6`；005 `A4D1AFABC540C5E9`，精确37字节记录尾门控。group0 bit3/4，习得熟练/10阈值32/54；原UJ02角色1天然都有。原`repne scasb`反向剩余计数分派：effect9→case3→`466AB5`，effect44→case13→`466E0E`；**IDA case标签不等于effect号**。详细原字节和地址在既有`analysis\skills-evidence.txt`。
- **004原契约**：半径3、mode2九格友军含自身。formula3=`(ST+装备18加成)*config16h/config17h+15`，再`407EE5`随机倍率config8/9、/10最小1；先power RNG后percent。状态runtime38 bit20，slot5 timer57=3、WORD58存首次随机增量、当前12A即Unit.sp增量。重复不叠加、不显示数字，仅延短计时；永久255不缩短；新状态覆盖旧计时。不是祝福bit4，也不是case9清除效果。
- **005原契约**：半径3、mode1**单格友军含自身**；不冒充九格。effect44固定反馈150，`442CD5` mask20即3C bit20、timer8A=2。原`40A622`受伤减150/最小1，**不是防御+150或补HP**。命中友军返回100钳99，每目标一个percent；004另有power draw，005无power RNG。两项record29=0无睡眠免疫/角色排除/唤醒分支。首次显示原负数反馈，HP保持不变，重复静默。
- **真实CEL/共享事务**：004 dispatch4/0/233/15，24帧97×242、锚48/242、四索引各50%混合；005 dispatch5/0/233/60，11源帧按原order呈现22步128×213、锚64/213、40ms（00450ms）。复用共享CEL/RLE/sprite，不改`original_sprite.hpp`。逐帧BMP+sequence.csv/hash；所有目标反馈结束前实际save字节不变，末尾状态/支付/奖励/RNG一次发布。紧耦合将004到期后的12A及005到期后的3C同步回Unit，防止capture写回过期值。
- **自然来源与受控分开**：最终独立包只读个人原UJ02。自然004角色1对原友方4/1/27/0四目标，24呈现/8draw，MP152→132/AP106→61/熟练2827→2889/XP135→141；自然005原来已经有护体，仅静默刷新，MP152→122/熟练→2914，22呈现/1draw，**不称自然新施加**。另显式`mixed`仅注入状态/计时、不改学习/坐标：004四目标新施加+已有短计时+永久255在同一生产SDL/串行反馈链联合验收，8draw且只付一次；005清除原状态后验新护体150数字。日志明确CONTROLLED，不能称自然。SDL右键→总检名册→定位→指令技能→目标取消→再确认。
- **回滚/保存**：两项各在同一生产路径受控奖励拒绝，全目标/MP/AP/奖励/RNG回滚，cast-before与rollback-after实际sav一致；预期诊断后PASS不是失败被隐藏。专项`tests\original_skill45_test.cpp`核习得319/320、539/540、原名/目标/公式原数值、状态首次/重复/永久/到期、半MP/payment2/自身别名/多目标一次奖励、取消/敌方/奖励拒绝/RNG回滚/005伤害减150最小1；实际独立包sav decode+restore验证004四目标/永久/12A增量、005护体、取消/全反馈未发布/500ms不重付/拒绝字节。`out\batch45-test.log`全部通过。CMake登记`original-skill45-evidence`（须先生成独立包生产证据）。
- **最小相关回归**：重编`batch45-regression3.log`（003及002持久证据）、`batch45-regression1.log`、`batch45-sprite.log`通过；生产C++23/main-only catch/Result，修改C++均clang-format。未扩006或094。
- **Debug→Release根同步/独立包**：`out\batch45-debug.log`、`batch45-release.log`、`batch45-package.log`；根/Release/独立`out\batch45-final-independent` EXE一致。独立`batch45-final-verify.log`、`batch45-final-sdl.log`、`batch45-final-{mixed4,new5,natural4,natural5,rollback4,rollback5}.log`均exit0；包15项无SAVE。边界：SDL dummy验证生产事件/绘制/反馈，不冒称桌面人工验收；未扩敌方施法。004多目标完整混合及一次支付已验，005原单格不要求伪造多目标。
- SHA256 EXE：`9BDEF8C60FBB9A59DE769DC29E01AC4665A743B123A0271B608D3B0246F2E5D8`；ZIP：`87963C5A656037876E8B1E29C5AE6CA00DE763F8FC6CB67AD15F7DFFCDF4378A`。

复现：在独立目录运行，ROOT为此独立目录绝对路径，SOURCE为原UJ02只读路径；自然不加最后参数，拒绝改为rollback。
```powershell
.\S2NE.exe --verify
.\S2NE.exe --smoke
.\S2NE.exe --smoke-skill4-input $ROOT $SOURCE skill4-mixed mixed
.\S2NE.exe --smoke-skill5-input $ROOT $SOURCE skill5-new mixed
.\S2NE.exe --smoke-skill4-input $ROOT $SOURCE skill4-rollback rollback
.\S2NE.exe --smoke-skill5-input $ROOT $SOURCE skill5-rollback rollback
```

### 002／003生产联合验收补齐（2026-11-11，当前最终口径）

**完成6/115；未完成109/115。** 完成：001《治療》、002《催眠術》、003《祝福術》、032《高溫氣化》、033《詛咒術》、088《療傷心法》。094已实现待验，其余108项未逐项验收。此前002单目标生产覆盖不足，继续验收时先退回4/115；本次九格生产混合及回滚补齐后才重新计入，并新增003。不扩004、不改个人SAVE、不操作DOSBox、不广播siblings。

- **002生产九格联合（显式受控）**：读取原UJ02后，专用smoke选项在内存激活/移动原敌方槽，设置HP/ST/role/能力免疫/seed1，明确打印CONTROLLED，绝不称自然。范围中心16,40，九格按原顺序收集3目标：20能力免疫（hit反馈但不睡眠）、39命中睡眠96h=2、200角色限制MISS；其余空格/其他阵营不处理。5次暂存RNG；40帧CEL→三目标串行反馈→统一发布。逐帧存档断言反馈未全部呈现时所有状态字节不变；最后MP18/AP45/熟练57/XP6各一次；500ms不重付；落盘恢复后39睡眠、20/200不睡眠通过。
- **002错误与取消**：同一生产区域输入路径，在奖励适配器处受控拒绝；动画后收集/解析所有目标，但状态、MP/AP、奖励和RNG均回滚，before/after实际sav字节一致。正常目标取消另有字节断言。受控注入只存在`--smoke-skill2-input ... <folder> mixed|rollback`，非普通游戏规则。多目标原来未验的限制现已消除；dummy桌面边界仍保留。
- **003原契约**：原名Big5 AF AC BA D6 B3 4E，精确BIN2尾24字节门控；MP12/半价6、半径4、targetmode2九格、side1友方（含自身）、effect19/formula2/WORD21=10/WORD23=100/WORD25=4。`466544→4668F0→466FAA→442CD5`设置4000000h，即3F bit4、timer9F=4。先判命中；友军同军`46D9ED`返回100，46D848钳99，不消耗power倍率随机，仅每目标percent一次。公式2计算后effect19由46E1D0返回固定10，无额外随机；不是治療公式、无敌方睡眠免疫分支。已有状态仅延长较短计时，不缩短、不覆盖永久255；不新增显示数字；新状态468BAC(-10,arg0=0)显示10但**不补HP**。本次复用有符号数字反馈并保持HP前后相同，不冒充002闪烁。
- **003真实CEL**：dispatch visual3/color233/40，CEL1第四项，27源帧，161×401，锚81/390；6独立混合索引175..170百分比30/45/60/75/85/95；实际27次呈现及逐帧hash。复用既有统一解码/精灵及状态事务。
- **003自然生产**：原只读UJ02角色1天然learned[0] bit4；SDL右键/名册定位→指令→技能→取消→再次确认；中心16,41，原友方4、1、27、0四目标，4次暂存percent，27帧→逐目标数字反馈→一次发布，MP152→140/AP106→61/熟练2827→2869/XP135→141，HP不变。四目标状态/计时落盘恢复、取消字节、500ms无重付通过。另运行同一自然来源但显式受控奖励拒绝（不改角色），四目标及RNG/支付/奖励全部回滚。
- **专项与回归**：`out\batch3-test2.log`、`batch3-test3.log`；003专项覆盖自身别名与其他友方批量合并、永久/重复/倒数过期、半MP/payment2、敌方/奖励拒绝回滚、实际CEL；独立包002混合及002/003错误sav再decode/restore验证。`batch3-original_skill1_test.log`、`batch3-original_skill33_trace_test.log`、`batch3-original_sprite_test.log`通过。新增CMake `original-skill3-evidence`，生产仍C++23/main-only catch/Result；本批修改C++格式化。
- **最终统一交付**：`batch3-debug.log`→`batch3-release.log`→`batch3-package.log`，根EXE为Release；包独立解压`out\batch3-final-independent`。`batch3-final-verify.log`、`batch3-final-sdl.log`、`batch3-final-skill2-controlled-area.log`、`batch3-final-skill2-controlled-rollback.log`、`batch3-final-skill3-input.log`、`batch3-final-skill3-rollback.log`全部exit0。回滚日志里的Sleep2/Blessing3 invalid request/award错误是**故意注入拒绝的预期诊断**，后接PASS，不隐瞒或当自然成功。
- SHA256根及独立EXE一致：`D2F0A337A2A41DFC6800818E8FD97478C41CDE3377AD277A67DFE4574D7439DF`；包`S2NE-game-win64.zip`：`4AB56BCD2E932DAA117513CF875AECC737130F99C836D668076E634F9D0E6D73`。15项运行资源布局，无SAVE。边界：SDL dummy非桌面人工验收；002多目标是显式受控，003主成功路径为自然原档；无敌方施法拓展。完成停写。

复现（在最终独立解压目录运行，ROOT为该目录绝对路径，SOURCE为原UJ02只读路径）：
```powershell
.\S2NE.exe --verify
.\S2NE.exe --smoke
.\S2NE.exe --smoke-skill2-input $ROOT $SOURCE skill2-controlled-area mixed
.\S2NE.exe --smoke-skill2-input $ROOT $SOURCE skill2-controlled-rollback rollback
.\S2NE.exe --smoke-skill3-input $ROOT $SOURCE skill3-input
.\S2NE.exe --smoke-skill3-input $ROOT $SOURCE skill3-rollback rollback
```

### 最新批次002／003（2026-11-11，覆盖下方旧计数）

**总115：逐项验收完成5；未完成验收110。** 完成ID：**001《治療》、002《催眠術》、032《高溫氣化》、033《詛咒術》、088《療傷心法》**。**003《祝福術》未完成验收**；094仍已实现待验，其他108项未逐项验收；004以后未扩项。本批唯一写入者，未改个人SAVE、未操作DOSBox、未向旧agents广播。

- 002按原名Big5 `B6 CA AF 76 B3 4E`、BIN2精确24字节尾契约启用，不按effect猜名。ASM `465342→40244E` **半径3、九格交错区域**，支持含敌人的空中心，按原顺序过滤地形与目标；不是把targetmode2冒充单体。`466544→467625→467FDE→442CD5`状态20000h、计时96h=2；命中先唤醒再判免疫，因此永久睡眠也先清后重置2。runtime2F=1先置chance0且不取power RNG；原47328B允许0，**roll0仍可命中chance0**，不得擅自变成必免。命中、失手、状态免疫/能力免疫、计时倒数/过期均有测试。
- 原动画BIN10 visual2→CEL1第三项，16源帧、40次有序呈现，167×167，锚84/100，4独立blend百分比、背景59/25；顺序含重复及逆序，未截成16帧。复用既有CEL/RLE/调色路径及统一sprite，`original_sprite.hpp`未改。
- **自然来源**只读原`SAVE\UJ02.SAV`，角色1天然learned[0]含bit1；生产SDL事件经右键→总检→定位→命令→技能→目标→取消→再施放，40帧逐帧hash，MP152→134/AP106→61/熟练2827→2884/XP135→141，目标HP不变，反馈完成后一次发布及实际save落盘decode/load、settled字节和500ms无重复支付通过。最终自然日志为chance0/roll0 HIT；此前两次自然MISS同样保留，不筛种子、不注入后称自然：`out\batch2-natural.log`。
- 受控专项`tests\original_skill2_test.cpp`／`out\batch2-test.log`：两目标HIT/免疫顺序、role排除RNG次数、奖励回调变更后失败/资源/条件错误回滚、取消、半MP/重复AP/payment2、永久睡眠重施/倒数/存取档、多目标成功flash+MISS串联且全部完成前不发布。受控正例**不是自然来源**。曾尝试独立受控SDL正例因名册入口超时未成功，失败保留`batch2-controlled-sdl.log`，**不作为通过证据**；未保留注入生产选项。自然SDL末次HIT已覆盖实际成功反馈与状态保存；多目标组合仍为受控反馈测试，dummy不声称桌面人工验收。
- **003明确待验，未接半成品执行器**：核原名`AF AC BA D6 B3 4E`、MP12/半径4/targetmode2/effect19/formula2/WORD21=10/命中100/WORD25=4/友军；ASM走`466544→4668F0`的case19状态4000000h，不借用002/001公式。其完整状态属性契约、动画、结算及SDL闭环留下一批。详见已有`analysis\skills-evidence.txt`新增段，无新md。
- 最小回归：`batch2-original_skill1_test.log`、`batch2-original_skill33_trace_test.log`、`batch2-original_sprite_test.log`通过；C++23/main-only catch/Result及clang-format。最终Debug→Release、根同步/包哈希及独立验证见本段后续记录。

- **最终交付已验证**：`out\batch2-debug.log` Debug通过→`out\batch2-release.log` Release通过→`batch2-package.log`根同步打包；全部本批C++ clang-format dry-run通过。独立解压`out\batch2-independent`，`batch2-independent-verify.log`、`batch2-independent-sdl.log`和`batch2-independent-skill2.log`均exit0；独立002自然原UJ02再次完成40帧、取消、支付奖励/保存/500ms稳定（本次合法MISS，roll25）。包15项（含MP3目录），无SAVE，无个人档。独立验证仅SDL dummy，不等于桌面人工验收。
- 自然HIT落盘正例现由`original_skill2_test.cpp`显式断言角色39 `status3E&2`、`timer96h=2`，保存后恢复通过；证据固定在原`out\skill2-input`，独立运行另写解压目录证据，无覆盖。测试重新编译及执行通过。
- SHA256：根Release及独立`S2NE.exe`一致 **`4D07D25FC8816672F35AFBCDC3B5EA5C904BDCA50344C37236227432B73F758E`**；`S2NE-game-win64.zip` **`6DFCF39DAB39A9C977ABAAEFCC9C228FD036E88022C6967345E809CE5C4F5B01`**。完成即停写，未继续004或003实现。

### 115技能逐ID验收表（当前完成35／未完成80）

**当前总115：逐项验收完成35；未完成验收80。** 以顶部最新35项及下表为准；094独立待验不计完成。历史批次数字仅代表当时边界。

- 先读并保留共享sprite交付；`original_sprite.hpp`未修改。最终真实25 CEL×双模式、literal0/零游程/opaque及回滚/SDL像素回归仍通过：`out\batch-original_sprite_test.log`。
- 名字按原ASM `426598/464767/464900`加载路径对应PAL856→BIN2组目录与37字节记录+2的Big5原字节校验；五项并未根据effect猜名。`original_skill1_test`直接比较001/032/033/088/094原字节；原表115项名字沿用下表。
- **032完成**：既有合法习得131角色1原档复核（源含历史强化值，HP30000在原生升级重算为5000，不称原始平衡）；生产SDL右键→总检→定位→命令→技能→目标→取消/再确认，23帧、MP40/AP45、原生升级与熟练/XP、存档decode/load和500ms无重复支付通过。修复当前dummy下旧边滚定位超时：改用生产名册定位及真实地形pick_surface点击；未改角色位置。补原effect34装备消除受控64固定种子正例，不伪称本次自然命中。日志`batch-current-32`、`batch-original_skill_input_evidence_test`、`batch-original_skill_visual_test`、`batch-original_skill_followup_test`、`batch-skill1-test`。
- **033完成**：原UJ02角色4天然learned[3]=1023；21帧，MP200→193/AP102→57/熟练2837→2867/XP25→31，目标273HP不变，自然MISS；专项受控命中验证睡眠清除、咒状态/95h计时、免疫/永久计时/失败RNG回滚及save恢复。日志`batch-current-33`、`batch-original_skill33_trace_test`。自然MISS不是缺实现，不要求筛自然命中。
- **088完成**：有效证据仍是`original_skill88_positive_evidence_test-94-regression.log`，本批重新编译复核同链`batch-original_skill88_positive_evidence_test.log`；原UJ02角色3正常移动/AI受伤814HP后保存的合法续档，本批SDL35帧正治疗814→878、ST70→45/AP88→43/熟练3635→3655/XP0→12，保存与取消一致。不是失败的旧skill88-positive-input.log。紧耦合修复：治疗效果现在经468BAC负数/逐帧HP反馈后一次发布，而非动画结束立即静默结算；资源/RNG/奖励都暂存至反馈完成。日志`batch-current-88`、`batch-original_skill88_test`。
- **094已实现待验，未计完成**：重编译算式、结算、事务、15帧、反馈、持久MISS/HIT来源证据全部通过（`out\batch-original_skill94_{input_evidence,formula,resolution,transaction,feedback,death_visual,visual}_test.log`，前者实际文件名`batch-original_skill94_input_evidence_test.log`）。普通存活HIT/MISS、ST5/AP45/取消/恢复已有证据；死亡+loot、复活25/消耗28/反转29仅固定种子受控事务，死亡动画仅fixture挂接，尚缺真实生产SDL中这些分支反馈与发布/存取档联合验收。带脚本死亡仍fail-closed，明确未完成而不无限扩项；不把fixture冒充自然死亡。
- **新001《治療》完成**：选择与088同effect18的最近原链，按`4654A6→465C14→46D431/46D496→46E858 case1(46EB27)→470427`实现单友军/自身半径5治疗，ST/config11h/12h+60、装备18/19修正、诅咒抑制、原命中及XP/熟练/AP/MP规则；不泛化所有effect18。BIN10 visual1→CEL1第2项26帧132×148，原mode1/零背景变换保留。真实来源UJ02合法续档角色1→AI受伤角色3，未注入学习/属性/坐标/物品；SDL事件链26帧，MP152→144/AP91→46/熟练2827→2847/XP135→147，目标正治疗，负数HP反馈后原子发布。取消/资源/越界/奖励失败不付费、不推进RNG；save实际落盘decode/load、settled字节一致；诅咒/失败数值仅受控测试。`out\batch-current-1.log`、`batch-skill1-test.log`、`tests\original_skill1_test.cpp`，CTest登记`original-skill1-evidence`（需生产输入生成证据目录）。
- 032也补接同原状态技能成功flash/MISS反馈后再发布；33与32共用反馈回调保留各自effect，不借用物攻。88/001共用解码器仅精确双布局，继续使用Result；生产catch仍只在main，C++23/clang-format。最小相关四候选/新项/技能学习/像素测试均重新编译通过；旧宽stage60测试含无关结局且历史hit/miss失败，不当成本批通过证据。
- **最终Debug→Release及根同步成功**：`out\batch-{debug,release}.log`。包`S2NE-game-win64.zip` SHA256 **938B1E19A642298B35FA3F10A463A2EEDFA4B8D5E3F8976B2E9F59B20E3884D3**；根/Release/独立解压EXE一致 **DBC4D90DEADA61188FCF10411186499CC62CF19FB421E9E2EE1840D3FEABF464**。包15项无SAVE；`out\batch-independent`独立`--verify`、SDL标题音频85帧和001完整生产输入26帧均通过：`batch-package{,-verify,-sdl,-skill1}.log`。独立技能输入读外部原始续档，只写独立目录S2NE/out，不改个人SAVE。
- 限制：本批SDL为dummy，证明实际事件/生产绘制/反馈状态机，不声称可见桌面人工验收。受控数值分支已标注，不强求所有分支自然出现才计实现闭环。首次旧32定位超时及打包受运行中根EXE占用失败已保留日志/顺序修正，最终结果以上述成功日志为准。

| ID | ASM/BIN原名 | 当前状态 | 具体缺项/边界 |
|---|---|---|---|
| 001 | 治療 | 逐项验收完成 | 本批闭环；分支受控证据见下 |
| 002 | 催眠術 | 逐项验收完成 | 九格生产混合HIT/免疫/MISS/空格、40呈现、一次结算、回滚及存取档已补齐；显式受控见顶部 |
| 003 | 祝福術 | 逐项验收完成 | 原ASM4000000/timer9F、友方含自身九格、CEL27、自然四目标SDL/反馈/一次结算/存取档及受控拒绝回滚通过 |
| 004 | 魔力強化 | 逐项验收完成 | 原九格友军38:20/57/58/12A；CEL24；生产新/短计时/永久四目标混合及一次支付/全反馈原子发布/回滚/重载通过；受控标明 |
| 005 | 天神護體 | 逐项验收完成 | 原单格友军3C:20/8A=2、减伤150最小1；CEL11源帧22呈现；自然刷新/受控新状态及回滚/一次支付/重载通过 |
| 006 | 集體治療 | 逐项验收完成 | 公共治疗管线；850门槛前/等/后、原formula4独立期望/装备18/19、38帧、五目标混合反馈/半价16、回滚/存取档及独立SDL通过；受控非自然分支见顶部 |
| 007 | 聖言 | 未完成验收 | 未逐ID核实习得→目标/原效果→动画/消耗/反馈→回滚/存取档→SDL输入；不等于完全未实现 |
| 008 | 祈求 | 逐项验收完成 | 本轮特殊/清除七项族：原BIN/学习/目标/专用效果与恢复/命中免疫/RNG/支付奖励/CEL、独立模块、生产SDL及save回滚联合通过；详见顶部最终证据 |
| 009 | 新生術 | 逐项验收完成 | 公共治疗管线；2140门槛前/等/后、原formula6独立期望/装备18/19、33帧、四目标混合反馈/半价20、生产奖励拒绝回滚/存取档及独立SDL通过；受控见顶部 |
| 010 | 神之干涉 | 逐项验收完成 | 公共状态管线；2600门槛前/等/后、mask40/8B计时2及永久255、固定180反馈/真实减伤最低1、59帧、奇数半价27、回滚/存取档及独立SDL通过；受控见顶部 |
| 011 | 邪惡之箭 | 未完成验收 | 未逐ID核实习得→目标/原效果→动画/消耗/反馈→回滚/存取档→SDL输入；不等于完全未实现 |
| 012 | 邪惡祈求 | 逐项验收完成 | 本轮特殊/清除七项族：原BIN/学习/目标/专用效果与恢复/命中免疫/RNG/支付奖励/CEL、独立模块、生产SDL及save回滚联合通过；详见顶部最终证据 |
| 013 | 衰弱術 | 逐项验收完成 | effect69→46801A mask40000/timer97=4；学习200；CEL13/25帧；独立oracle及受控生产输入/支付/恢复通过；明确CONTROLLED，非自然学习/战斗来源 |
| 014 | 亡靈之觸 | 未完成验收 | 未逐ID核实习得→目标/原效果→动画/消耗/反馈→回滚/存取档→SDL输入；不等于完全未实现 |
| 015 | 恐懼 | 逐项验收完成 | effect67→467FFC mask80000/timer98=4；学习560；角色1排除；CEL15/38帧；独立oracle及受控生产输入通过；明确CONTROLLED，非自然学习/战斗来源 |
| 016 | 邪月氣流 | 未完成验收 | 未逐ID核实习得→目标/原效果→动画/消耗/反馈→回滚/存取档→SDL输入；不等于完全未实现 |
| 017 | 毒擊 | 已完成（CONTROLLED） | effect16→442F74/442E96，mask400/timer68=5，两项能力各减1/5；学习/独立oracle/原CEL/生产SDL/到期恢复/回滚/save已验 |
| 018 | 吸血大法 | 未完成验收 | 未逐ID核实习得→目标/原效果→动画/消耗/反馈→回滚/存取档→SDL输入；不等于完全未实现 |
| 019 | 黑暗漩渦 | 未完成验收 | 未逐ID核实习得→目标/原效果→动画/消耗/反馈→回滚/存取档→SDL输入；不等于完全未实现 |
| 020 | 魔神怒吼 | 未完成验收 | 未逐ID核实习得→目标/原效果→动画/消耗/反馈→回滚/存取档→SDL输入；不等于完全未实现 |
| 021 | 冰錐術 | 未完成验收 | 未逐ID核实习得→目标/原效果→动画/消耗/反馈→回滚/存取档→SDL输入；不等于完全未实现 |
| 022 | 護甲 | 未完成验收 | 未逐ID核实习得→目标/原效果→动画/消耗/反馈→回滚/存取档→SDL输入；不等于完全未实现 |
| 023 | 嗜血 | 未完成验收 | 未逐ID核实习得→目标/原效果→动画/消耗/反馈→回滚/存取档→SDL输入；不等于完全未实现 |
| 024 | 閃電術 | 未完成验收 | 未逐ID核实习得→目标/原效果→动画/消耗/反馈→回滚/存取档→SDL输入；不等于完全未实现 |
| 025 | 遲緩術 | 已完成（CONTROLLED） | effect46→467E9D，mask80/timer5D=3，AP上限/当前减10及携带钳制；面积mixed/逐ID oracle/生产SDL/恢复/回滚/save已验 |
| 026 | 地震 | 未完成验收 | 未逐ID核实习得→目标/原效果→动画/消耗/反馈→回滚/存取档→SDL输入；不等于完全未实现 |
| 027 | 火球術 | 未完成验收 | 未逐ID核实习得→目标/原效果→动画/消耗/反馈→回滚/存取档→SDL输入；不等于完全未实现 |
| 028 | 迅捷術 | 未完成验收 | 未逐ID核实习得→目标/原效果→动画/消耗/反馈→回滚/存取档→SDL输入；不等于完全未实现 |
| 029 | 裂地術 | 未完成验收 | 未逐ID核实习得→目标/原效果→动画/消耗/反馈→回滚/存取档→SDL输入；不等于完全未实现 |
| 030 | 閃電光束 | 未完成验收 | 未逐ID核实习得→目标/原效果→动画/消耗/反馈→回滚/存取档→SDL输入；不等于完全未实现 |
| 031 | 寒冰風暴 | 未完成验收 | 未逐ID核实习得→目标/原效果→动画/消耗/反馈→回滚/存取档→SDL输入；不等于完全未实现 |
| 032 | 高溫氣化 | 逐项验收完成 | 本批闭环；分支受控证据见下 |
| 033 | 詛咒術 | 逐项验收完成 | 本批闭环；分支受控证据见下 |
| 034 | 定身術 | 逐项验收完成 | effect43/mask8000/timer94=3；学习100、角色1排除、CEL34/36帧、MP13半价6；独立oracle/生产SDL/恢复通过；CONTROLLED |
| 035 | 狂亂之蠱 | 逐项验收完成 | 本轮特殊/清除七项族：原BIN/学习/目标/专用效果与恢复/命中免疫/RNG/支付奖励/CEL、独立模块、生产SDL及save回滚联合通过；详见顶部最终证据 |
| 036 | 金蠶蠱 | 未完成验收 | 未逐ID核实习得→目标/原效果→动画/消耗/反馈→回滚/存取档→SDL输入；不等于完全未实现 |
| 037 | 報復蠱 | 已完成（CONTROLLED） | 原BIN/ASM独立oracle、mask/计时/延期/永久/到期、学习/CEL、公共事务/取消/回滚/save及独立包SDL通过；见顶部直接正面计时族 |
| 038 | 封咒 | 逐项验收完成 | effect17/mask4000/timer93=4；学习900、角色1排除、CEL38/30帧、MP20半价10；独立oracle/生产SDL/恢复通过；CONTROLLED |
| 039 | 瘴氣 | 逐项验收完成 | 本轮特殊/清除七项族：原BIN/学习/目标/专用效果与恢复/命中免疫/RNG/支付奖励/CEL、独立模块、生产SDL及save回滚联合通过；详见顶部最终证据 |
| 040 | 傀儡血咒 | 未完成验收 | 未逐ID核实习得→目标/原效果→动画/消耗/反馈→回滚/存取档→SDL输入；不等于完全未实现 |
| 041 | 詛咒之風 | 未完成验收 | 未逐ID核实习得→目标/原效果→动画/消耗/反馈→回滚/存取档→SDL输入；不等于完全未实现 |
| 042 | 妖靈附體 | 未完成验收 | 未逐ID核实习得→目标/原效果→动画/消耗/反馈→回滚/存取档→SDL输入；不等于完全未实现 |
| 043 | 幻之箭 | 未完成验收 | 未逐ID核实习得→目标/原效果→动画/消耗/反馈→回滚/存取档→SDL输入；不等于完全未实现 |
| 044 | 幻術攻擊 | 已完成（CONTROLLED） | 原BIN/ASM独立oracle、mask/计时/延期/永久/到期、学习/CEL、公共事务/取消/回滚/save及独立包SDL通过；见顶部直接正面计时族 |
| 045 | 靈視術 | 未完成验收 | 未逐ID核实习得→目标/原效果→动画/消耗/反馈→回滚/存取档→SDL输入；不等于完全未实现 |
| 046 | 心靈護盾 | 已完成（CONTROLLED） | 原BIN/ASM独立oracle、mask/计时/延期/永久/到期、学习/CEL、公共事务/取消/回滚/save及独立包SDL通过；见顶部直接正面计时族 |
| 047 | 隱身 | 未完成验收 | 未逐ID核实习得→目标/原效果→动画/消耗/反馈→回滚/存取档→SDL输入；不等于完全未实现 |
| 048 | 解除魔法 | 逐项验收完成 | 本轮特殊/清除七项族：原BIN/学习/目标/专用效果与恢复/命中免疫/RNG/支付奖励/CEL、独立模块、生产SDL及save回滚联合通过；详见顶部最终证据 |
| 049 | 心靈震爆 | 未完成验收 | 未逐ID核实习得→目标/原效果→动画/消耗/反馈→回滚/存取档→SDL输入；不等于完全未实现 |
| 050 | 真實之眼 | 未完成验收 | 未逐ID核实习得→目标/原效果→动画/消耗/反馈→回滚/存取档→SDL输入；不等于完全未实现 |
| 051 | 傳送術 | 未完成验收 | 未逐ID核实习得→目标/原效果→动画/消耗/反馈→回滚/存取档→SDL输入；不等于完全未实现 |
| 052 | 衰退打擊 | 已完成（CONTROLLED） | effect15→467CFD，mask4000/timer7C=4，AP上限减40及攻防各减1/10；独立oracle/原CEL/生产SDL/恢复/回滚/save已验 |
| 053 | 眩光術 | 逐项验收完成 | effect8→467C63 mask1000/timer91=3；学习0；角色1/2排除；visual55/27帧；三敌人HIT/免疫/MISS生产联合/回滚/恢复通过；明确CONTROLLED，非自然学习/战斗来源 |
| 054 | 冰凍術 | 逐项验收完成 | effect13/mask2000/timer92=4；学习100、角色1/2排除、dispatch56/26帧、MP20半价10；独立oracle/生产SDL/恢复通过；CONTROLLED |
| 055 | 飄浮術 | 未完成验收 | effect37→466DF1/mask2已定位；CEL header[6]=65534 signed -2专用序列未认证，保持关闭，不计完成 |
| 056 | 平靜 | 逐项验收完成 | 本轮特殊/清除七项族：原BIN/学习/目标/专用效果与恢复/命中免疫/RNG/支付奖励/CEL、独立模块、生产SDL及save回滚联合通过；详见顶部最终证据 |
| 057 | 鼓勵 | 未完成验收 | 未逐ID核实习得→目标/原效果→动画/消耗/反馈→回滚/存取档→SDL输入；不等于完全未实现 |
| 058 | 再生 | 已完成（CONTROLLED） | 原BIN/ASM独立oracle、mask/计时/延期/永久/到期、学习/CEL、公共事务/取消/回滚/save及独立包SDL通过；见顶部直接正面计时族 |
| 059 | 連環閃電 | 未完成验收 | 未逐ID核实习得→目标/原效果→动画/消耗/反馈→回滚/存取档→SDL输入；不等于完全未实现 |
| 060 | 石化術 | 逐项验收完成 | 本轮特殊/清除七项族：原BIN/学习/目标/专用效果与恢复/命中免疫/RNG/支付奖励/CEL、独立模块、生产SDL及save回滚联合通过；详见顶部最终证据 |
| 061 | 牽絆術 | 已完成（CONTROLLED） | effect46→467E9D，AP减12，mode7原镜头403717非全图；飞行排除/mixed/独立oracle/原CEL/生产SDL/恢复/回滚/save已验 |
| 062 | 譴責 | 未完成验收 | 未逐ID核实习得→目标/原效果→动画/消耗/反馈→回滚/存取档→SDL输入；不等于完全未实现 |
| 063 | 生命力量 | 未完成验收 | 未逐ID核实习得→目标/原效果→动画/消耗/反馈→回滚/存取档→SDL输入；不等于完全未实现 |
| 064 | 地元素 | 未完成验收 | 未逐ID核实习得→目标/原效果→动画/消耗/反馈→回滚/存取档→SDL输入；不等于完全未实现 |
| 065 | 火元素 | 未完成验收 | 未逐ID核实习得→目标/原效果→动画/消耗/反馈→回滚/存取档→SDL输入；不等于完全未实现 |
| 066 | 風元素 | 未完成验收 | 未逐ID核实习得→目标/原效果→动画/消耗/反馈→回滚/存取档→SDL输入；不等于完全未实现 |
| 067 | 水元素 | 未完成验收 | 未逐ID核实习得→目标/原效果→动画/消耗/反馈→回滚/存取档→SDL输入；不等于完全未实现 |
| 068 | 天使 | 未完成验收 | 未逐ID核实习得→目标/原效果→动画/消耗/反馈→回滚/存取档→SDL输入；不等于完全未实现 |
| 069 | 惡魔 | 未完成验收 | 未逐ID核实习得→目标/原效果→动画/消耗/反馈→回滚/存取档→SDL输入；不等于完全未实现 |
| 070 | 巨神 | 未完成验收 | 未逐ID核实习得→目标/原效果→动画/消耗/反馈→回滚/存取档→SDL输入；不等于完全未实现 |
| 071 | 吸血鬼 | 未完成验收 | 未逐ID核实习得→目标/原效果→动画/消耗/反馈→回滚/存取档→SDL输入；不等于完全未实现 |
| 072 | 異界獸 | 未完成验收 | 未逐ID核实习得→目标/原效果→动画/消耗/反馈→回滚/存取档→SDL输入；不等于完全未实现 |
| 073 | 女妖 | 未完成验收 | 未逐ID核实习得→目标/原效果→动画/消耗/反馈→回滚/存取档→SDL输入；不等于完全未实现 |
| 074 | 龍 | 未完成验收 | 未逐ID核实习得→目标/原效果→动画/消耗/反馈→回滚/存取档→SDL输入；不等于完全未实现 |
| 075 | 破壞光束 | 未完成验收 | 未逐ID核实习得→目标/原效果→动画/消耗/反馈→回滚/存取档→SDL输入；不等于完全未实现 |
| 076 | 自然祝福 | 未完成验收 | 未逐ID核实习得→目标/原效果→动画/消耗/反馈→回滚/存取档→SDL输入；不等于完全未实现 |
| 077 | 煉獄之火 | 未完成验收 | 未逐ID核实习得→目标/原效果→动画/消耗/反馈→回滚/存取档→SDL输入；不等于完全未实现 |
| 078 | 天降甘霖 | 未完成验收 | 未逐ID核实习得→目标/原效果→动画/消耗/反馈→回滚/存取档→SDL输入；不等于完全未实现 |
| 079 | 時間妖魔 | 未完成验收 | 未逐ID核实习得→目标/原效果→动画/消耗/反馈→回滚/存取档→SDL输入；不等于完全未实现 |
| 080 | 末日禁咒 | 未完成验收 | 未逐ID核实习得→目标/原效果→动画/消耗/反馈→回滚/存取档→SDL输入；不等于完全未实现 |
| 081 | 七彩光錐 | 未完成验收 | 未逐ID核实习得→目标/原效果→动画/消耗/反馈→回滚/存取档→SDL输入；不等于完全未实现 |
| 082 | 狂嵐焚風 | 未完成验收 | 未逐ID核实习得→目标/原效果→动画/消耗/反馈→回滚/存取档→SDL输入；不等于完全未实现 |
| 083 | 發勁 | 未完成验收 | 未逐ID核实习得→目标/原效果→动画/消耗/反馈→回滚/存取档→SDL输入；不等于完全未实现 |
| 084 | 運功護體 | 未完成验收 | 未逐ID核实习得→目标/原效果→动画/消耗/反馈→回滚/存取档→SDL输入；不等于完全未实现 |
| 085 | 劈空掌 | 未完成验收 | 未逐ID核实习得→目标/原效果→动画/消耗/反馈→回滚/存取档→SDL输入；不等于完全未实现 |
| 086 | 輕功 | 已完成（CONTROLLED） | 原BIN/ASM独立oracle、mask/计时/延期/永久/到期、学习/CEL、公共事务/取消/回滚/save及独立包SDL通过；见顶部直接正面计时族 |
| 087 | 氣功 | 未完成验收 | 未逐ID核实习得→目标/原效果→动画/消耗/反馈→回滚/存取档→SDL输入；不等于完全未实现 |
| 088 | 療傷心法 | 逐项验收完成 | 本批闭环；分支受控证据见下 |
| 089 | 如形化影 | 未完成验收 | 未逐ID核实习得→目标/原效果→动画/消耗/反馈→回滚/存取档→SDL输入；不等于完全未实现 |
| 090 | 氣功砲 | 未完成验收 | 未逐ID核实习得→目标/原效果→动画/消耗/反馈→回滚/存取档→SDL输入；不等于完全未实现 |
| 091 | 金身護體 | 未完成验收 | 未逐ID核实习得→目标/原效果→动画/消耗/反馈→回滚/存取档→SDL输入；不等于完全未实现 |
| 092 | 無想轉生 | 已完成（CONTROLLED） | 原BIN/ASM独立oracle、mask/计时/延期/永久/到期、学习/CEL、公共事务/取消/回滚/save及独立包SDL通过；见顶部直接正面计时族 |
| 093 | 氣功波 | 未完成验收 | 未逐ID核实习得→目标/原效果→动画/消耗/反馈→回滚/存取档→SDL输入；不等于完全未实现 |
| 094 | 虛空劍法 | 未完成验收（核心普通分支已联合验） | 普通死亡掉落、25/28/29生产SDL+反馈+支付+存取档已补；缺094脚本死亡生产联合（131续档无可触发事件，未知generic谓词仍安全拒绝），见顶部close94批 |
| 095 | 長虹劍法 | 未完成验收 | 未逐ID核实习得→目标/原效果→动画/消耗/反馈→回滚/存取档→SDL输入；不等于完全未实现 |
| 096 | 破魂劍法 | 逐项验收完成 | effect43/mask8000/timer94=3；学习200、无角色排除、第二菜单ST15非MP、dispatch89/30帧；独立oracle/生产拒绝回滚/SDL/恢复通过；CONTROLLED |
| 097 | 裂地劍法 | 未完成验收 | 未逐ID核实习得→目标/原效果→动画/消耗/反馈→回滚/存取档→SDL输入；不等于完全未实现 |
| 098 | 爆烈劍法 | 未完成验收 | 未逐ID核实习得→目标/原效果→动画/消耗/反馈→回滚/存取档→SDL输入；不等于完全未实现 |
| 099 | 混沌劍法 | 未完成验收 | 未逐ID核实习得→目标/原效果→动画/消耗/反馈→回滚/存取档→SDL输入；不等于完全未实现 |
| 100 | 無極劍法 | 未完成验收 | 未逐ID核实习得→目标/原效果→动画/消耗/反馈→回滚/存取档→SDL输入；不等于完全未实现 |
| 101 | 九天劍法 | 未完成验收 | 未逐ID核实习得→目标/原效果→动画/消耗/反馈→回滚/存取档→SDL输入；不等于完全未实现 |
| 102 | 乾坤劍法 | 未完成验收 | 未逐ID核实习得→目标/原效果→动画/消耗/反馈→回滚/存取档→SDL输入；不等于完全未实现 |
| 103 | 雪山飛劍 | 未完成验收 | 未逐ID核实习得→目标/原效果→动画/消耗/反馈→回滚/存取档→SDL输入；不等于完全未实现 |
| 104 | 霸邪劍法 | 未完成验收 | 未逐ID核实习得→目标/原效果→动画/消耗/反馈→回滚/存取档→SDL输入；不等于完全未实现 |
| 105 | 紫青雙劍 | 未完成验收 | 未逐ID核实习得→目标/原效果→动画/消耗/反馈→回滚/存取档→SDL输入；不等于完全未实现 |
| 106 | 破邪劍法 | 未完成验收 | 未逐ID核实习得→目标/原效果→动画/消耗/反馈→回滚/存取档→SDL输入；不等于完全未实现 |
| 107 | 日月劍法 | 未完成验收 | 未逐ID核实习得→目标/原效果→动画/消耗/反馈→回滚/存取档→SDL输入；不等于完全未实现 |
| 108 | 潮雨劍法 | 未完成验收 | 未逐ID核实习得→目标/原效果→动画/消耗/反馈→回滚/存取档→SDL输入；不等于完全未实现 |
| 109 | 四象劍法 | 未完成验收 | 未逐ID核实习得→目标/原效果→动画/消耗/反馈→回滚/存取档→SDL输入；不等于完全未实现 |
| 110 | 八龍劍法 | 未完成验收 | 未逐ID核实习得→目标/原效果→动画/消耗/反馈→回滚/存取档→SDL输入；不等于完全未实现 |
| 111 | 守宮劍法 | 未完成验收 | 未逐ID核实习得→目标/原效果→动画/消耗/反馈→回滚/存取档→SDL输入；不等于完全未实现 |
| 112 | 萬元歸宗 | 未完成验收 | 未逐ID核实习得→目标/原效果→动画/消耗/反馈→回滚/存取档→SDL输入；不等于完全未实现 |
| 113 | 有無相生 | 未完成验收 | 未逐ID核实习得→目标/原效果→动画/消耗/反馈→回滚/存取档→SDL输入；不等于完全未实现 |
| 114 | 天臨神兵 | 未完成验收 | 未逐ID核实习得→目标/原效果→动画/消耗/反馈→回滚/存取档→SDL输入；不等于完全未实现 |
| 115 | 保留 | 未完成验收 | 未逐ID核实习得→目标/原效果→动画/消耗/反馈→回滚/存取档→SDL输入；不等于完全未实现 |

### 菜单装饰黑边与共享sprite交付（2026-11-11重启续作，优先于下列历史发布记录）

- 从磁盘保留并续验12:55已有实现，未重应用旧差异或撤销文件。根因是同类CEL解码副本把透明零游程写成调色板0；不是“黑色全部透明”。原 `474E3E` 的 `arg_0=0/1` 分别保留／覆盖零opcode及零repeat；负opcode的literal逐字节复制，即使索引0也必须保留。原依据 `analysis\ida\opening.idb.asm:201141–201689`，重点 `475009/475058/475096/475151`；调用参数审计见 `out\sprite-asm-call-audit.log`。
- 统一入口 `src\original_sprite.hpp` 的 `draw_original_sprite/draw_original_cel`，显式 `OriginalZeroRun::transparent/opaque`；非法流失败不发布半幅图像。共享调用受影响清单：难度 `new_game`、序章及战场 `prologue`、存取档 `original_save_menu`、设置 `original_settings`、战场菜单 `original_battle_menu`、总检／人物 `original_battle_reports/original_unit_status`、确认 `original_confirmation`、转职 `original_promotion`、关间 `original_campaign`、商店／服务共享面板 `original_shop_menu`、技能菜单 `original_skills`。商店 `41144E/411587/411DE6/411FA1/4125FC` 等按原push0/1保留不同覆盖行为；确认框和指定技能底板仍不透明。标题PCX `title_image`、字体、地图、单位动作及效果原语不硬并入此RLE入口；typed raw索引0掩码保留原独立语义。
- 底部51/52/53不是裁切垃圾：`4354C0` 原 `435614→435644` 无条件绘制13行，前10行随页，末3行固定51/52/53；模式1仅禁止选择固定槽，不隐藏它们。CEL204为437×461，居中(102,10)，固定行y=376/406/436，面板底471，均在480画布内。原+366、30行距及分页+341保持不改。
- 像素回归 `tests\original_sprite_test.cpp` 复用独立有符号opcode原资源oracle：25种真实CEL×透明／不透明，全RGBA比对、左右上下及完全画外裁切、背景孔洞／装饰保留、四种存取档模式13行、literal零／非零索引有效黑色、typed raw掩码及非法流回滚；SDL事件翻页与三帧呈现通过。真实样本literal零数量为0，因此该边界由合成契约例覆盖，不虚称原资源出现。日志 `out\sprite-resume-{test-build,test,package-layers}.log`。
- clang-format完成，最小关联回归通过：战场菜单、总检、关间、确认、对话、关间入口、转职、存取档、设置、商店、技能资源、UI布局、人物及标题菜单。逐项日志 `out\sprite-resume-original_*.log`；沿用此前 `sprite-original_{inn,church}.log` 的成功证据。本次宽服务／全存档装备测试过久已终止，不计本次通过；没有伪造空的旧army日志。Debug初次根同步受本次测试占用DLL失败，停止自有测试后顺序Debug→Release及根同步成功，见 `out\sprite-resume-{debug,release}.log`。已有编译警告未扩项修改。
- 精简包 `S2NE-game-win64.zip` SHA256 **C9E7CA41A3BA4A60584C2541E0D1D11136D3D8B683B51024232E595EA94523EE**；Release／根／全新独立解压EXE一致 **545856FED555265C7228A3576A55173E4A9658AD02A11502C780338F3E424660**。独立目录 `out\sprite-resume-independent` 的资源 `--verify`、SDL标题音频86帧、新游戏43帧及共享像素测试通过，见 `out\sprite-resume-package{,-verify,-title,-newgame,-layers}.log`。包15项，无SAVE；不声称桌面截图逐像素或所有菜单人工点击已验。

#### 115技能后续只读交接（本次不开技能实现）

严格完成按每个技能ID分别要求：合法习得条件、目标／原效果、完整动画、资源／AP消耗、命中与失败反馈、保存重载、取消／失败回滚、生产菜单输入。仅表审计、共用effect、资源解码、直接execute或受控fixture不能替代闭环；不得把未充分验证等同未实现。

- 当前可复验表审计：115记录=96普通+19协作，13条件标志是重叠子集，60种effect；`out\sprite-resume-skill-audit.log`。原BIN名称／ID逐项清单见 `out\sprite-resume-skill-inventory.log`（115项均暂列严格待验，不是抹掉历史进展）。
- 优先逐项复核实际名称：32《高溫氣化》（23帧生产输入／消耗／回滚，源档既有强化值边界）；33《詛咒術》（21帧生产输入，原档自然MISS；命中靠专项）；88《療傷心法》（`original_skill88_positive_evidence_test-94-regression.log` 明确原UJ02合法续作、AI受伤后35帧正治疗／保存／回滚；不能误用失败的 `skill88-positive-input.log`）；94《虛空劍法》（15帧生产MISS／续档HIT，复杂死亡／特殊反馈仍有边界）。其余111项无本次逐项闭环认证，按清单待验。
- 给父任务计数建议：**总数115；本次严格完整认证0、待逐项复核115（其中4项有较强闭环候选证据，其余111项未逐项认证）**。这不是“已实现0／未实现115”，现有证据不足以可靠给出实际实现完成／未完成数。先复核上述4项再逐项推进，不猜数字、不按60种effect批量算完成。

### 技能33自动生产输入与最新包（历史交付，最新包以上方sprite记录为准）

- 新增 `--smoke-skill33-input`，复用现有SDL事件队列和真实生产事件分发；不是直接调用技能execute，也不是桌面点击。CTest登记 `original-skill33-production-input`。原生 `SAVE\UJ02.SAV` 的逐字节隔离副本用于验收，玩家苏id4位于(16,39)，自然learned[3]=1023，未注入学习、物品、flag、坐标或属性。
- 自动输入依次经过右键战场菜单→总检表→检阅人物→定位苏→角色行动→技能→妖术33→敌军目标39→取消→重新选择→确认施法。菜单/名册状态转换均有断言，21帧均经SDL呈现；自然ST差使命中率1，本次结果MISS，不以此宣称自然命中覆盖。MP200→193、AP102→57、妖术熟练2837→2867、XP25→31，敌HP273不变；取消前后及结算后500ms快照相同，保存decode/load往返通过。命中状态/计时仍由专项测试覆盖。
- 命令：设置 `SDL_VIDEODRIVER=dummy`、`SDL_AUDIODRIVER=dummy` 后运行 `S2NE.exe --smoke-skill33-input . .\S2NE\out\curse33-live\SAVE\UJ01.SAV`；日志 `out\curse33-input.log`。目标回归日志 `out\curse33-{test,save-test,skills-test,feedback-regression}.log`；clang-format及Debug→Release完成，构建日志 `out\curse33-input-{debug,release}.log`。
- 精简包已重建：`S2NE-game-win64.zip`，SHA256 `7C7318FC7E709638E5C5B7791C0495FE6FC0DDE72DC385E24B1478D2D0D17BD7`。根／Release／独立解压EXE一致：`7B998737958D9215D7FEA39F99A788AF6E6EE0254D8AC243C81251C3AA63EEDA`；包含本次之前父核查的CEL190剑／191盾地形图标修复。包无个人存档。独立解压资源 `--verify` 与同一完整SDL事件输入链通过，日志 `out\curse33-package{,-verify,-input}.log`。
- 原档与隔离输入副本SHA256保持 `2F4D6BDB217E38F13BC6B2B18B6F9F7F3B0AED086531CDCA5A2E87EA36558EF9`。桌面Locked导致真实鼠标链仍未完整完成；dummy驱动证明生产事件/渲染路径，不代表可见Windows桌面或硬件显示验收。下文旧“未打包”及旧哈希仅属历史时点。

- **右上误画转职图标已修复**：地形攻防效果图标转换原指针索引时多减1，错误使用CEL189转职／190剑；`426598` 从 `4F7064` 写入CEL1，`42C3C6` 的 `4F7358/4F735C` 应对应CEL190剑／191盾。修复保留原位置与条件，非CEL143面板问题。第一处误画在(527,91)，底部面板下移363。真实资源完整RGBA对照覆盖第1／71关及上下两侧，相关UI回归通过；证据 `analysis\terrain-evidence.txt`、`out\terrain-icon-test.log`、`out\terrain-icon-ui.log`。原测试只检查文字而遗漏图标，已补回归。Debug→Release及根同步完成，根／Release SHA256均为 `549A5F41D74C749C4164E73A749C7D19BADFB0249F579CE6161A33FA323A04AE`；游戏包尚未随本次更新。

- **技能33《詛咒術》：effect55、敌方目标。** 最新技能代理已停写交付生产接线：成功反馈采用 `452E7F` 闪白，未命中采用 `468BAC` 八帧MISS；反馈结束才发布状态／计时／奖励／MP／AP／RNG。`original_skill33_trace_test` 的真实SDL **21帧**、成功／MISS反馈、回滚及存档decode/reload通过，`skill94_feedback`回归通过。日志：`out\curse33-test.log`、`out\curse33-feedback-regression.log`、`out\curse33-debug.log`、`out\curse33-release.log`。Debug→Release及根同步已完成，**未打包；仍缺完整窗口真实菜单→目标→施法点击验收**。本项来自最新停写交付，不是本次文档操作重新运行测试，也不代表全技能完成。
- **技能33真实输入验收阻碍（2026-11-11续查）**：已从隔离原生存档定位玩家苏（id4，16,39），此前实际进入行动→技能→妖术并看到《詛咒術》，尚未完成目标→施法。原窗口关闭后再次启动，Computer Use在点击“载入进度”时返回桌面Locked、session_active=false、Nothing was performed；未绕过桌面权限，也未将未执行动作算作通过。该轮未改源码／存档、未重新构建或打包；隔离程序仍是图标修复前副本，不能作为最新根Release的交互证据。需桌面恢复可访问后，用最新程序续验。
- 原目录115项＝96非合作＋19单人合作。32与88有已记录正常输入路径；94有角色11 MISS／续档角色16 HIT等受限生产证据，复杂死亡／特殊反馈不能外推；33现为生产接线与专项通过、真实鼠标链待验。其它技能逐项习得／目标／效果／视觉／支付尚缺，不以审计、包装或拒绝测试计算完成数。
- 非技能仅**有限清单收口**：资源、存档、菜单、画面、普通战斗／AI及死亡各有定向正常或受控证据；不是所有原函数、所有自然触发、完整原战役或原版逐帧一致性认证。各项原地址、实现、具体测试和失败证据按下方专题归档。
- 两条独立**强化**路线已到原终局61／131并完成各自完整视觉结局和清理退出；不等于正常属性平衡通关、全部分支或所有技能完成。历史换盘证明保留为逻辑资源域验证，当前平台免物理换盘。
- 最新画面发布记录为LETTERBOX比例铺满／最终SDL LINEAR，历史EXE哈希 `699C97C0400B150648754FF9858C45BFEDB1503A889C7DAE25B51BE176E895D3` 仅属于该画面包。其后33有新根Release、未新打包；**不将旧包哈希冒充技能33交付哈希**。

<a id="remaining"></a>
## 未完成与完整性边界

| 分项 | 仍需验证／补全 |
|---|---|
| 33詛咒術 | 完整窗口真实菜单、敌方目标选择与施法鼠标链；专项通过不替代该E2E。 |
| 其余技能／单人合作 | 逐ID合法学习、条件与目标、原效果、支付奖励、动画及回滚；19合作保留，不与双人对战排除混淆。 |
| 普通战斗／AI／死亡 | 自然status40／10来源、全部事件handler、完整战役与特殊状态／技能effect39；原关初始化扫描／受控接敌不能替代自然触发。 |
| 菜单／物品 | 有限菜单树已接；283／296及effect61／67仍无原库存／商店自然覆盖，不能造物品充数；教会死亡／钱与12人滚动受控证据如实保留。 |
| 存读档 | 特殊半事务仍禁存；未模拟断电、写满、硬件Flush失败；slot53备份与目标是两次单文件替换，不保证跨文件原子性。 |
| 画面与时序 | 全DOS／Windows逐帧对照、特殊隐藏／动作类别及独立A8生命周期仍不足；最终LINEAR不是Super2xSaI等价实现。 |
| 全流程与发布 | 正常属性连续通关、全部剧情分支、全新无开发环境机器运行库认证未完成；本次不构建、不测试游戏、不打包。 |
| 音频 | 完整原音效／音乐事件与淡入淡出还原暂缓；设备非静音输出不等于音频完整验收。 |

<a id="approved-differences"></a>
## 准许保留的差异

以下为集中维护的获准范围；其它未实现项不是默认例外，技能名称／调用链误判也不是获准差异。

| 项目 | 获准范围与约束 |
|---|---|
| 双人对战 | 不实现、不列通关验收；**单人模式19项合作技能仍保留为待办**。 |
| 音视频实现现代化 | 音频与视频的后端、编解码及播放实现允许采用现代化方案；不要求保留旧平台API或旧解码器实现。仍须保留原内容、事件触发与顺序等已查明语义，不得自行换曲、换片、删事件或以实现替换冒称原内容完整验收。既有SDL3／SDL3_mixer、FLIC解码及音频设备记录只证明各自已验证范围。 |
| 明显原版错误修正 | 经原反汇编／资源等证据确认的明显错误允许修正；每项须记录原行为、修正原因及影响范围，并保留证据。此许可不等于任意改技能公式、规则、状态机或内容；未查明行为仍拒绝猜测。下行对话箭头是已记录实例。 |
| Windows对话箭头修正 | **原行为**：所持Windows EXE选上框238／下框237，与CEL实际尖端相反。**原因**：纠正箭头朝向错误。**修正与影响**：上框237向下、下框238向上，仅调整对话定位箭头选择；保留原位置、资源、透明和层序，不声称逐指令相同，也不推断所有发行版或DOS相同。原链 `41CA6E → 426CEC → 474E3E`。 |
| 最终窗口输出柔化 | 640×480逻辑帧保持；按比例铺满、仅必要黑边、不裁切，最终SDL LINEAR轻柔化，不改PAL／透明／遮挡／原游戏像素，不宣称等价DOSBox Super2xSaI。 |
| 平台与免换盘 | 原生Windows x64／C++23／SDL3，单DAT／单PRG去重封装、免物理换盘；原资源字节与逻辑剧本域兼容保留。 |

**音频完整原音效／音乐事件与淡入淡出还原只是暂缓，不是永久省略许可；暂缓与音视频现代后端许可不矛盾。** 资源合并去重／免换盘已获准，但不得丢失原资源字节与逻辑剧本域。超出上表范围的新增差异仍需另行明确批准；明显原版错误修正须逐项补全原行为／原因／影响记录。

<a id="corrections"></a>
## 已撤销误判与历史优先级

1. **33不是治疗术**：原effect55敌方詛咒链为 `4654A6 → 466544 → 467625`，含 `471B2C` 阻抗；旧 `46618D(effect1)` 治疗解释及其完成推断撤销。历史friendly gate／NativeEffect55包装与字段fixture只证明当时的窄测试，不证明正确诅咒效果。最新生产边界见顶部。
2. **spellsp不是缺失外置文件**：它是CEL1读取错误标签，`473CDB`实际开 `SUPERSP.CEL`，`463EA6`作组相对seek。旧EXE字符串／lseek／目录搜索推断撤销。33＝visual33＝CEL34／21帧；23帧CEL33属于32。原归档大小、哈希及偏移核查保留于技能专题，不再据此报缺文件。
3. “整数缩放已修复默认满窗”及“Windows原238／237方向就是正确方向”撤销，以获准比例铺满与箭头纠正为准。
4. “bridge无baseline／跨stage拒绝＝正常新游戏与跨关初始化缺失”、DAT12城镇世界图、PRG固定安装路径即换盘缺陷、94实际命中走D874／reserve32可操作等旧推断均撤销；纠正与原失败现场仍保留。
5. 旧events第4回合campaign_modal失败后来按原对白输入修正并有完整该测试通过记录；这不保证所有事件handler已实现。旧Debug span越界后由settings全局块读取修复。历史失败不删、不把某次专项或某一版全套通过升级为当前全功能保证。

<a id="validation"></a>
## 分项验证导航与阅读规则

| 专题 | 内容 |
|---|---|
| [技能](#skills) | 原115项审计、学习、32／88／94逐步实现、33纠错与资源证据 |
| [普通战斗／AI／死亡](#combat) | 原函数、字段、交换／反击、反馈、死亡与受控／自然边界 |
| [菜单／关间](#menus) | 旅店、装备／物品、商店、教会、原输入与交易回滚 |
| [存档／初始化](#save) | codec、bridge、原子写、fresh／transition、隔离写档 |
| [画面／移动／城镇](#visual) | 箭头与最终显示、遮挡、特殊移动、传送、present门控 |
| [双线流程／终局](#campaign) | 正常与强化路径、原名／资源阶段、存档／日志／结局证据 |
| [资源／发布](#resources) | DAT／PRG来源、去重、历史包与哈希 |
| [工程／诊断／音频](#engineering) | 原分析、架构、构建方法、故障与音频边界 |
| [跨项历史](#cross) | 原跨类别调查、旧清单与当时计划；不是当前待办 |

每个专题先给当前索引，再保留标记为历史的原证据。历史标题中的“当前／最新／未接／已收口”仅表示**当时**，不能覆盖顶部；相邻方向词（上节／下节／顶部）仅是原记录的时间关系。原日志名、存档名、命令、哈希、失败与特殊fixture限制保留；旧EXE／C++20／CDS命令只供历史复现，不是现行开发入口（现行见README）。测试呈现帧数不代表原版固定时长。本次合并未运行其命令、未读取或改写代理源码／日志。

<a id="skills"></a>
## 技能：原函数、实现与逐项验证

115项审计不是115项完成。33最新生产反馈／21帧专项见顶部，尚缺完整真实鼠标链；旧治疗链撤销。32／88真实输入、94受限MISS／HIT及各自事务证据分开保留。

| 项目 | 原函数／实现 | 已有证据 | 边界 |
|---|---|---|---|
| 33詛咒術（最新停写交付） | effect55敌方链4654A6→466544→467625／471B2C；命中452E7F闪白／MISS走468BAC八帧，反馈后单次发布 | original_skill33_trace_test：真实SDL21帧、命中／MISS、rollback及decode/reload；skill94_feedback回归；curse33-test／feedback-regression／debug／release四日志 | 未完整窗口真实菜单→目标→施法点击；未打包，不计全技能完成 |
| 32／88／94 | 各自原效果、独立资源与支付／反馈链；禁止按共用effect批量授权 | 32正常输入23帧，88 UJ02受伤后35帧正疗伤，94角色11 MISS与角色16续档HIT分开记录 | 32源档含既有强化值；94复杂死亡／特殊反馈与全部自然分支未全覆盖 |


> **历史区域**：下方各原标题仅表示当时状态，不能覆盖[最新状态](#current-status)；误判按[撤销表](#corrections)阅读。

#### 历史原证据区域（保留原标题便于追溯）

#### Skill entry 2026-11-11 04:54-05:10: original_skills_test passes (58 learned records/7 scenes only, not115 completion); original_skill94_resolution_test passes from workspace resource root, while feedback remains a save-dependent test when launched outside its SAVE working directory. original_skill_audit_test now reads the current skill table and passes all 115 records: 96 ordinary, 19 cooperative, 13 conditional, 60 unique effects, all 11 group thresholds and 45C8CA relation truth tables. Skill88 stage60《返鄉的遊子》actor2 fixture was corrected to current S2NEPK03 plus legal original map resource domain; rebuilt SDL path passes strict sprite validation, threshold90, ST25/AP payment, status34 feedback, 35 present-gated 50ms frames, and failure RNG/reward rollback. Its own log explicitly labels this a resource fixture rather than saved-game E2E, so skill88 is not overstated as full production completion.


#### Skill production batch 2026-11-11 05:00-05:12: `original_skill_followup_test` passes from workspace root. For skill32《高溫氣化》, it verifies original item learning records 226/227/241/294/295/297/298/299/301, 451DDA range/floor and invalid-range rollback, stage54 learning events and stage62/133《?》 scripted learned id104 boundaries, plus a capped-proficiency normal hostile cast transaction from the stage61《礦坑的秘密》 saved route: MP400→360, AP consumed, equipment removal, XP/proficiency bridge and save-compatible state. The same test explicitly reports conditional direct grants are not reachable-caller proof. `original_skill_input_evidence_test` passes normal-input SDL23/cancel/settled evidence with MP40/AP45 and isolated rejection/RNG rollback. `original_skill94_visual_test` passes stage131《超時空傳說》 actor32 learned record and SPELLSP85 15-frame restored visual subchain, but explicitly remains visual subchain only—not payment or complete normal-input E2E. No package yet.


#### Skill production batch follow-up 2026-11-11 05:12: `original_skill94_input_evidence_test` passes from workspace root using authentic stage131《超時空傳說》actor11: normal-input accepted learned94 path, miss settlement preserves identity/equipment/position, native ST5/AP45/proficiency26/XP1, cancel and idle persistence equality; actor32 remains a reserve fixture and is not claimed as actor32 E2E. `original_skill_schedule_test` passes after launching from workspace root: four deterministic present/stall schedules and all 64 native skill32 growth seeds, exact MP40 payment/no duplicate commit. These are evidence for skills32/94 only; no other IDs are marked complete.


#### Skill33 ASM trace 2026-11-11 05:33-05:40: `analysis\\ida\\opening.idb.asm` verified present (18,686,203 bytes) and was read directly. New `original_skill33_trace_test` scans the original stage tables, finds the first genuine skill33 learned actor at stage60《返鄉的遊子》 (actor4, hostile target202), and verifies group3/bit0, effect55, MP7, radius4, mode1, feedback55, AP configuration, and the production runtime field mapping available in `OriginalEvents::Unit`. It passes without mutating state. This is a read-only field/fixture trace only; no effect55 settlement, payment, RNG, target immunity, save rollback, input lock, or complete animation was claimed.

> **已撤销的33治疗链／资源归属推断（历史）**：以下旧门控、包装、字段与拒绝测试不是正确effect55诅咒生产证明；旧外置文件、23帧归33结论撤销，最新33交付及真实鼠标链缺口见[最新状态](#current-status)。

#### Skill33 transaction scaffold 2026-11-11 05:43-05:48: `OriginalSkills::execute_support` now stages actor/target/RNG, enforces the exact effect55 record, original radius4/mode1 friendly target gate, resource check, partition AP/carry/status payment and explicit payment-state MP handling, then commits only after the adapter succeeds. The adapter boundary is intentionally narrow: it must supply ASM-derived effect55 target mutation, immunity, amount and reward-side RNG; unsupported adapters fail closed. The existing skill regression and stage60 trace pass after formatting. This is not yet production completion: `app_runtime` has not wired effect55 animation/input-lock/46C45B/46B9A9 or a native 46618D/469101 adapter, so no skill33 payment/effect/RNG/save E2E is claimed.

> **已撤销的33治疗链／资源归属推断（历史）**：以下旧门控、包装、字段与拒绝测试不是正确effect55诅咒生产证明；旧外置文件、23帧归33结论撤销，最新33交付及真实鼠标链缺口见[最新状态](#current-status)。

#### Skill33 ASM adapter follow-up 2026-11-11 05:47: direct offset tracing narrowed the remaining production gap. `46618D` invokes `471F51` then `468BAC` with `arg0=0/1`, and invokes `469101` with the effect result plus counters; `468BAC` mutates runtime HP at `+124h`, reads status/secondary state at `+122h`, uses call-context `+3Ch`, and updates effect presentation state at `+5F8h` while driving `4239AD/457D84` presentation. `469101` reads record `+0Bh/+0Dh/+13h/+20h`, runtime ability/proficiency values through `45CFBB`, and resistance via `46D496`; payment `464CFA` writes AP `+12Eh`, carry `+130h`, and resource fields `+126h`. The current `Unit` exposes HP/AP/carry/MP/ST but has no faithful `+5F8h` effect state, no typed `471F51` target packet, and no adapter for `4239AD/457D84` or the native reward/global RNG path. Wiring a guessed adapter would violate the original call contract, so effect55 remains fail-closed and uncounted. No package.

> **历史误判，已于2026-11-11撤销（保留下段原失败记录，不作当前结论）**：下段关于“463EA6打开外置spellsp”“473CDB分配缓冲”“现有CEL不能提供资源”及技能33外置依赖阻碍的推断均错误。473CDB实际打开SUPERSP.CEL第1组，463EA6作组相对seek，spellsp仅为4729F3的Read标签。严格拒绝测试通过只证明失败保护，不能证明缺文件或完成技能；当前缺口为实现／契约映射。23帧属于skill32的CEL33，skill33应CEL34、21帧。以本文件顶部只读复核为准。

> **已撤销的33治疗链／资源归属推断（历史）**：以下旧门控、包装、字段与拒绝测试不是正确effect55诅咒生产证明；旧外置文件、23帧归33结论撤销，最新33交付及真实鼠标链缺口见[最新状态](#current-status)。

#### Runtime-offset mapping batch 2026-11-11 05:49: `OriginalEvents::Unit` already maps durable runtime words +124h HP, +126h MP, +12Eh AP and +130h carry through `OriginalSaveBridge`; `original_combat` and `Battle` already use +124h for staged HP/status operations. A new non-serialized `Unit::effect_state_5f8` was added for the transient `468BAC`/`+5F8h` state and initialized/reset at runtime boundaries; it is deliberately excluded from packed SAVE encoding. The trace and skill regression remain green. ASM cross-reference confirms `+5F8h` is presentation state updated by `468BAC` around `4239AD/457D84`, not a durable unit field. The remaining faithful adapter still requires the native `471F51` target packet, `46D496` resistance helpers, `469101` global RNG/reward context, and `46C45B` presenter lifecycle; no guessed HP increment or fake reward RNG was wired, and skill33 is still not complete. A strict `NativeEffect55Context` layer now exists: it accepts only the verified skill33 record and requires both runtime blocks; incomplete result/presenter state fails closed. The stage-60 actor4/target202 fixture explicitly exercises that failure path and passes. ASM confirms 46618D preserves EAX/EDX/EBX/ECX as source/target/action context, 471F51 calls 46D431 then 47328B, 46D496 receives its own EAX/EDX/EBX context and calls 440F56/46D874/407E2D, 469101 receives the preserved context plus three stack arguments, and 46C45B calls 46B9A9 then 46618D. These register contracts are recorded without inventing unknown offsets; The missing external data are the native 46D431 packet layout, global 505C0C/4F6FB8 state, and validated effect55 presenter resource mapping. New strict layer `NativeEffect55Globals` records the proven global initialization: `463914` allocates the skill table (`505C08`), derives `505C10`, creates `505BE4/505BE8`, then sets `505C14` and `505C0C` from table dimensions; `427E80` loads `itemsp.bin`, stores the signed item count at `4F6FB8`, and `427EE9` frees both pointer/count. No pointer may be used without a supplied snapshot. `NativeEffect55Packet` now names only verified packet outputs (`target_runtime_122`, transient `effect_state_5f8`, completion flag); its fixture asserts global-table absence fails closed. The stage-60 skill33 regression remains green. `46D431` callsites prove output is written through caller locals (`var_10/var_14` or `var_4/var_8`) and then compared with runtime `+122h`; the full source/target packet layout still depends on `unk_5055D8` and native table pointers, so no settlement was added. PRG/PAL inspection is likewise not counted: existing CEL1/32/88/94 presenters do not prove an effect55 resource ID, and no new ID was invented. `NativeEffect55Globals::create` now loads the merged primary `S2NEPK03` PRG through `OriginalDiscResources::try_prg_image`, loads `ITEMSP.BIN` through the same workspace-root resource path, snapshots the table bytes/count, and `release()` clears spans, storage, and snapshot state. The lifecycle test passes with the actual 302-item table and verifies post-release use cannot pass the snapshot contract. The current merged PRG/PAL index contains only the validated resource domains/entries consumed by existing presenters; no effect55-specific CEL/PRG frame identifier was found in the current package, so the original effect55 presenter remains externally unresolved rather than treated as complete. Further ASM tracing identifies this as dynamic, not a fixed PAL index: `46C45B` loads `505C1C[skill_id-1]`, allocates a native item/resource record using `4F6FB8`, then calls `46B9A9`; `46B9A9` derives mode 1/2/3 from record `+40/+42`, derives frame offsets from `+150/+154/+158`, and passes dynamic entries to `47065B`, `424F70`, `42AFBA`, `4468DF`, and `44919B`. For payment selector 7 it additionally calls `42C50A` with `4FAC58`; no fixed effect55 CEL ID is present. `NativeEffect55PresenterContract` now decodes only the proven packet offsets (+40/+42/+4A/+150/+154/+174) and rejects zero dynamic pointers. Its zero-packet failure test passes. The unresolved external dependency is the runtime allocation/population of those native records (`505C1C`, `4F7FB?`/`4F7D04`, `4FAC58`, and the `+150/+154/+158` tables), not a missing static PAL entry; no presenter was connected or marked complete. New ASM-backed `NativeEffect55Record::initialize()` reproduces the proven `463BCA` zeroing/copy chain: +30/+40/+154/+174, then +15C→+158→+150→+148→+144, +160/+168/+170 and +170→+16C→+164, +178, +3C, +20→+1C, +28, and transient +5FC. The test asserts the stage-60 actor4/target202 path starts with this zeroed record and rejects presenter use until the dynamic writes occur. `463D04` destruction is also recorded: free +30 and +5FC with `430378`, then clear both. ASM write cross-reference found only the selector-table initialization at `428658` for `505C1C`; no effect55-specific constant writes for +150/+154/+158 exist outside runtime record construction. This confirms the remaining data is runtime-populated native state, not a static merged PRG/PAL entry. Further tracing reaches the actual builder `463EA6` after `463BCA`: it reads the open `spellsp` table record at `(+30 + index*8 - 4/-8)`, allocates a per-entry buffer with `473CDB`, seeks to the record offset with `lseek`, validates the loaded size through `4797BF`, assigns the loaded record at `+40`, then writes `+148/+14C/+144`; after processing the subrecords it writes `+150 = cursor`, `+154 = cursor + signed WORD(+40)`, `+158 = cursor + WORD(+40)*8`, and allocates `+174` with `430330(WORD(+40/+4) * WORD(+40/+6) + 4)`. `46418D` frees +15C/+144/+174. `NativeEffect55RecordBuilder::build` now reproduces the proven cursor arithmetic and rejects absent/invalid inputs; the stage-60 actor4→target202 test asserts offsets 0x150/0x154/0x158. The remaining unknown is the actual loaded `spellsp` bytes and native file handle/offset (`5055D8`/`spellsp`), not selector or static PAL data. Workspace/archive search found no standalone `spellsp` file and no `spellsp` signature in merged `SUPERSP.PRG`; the only executable evidence is the original `WSUPERSP.EXE` string `spellsp` at file offset 929661 (the corresponding `SUPERSP.EXE` string is at 1055388), followed by the original Open/Read/Write error strings. `463EA6` opens that external filename, uses the handle stored through the `5055D8` context, seeks to the runtime table offset, and validates the read with `4797BF`. Existing CEL1 entries (SPELLSP33/79/85) are separate validated visual resources and do not supply this native `spellsp` file. Therefore no current project/merged resource can provide the missing bytes or effect55 dynamic presenter record; the production adapter remains correctly refused rather than substituted. Skill-state closeout: 115 records are classified as 96 non-cooperation plus 19 cooperation. Production evidence remains limited to 32《高溫氣化》、88《正療傷》 and 94《神劍技》 with their previously documented boundaries; 33《治療術》 is an external `spellsp` dependency blocker, not a completed skill. A recursive workspace/archive search found no standalone spellsp and no signature in primary, archived-installed, or disc2 merged PRGs; only original EXE string evidence remains. No additional skill was promoted from static CEL, audit, or refusal-only evidence, and no package was produced. The trace fixture now exposes a typed `Effect55RuntimePacket` over the existing Unit durable words and transient +5F8 state; the corrected output identifies stage 60, actor 4, target 202 (not concatenated stage text).

> **已撤销的33治疗链／资源归属推断（历史）**：以下旧门控、包装、字段与拒绝测试不是正确effect55诅咒生产证明；旧外置文件、23帧归33结论撤销，最新33交付及真实鼠标链缺口见[最新状态](#current-status)。

#### Skill production batch 2026-11-11 05:05-05:18: `original_skill_visual_test` was rebuilt after correcting the test's stacktrace API use (`empty()`; production error propagation unchanged). On the legal stage61《礦坑的秘密》 saved scene, the skill33《治療術》 visual entry restores the scene and presents all 23 original CEL1 frames at 50ms with transparent index 0, palette transform 140/45, 21 distinct frame hashes, and exact background/unit-state restoration. This is a real production visual path, but not yet proof of skill33 payment/effect/target transaction; it is not counted as full skill33 completion. Follow-up inspection confirms `OriginalSkills::execute` still deliberately rejects every record except skill88, while `app_runtime` only has production settlement branches for 88, 94, and hostile32. Skill33's table record is effect55, group3/bit0, MP7, radius4, target mode1, hit/feedback value55; the ASM/transaction implementation and original effect55 animation/effect semantics are not yet present. No generic healing was invented.



<a id="2026-11-11-技能33验证归属纠正"></a>
<a id="history-v-001"></a>
### 2026-11-11 技能33验证归属纠正

当前实际表核查确认：技能33为《詛咒術》、effect55、敌军目标，不是治疗术。历史将其按 `46618D(effect1)` 治疗链解释的结论作废；不得将旧native_effect55包装／字段映射测试当作effect55原效果生产施法证明。当前实现者正在追踪 `4654A6 → 466544 → 467625` 及 `471B2C`；尚未交付完整验证，资源映射与效果契约分别核实。下方保留的历史日志仅证明当时具体测试执行过，不覆盖本项纠正。

<a id="2026-11-10-2348续作：class4真实非技能来源与class5边界"></a>
<a id="history-v-010"></a>
### 2026-11-10 23:48续作：class4真实非技能来源与class5边界

不再以“现有开场角色无状态”停止调查：ITEMSP记录special17且type0的实际物品为**141**；原BIN4正常商店category2可售town55/56/59/62/124..129/133。只读所有1..158 source含迟到角色未发现初始携带141，不据此说它不可获得。`visual-source-coverage.log`保存实际可售列表。

新增`original_class4_equipment_test.cpp`沿保留的真实强化战役checkpoint `run-79984-stage124-start.sav`（第124关《力量與魔法》）正常原商店option2→原offer列表141→买入／装备对象／确认；未改town、库存、学习、flags或金钱，原已有钱支付。actor0 role1实际装备141，`events.movement_class`自然得4，原Battle选人→移动命令→合法目标→生产route pose.movement_visual_class4→OriginalMap compositor→真实SDL呈现通过。动画期间不能取消，等待原120ms动作完成后取消准确恢复原坐标，装备保留且capture／restore仍class4。`visual-class4-equipment-{build,}.log`exit0。明确这是**原强化checkpoint上的正常商店／非技能移动链**，不是未强化全战役自然通关，也不是注入class4状态的renderer fixture；SDL仅展示该实际生产frame，非整个app鼠标端到端。

class4另一个来源status3C bit1仅原4668F0状态技能dispatcher写入，属下一轮技能。class5 status33 bit4直接置位／xor写点仅45DC37（161904/161999行），caller466544/effect39，原45CAA0仍被普通route／camera共享调用，但状态来源是技能；无独立普通“起降菜单”可臆造。原45CA1D先绘共享503464 phase再mode1逐call切换，否则19cs；45CAA0按direction&3原资源不切phase，当前生产一致。上批class4/5图元fixture仍只证明渲染分支，不替代本次合法物品来源证据。未继续菜单／AI／死亡新功能，保持逐类。

<a id="2026-11-10-本轮非技能接线与六类缺口核实（仍未总完成）"></a>
<a id="history-v-016"></a>
### 2026-11-10 本轮非技能接线与六类缺口核实（仍未总完成）

45DC37依466544/effect39证据明确移交**下一轮技能**，本轮不再为此注入学习位/寻找伪合法样本。class4/5并非只有该技能caller：42B461/42B6C3镜头、452450普通特殊路线、4535A1确认重绘均有直接45CA1D/45CAA0调用。本批解除其**非技能**route pose/撤离/death preflight拒绝，复用已验45CADA与shared phase/present事务；Result资源读取失败仍回滚draw/cache/pending，未开放effect39。原资源第65关《龍穴》actor14受控fixture补status4导致mode3/4画面差异，原battle_test class3/4/5正常click→route→120ms/逐present→取消回滚通过；不声称原SAVE自然产生class4/5（只读五档仍为0）。这关闭调用链遗漏，不等于自然特殊状态全验。

菜单/关间实核而非沿旧表：`original_inn_test`当前真实410EEE→4732ED(2)/414C4A城镇闲谈、auto-equip取消/提交通过；`original_church_test`41430B复活确认/金额/SAVE往返与44ACAD大额捐献难度1..5通过（受控死亡/现金fixture，非自然死亡）。日志`systems-inn.log`、`systems-church.log`。实际尚缺旅店**option5/7**的44D439(mode0/1)人物装备/物品完整子页与option8的43183B返回标题确认，现明确blocker不做替代。此入口可在第65关《龍穴》原SAVE进入关间旅店复现；option10原451ABC是自动装备，不是住宿，不造“住店恢复”规则。

死亡最小正常交互新增无throw测试`original_death_input_test`：第6关《沙盜營地》role104、第82关《惡德商人》role147，**完整原事件表**未删，直接受控死亡dispatcher入口后正常advance/cancel/present/try_tick，保存锁/恢复一次通过；非自然击杀。避免旧continuation测试末尾故意event-index损坏导致fatal而掩盖前段。`systems-death.log`还列具体原表阻碍：1《第一神劍》records4/6 type9 cond1/2；11《惡德商人》records6..12 type5/14/type9cond6；15《沙漠名珠》type6/type9cond1/3/type12；42《小丑獻媚》type6/7/12/13。41C02A→41FF30→41C33E→41B9A3同步栈仍只支持type9cond0、4211A3角色谓词及4214C1/4214D5货币，完整其它handler及41FF6E completion1/3未开放。这些是明确生产缺口，不以两关对话通过宣称死亡全完成。

AI实核：原40D171→40C425/40C659低HP撤退生产及role14=5存档往返通过（`systems-retreat.log`，第1关《第一神劍》受控HP=1）；40D351行为6/7仍仅第15关《沙漠名珠》受控行为/无敌fixture通过，未自然AI全认证。非技能特殊移动渲染接线不等于飞行route规则所有分支完成。六类其余：资源v3已验；SAVE特殊半事务仍禁止保存、完整events历史campaign_modal回归未收口；画面遮挡自然样本不足；上述菜单/AI/死亡未完，不发布六类完成结论。

<a id="2026-11-10-技能专项：第一轮磁盘全目录差距审计（非全技能完成）"></a>
<a id="history-v-035"></a>
### 2026-11-10 技能专项：第一轮磁盘全目录差距审计（非全技能完成）

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

<a id="2026-11-10-技能专项后续：合法习得与32前置原角色动画链"></a>
<a id="history-v-036"></a>
### 2026-11-10 技能专项后续：合法习得与32前置原角色动画链

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

<a id="技能专项续作：156身份归因与真实spellsp33合成"></a>
<a id="history-v-037"></a>
### 技能专项续作：156身份归因与真实SPELLSP33合成

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

<a id="32正常输入端到端验收（本轮）"></a>
<a id="history-v-038"></a>
### 32正常输入端到端验收（本轮）

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

<a id="88療傷心法：有界生产接线与原资源视觉（本轮）"></a>
<a id="history-v-039"></a>
### 88療傷心法：有界生产接线与原资源视觉（本轮）

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

<a id="技能3288后续：确定性误断言归因与真实uj02输入（本轮）"></a>
<a id="history-v-040"></a>
### 技能32/88后续：确定性误断言归因与真实UJ02输入（本轮）

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

<a id="技能有界收尾：32独立难度核对、88原档正疗伤e2e"></a>
<a id="history-v-041"></a>
### 技能有界收尾：32独立难度核对、88原档正疗伤E2E

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

<a id="下一条剑术94「虛空劍法」：确证独立原视觉子链（非整技能完成）"></a>
<a id="history-v-042"></a>
### 下一条剑术94「虛空劍法」：确证独立原视觉子链（非整技能完成）

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

<a id="94后续：原命中基础原formula44算术子链（未开放事务）"></a>
<a id="history-v-043"></a>
### 94后续：原命中基础/原formula44算术子链（未开放事务）

本轮新增`src\original_skill94_formula.hpp`与`tests\original_skill94_formula_test.cpp`，仅提供无状态算术，不改生产菜单白名单，不扣资源、不造成伤害。确证并实现：

- 46D874 effect1（asm187941..188084）：**两次熟练读取都是施法者**同一原+D7数组索引，不是双方熟练；各/10且最少10。`[unit+128h] sar16`实际读取+12Ah SP，不是+128h ST，也不是fencing。双方SP限10..99，差值`((p+500)*actorSP-(p+500)*targetSP)/500`，C++有符号除法向0截断。46DBEF..46DC95分段转命中率，再乘原power50/100。覆盖-120/-80/-40/0/10/42等17个边界，保留原-80分段不连续，不自行平滑。**这是407CD0随机/难度修正前基础值，不是最终命中率。**
- 46E858中94 flags0xC2只进入bit2属性分支，不进入bit1的装备special18加成分支；`[unit+C5h] sar16`实际+C7h fencing，双方取max1；熟练为actor group9/10，无合作平均。case44确定位于46F89E（asm191092起），不是其它剑术公式。
- 46F89E→4704AC（asm192102..192149）寄存器/栈顺序：`((proficiency/10+1000)*max1(actor.fencing))/1000 - max1(target.fencing)*config[98h]/config[99h] - target.defense + power50`。配置从原BIN10后表直接读有符号byte，本资源10/10，除数非正failclosed。**允许原始结果负数，不在此提前加最小伤害。**原131角色32/敌20实际算术探针baseHit15/rawDamage-200，无状态变更、未筛RNG。

继续追查但尚未实现的明确边界：46D496依据46D874返回mode2进入407CD0，再4080E7原军队/模式/难度修正及最终命中；46E058先46E858后46E2D6，后者调用440F56、处理难度1+目标116h bit20、技能flags21h的70h分组、目标3Dh/职业元素抗性和后续修正；46E058还单独处理目标3Dh bit8。不能把pre-modifier算术称为实际最终伤害，更不能在这些未核完时接奖励支付。目标合法性、HP状态写入、原XP/熟练/ST5/AP及生产输入仍未接94，本轮**没有94正常输入E2E**。原asm边界189025..189673与46E058分发表应作为下一次有界追踪，不扩大其它effect1。

验证：formula测试当前源码cl C++20重新编译exit0（`out\skill94-formula-compile.log`/`skill94-formula.log`）；原生Release/Debug均成功（`out\skill94-formula-Release.log`/Debug）。真实Windows SDL94视觉15帧仍PASS，32 schedule64/4、32持久输入、88正疗伤持久输入均PASS：`out\*-formula94.log`。无新增辅助进程。没有写PROGRESS/README，没有改真实验收档或结局证据。数字仍2/96完整生产覆盖、94/96未完成（94现为视觉+基础命中+raw formula部分）；19单人合作待实现而非排除。

<a id="94继续溯源：重要命中入口纠正及后修正算术子链"></a>
<a id="history-v-044"></a>
### 94继续溯源：重要命中入口纠正及后修正算术子链

**修正此前计划依据，非生产回归：**94真实record flags33=0xC2。46D496在flags21h bit1分支才调用46D874；94 bit1=0、bit2=1，因此实际命中入口是`46D748→46DD11`，不是46D874。上一轮base_hit函数保留为已核实46D874算术，但**它不是94实际命中入口**，不能拿其结果当94最终命中率。本轮未把错误前提接生产。

已追46DD11（asm188425..188749）：先两次42C88F取地图/朝向数据，原非合作actor/target各读group9熟练；双方+C5 DWORD高半是fencing，目标还将+136 DWORD高半（+138 tactics）与fencing平均。随后将双方熟练+1000、fencing/目标平均值、42C88F所返回的signed byte参数参与对抗，分段曲线后乘power50。此**地图/朝向参数42C88F尚未完成映射**，因此完整命中仍阻塞在具体原地址42C88F→46DD11，不能以双方SP旧base_hit替代。94 flags80再启用46D7D5原地势差429F34，difficulty1跳过该差异。

新增`original_skill94_resolution.hpp`与对应真实配置隔离回归，严格只提供无状态算术，无target授权、无HP写入/支付接口：

- DD11调用后的46D748/407CD0会对**原累加器0**消耗一次命中随机，乘/10、4080E7后限为1，再加DD11单独结果；不是对DD11结果本身随机缩放。随后原地势差clamp±5，每差一级20%，difficulty1不修，最终限1..99。函数明确要求外部DD11结果，未自称完整最终命中。配置原不对齐DWORD读取分别是config6/7（命中）与config4/5（伤害），不能直接照抄别类10/11。
- 46E2D6对94已精确裁剪：raw29=0，故元素族/440F56族加成标志全部不触发；flags0xC2没有70h，**原军队地形/目标116h20分支整个跳过**，不得替94虚构这些修正。职业抗性/弱点WORD与原技能mask64相交时分别/10、*3；special13或status3F bit4使*2/3，status39 bit10再*80/100、bit20再*70/100；94 group byte10不触发仅2/4/3/5/6的特判，也非合作，不触发双方special2加成。
- 然后407D88原随机→4080E7军队/模式/难度→至少1；目标3C bit40护盾≤180变1，否则减180。46E058再处理目标3D bit8清0，**清0发生于随机消耗之后**。专项覆盖顺序、抗性、负raw最低值、1..5与0/6 failclosed，所有随机运行仅隔离fixture，不改真实档或筛随机。

**明确尚未完成：**42C88F地图朝向结果/46DD11完整命中及真正命中roll调用点；46D431/其调用者伤害写入与死亡/原XP熟练及ST/AP提交顺序尚未追完；因此没有生产94事务，也没有声称失败原子性或正常输入94 E2E已完成。没有为输出看似完整而使用近似hit、伪死亡或32/88奖励桥套装。本轮仅交付上述确证子链，下一步应先完成42C88F输出布局以及原46D431调用者的写入奖励路径。

验证：新resolution专项当前源码C++20编译exit0（`out\skill94-resolution-compile.log`/`skill94-resolution.log`）。Release/Debug均成功（`out\skill94-resolution-Release.log`/Debug）；resolution/formula/94实际Windows SDL15帧、32 schedule与持久输入、88正疗伤持久证据六项exit0，日志`out\*-resolution94.log`。未写PROGRESS/README，未修改真实验收档/结局档，无辅助进程。完整覆盖仍2/96，94仍部分，19合作未实现非排除。

<a id="94本轮：42c88fdd11及实际effect1-roll子链（仍非生产事务）"></a>
<a id="history-v-045"></a>
### 94本轮：42C88F/DD11及实际effect1 roll子链（仍非生产事务）

纠正上节“地图/朝向”措辞：`42C88F→42C8D1`按单位x/y与地图宽度查地面/物件属性，输出三个byte `{movementCost,hitPercent,defensePercent}`，不是面向。复用Battle既有地形映射，仅新增只读`original_42c88f_bytes`入口，不改变private区段或移动规则。DD11读取actor byte1与target byte2，均有符号；双方group9熟练分别参与，目标用(fencing+tactics)/2。DWORD乘法/差值显式模拟原wrap，分段后乘50/100。只读真实`run-43140-stage131-start.sav`角色32/敌20：地形10/0，DD11=45；未执行该档随机或改写档。94原record flags0xC2、mask64、raw29=0等已钉死，D874仍仅无关辅助。

**新增调用链纠正非常重要：**上一定位的4668F0并非94 effect1入口，不能将其case1当94。`4654A6+CE`检测record+13h(effect)==1后直接`466082→46618D→471F51→46D431`。46D431先46D496命中、非零再46E058伤害（含伤害随机）。471F51随后按目标当前HP封顶伤害，伤害0再提升为1，最后在`471FB9`调用47328B，roll<=chance命中，miss输出0。因此46E058的3D bit8清0不等于最后可直接宣称免伤：471F51还提升为1。未把未执行flags或其它effect分支计入94。

删除不完整的chance→percent `resolve_hit`封装，改为94算术packet `resolve`：chance随机→damage随机→HPcap/zero1→percent。只在成功后发布暂存RNG，不写HP/熟练/XP/ST/AP。64个固定fixture seed穷举校验同一原随机流顺序及清0后提升1；非法上下文不推进RNG。它不等于完整施放事务，不声称真实档命中或失败无支付已完成。

**已定位但未完成接线/核全：**46618D在471F51后调用468BAC反馈/HP更新，再依HP0走special25/28复活、452FB1死亡处理、468FDF击杀奖励；存活则special29可能追加percent反疗，随后469101奖励。原HP字段+124h，468BAC还保存到+132h。上述死亡特殊技/反馈副作用、468FDF/469101全部奖励与4654A6尾部ST/AP提交仍需逐分支核实，不能用32/88桥或简化死亡替代。本轮到确证命中/伤害packet子链为止：**未接生产94菜单、未实现完整94事务/失败无支付、没有94正常输入E2E**。后续有界目标正是这些地址，不再追4668F0无关case1。

最终验证（改动后重新执行）：原生`MSBuild S2NE.sln /p:Configuration=Release /p:Platform=x64`及Debug均exit0，日志`out\skill94-hit-Release.log`/Debug。resolution用cl C++20重编译exit0，`out\skill94-hit-compile.log`。从资源根执行`S2NE\out\original_skill94_resolution_test.exe`、formula、visual、schedule、input_evidence、88_positive_evidence六专项均exit0，逐项日志`out\original_skill*-hit94.log`；其中Windows SDL94为15个不同原帧、50ms时钟及恢复，仍仅视觉隔离链，不冒称正常输入E2E。32 schedule和持久输入、88正疗伤证据保留通过。两结局SHA256保持原值：white `1D64E38324595851F57857A8BED88D696DBB033F9834F9D97234D3F22EBD0874`；natalia `A89CF281CC67FB481AFEEC2FE2D499A5A1D645ED09132A1D8AC485ECC799041F`。

本轮未写PROGRESS/README，未注入真实档、未筛真实随机，无辅助进程留存。完整生产覆盖仍2/96，94是部分，另94/96未完成；19单人合作仍未实现，不属于用户排除。

<a id="94本轮事务边界：hp反馈、常驻复活、存活反疗与原支付（非完整生产交付）"></a>
<a id="history-v-046"></a>
### 94本轮事务边界：HP反馈、常驻复活、存活反疗与原支付（非完整生产交付）

逐分支追到以下原证据：468BAC先保存目标HP到+132h（MISS也保存），然后effect1按伤害递减；负值治疗封顶maxHP。46618D在HP0时优先441088 special25常驻装备复活，负maxHP调用468BAC故+132重新为0；其次special28同样复活后451D28→44D11B消耗首个匹配装备并重算属性；否则452FB1增加loss，执行41C02A死亡事件分派（可变身/移除/清激活），然后468FDF→469101及战利品缓冲累加，最后增加actor击杀数。存活路径special29另抽47328B，严格roll<30才以原damage*2反疗；MISS也会抽这个roll，反疗再次更新+132，不能始终用最初HP作为奖励快照。

469101使用45CFBB selector=1的**+132 HP快照、其它当前属性**（不是新造战力缓存公式），94非合作second分区取config E6倍率、E1除数：damage*E6*targetValue/(targetValue+actorValue)/E1，末尾至少1；真正死亡另加config E2*targetValue/actorValue+3。466082熟练为(成功目标数+本次击杀数增量+6-difficulty)*10/3+10。4654A6完成熟练学习再458B03/459309，**AP/ST实际在其调用者随后464CFA**，不是4654A6尾内：second mask8、config EF基本AP、已有mask8再扣F3、carry下限同步；505C1C状态2仅免资源，仍扣AP；94 flagsC2只扣ST5，无MP/HP支付。

新增`original_skill94_transaction.hpp`为**菜单请求所有权约束的部分事务模块**，不是已开放生产执行：仅94、真实learned bit、second、合法range3、ST/AP/运行态门控；暂存actor/target/RNG后执行现有原resolution→HP/+132→常驻special25或存活special29→469101存活公式→外接已验证学习/XP桥→464CFA支付，成功消费请求。回调拒绝、取消、重复提交、未知runtime不发布。special28消耗/重算缺完整army inventory桥，以及普通死亡41C02A/452FB1/468FDF战利品/脚本副作用**仍明确原子failclosed**，不是简化死亡；对应失败不扣费不写HP/RNG。没有连接94生产菜单执行授权或动画触发，避免真实施放遇到未覆盖死亡副作用被冒称完整。

紧耦合修复：OriginalEvents::combat_value(mode1)现在严格要求已有+132快照，并复用原战力算术；OriginalSaveBridge写回可选+132，以前只读不写会丢失本次反馈。没有改变mode0/2或身份校验。测试只修改隔离fixture副本，原131档只读，绝不称正常输入E2E。

专项`original_skill94_transaction_test`固定64 seed全部执行，不筛选：命中27/未命中37、常驻复活27、反疗roll触发27；覆盖HP快照、原额外随机顺序、原熟练、ST/AP、奖励回调失败rollback、死亡/消耗复活failclosed、取消/重复提交/未知runtime及+132持久往返。测试award为隔离断言adapter，不能冒称真实成长E2E。测试开发初期反复构造大资源导致长运行，终止对应工具进程后将不可变rules/formula移出64循环；最终单次执行全部通过，没有将旧中断算PASS。

最终源码重新cl C++20编译七专项：transaction94、resolution94、formula94、visual94、schedule32/88、32持久输入、88正疗伤持久输入均exit0。命令从工程编译`cl /std:c++20 /EHsc /utf-8 /I src /I D:\project\vcpkg\installed\x64-windows\include tests\original_skill94_transaction_test.cpp ... SDL3.lib`，从资源根运行`S2NE\out\original_skill94_transaction_test.exe`；其它同名专项同理。日志`out\*-transaction94-compile.log`与`out\*-transaction94.log`。Release/Debug原生MSBuild均exit0，`out\skill94-transaction-Release.log`/Debug。真实Windows SDL仍15个不同原帧/50ms/恢复通过，**仅视觉隔离链，不是正常输入E2E**。修改文件Problems无诊断。最后追加取消/runtime测试后重编译重跑transaction exit0。

**未完成且阻塞可用94生产路径的准确范围：**special28的451D28→44D11B装备消费/重算；452FB1→41C02A完整死亡脚本/状态和468FDF战利品提交；因此没有完整死亡奖励事务，也没有94真实131角色32正常输入施放E2E。下一轮应围绕这三项收口及生产接线，不重新做已核命中。完整覆盖仍2/96、94仍部分；19单人合作未实现而非排除。未写PROGRESS/README，未修改真实验收档；两结局hash核验仍原值。所有本轮辅助进程已结束，停写交付。

<a id="94收口续轮：special28与普通死亡掉落持久化（仍未生产e2e）"></a>
<a id="history-v-047"></a>
### 94收口续轮：special28与普通死亡/掉落持久化（仍未生产E2E）

本轮只推进前轮三项，不重做命中。新增451D28首匹配槽查找：按slot0..5、item special28、item+15类型0/1取首个，复用已还原44D11B的`OriginalPromotionTransaction::remove_equipment(...,consume=true)`同步移除装备/属性，不退库存。原441088激活判定仍只接受类型0；找首匹配与是否激活不是同一条件。事务现在可接调用方的完整army inventory vector及equipment资源桥，库存长度必须精确item_count，副本完成后才发布；未接main真实rawSave库存，不能声称真实army生产联通。

普通死亡仅授权原41C02A无匹配type3/condition2/roleID事件、且非status33 bit40移除/bit10召唤队伍槽的分支：452FB1 loss WORD+1、普通corpse bit20、保留原激活bit；actor kill WORD+1。匹配死亡脚本/移除/召唤分支仍failclosed，不调用物攻的简化队列冒充41C02A。该函数有即时脚本返回11/12/13及变换/移除/队伍副作用，现queue_original_death不能完整替代；**未实现这些完整死亡脚本，不开放94生产施放**。死亡消失视觉455048也未接本事务。

重要奖励纠正：前轮记录“死亡另加config E2*targetValue/actorValue+3”只是4693E0通用尾分支，**不能用于94真正击杀**。468FDF先调用469101时arg4=0、arg8=1，HP0立即在46913D给XP accumulator+1并返回，跳过后面所有战力/击杀加成算术。新94普通死亡rawXP严格1；熟练成功数与kill增量均计入466082。

468FDF mode0/friendly army读取目标+11B四个WORD（现Unit.initial_status），不是装备；按ID累加byte loot buffer（重复ID叠加），458B03仅非零loot项并入我军inventory并cap99，无loot的项不改。未触碰敌军装备/其它inventory。事务对库存/装备/目标/actor/RNG同一暂存，奖励回调失败全部回滚，成功消费菜单请求。OriginalSaveBridge紧耦合新增写回原+1F kills/+21 losses（原先只读），不改变单位身份/顺序/分组校验。

隔离94固定64 seed全运行：hit27/miss37；新增双special28装备仅消费首槽且不退库、奖励失败不泄露移除；普通无脚本死亡/重复掉落/99封顶/无掉落项保留/击杀及loss；死亡奖励失败不泄露库存/HP/kill/loss/RNG。新增`out\skill94-death-fixture.sav`为**明确隔离fixture**，使用原Save encode/load/bridge完整往返核对corpse/我军库存/actor kill/loss；该fixture把目标ID60000副本在编码前恢复原roleID，不属于真实验收档或正常输入E2E。没有修改原131或两结局证据。

最终验证：Release/Debug原生MSBuild exit0（`out\skill94-close-Release.log`/Debug）；九项均以当前源码cl C++20重新编译并执行exit0：94 transaction、94 resolution、94 visual、32/88 schedule、32 input evidence、88 positive evidence、save_bridge、promotion_transaction、combat。编译/运行日志`out\*-close94-compile.log`/`out\*-close94.log`；末次补充special28失败rollback与kill持久断言后transaction重编译重跑exit0。复现从资源根`S2NE\out\original_skill94_transaction_test.exe`，其它同名专项同理。Windows SDL仍15不同原帧、50ms、恢复PASS，仍仅视觉子链。Problems四个修改代码文件无错误。两结局SHA256保持原值，未写PROGRESS/README，无本轮运行辅助进程。

**准确剩余：**41C02A即时死亡脚本/返回值→452FB1移除/变换/召唤槽清理与持久化尚未完成；main真实army inventory及94生产菜单/准备/15帧/目标集成未接；真实131正常输入E2E未执行。本轮比上轮新增特殊28与普通死亡loot可提交的隔离事务，但仍不是完整94技能交付。完整生产覆盖仍2/96、94部分，19单人合作仍未实现非排除。停写交付，不将已通过隔离fixture包装成可用生产路径。

<a id="94最后阻塞续核：41c02a有序扫描与原返回值（本轮未完成生产）"></a>
<a id="history-v-048"></a>
### 94最后阻塞续核：41C02A有序扫描与原返回值（本轮未完成生产）

本轮直接重读opening.idb.asm 47195..47510（41C02A）、47757..47838（41C57C/41C5EA）、48301..48830（41CA6E）、53886..53930（41FF30）、144363..144620（452FB1/4531AB）、147686..147835（454ED5），与现prologue的queue_original_death/dispatch_campaign及Save.Group.status对照。未将现异步queue改名伪装同步原routine。

新增`OriginalDeathDispatch`只做原扫描计划，不伪称完整脚本执行：按原记录顺序，只匹配type3/condition2/role WORD；第一个匹配的line count按signed WORD解释，<=0在41C0FE直接退出整个扫描，返回0，不继续后续同ID记录、不完成事件、不置completion flag。正行数在41C148置合法completion flag，41C182将返回值设为**十六进制12h（十进制18）**，随后41C57C准备、41CA6E(mode1)同步呈现、41C5EA释放、41FF30同步扫描触发链后才回452FB1。新计划将该类标为synchronous_script且仍精确拒绝，事务不发布临时HP/支付/库存/RNG。此前“任何匹配type3/cond2均拒绝”的门控过严：现精确允许首个匹配空记录退出这一无脚本分支，不开放任何正行数记录。

重要控制流澄清：41C161/41C1A3里condition1/3/4的11h/12h分支在此函数type3/cond2过滤后不可达；41FF30循环41C33E直至0，41FF5B显式返回0，所以不能把11h/13h当此死亡入口实际产出，更不能为测试随意造一个返回11/13适配器。452FB1自身仍有防御性11h移除/13h特效分支，尚未实现其生产副作用；**不是已完成全死亡脚本**。454ED5的FE是十轮原位图调色/重绘、末尾恢复AD缓存，不是将roleID或appearance设FE；本轮未修改身份序列化。status33 bit40按4531AB提示后454ED5色254十帧40ms离场并清active；普通分支保留active并置dead。status33 bit10按appearance在army+286的11个三字节槽找首匹配置首字节0，再以Save.Group.status桥接保存。军团metadata按441143可验证字段重算，角色槽、id/equipment/inventory和脚本索引不压缩。当前仅受控生产接线与第1《第一神劍》／第6《沙盜營地》回归通过，尚待实际status40/10原关角色的可达输入专项。

测试增加：有序扫描略过cond3/其它role，首空匹配阻断后续非空，signed FFFF按空退出，正行数返回12h且拒绝/no match；固定64事务增加两种0x90/0xC0死亡分支原子拒绝，检查HP/ST/status/inventory/RNG(draw count与seed)未泄露。真实131只读审计：角色32原已学94，xy12,52、ST142、AP70；range3活敌仅1个。role20 xy10,52本身距离4，不在range3，虽无死亡事件也不能拿它假称合法施放目标；部署中活敌正行数死亡脚本计数0。前后OriginalSave capture/encode逐字节相同，未执行移动或RNG。此审计只证明脚本扫描边界，未证明真实生产施放；未注入/筛随机/更改实际档。

统一验证exit0：Release/Debug原生MSBuild（`out\skill94-dispatch-Release.log`/Debug）；九项当前源码cl C++20专项：original_skill94_transaction_test、original_skill94_resolution_test、original_skill94_visual_test、original_skill_schedule_test、original_skill_input_evidence_test、original_skill88_positive_evidence_test、original_save_bridge_test、original_promotion_transaction_test、original_combat_test。日志`out\<name>-dispatch94-compile.log`/`out\<name>-dispatch94.log`。复现资源根运行`S2NE\out\original_skill94_transaction_test.exe`；输出固定64 hit27/miss37，真实131只读range/death诊断。Windows SDL=windows独立原SPELLSP85仍15个不同帧/50ms/恢复PASS，仅视觉子链，不是正常输入94 E2E。新增只读诊断后transaction重新编译重跑exit0。

**本轮未达用户期望的生产收口，明确未完成：**41CA6E即时对话/41FF30触发链的通用事务同步复用，452FB1移除/召唤槽清理与441143提交，94 main真实army库存/菜单/准备/15帧/range3/正常输入结算尚未接。现生产仍只32/88（2/96），94部分，19单人合作仍缺失非排除。不可因本轮scan/fixture/视觉通过称94技能可用。后续可执行范围：先把原同步死亡脚本所需flags/event states/单位与army33字节同事务呈现提交，再将该支持能力而非脚本queue接入94；或严格预检无脚本普通目标后接生产，但仍需补全真实运行态39/职业mask来源、原反馈时序和保存提交，不能默认零。未写PROGRESS/README，停写前复核两结局hash及进程退出。

<a id="94有限生产路径与真实sdl输入（131角色11；角色32原为reserve）"></a>
<a id="history-v-049"></a>
### 94有限生产路径与真实SDL输入（131角色11；角色32原为reserve）

本轮不再以完整41CA6E作为全部目标前置。main接入独立94事务、原army groups[0].inventory、原菜单后分区、46C078准备、独立SPELLSP85 15帧/tint233/35/50ms、原范围3/公式/支付奖励、OriginalSave capture保存。未批量开放effect1。OriginalSkills新增显式94 record gate；后分区可操作性只加原已学group9 bit0。target_supported在准备前及事务随机前拒绝status33 10/40、首匹配正行数同步死亡脚本或未知39运行态；空首匹配仍按41C0FE退出，不改脚本旗标。不支持目标不会消费随机/HP/ST/AP/inventory。actor新增active/dead gate，避免此前隔离fixture漏验真实reserve资格。复杂死亡仍未实现。

运行态来源：OriginalSaveBridge从每个原unit record+39保存optional byte，不依赖仅bit400才存在的action_status_runtime；若runtime snapshot存在，生产读live snapshot+39/+3C/+3D/+3F，否则读原解码缓存。不以未知零冒充已知。KINDSP+18 DWORD职业表、(appearance-1)*37，+16/+18 WORD按46E489/46E4DB给mask64抗性/弱点；main调用OriginalEvents::skill_class_masks，不复用32/88公式。army来自rawSave完整group0 inventory，事务同单位/RNG暂存并在成功后捕获持久化。39缓存写回同原byte；保存身份校验未放宽。实际验证mask0/256、runtime39=0，是真实读取而非默认猜零。

纠正前轮源档可达断言：run-43140-stage131-start.sav角色32虽原已学94、ST142/AP70，但status33=0，是reserve，原occupant返回-1，不能正常选中。已有五个131原档角色32均inactive。原日志曾把其range3唯一敌当可达施放，遗漏active前提，不能再沿此断言。本轮保留原旗标，未激活角色32。只读查实际沿途存档：stage40/46有active32但初始无range3目标；有界正常接近/回合尝试未形成施放，未用此掩盖完成。探索性stage46单waypoint输入已删除，未加入生产战术参数。

**实际成功路径：同一未改131原档另有角色11原已学94且active。**正常输入驱动严格过滤active/dead，选原角色11、原现地敌role0（原资源普通敌合法ID，非boss20），未注入技能/熟练/位置/HP/ST/旗标，未设置seed或按命中重跑。固定这次结果MISS：chance50/roll53、damage0；原ST219→214/AP71→26/熟练2800→2826/XP2→3，MP401不变，无升级。生产正常选人→攻击后分区→94→原目标→取消无支付→重开确认→原准备→SPELLSP85帧0..14→事务→画面恢复，500ms idle无重复支付。Windows SDL实际Presented312，15帧逐present序列/图像见out\skill94-input\sequence.csv及frame-*.bmp；before/after/cancel/settled原档在同目录。成功日志out\skill94-production-input.log（原首写文件名input-blocked.log来自预期reserve阻断探测，但结果实际为角色11成功，已复制为准确success日志；不把角色11冒称角色32）。没有为获取hit而重跑成功施放；首次达到施放即MISS验收。

新增original_skill94_input_evidence_test核对源131角色11已学/active、before与源学习位/装备/位置/ST一致，after原ST5/AP45/熟练26/XP1、库存未变，cancel及settled逐字节相同。复现资源根S2NE\out\original_skill94_input_evidence_test.exe。正常输入命令：S2NE\out\x64\Release\envoy-opening.exe --smoke-skill94-input . S2NE\out\strengthened-flow-acceptance\run-43140-stage131-start.sav。会重新运行真实随机施放，不应要求仍roll53；持久证据测试不重新掷随机。

统一Release/Debug MSBuild exit0：out\skill94-production-Release.log/Debug；当前源码十专项重新cl并执行exit0：94 source/transaction64/resolution/visual、32/88 schedule、32 input evidence、88 positive evidence、save bridge、promotion transaction、physical combat；另新增94 input evidence重新编译执行exit0。日志out\*-production94-compile.log/*-production94.log及skill94-input-evidence.log。原15帧独立Windows SDL仍PASS；真实输入另有上述E2E，不再仅视觉。调度32仍64案例34旧误报、无双支付。真实输入驱动修正鼠标像素落在原picker内部，禁用smoke期间外部真实鼠标edge scroll干扰，未改生产玩家点击/地图或单位位置。

准确覆盖：94现在有限生产接线并有真实角色11 MISS完整输入及存档证据，不是全目标/全死亡脚本完成。32/88保持；96非合作中2条既有完整路径+94受限路径，其余93未有此种完整生产验收；19单人合作仍未实现而非排除。41CA6E同步死亡、0x10召唤/0x40移除、死亡特殊呈现/数字反馈进一步原时序仍缺，不声称本轮完成这些。原角色32的131正常输入不可达因reserve，未帮改旗标；下一范围可独立扩原部署或同步脚本，不能把本轮角色11成功推广全94目标。未写README/PROGRESS，结局档hash不变，结束停止辅助并停写。

<a id="94命中数字反馈：原468bac新生产分支与续档hit证据"></a>
<a id="history-v-050"></a>
### 94命中数字反馈：原468BAC新生产分支与续档HIT证据

重读468BAC asm180011..180415，确定原数字循环与物攻不同：直接step0，不走物攻452E7F silhouette/20ms flash；damage0显示MISS，正damage用3C/56，MISS68/24；x=32-字数*8，y=-64-step*8（仅前8步更新y）；每步扣min(remaining,1<<step)，至少8次present，剩余伤害未扣完则继续；前0..2步30ms、step3为20ms、step4..5为10ms，6以后无时钟等待但仍须实际present；文字仅step<7。新增Battle::begin_skill94_feedback单94专用入口，复用原Feedback文字投影/字体与presented HP chunks，明确绕开settle_physical（不得二次物攻XP/死亡回调）。present gating保证卡顿不会跳过原数字帧；presentation_units用反馈HP临时显示，不将暂存计算当提前live写入。

main在原15帧结束计算94完整暂存副本（unit/inventory/menu/RNG及奖励），仅接受后挂起在数字显示；数字完成callback一次性发布actor/target/ST/AP/熟练XP/inventory/RNG并capture原save。此期间live值未提交、游戏输入由combat_feedback_active锁定；不会用物攻routine再结算94。不是异步死亡脚本队列：**41CA6E/41FF30仍未接，不支持同步事件的目标仍在选择和随机之前拒绝**。本轮新增可用分支是普通目标命中伤害数字显示+延后提交，不是全死亡/脚本完成。原特殊25/28/29事务算术仍在，但其独立多次数字恢复与死亡淡出完整呈现尚未全覆盖，不能将一次普通hit外推特殊动画已完成。

真实HIT并非从同起点为命中重跑：从上一轮真实MISS的持久结果`out\skill94-input\cast-after.sav`继续，仅执行一次正常输入；未设置seed/筛roll。此时原角色11已支付后AP不足，正常驱动按已有合法门控选下一位原active且原已学94角色16（非reserve32），原位置22,81、原现地普通敌role0。取消后重新确认是原既有cancel验收，不是重掷；最终HIT chance50/roll32、damage1，目标实际HP减少1。actor16 ST230→225/AP72→27、熟练2800→2830、XP60→61、MP400不变、level60不变。数字完成后保存，500ms idle无重复。SDL=windows，Presented359，独立15帧后数字恢复完成日志`out\skill94-feedback-input.log`；证据`out\skill94-feedback-input\cast-before.sav`/cast-after.sav/cancel-before.sav/cancel-after.sav/settled-after.sav/sequence.csv/frame-*.bmp。旧MISS证据目录保留未覆盖。**MISS是原角色11，HIT是续档原角色16，两者不是同一次结果，不冒称角色32或击杀。**

新增`original_skill94_feedback_test`：固定300 damage验证step0、暂停20 tick未present不前进、不发布live HP，至少8帧、无物攻flash、commit仅一次；持久HIT证据核实角色16与原MISS after存档学习位/熟练/HP/ST/位置一致，后ST5/AP45/熟练30，恰一个敌HP减1、cancel/settled逐字节一致。复现资源根`S2NE\out\original_skill94_feedback_test.exe`，不重新掷随机。真实输入复现命令`S2NE\out\x64\Release\envoy-opening.exe --smoke-skill94-input . S2NE\out\skill94-input\cast-after.sav skill94-feedback-input-new`，新运行不保证hit（不能用重跑筛hit），指定新目录保留既有证据。

统一Release/Debug MSBuild exit0，日志out\skill94-feedback-Release.log/Debug；十二专项当前源码编译运行exit0：94 feedback/transaction64/input-evidence/resolution/visual、32/88 schedule、32 input evidence、88 positive evidence、save bridge、promotion transaction、physical combat、physical combat visual。日志out\*-feedback94-compile.log/*-feedback94.log。Windows独立15帧仍PASS；真实SDL命中数字新链已执行。最后HP presentation与source continuity断言变更后两配置重建、feedback专项重编译exit0；没有再次从原起点重复施放求hit。

剩余精确边界：未实现41CA6E同步对话及41FF30触发链的共享dispatcher挂起/恢复，未开放正行数死亡事件/0x10召唤槽/0x40移除。普通HP0死亡数字后还缺完整455048死亡呈现，故本轮不声称真实击杀E2E或完整死亡动画。同步脚本事务后续须在原completion flag→即时对话→41FF30整条链结束后再交452FB1，不能拿queue_original_death替代。本轮无脚本能力新增；交付确证普通HIT+数字延后提交这一新生产分支。reserve32纠正保持，19单人合作仍未实现非排除；仍94受限覆盖、其余93非合作未有此类完整生产验收。未写PROGRESS/README，未改两结局证据，结束无辅助进程并停写。

<a id="94普通hp0：数字→原4550483c0死亡模式→延后单次发布"></a>
<a id="history-v-051"></a>
### 94普通HP0：数字→原455048(3C,0)死亡模式→延后单次发布

重读452FB1与455048 asm147835..148110发现不能复用现故事消失的五帧模式：普通死亡loc4530C5传EBX=3Ch/ECX=0，455195先424873把原masked source用3C、80%映射，再4551B7选择初始weight10、10帧、step10、间隔3 DOS百分秒即30ms；无色参数故事消失才是初始40、5帧、20ms。本轮OriginalMap::begin_disappearance增加显式death模式，复用原AD/B1、脚影、地形与遮挡mask，单独原tint3C/80及10..100十次原palette mix；旧五帧故事模式默认未改，不拿几何/RGBA淡出替代。

Prologue新增单94 death continuation：数字完成且暂存finalHP==0时先begin_skill94_death，不立刻公布corpse/loot/payments。原live目标保持可绘制，Battle专用death lock继续阻挡选择、取消、移动、物攻、脚本扫描；每帧须实际present并30ms才推进，末帧结束解除锁并只调用一次原先暂存publish。main最终actor/target/inventory/RNG/reward/ST/AP及save capture全部在该点发布。普通目标不再数字结束即突然变尸体，亦不调用物攻settle_physical/queue重复奖励。特殊同步死亡脚本/0x10/0x40预检仍拒绝。

新增tests\original_skill94_death_visual_test.cpp实际Windows SDL：原资源普通敌indexed十帧distinct10、weight10..100/30ms；29ms不提前帧。再用真实资源Prologue隔离lethal数字→死亡→单次fixture发布，20次无present tick不公布HP，23次实际present完成整个numbers/death锁链，commit计数1，额外idle不重复。此整链为明确隔离fixture：末回调发布transaction64已生成的skill94-death-fixture.sav事件副本，**不是自然击杀E2E，也不是在此测试里重新调用完整main菜单事务**；对应必杀/loot/库存99/kill-loss/一次支付/RNG原子与原save往返由original_skill94_transaction_test固定64隔离回归覆盖。本轮没有正常输入自然击杀，不把上轮damage1非致死HIT当击杀，未修改任何真实验收档造HP0。

Release/Debug原生MSBuild exit0（out\skill94-death-Release.log/Debug）；十一当前源码专项编译执行exit0：94 death visual/feedback/transaction64/input evidence/visual、32-88 schedule、32 input evidence、88 positive evidence、save bridge、physical combat、physical combat visual。日志out\*-death94-compile.log/*-death94.log。集成death测试补充后重新cl与真实SDL执行exit0；复现资源根S2NE\out\original_skill94_transaction_test.exe后S2NE\out\original_skill94_death_visual_test.exe。原Windows SDL截图链没有生成新的自然kill存档。

本轮确证新增生产能力为普通无脚本HP0的专属原死亡十帧与数字后挂起/提交，非全94死亡事件。41CA6E→41FF30同步dispatcher仍未接，本轮没有开放任何正行数事件：completion flag必须在即时对话前、41FF30整条触发链必须在452FB1继续前完成；现queue不是等价，不以“设计”名义开放。未知脚本/移除/召唤持续精确拒绝。特殊复活/反疗多段反馈仍缺独立完整呈现。原reserve32资格纠正保留；旧角色11 MISS及角色16 HIT证据未改，两结局hash未改。未写PROGRESS/README，结束无本轮辅助进程并停写。

<a id="94同步死亡：真实原对白挂起恢复（严格受限，不等于完整dispatcher）"></a>
<a id="history-v-052"></a>
### 94同步死亡：真实原对白挂起/恢复（严格受限，不等于完整dispatcher）

本轮接通最小确证链：数字结束→41C02A completion flag先置位→Prologue原prepare/分页/逐字/输入advance对白→41FF30可证明无操作扫描→返回452FB1原死亡十帧→原暂存奖励/支付/库存/RNG一次发布。没有使用script_queue，也没有跳过对白输入。death record按原保留type3；重复begin拒绝、tick/cancel不能跳过、尾resume仅一次。正行数死亡目标的选择/提交能力预检复用同一OriginalDeathDispatch::continuation。

严格纠正预检：41C33E每个扫描位置还调用41B9A3；41B5DF接受type4..16，不能只凭+3旗标未激活就认定无副作用。因此本轮仅接受整张当前event表无type4..16的正行数死亡，任何此类记录均保守拒绝（含type9/cond0，尚未实现其嵌套扫描顺序）。41FF6E对completion1/3有原显示、7有原例程，均拒绝；其它completion在该routine无操作。无脚本/首空仍保持原早退，不擅自执行41FF30。此前候选stage2/15含其它dispatcher指令，已改为拒绝，未以其假称安全。

新增original_death_continuation_test真实资源fixture：stage6 role104首匹配一行、completion0；stage82 role147一行、completion13。后者验证13在对白开始后且恢复前已置1、死亡record未消费、链尾仅回一次。draw/advance使用原Prologue，不是正常存档自然击杀E2E。stage1 role90与stage15 role202拒绝且flags/锁/return计数不变；重复begin、无present多tick/取消不跳过覆盖。原资源扫描器tests\original_death_event_scan.cpp及out\death-event-scan.log提供选链证据（扫描器仅诊断，不改变资源）。

中途原sav策略为禁止写半事务，不序列化continuation：original_save_boundary_ready在数字/同步对白/死亡呈现期间false；main原保存/读取提交与autosave均门控，SDL quit期间拒绝跳过；完成后恢复保存。prepare任意后续行解析异常会恢复events baseline/flags、清pending callback及锁并抛出，不继续奖励支付。外部强制终止不写新半事务存档。尚未建立复杂脚本中途可恢复sav协议。

验证：原生MSBuild S2NE.sln /p:Configuration=Release及Debug /p:Platform=x64 /m /nologo /v:minimal均exit0，日志out\skill94-sync-Release.log、Debug.log。当前源码cl C++20编译、资源根运行以下11项均exit0：original_death_continuation_test、original_skill94_transaction_test、original_skill94_feedback_test、original_skill94_death_visual_test、original_skill94_input_evidence_test、original_skill_schedule_test、original_skill_input_evidence_test、original_skill88_positive_evidence_test、original_save_bridge_test、original_combat_test、original_combat_visual_test；日志out\*-sync94-compile.log及*-sync94.log。原事件专项original_events_test --finish-facing-only exit0。

完整original_events_test本轮执行exit10：Automatic battle cycle branch0 turn4 expected4 campaign_modal1，不能列作全绿。已用out中的隔离prologue头副本禁用本轮prepare异常包装与advance同步分支重新编译原同一测试，仍同处exit10（out\original_events_baseline.log）；确认此失败不依赖本轮同步执行路径，未放宽测试/生产门控。隔离头已清理，保留复现日志。未声称完整原事件套件通过。

未完成：41C33E type9cond0嵌套扫描及41B9A3 type4..16有副作用链、41FF6E 1/3/7原呈现；下一最小范围为独立实现41C33E每记录后41B9A3调用栈continuation，而不是预展平queue。未取得正常输入自然击杀E2E，既有角色11 MISS/角色16 HIT与两结局档均未改。94仍仅受限生产覆盖；其它93非合作技能与19单人合作原缺口保留，不称全94目标。未写README/PROGRESS。

<a id="94同步dispatcher：41c33e→41b9a3逐记录调用栈（本轮有界扩展）"></a>
<a id="history-v-053"></a>
### 94同步dispatcher：41C33E→41B9A3逐记录调用栈（本轮有界扩展）

OriginalDeathDispatch::Stack现在保存death/41FF30-repeat/41C33E-flags/41B9A3-generic原调用帧、逐记录index/返回PC/本次change返回值。每次next只执行到下一条实际对白，原输入结束后从原PC恢复；不是预展平对白队列。41C33E每个记录（含不匹配）后进入41B9A3；41B9A3成功type9/cond0处理后进入嵌套41C33E，再返回原generic索引，最后返回外层scan。依据asm47512..47750，41C33E只有合法completion置位才返回change=1，不把仅消费记录且completion0当作1。41FF30按原返回值重扫至0。依据420A3B cond0的确定分支，generic在合法trigger且flag>0时返回true；其余type4..16或非cond0 type9继续整表保守拒绝，不按未激活旗标猜安全。41FF6E completion1/3/7继续拒绝，其它无操作分支可用。

共享能力预检用副本完整运行同一Stack并验证原line资源，只保留entry索引，不保存预展开执行顺序；实际Prologue另建live stack，completion仍在death对白前、flag对白后complete。原prepare/advance分页与输入锁继续复用。advance新异常边界同时保护stack.next中的后续complete/资源异常，恢复完整events baseline（含原event bytes/flags）、清callback/stack并解锁；不返回452FB1、不发布支付。原save/input/quit门控沿用。

真实资源fixture：stage11 role147 event1 completion13、event2 type9cond0/trigger13/空行/completion11、event5 type9cond0/trigger13/三行/completion14。**整张真实stage11另有currency等未支持handler，生产仍拒绝**；隔离测试仅把其它event的type状态设0，原保留三条目录/对白/条件/旗标字节均来自原资源，不是正常E2E档。精确验证death event1先显示且flag13=1；外层到event0后的generic扫描消费空event2并置11；event5在深度5的repeat→flags→generic→flags→generic调用帧暂停（event2成功后先嵌套扫描，再进入下一generic），type仍9且14尚为0；三行完成才type0/14=1；death record仍3，整栈仅return一次。Prologue原draw/advance完成同链、取消无跳过/save锁验证；在嵌套暂停后人为破坏运行stage的隔离故障测试触发原index异常，确认flags/event状态恢复、return0且解锁。未修改任何真实验收sav或资源。

资源扫描stage1..156当前初始flags没有找到无需隔离就有多对白nested链的安全死亡目标；不可把隔离stage11当真实自然击杀。已支持原stage6/82无副作用死亡对白仍PASS；复杂stage1/15和完整stage11预检无旗标泄漏。新增能力为实际嵌套type9cond0调用栈，不是开放全部死亡脚本，普通自然击杀E2E仍未验。

验证原生命令：MSBuild S2NE.sln /p:Configuration=Release（及Debug） /p:Platform=x64 /m /nologo /v:minimal，均exit0；out\skill94-stack-Release.log、Debug.log。当前源码C++20 cl编译及资源根执行12专项exit0：original_death_continuation_test、original_skill94_transaction_test、original_skill94_feedback_test、original_skill94_death_visual_test、original_skill94_input_evidence_test、original_skill_schedule_test、original_skill_input_evidence_test、original_skill88_positive_evidence_test、original_save_bridge_test、original_combat_test、original_combat_visual_test、original_events_test --finish-facing-only；out\*-stack94-compile.log/*-stack94.log。完整events既有对照失败边界不变，本轮未重复跑全套、不声称全套通过。未写PROGRESS/README，未扩其它技能。

<a id="2026-11-11-当前技能事实纠正（覆盖下方旧推断）"></a>
<a id="history-p-001"></a>
### 2026-11-11 当前技能事实纠正（覆盖下方旧推断）

技能33按实际表为《詛咒術》、effect55、敌军目标，不是《治療術》。撤销旧文将其视为治疗及据此沿 `46618D(effect1)` 建立的完成／验证推断；effect55正向调查链为 `4654A6 → 466544 → 467625`，含 `471B2C` 阻抗处理，仍需完成生产接线与验证。既有包装、拒绝测试及动画资源可读不代表原效果已实现。当前独占实现任务聚焦33号诅咒，不扩展为另一治疗技能；未完成施法验收。

<a id="当前状态：非技能画面、菜单／关间、战斗／ai与死亡回归已收口，进入技能逐项补全（2026-11-11续作）"></a>
<a id="history-p-003"></a>
### 当前状态：非技能画面、菜单／关间、战斗／AI与死亡回归已收口，进入技能逐项补全（2026-11-11续作）

最新源码普通43BF9F→43CB4A已实际接入前向／反击多目标context、连击／秒杀、共享RNG、反向tactics快照、AP一次消费、整组一／二次动作后分目标反馈、状态尾／装备proc、458A6F熟练度、反击后前向XP合并与119奖励、死亡装备25／28及活目标29补充。第1《第一神劍》／71《惡兆》各60seed受控原角色交换，双方死亡／存活反击／连击、反击不扣AP、反击奖励先于前向、同步／动画两端HP和RNG完全一致、反击失败整roster／AP／RNG回滚且外部死亡／奖励不发布均通过（combat-exchange.log）。combat core／visual／save-bridge重编通过。459227已接AppCampaignAdapter暂存整批原掉落库存，失败整交换rollback，原重复掉落／cap99／敌军不收／损坏拒绝专项通过。新增452FB1→41C02A同步死亡暂停交换，先原死亡脚本后455048十帧indexed消失再续反击／奖励；第1《第一神劍》普通无脚本死亡和第6《沙盜營地》原104死亡脚本，真实普通输入但受控接敌／属性fixture均29绘制帧通过，整交换拒绝保留flags／事件／镜头回滚。原死亡输入第6《沙盜營地》／82《惡德商人》重编通过。仍未宣告战斗AI收口：status40移除／召唤回收及完整死亡分支、所有实际可触发事件待核；本批修正死亡续作对事件表的边界保护，生产链在第6《沙盜營地》/第82《惡德商人》普通死亡对白最小回归可推进，完整死亡回归已拆分并通过：第6《沙盜營地》104、第82《惡德商人》147、stage33货币/后续事件、stage11嵌套generic/flags、输入锁恢复与无副作用拒绝均通过；失败上下文曾确认为stage82 event=5 count=2的fixture把跨stage事件索引带入本地事件表，已改用同一原事件表合法记录并保留生产边界校验，未把受控样本外推自然杀敌。最新Debug→Release统一成功，根最新Release与out\x64\Release\S2NE.exe同SHA256 `AFD7A2908D0D6E55EBB4EDBADC25F066F94E4374D13E0FCCC2C883AFC8FF8D61`，正常140开场→第1《第一神劍》自动友敌动作→玩家恢复真实SDL1393呈现帧通过；不是最终包，未打包。status40／10特殊死亡已接生产：保留原角色槽、id/equipment/inventory和脚本索引，按4531AB提示等待、454ED5色254十帧40ms离场，仅清status80；status10按appearance扫描对应军团11×3槽，仅清首匹配active字节并保留其余槽字节，结束后桥接保存status33字节及军团metadata重算。AppCampaign掉落仅对仍status20死亡目标提交，避免离场误收。原第1《第一神劍》／第6《沙盜營地》普通死亡回归、Release构建及根--smoke-auto通过；已完成原关素材扫描：第43《誰是王者》、第44《奧帕兒女王》、第54《力量與魔法》、第56《和平會談》、第126《烽火議和》加载出原behavior=4/status=C0角色（分别appearance 94/133/91/91/86）；这些是原关脚本角色初始化，不是可被普通战斗选中的敌军，实际物理接敌探针在第43关明确无合法接触，因此不能伪造自然战斗触发。status10也未在1..200关初始化角色中自然出现；其唯一自然来源是原召唤/军团脚本尾状态，需由对应事件链进入，不能注入旗标。已将边界记录于out/status40-natural.log；生产实现继续保留原触发路径。下文“尚只单向／未接生产”是旧调查状态，由本段替代，不作当前结论。

最新稳定检查版已恢复为本批Release，Debug→Release均成功，根与Release输出同步（SHA256 `6D82E109F0585E6798A917B4DBACAB92BDB4C9EA4C185E00502F998F338DF8B4`）；根`--smoke-auto`正常140开场→第1《第一神劍》友敌自动行动→玩家恢复真实SDL通过（最新1400呈现帧）。同链409BBA预估地形／穿防／免疫与AI命中runtime状态缺口已修，第15《沙漠名珠》原turn4专项仍42／32，受控免疫预估0／runtime字段优先断言通过。不是最终包交付，未新打包／独立解包。新增4093DE反击minimum1／6／16／46原阈值算法修复及边界断言；408E6F／408E8C普通生产传入status39／3C DWORD／3D，免伤与动画重入不重复扣AP受控fixture通过。前向伤害抽取已保行为提取，完整43BF9F→43CB4A多目标context／共享RNG／反向tactics快照／装备死亡补充仍未接完，不宣告战斗AI收口，不跨死亡或技能。菜单effect61／67与整app鼠标实证保持原边界，未移出清单。详[分项验证记录](#validation)。下文此前“根为Debug”属于历史，被本段稳定Release替代。

菜单本轮生产清单已验收：旅店410EEE十入口（探索、传闻、读写档、装备、人物、物品、标题确认、设置、自动装备），商店411587买卖／数量／对象／不足资金／取消，教会41430B复活及413F9B→44ACAD捐献，战场／关间菜单和既有存档锁。新增5／7不再是blocker。`menus-production-sdl.log`为最新Debug整app --smoke-shop：真实SDL事件队列鼠标进入5／7、16呈现阶梯期间Escape锁定、取消返回，8标题取消／肯定，原库存商店买卖及装备／保存往返全部通过；教会死亡／资金及12名单滚动明确为受控fixture，不称自然取得。`menus-army-equipment.log`五原SAVE第65《龍穴》、115《小丑奪權》、1《第一神劍》三档，双资源域原旅店入口／返回、实际327次SDL present、两档库存装备卸下事务通过。原44D675核对修AP用+12C最大值、末行原+C5、防隐名bit10；456EEA装备名固定233；ITEMHLSP缺失／截断在create返回Result，不发布页面。settings、reports、status、inn、shop、church、interbattle、battle-menu及item-use/item-shop专项均通过。effect61／67已沿450F77／451072／45154D逐字段核实，67保留WORD加法再cap；目前无自然283／296库存或商店offer，仍保留自然覆盖限制，不伪造。本轮菜单收口不是全部物品自然流程／原EXE录像逐像素认证。

战斗／AI当前完整有限清单已逐项复核：①原438EC3选人／442B08资格/AP/移动取消；②普通441B15范围、43BF9F→43CB4A多目标连击/反击、共享RNG/AP/逐present与失败rollback；③409BBA装备触发、40A4B3回避、442F74状态尾/回合保持；④40B0BA→40AED5→40D171普通扫描、AI6/7目标、不可达fallback、40D25A追击与40CFE3低血撤退；⑤status40生产离场与status10召唤槽回收；⑥死亡同步→奖励/掉落/XP/熟练度顺序；⑦输入锁、栈/动画不可重入、SAVE与RNG rollback；⑧自动友敌→玩家恢复。第1《第一神劍》、第6《沙盜營地》、第15《沙漠名珠》、第71《惡兆》均有正常或明确受控证据；effect39技能专有排除。核心定向测试与Release第1《第一神劍》SDL通过；battle_architecture/阶段15 tactics工具失败属于资源根/地图sprite fixture错误，不降低生产校验，仍需修正测试资源入口后才宣告类别收口。新重编combat/AI/AI-special/stage15-value/equipment/retreat/battle/special-action/stage15-actions/random/promotion-transaction全部通过；`combat-production-auto-sdl.log`正常新游戏140开场→第1《第一神劍》原自动菜单→友方／敌方可见动作→玩家恢复，499帧通过。第15《沙漠名珠》特殊动作／状态和AI6/7隔离测试保留受控边界，不外推自然触发；已确证普通连击／反击不是仅缺证据：Battle::strike尚只单向physical_feedback，409A2D候选与4096F3／40976F／409060算法未接43BF9F→43CB4A完整生产结算，本类未收口。已完成同链真实字段修复409BBA按动作武器type取+D7..E1／special+F9而不是固定proficiency[3]，以及40A4B3装备evasion扣减接实际攻击与AI预估。第1《第一神劍》／15《沙漠名珠》／71《惡兆》158动作字段与六武器契约fixture通过，battle／special-action／stage15-value改后通过，MSBuild ClCompile通过；第15原turn4两候选命中值按装备修为42／32，不隐藏旧45／35期待失败。尚未把不完整交换草稿留在生产，后续沿原43CB4A完整反击与逐present结算继续。死亡剧情尚未开始本轮复核。四类全部收口后依用户新授权自动继续115技能（96非合作／19合作），不得提前用注入习得冒充自然入口。完整全流程复验仍后续。最新Debug构建为SDL专项已暂发布根，**根当前不是最终Release交付**；尚未打包，最终统一Debug→Release／根同步／独立解包验收仍待完成。以下旧状态保留为调查历史，以上述新验收为准。

本轮**非技能画面清单验收完成**，不是全技能动画／全战役／原EXE录像逐帧认证：普通选人／范围／路径／移动取消与确认由新重编`visual-battle.log`覆盖；角色／地形／前后遮挡及signed帧位移由`visual-map-frame.log`；普通攻击受击／浮字／分段HP及反应由正常第1《第一神劍》真实SDL `visual-feedback-final-sdl.log`；阵营提示九姿态／未present不推进及边缘输入计时由`visual-ui-timing.log`，流水原472D78调色轮转／4637DA映射由`visual-palette.log`；传送第60《返鄉的遊子》／撤离第42《小丑獻媚》生产SDL（受控触发边界保留）；关间／城镇第65《龍穴》滚动／淡入淡出／输入锁／返回由`visual-worldmap.log`；class4第124《力量與魔法》正常购买141／合法移动／取消保存重载及真实SDL帧，非注入状态。class5唯一状态获取effect39界外依赖已列，不继续阻塞非技能画面。具体地址和fixture／正常流程区别均保留本文件分项历史证据，不宣称fixture等于自然流程。

当前进入菜单／关间，未跨到AI／死亡。新增旅店5→410FFE／44D439／44DC93生产页：原可用角色筛选、十角色名单／滚动、六装备类别、五行库存／灰显、原坐标鼠标装备／卸下按钮、取消返回；提交经独立State与bridge重建成功后发布。`menus-army-equipment.log`五原SAVE（65《龍穴》、115《小丑奪權》及1《第一神劍》三档）入口／取消字节不变、两档合法库存装备／卸下精确delta／保存重捕获、五真实SDL帧通过；115原名由实际stage_name Big5 A470A4A1B9DCC576解码为《小丑奪權》，日志保留原字节。新增页尚待名单像素／鼠标边界与app整链复核，不能称完整菜单收口。旅店7已接44F4DB→44FD8B→45024C→450550生产：先当前角色六装备有use项，再库存type12有use项；use2原非战场拒绝，use1原(200,400)字符串80确认，原229／ITEMHLSP说明／角色选择／取消返回与事务发布；未扩技能施放。五原SAVE原有306角色物品组合，168合法提交／138原条件拒绝与保存字节往返通过；第124《力量與魔法》保留强化checkpoint经原商店category3正常购买250／254／257／262→使用→库存减一／保存重捕获通过，无注入库存旗标金钱。尚有未获自然覆盖的effect61／67精确字段、页选择框／时序／整app鼠标链待核；不称菜单完成。以下旧“当前仅画面”文字是调查历史。

- 当前优先画面真实来源收口，未继续菜单5/7、AI或死亡新功能。新闭环：第124关《力量與魔法》保留的原强化战役checkpoint，**正常原商店买入／装备141→原class4判定→合法移动route→生产特殊合成→真实SDL帧→完成动作后取消坐标恢复→保存重载保留class4**通过；未注入物品／flags／金钱。不是未强化全战役E2E，详本文件分项历史证据及`visual-class4-equipment.log`。
- 原class4非技能确有装备special17（物品141）来源，BIN4正常出售城镇55/56/59/62/124..129/133；不再以开场无状态为缺样本终点。class4 status3C bit1来源4668F0技能状态；class5 status33 bit4置位／xor仅45DC37，经466544/effect39，技能界外。普通route/camera共享45CAA0不等于存在独立非技能状态获取入口，不造起降规则。
- 本批仅新增针对测试／证据，不重复全工程构建打包；生产根与包维持上一稳定Release。画面本轮完整清单仍以下表逐项为准，不把新链一个frame当全部画面全等价。

资源和存档本轮验收保留，不重新误报低层bridge限制。执行次序固定画面→菜单／关间→战斗／AI→死亡及剧情。画面本批有限生产清单及未覆盖证据已记录；现进入菜单／关间，不删除自然class4/5样本不足与原版录像未逐帧对照边界。

菜单有限清单：旅店410EEE options1..10（原入口、资格、取消返回、SAVE无意外变化）；商店411587买入／出售／数量／装备对象／钱不足／确认回滚；教会41430B复活与413F9B／44ACAD捐献；战场／关间人物页、装备物品页、设置与标题返回。旅店5→44D439(mode0)→44DC93→44EA2D／44EC85、7→44D439(mode1)→44F4DB已在本次接线，针对证据见顶部与本文件分项历史证据；不再沿用“仍未实现”旧结论。8→41104F／43183B标题确认本批已接生产，65《龍穴》正常鼠标入口／Escape取消／Enter肯定、完整SAVE字节不变专项通过；app_runtime肯定后释放当前局面并返回标题。尚未专项真实SDL点击此新增分支，不能称菜单类收口。

画面有限完整验收清单（当前实核，不等于全通过）：

|项|原链／正常入口及条件|取消／失败／证据要求|状态|
|---|---|---|---|
|地形／物件／角色遮挡及选中合成|42AFBA／42B1D2／4468DF；正常战场draw／选人／目标，原PRG signed frame displacement|镜头边界、前后军团、索引destination及RGBA一致，错误不发布buffer/cache|新重编map_frame通过1《第一神劍》／71《惡兆》122帧，尚核其余生产调用|
|普通／特殊移动class0..5|452450／42B327／448B99／45CA1D／45CAA0／45CADA；正常路线、确认返回、撤离调用|逐present／120ms／19cs、取消回滚、原raw图元；65《龍穴》class3/4/5受控fixture不得称自然来源|已有实现，当前逐项重新验收|
|传送到达／消失／镜头|454ED5／455048／42B461；侧末原传送格及死亡／撤离|输入与保存锁、完整呈现、结束一次／失败回滚；60《返鄉的遊子》、42《小丑獻媚》分别标受控范围|已有生产，需本类针对回归|
|伤害／MISS／反应／目标独立|45345F／45351C与原反馈链；正常攻击进入|原indexed B1／AD、八次扣血、未present不跳、结束恢复；非技能普通攻击|算法／renderer已有测试，需正常SDL核对|
|城镇／关间淡入淡出与返回|414D5B／415415／4157C6／472EF1；65《龍穴》旅店探索／商店|持续滚动、帧时序、区域门控、正常返回不改SAVE|已实现需重编相关针对测试|

技能专有45DC37仅466544/effect39 caller已确证，属于下一轮技能，不借此排除普通移动／反击／状态／装备。画面本批新增原选中／hover／空地光标层及取消专项（battle_test）已通过；只读扫描五SAVE及1..158原关开场全军团未发现可见class4/5，不注入状态伪称自然流程。现有非技能画面清单生产接线与针对验证通过，class4/5自然状态来源仍无样本，不宣称全原版逐帧认证。后续进入菜单时不以此删除未覆盖证据；其它三类未收口。

<a id="当前缺口总表（2026-11-11技能阶段，优先于下方历史）"></a>
<a id="history-p-010"></a>
### 当前缺口总表（2026-11-11技能阶段，优先于下方历史）

已完成的边界：两条独立强化路线到61／131《超時空傳說》及完整视觉结局、实际DAT换盘、主要自动档重载；不等于原数值全流程、全部剧情分支或全部技能完成。

| 范围 | 当前尚缺／未验收 |
|---|---|
| 技能与剑术 | 原表审计确认为115项：96非合作、19合作，11组门槛／关系真值表通过。当前生产批次已覆盖32《高溫氣化》（第61《礦坑的秘密》正常输入、学习物品、MP/AP、装备移除、XP与保存回读）、88《正療傷》（第60《返鄉的遊子》角色2严格地图域资源夹具，阈值90、ST25/AP、35帧及失败rollback）和94《神劍技》（第131《超時空傳說》已学习角色32的15帧视觉／恢复子链；公式、MISS/HIT和反馈／死亡结算有专项，但仍标明非完整正常施法E2E）。历史曾把23帧/50ms的CEL第33项视觉证据记为技能33《治療術》，现撤销该归属：那是技能32的visual32资源，不构成技能33视觉生产验收；技能33对应CEL第34项21帧，仍需实现与契约映射。其余92项非合作及19项单人合作仍未达到逐项生产验收，拒绝或直接授权样本不计完成。技能33《治療術》（第60《返鄉的遊子》actor4→target202）的当前缺口为CEL1原资源到NativeEffect55运行时字段／呈现契约的实现与映射，仍严格拒绝、不计完成；此前因EXE字符串及lseek推断缺外置 `spellsp` 是误判历史，已由473CDB实际打开SUPERSP.CEL及组相对寻址证据推翻。新增技能状态表按“生产闭环/受控证据/实现与契约映射缺口/未实现”区分；当前除32/88/94外没有另一项达到完整生产闭环，不能为了批次数量把静态CEL视觉或审计样本计入完成。 |
| 94死亡与特殊效果 | 普通死亡续作回归已全绿：第6《沙盜營地》104、第82《惡德商人》147、stage33货币后续、stage11 generic/flags嵌套，输入锁／事件栈／prepare失败rollback通过。完整自然战役与技能专属effect39仍未全覆盖。 |
| 战斗与AI | 有限清单已逐项复核并通过定向测试：普通范围、连击／反击、共享RNG／奖励顺序／rollback、装备触发／回避、状态回合、AI6/7与fallback、低血撤退、status40/10、死亡掉落／XP顺序、输入锁与自动友敌恢复；第1《第一神劍》、第6《沙盜營地》、第15《沙漠名珠》、第71《惡兆》均有正常或明确受控证据。完整战役与技能效果不外推。 |
| 关间与完整菜单 | 旅店／商店／教会全部下级交易、人物详情、条件撤离及完整菜单树仍需逐条核实；不能因双线路线可走通视为所有可选菜单完成。 |
| 剧情与平衡 | 已验证各一条独立强化路线；正常属性连续通关、所有可选分支／未走中间剧情／特殊事件未全验收。强化不改学习／物品／旗标，但不是原数值平衡认证。 |
| 画面时序 | 传送／起降／特殊移动、部分遮挡与完整特效合成、淡入淡出及原版逐帧／实际手感比对仍不足；32／88／94专项不能外推所有动画完成。 |
| 存读档与回归 | 全部特殊状态／完整嵌套剧情的半事务恢复未覆盖，当前相关半事务禁止保存。完整original_events_test第4回合campaign_modal失败已隔离对照同点复现，尚未消除；针对专项通过不等于全套通过。 |
| PRG与打包 | 原安装模式固定根PRG符合现CFG，不能将其定性为换盘错误。双PRG前8组相同，第9组／尾部用途尚未完整解析；包暂保留两份不同文件，是否可再精简待证。独立包启动通过，尚未在全新无开发环境机器验运行库。 |
| 排除／暂缓 | 仅双人对战明确排除；音频完整还原暂缓，单人合作技能不排除。 |

详细原地址、具体通过／失败命令、截图与存档证据见本文件分项历史证据。下方旧“未通终局／未实测换盘”等已被更新证据替代，不再是当前阻塞。

<a id="2026-11-11-画面基线：暂停技能，整数呈现首批"></a>
<a id="history-p-030"></a>
### 2026-11-11 画面基线：暂停技能，整数呈现首批

技能补全暂停。本批仅修窗口呈现：原固定640×480帧缓冲原先在默认960×720窗口以1.5倍LETTERBOX显示，即使NEAREST也产生交替1/2像素宽；现统一INTEGER_SCALE、NEAREST、NONE framebuffer blend，并检查SDL错误。960×720视口为(160,120,640,480)，1280×960为整2倍，1000×800居中1倍。未改资源/PAL或发明半透明效果。

最小固定场景为标题DAT19（非关卡）；SDL软件renderer逐像素核对原资源RGB、整数像素块及黑边，三尺寸截图成功；生产app_runtime.cpp C++23编译成功。当前没有可用DOSBox参考截图，不能称原版逐像素对照通过。主菜单半透明、对话框纹理/头像/姓名框、战场地面栏、高低差/遮挡和动态present节奏仍待分别对照；本批不代表画面类收口，不打包、不替换根Release。

<a id="combat"></a>
## 普通战斗、AI与死亡

有限清单包含43BF9F→43CB4A交换、共享RNG、反击、掉落／奖励顺序与死亡挂起。status40／10等受控样本不替代自然事件来源；不同版本的缺口／修复按记录日期解释。

> **历史区域**：下方各原标题仅表示当时状态，不能覆盖[最新状态](#current-status)；误判按[撤销表](#corrections)阅读。

#### 历史原证据区域（保留原标题便于追溯）

#### 2026-11-11 04:27 death continuation split: out\\death-continuation.log exit0. Failure context was stage=82 event=5 count=2: test fixture changed loaded stage to82 while nested stack belonged to stage11, causing cross-stage event index. Production now reports stage/event/count in Result errors, validates Stack event-count and next event bounds, checks try_line, and rolls back failed prepare. Same-table stage11 generic/flags nesting, stage33 currency follow-up, stage6《沙盜營地》104 and stage82《惡德商人》147 pass; input lock and stack resume verified. No skill or package work started.


#### 2026-11-11 04:34 battle/AI full finite checklist reviewed. Root-run targeted results: original_ai_test, original_ai_special_test, original_retreat_test, original_stage15_ai_value_test, original_stage15_actions_test, original_campaign_persistence_test, original_physical_death_test exit0; Release --smoke-auto第1《第一神劍》1401 SDL frames exit0. battle_architecture failed only because its test calls Battle::load with merged resource fixture from wrong root (Invalid merged resource magic/version/type); original_stage15_tactics failed Invalid original map sprite index in its generated map fixture. These are test-resource setup failures, not production acceptance; no category closure or skill entry until corrected. Checklist includes ordinary attack, combo/counter, AI6/7 target/fallback, retreat, status40/10, equipment/evasion/status turns, death loot/XP order, locks and rollback; effect39 remains skill-only exclusion.


#### 2026-11-11 04:39 battle/AI infrastructure fixed. 
ead_resource now resolves project-local resource paths to the workspace merged S2NEPK03 bundle (current SUPERSP.DAT/PRG), preserving strict magic/version/type validation. Rebuilt merged resources from archived Windows sources: DAT SHA256 24debf1d..., PRG SHA256 b402d81e.... attle_test rebuilt and passes. Stage15 tactics fixture now runs with legal original map sprite indices and resource root; strategy14 completed production actions until its bounded 20-turn harness limit (no Invalid sprite index). Full targeted suite rerun from workspace root: battle_test, original_ai_test, original_ai_special_test, original_retreat_test, original_stage15_ai_value_test, original_stage15_actions_test, original_campaign_persistence_test, original_physical_death_test all exit0. Release --smoke-auto第1《第一神劍》1397 SDL frames exit0. Battle/AI production is now finite-list complete; effect39 remains skill-only exclusion. No package yet.


<a id="2026-11-11-续作：普通死亡同步暂停与交换续作（战斗类别未收口）"></a>
<a id="history-v-004"></a>
### 2026-11-11 续作：普通死亡同步暂停与交换续作（战斗类别未收口）

- 原452FB1在HP反馈后调用41C02A，再455048消失，完成才允许反击／奖励；现普通交换不再仅末尾queue：死亡pending暂停exchange，Prologue调用现有原同步Stack，present门控十帧indexed消失后单次resume。不重抽伤害／反击池、不双重XP，不把已死亡HP=0直接交给要求活姿态的bitmap捕获；单独只绘制姿态副本保持原HP状态。
- 新`tests\original_physical_death_test.cpp`与`combat-death.log`：第1《第一神劍》原普通死亡、第6《沙盜營地》原104死亡脚本完整原事件表，经普通选人／strike／绘制present／输入推进／消失／交换续作，各29帧通过；明确受控接敌、HP及攻防战术，不是自然杀敌E2E。既有`combat-death-input.log`第6《沙盜營地》、82《惡德商人》完整原表同步输入／锁／返回重编通过。
- 同步失败恢复整交换roster／RNG，Prologue恢复flags／event state／镜头，清理同步锁；直接生产strike镜头优先moving actor并验证范围。status40／10现已接入生产移除、召唤槽首匹配回收、Save.Group.status桥接及军团metadata重算；自然关卡触发样本仍需按原脚本边界说明：扫描原第1..200关发现第43《誰是王者》、第44《奧帕兒女王》、第54《力量與魔法》、第56《和平會談》、第126《烽火議和》各有原behavior=4/status=C0角色，但其初始化角色不是普通战斗敌军；第43关正常物理接触探针无合法目标，不能把受控接敌冒称自然触发。status10未在原关初始化角色中出现，仅由原召唤/军团事件尾扫描生成，当前无未注入的自然SAVE样本。证据`out\status-role-scan.log`、`out\status40-natural.log`；保留生产脚本路径，不以写入status旗标替代自然证据。
- `status40-release-build.log`与根`status40-smoke.log`通过：正常140开场→第1《第一神劍》友敌自动动作→玩家恢复1383呈现帧，根Release SHA256 `AB903E7A591C7EA1D71DC664D6126F7CF991E3383C06DADEBAD4E8247411C56E`与out\x64\Release\S2NE.exe一致。不是最终包，未打包。

<a id="2026-11-11-续作：普通交换生产接线及定向事务验证（战斗类别未收口）"></a>
<a id="history-v-005"></a>
### 2026-11-11 续作：普通交换生产接线及定向事务验证（战斗类别未收口）

- 原43BF9F全部40840D context先于伤害，方向格序附带目标后primary；43CB4A复用原primary context不重新读取／抽反击池；409060反向快照、4096F3／40976F连击，整组一次／任一连击两次动画，之后状态和逐目标HP反馈，AP仅前向扣一次。普通攻击已不是单向分支。
- `out\combat-exchange-build.log`／`out\combat-exchange.log`最新exit0。第1《第一神劍》／71《惡兆》各60seed原角色受控属性fixture覆盖存活反击、前向杀死不反击、反击杀死原攻击者无前向奖励、连击、反击不扣AP和奖励先后；明确修改HP／攻防／战术值，不冒称自然取得。相同seed的同步与动画交换双方HP、RNG seed／calls完全相同。
- 受控动画dispatch后把原攻击者坐标置无效，迫使反击range Result拒绝；全roster坐标／HP／AP与RNG恢复原baseline，外部死亡和奖励callback均0，diagnostic清空。错误路径延迟外部死亡发布避免污染剧情队列；这不是原自然死亡脚本时序完成证据，仍需后续检查。
- 43D93B清3Ebit2、3Dbit80百分比解除、状态免疫、普通装备／能力proc、MP／ST吸收、动作fields17/18/19尾；458A6F六武器／特殊熟练度一次且cap1500；25／28复活与原451D28第一匹配装备消费、29活目标治疗补充已生产接入。反击前向奖励分组累计，前向XP在全部反击之后计算，原119奖励字段新增桥接往返。
- `out\combat-core.log`、`out\combat-save-bridge.log`重编exit0，覆盖原伤害算法、反馈和原档兼容；未重跑强化战役，未打包。后续459227已接AppCampaignAdapter整批原掉落库存暂存提交，任何非法id拒绝整交换，无外部死亡／XP发布。`combat-loot.log`原重复247／248合并cap99、非法303不部分写入、敌军不收；`combat-feedback-signed.log`负伤害分段治疗及maxHP约束通过。`combat-exchange-debug-build.log`→`combat-exchange-release-build.log`均0，最新根Release SHA256 `67CCEB7C6208B18B34D64A624C81DE60C773C99B900F2FFDF0584F226FAA8C13`；`combat-exchange-auto-sdl.log`正常140开场→第1《第一神劍》原自动友敌动作→玩家恢复1392呈现帧exit0。未新打包；仍缺452FB1→41C02A在交换中间的同步死亡脚本／消失续作，不将目前末尾队列等同原同步，战斗AI仍未收口。

<a id="2026-11-11-续作：原反击最小值、生产状态与稳定release（类别仍未收口）"></a>
<a id="history-v-006"></a>
### 2026-11-11 续作：原反击最小值、生产状态与稳定Release（类别仍未收口）

- 原4093DE→409442在407C35后仅伤害小于1才抽minimum百分比，阈值为1／6／16／46，对应5／4／3／2／1。修正`try_counter_damage`原先简单max(1)的缺口，新增9个边界与正伤害不替换断言；`out\combat-counter-minimum-build.log`及`out\combat-counter-minimum.log`均exit0。这是原算法契约fixture，尚不是自然反击生产验收，无关卡流程宣称。
- 原408E6F的status39 bit2／3C DWORD 2A000必中，以及408E8C的status3D bit8免伤，原先HitChanceInput已有算法但普通strike未传运行时字段。现在以action_status_runtime优先、Unit字段后备传入。原伤害抽取提取为`draw_forward_damage`，保留现有AP和反馈顺序，供后续交换使用；不能将提取当连击接线。
- `out\combat-forward-extract.log`重编battle_test exit0，新增原type5范围受控fixture免伤实际攻击／呈现结束HP不变，以及动画windup直接重入拒绝且不再次扣AP。此夹具沿既有测试扫描原动作／装备并受控摆位和属性，明确不称第1《第一神劍》自然得到对应装备或免伤状态。
- 为避免根长期停留Debug，本批稳定后Debug→Release两构建均exit0，日志`combat-stable-debug.log`／`combat-stable-release.log`；根已同步本批最新Release。根Release `--smoke-auto`实SDL正常140开场→第1《第一神劍》自动菜单→友敌动作→玩家恢复通过，`combat-stable-release-auto-sdl.log`记录1392呈现帧（帧数受呈现计时影响，非固定499要求）。未强化、未注入库存或flags。尚未生成新包／独立解包，不宣称最终交付。
- 同链409BBA→40A0D4原预估还有真实缺口：physical_action_preview先前未填地形、穿防3C／118／kind及118 bit1／3D bit8免疫；现已传入，AI命中同样传runtime39／3C DWORD／3D。`combat-context-ai.log`第15《沙漠名珠》原turn4 actor18→202仍为42／32，`combat-forward-extract.log`受控type5免疫预估0及runtime原始数组优先／必中99后免伤断言通过。最新再Debug→Release与根SDL复验exit0，1400呈现帧；音频设备本次不可用已按既有Result降级继续，音频不纳本轮完整恢复。根与Release SHA256同为`6D82E109F0585E6798A917B4DBACAB92BDB4C9EA4C185E00502F998F338DF8B4`，以此覆盖上一1392帧中间检查版。
- 43BF9F前向连击是先408CB7、4096F3、4095B5，再可选第二次408CB7相加，之后动画与分段HP；43CB4A反击仅原攻击者用409060／40976F，附带目标用408CB7／4096F3。409060使用前向保存的调整后tactics3C，不能重新以反击当前HP调整，也不能调用普通strike冒充。多目标context预生成／共享RNG顺序、装备死亡补充43C820等与剧情挂起仍须完整生产实现；未留下临时反击草稿，战斗AI不收口，不越到死亡／技能。

<a id="2026-11-11：战斗／ai实核中的确证缺口（未收口）"></a>
<a id="history-v-007"></a>
### 2026-11-11：战斗／AI实核中的确证缺口（未收口）

`combat-current-*`新重编combat、AI、AI-special、stage15-value、equipment、retreat、battle、special-action、stage15-actions、random、promotion-transaction通过。`combat-production-auto-sdl.log`整app正常新游戏140开场→第1《第一神劍》自动菜单→友方／敌方行动→玩家恢复499帧exit0，无强化注入。原AI6不可达回退实际已有40B0BA→40AED5实现，不沿用旧拒绝结论；特殊6/7现测试为原第15《沙漠名珠》受控kind／无对手fixture，不冒称原关自然触发。

真实生产缺口仍有：`Battle::strike`当前只单向物理伤害与状态尾；409A2D反击池和4096F3／40976F连击、409060反击伤害虽有算法，尚未接43BF9F→43CB4A完整结算，不能称普通反击完成。正在连续读40840D→409A2D／409BBA→43BF9F→43CB4A，原先全效果/反击草稿未完成契约即已移除，保留验证过的安全生产，不把部分反击假接线提交。

已实修同链409BBA +28熟练度契约：动作type>=21用runtime+F9，否则原武器ITEMSP type选择+D7..E1六WORD，无武器0；原代码错误固定weapon_proficiency[3]。新增OriginalEvents::action_proficiency并接生产命中／伤害与AI预估；`combat-action-proficiency.log`第1《第一神劍》、15《沙漠名珠》、71《惡兆》158原动作字段及另列六武器／special／unarmed区别字段fixture通过；battle/special-action/stage15-value改后再重编全0。fixture只验字段契约，不注入角色冒称自然习得。

40A4B3原装备evasion（ITEMSP+23）原本未传生产命中modifier，现补攻击与AI preview。第15《沙漠名珠》原turn4 actor18→target202两动作159/160预估由45/35修为42/32，保留原命中差异排序；旧精确期待首次失败后按原字段修正并重跑通过，battle／special-action也通过，不隐瞒基线变化。此不是反击完成。

`original_random_test`旧异常期待改为try_bounded缺时钟Result失败且stream不变，再显式时钟恢复；没有新增throw或异常处理。战斗／AI尚未收口，死亡及技能尚未进入；根仍上一菜单SDL的Debug，包含最新proficiency源码的Release／根同步最终交付待统一完成。

<a id="手动交互问题记录（2026-11-10）"></a>
<a id="history-p-016"></a>
### 手动交互问题记录（2026-11-10）

最新反馈：原结束行动为同一个面向箭头＋O/X模态，已纠正ICONSO资源与四方向选中箭头，方向仅左键改变、悬停不改变，确认状态在输入时初始化而非绘制时。原454159模态前自动面向威胁已接入，使用43948D／4394ED有序目标、441020可见性、478AF3距离、45DA52范围及45CE57方向；取消按45369A／453FD6恢复自动调整前方向，确认前AP／状态不变。面向／取消／AP／威胁专项通过，统一Release／Debug及集成后实际SDL战斗短流程（1467敌方行动帧、1970呈现帧）和右键跳过专项（106呈现帧）通过。完整事件回归的NPC先于敌方阶段断言已修正且保留横幅时序／输入锁检查，后续遇第4回合剧情模态边界，未冒充全套通过。开场动画增加用户明确要求的右键跳过，仅在开场播放阶段生效，复用原Escape进入菜单流程，不透传成菜单点击；`--smoke-opening-right-skip`实际SDL专项、战斗短流程及统一Release／Debug通过。

按用户实测反馈，优先核对并修复：移动落点及原立体目标框、攻击目标选择、同类型敌人错误共享受击效果、屏幕边缘镜头响应、回合切换动画间歇缺失、行动确认O图标全红。须依据原汇编／资源恢复，不以强化流程跑通代替手动交互验收。当前已修确认框：原431997→475970(fill=0)仅画选择轮廓，旧实现错误填满28×29按钮；现保留原CEL197的O/X内部像素，按压仍用CEL198/199。新增像素与确认结果回归通过。移动／攻击框已改为原DAT27/28跟随目的格／目标，点击目标按407400枚举而非被首个非目标占位者截断；渲染缓存及受击／消失改用完整roster实例索引，battle、地图帧与消失专项通过，主场景现已接通完整roster索引的受击／消失与单实例闪白。滚屏按原配置最小时间调度，不追补积压、模态期间不滚动，帧末只等待剩余预算；回合横幅九个姿态须经SDL呈现确认后推进，保证首尾帧，不额外增加停留。统一Release／Debug及重编battle、地图帧、消失、O/X像素、UI计时专项均通过；实际SDL dummy短流程验证玩家移动、敌方可见移动与两轮完整阵营横幅（1461敌方行动帧、1961呈现帧）。短流程另暴露最低难度43939D→47736A目标寻路未实现，已按原分支补接并通过专项。第3阶段战斗回归改为隔离合法邻近观察者场景，保持原属性／隐形能力和限回合、清敌、死亡断言，增加隐藏→显现→隐藏检查；这不代表真实整段战役自动获胜。战役自动推进仍暂停，未做原程序录像逐像素对比或真实鼠标手感认证。

新增手动反馈修复：按4372EF→438EC3／442B08恢复原配置SET第四项控制的我方顺序选人，开战目标确认后及新玩家阶段选择首个合格角色并定位镜头，确认行动结束后按队列继续；取消不强制重选，资格按原状态而非AP（含零AP专项）。按43A62D的43A866／43AA0E在图标下方+40恢复当前选项原说明，小字及阴影保留，使用命令54–59／198、攻击60–67与原武器／特殊攻击名称；并非所有图标同时挤入文字。统一Release／Debug、battle顺序选人和原文字资源／像素回归通过；实际SDL短流程验证开场首个角色进入命令态、取消／移动及两轮回合横幅（1958呈现帧）。战役推进保持暂停。

后续流程报告同时记录原资源关卡名与阶段编号，不再把阶段编号称作连续战斗序号；日志中的名称从原关卡资源读取、以Big5解码后输出UTF-8，未命名阶段明确标注。

<a id="最低难度规则复核（进行中）"></a>
<a id="history-p-021"></a>
### 最低难度规则复核（进行中）

已从原汇编发现最低档简化规则在生产攻击／预估中的接线遗漏：方向模式应固定为1，并跳过高度及地形战斗修正；接触攻击预估也漏掉敌侧难度倍率。新专项首次固定目标格不是可攻击地表，导致未覆盖实击；已改用原动作射程枚举合法格。MSVC编译及运行通过1210项预估（1160接触／50远程）与接触、远程各一次实击，验证最低难度height=0／方向mode=1，Release／Debug构建通过。实际第12关正在重测，专项通过不代表通关。当前资源中 `4080E7` 实读配置倍率为−25／−15／−5／+5／+15%，与难度页显示不同；保留原读取规则，不按文字擅改数值。详见 [证据](analysis/easiest-combat-correctness.txt)。最新实际 SDL 从原未转职的真实第12关入口存档出发，于第17回合胜利，完成原胜利选择／奖励／关间并进入第15关、验证自动档重载。该结果支持此前最低档规则遗漏确实影响战斗，不能再把该次困难简单归因于玩家战术。随后第15关第7回合判败，主角仍有375HP，已核对原失败条件为角色11或22死亡，实际角色22为112HP友军NPC且确实执行行动；未证明该关胜败判定错误。独立续档另发现首次脚本tick早于事务适配接线，已将接线提前并实际验证消除opcode6适配缺失错误，但正常战斗仍会因角色22阵亡而败，已执行固定随机流86516的有限合法手动保护尝试，正常移动／攻击及原脚本均执行，但角色22仍阵亡；未获得本关胜利或预胜利检查点。当前队伍可见动作type12／40及能力命令仍有未实现边界：已查明type12被当前装备的原门控拒绝，type40应进入原攻击槽7，真实动作163／130为攻击附加状态、不是治疗。真实动作163已接通原攻击槽7→合法目标／射程／装备检查→AP消耗→动画3→命中／伤害→65%降攻防→4回合递减及恢复，并验证原档往返。端到端原生测试、battle回归、最低难度1210项及Release／Debug通过；动作130也已按441B15恢复1格射程、43BFF2恢复单目标、43E07E读取B7最大HP并在扣血前执行effect9结果替换；原攻击槽7、AP与动画4端到端回归通过。统一Release／Debug构建及特殊攻击、battle、动作清单、1210项难度预估回归通过。实际SDL特殊动作验收尚缺，独立动作夹具未冒充真实战役角色可用性或第15关胜利；不能据此断言原任务不可能通过。动作日志与真实中间原档保留在out的独立stage15-tactics目录。新增固定86516的第13种有限合法战术复测仍于第6回合因NPC22阵亡失败，主角仍375HP；动作130／163所属角色13／14均带死亡0x20且无活跃0x80，虽HP字段非零也不可合法操作，本次槽7执行数为0。因此这两项实现不能视作已在当前战役中使用或能直接解决保护任务；已核查存活队员：角色18的原动作160现已接通攻击槽7，恢复2–6格射程、原目标／装备门控、AP、动画3及物理结算；真实角色专项通过，固定86516的一次攻击按原随机流为MISS，未强制命中。统一Release／Debug、特殊攻击、battle及1210项难度回归通过。已接入原特效资源59的方向像素、飞行步进及扣血前投射生命周期，补上自动阶段人物绘制后的投射绘制调用。按40B61B→40B664核实原AI只排除type11／12、不排除40，已使受支持160／130／163进入既有候选评分，未添加优先级或强制命中；真实存活角色18的独占射程夹具验证自动选择160及投射像素／生命周期。集成后Release／Debug、特殊攻击、battle及1210项难度回归通过，编辑器无错误；新增固定86516的生产自动控制器实际流程复测于第7回合失败：敌122第6／7回合对NPC22分别造成99／13伤害，角色11仍375HP；角色18对202执行八次普通攻击（四次MISS、四次1伤害），未观察到160或投射执行。该测试为真实控制器夹具，不是SDL连续战役（后者无固定种子选项），日志位于out/stage15-tactics-14-17942348346123418。已查明并修复两个原AI差异：4086DF评分应为命中率×伤害，而不是仅伤害；相邻目标不应绕过候选评分。真实第4回合检查点中，角色18对202的159为45×1=45，160为35×1=35，因此选普通攻击在该比较中符合原规则，未人为优先160。新增真实状态评分回归、统一Release／Debug、特殊攻击、battle与1210项难度检查通过，编辑器无错误。修正评分后，以同一86516完成一次稳定编译产物的生产自动控制器复测，仍第6回合失败：敌122先后造成85／27伤害，NPC22由112HP降至0，主角375HP，敌122剩80HP。共记录55次攻击，队员18执行六次、未观察到160投射。证据保存在out/stage15-tactics-14-17942354121267768；earned-before-5.sav仅为结局前检查点，不是最终状态或胜利存档。未进入奖励／关间；停止盲目重复战术测试。继续核查发现43948D(mode3)威胁筛选漏传门槛：按40C956→43948D应要求目标战斗价值严格大于自身两倍，原实现传0；现已修复并通过真实状态Release／Debug专项及统一构建、battle、特殊攻击回归。真实第6回合NPC22价值582、门槛1164，敌122仍符合；NPC在移动回退之前已进入攻击候选分支，因此本修复尚不能解释或消除该次死亡。进一步确认生产候选评分缺少原反击预估／致命反击排除，正在追查原分支接线，尚未据此宣称NPC决策完全恢复；不能把独占射程专项通过当作本关自动攻击已实际使用或任务胜利。音频仍暂缓。效果18／19／61属于死亡角色13，存活角色11／17的type12仍被原装备门控拒绝，不能伪造成治疗。仍未全通关。

<a id="menus"></a>
## 菜单与关间

旅店十入口、商店买卖／装备与教会复活／捐献有有限生产证据；283／296自然样本及全部原像素／交互未全覆盖。旧表中拒绝入口仅是当时状态，不重新列为当前缺实现。

> **历史区域**：下方各原标题仅表示当时状态，不能覆盖[最新状态](#current-status)；误判按[撤销表](#corrections)阅读。

#### 历史原证据区域（保留原标题便于追溯）

<a id="2026-11-11：菜单／关间本轮有限清单验收"></a>
<a id="history-v-008"></a>
### 2026-11-11：菜单／关间本轮有限清单验收

- 最新`menus-production-debug.log`完整Debug构建成功；`menus-production-sdl.log`整app `--smoke-shop` exit0，真实SDL事件队列涵盖410FFE／411034旅店5／7原矩形点击、16次present期间Escape输入锁、页取消留旅店，41104F标题取消与肯定；另含真实库存买卖／对象装备／保存往返、自动装备、设置、传闻。来源是第65《龍穴》原SAVE派生关间入口，不称从战役胜利自然走到该旅店；教会复活／大额捐献以及12角色名单滚动日志明确受控fixture。第115《小丑奪權》及第1《第一神劍》三原SAVE额外由独立页专项覆盖。
- `menus-army-equipment.log`最终重编exit0：五原SAVE，双资源域OriginalInn生产鼠标入口／draw／逐present／取消，327次真实SDL present、两档合法装备与卸下精确SAVE delta；修复原44D675的AP+12C、末行+C5、status33 bit10隐名、原456EEA装备名色233。地图与共享panel沿原disc传递，不再默认disc1。ITEMHLSP通过Result预解析，缺失／截断拒绝构造，不静默打开残页；空slot卸下不误报changed。
- `menus-current-original_settings_test.log`、`original_battle_reports_test.log`、`original_unit_status_test.log`及inn/shop/church/interbattle/battle-menu、item-use/item-shop针对日志均通过。旧status_test期待异常已更新为try_draw Result失败与不发布buffer，匹配现生产契约，非放宽拒绝。
- 450F77 effect61→45154D逐dword sar16已核实为下一WORD（如+C1→+C3、+132→+134），当前同offset delta/clamp正确，不将其误认相邻错误；451072 effect67先WORD加法再999 cap已保真。118保留checkpoint及BIN4商店仍无283／296自然样本，未注入物品验证自然流程；此为明确自然覆盖不足，不是删除handler。已有原库存组合与第124《力量與魔法》正常买入四物品事务证据保留。
- 菜单本轮有限清单收口，战斗AI／死亡剧情尚未收口。为上述SDL已发布Debug到根，根暂不是最终Release；尚未重打包，最终统一发布另行记录，不能沿用下面旧稳定Release hash称当前根。

<a id="菜单续作：旅店装备／物品页生产接线（历史阶段，后由上项收口）"></a>
<a id="history-v-009"></a>
### 菜单续作：旅店装备／物品页生产接线（历史阶段，后由上项收口）

- 入口410FFE／411034→44D439，筛选原WORD+8!=1且status33&70==0；新增OriginalArmyEquipmentMenu使用原217／218／229，原44DC93／44F4DB鼠标区域、十角色滚动、六装备分类／五库存行、440A89灰显。角色页44D675复用现有OriginalRolePanel，新增可选origin=0，默认行为不改。装备44D018及卸下44D11B都先独立State，再bridge重建成功后发布；app沿既有take_equipment_changed→sync_roster更新角色。
- `menus-army-equipment-build.log`／`menus-army-equipment.log`：五原SAVE第65《龍穴》、第115《小丑奪權》、第1《第一神劍》三档；真实SDL呈现原页、旅店正常矩形入口／取消保持完整字节、两档合法库存鼠标装备／卸下与既有算法精确delta、capture往返通过。第115名从原stage_name字节A470A4A1B9DCC576经CP950核实。此为原SAVE派生菜单生产调用+真实SDL，不称从标题自然走到旅店的整app鼠标E2E或原录像逐像素认证。
- 原44FD8B先列当前角色六装备中use!=0，再列库存type12且use!=0；非战场45024C拒绝use2，use1弹43183B字符串80于(200,400)，取消不消耗。原ITEMHLSP.BIN按428736的零结束串表显示450055说明。已接原确认／返回／普通回复、解状态、永久属性、现有451DDA学习物品复用；没有扩展技能施放。未完成handler通过Result拒绝并保持State／RNG，不静默消费。
- `menus-item-use-build.log`／`menus-item-use.log`：五原SAVE原有库存／装备共306角色物品组合，无注入；168允许提交后的完整SAVE重捕获相等、138不可用对象／use2按原条件拒绝且字节不变。新增effect35／61／67等未在这五档获得自然覆盖，不能以306组合替代全部物品handler验收。已新增`menus-item-shop.log`第124《力量與魔法》保留强化checkpoint原商店category3正常买入250／254／257／262→使用→库存减一／HP／MP／ST／解状态精确字段与SAVE往返通过，无注入物品／旗标／金钱；这四项不是全部handler。另只读遍历118个保留stage-start原流程checkpoint没有283／296库存，穷尽原BIN4所有town三类offer也无283／296，不伪造样本。最终handler接线后整页专项重跑通过；现有旅店／商店／教会／关间／战场菜单五回归新重编全部通过（menus-current-original_*-test日志），生产app_runtime用MSBuild /t:ClCompile验证通过，未执行PublishGameRuntime。
- `visual-class4-equipment.log`再次通过第124《力量與魔法》正常购买141的卸下退库／class4消失与非法slot拒绝回滚。原SAVE及旧证据未覆盖；未重复构建打包，根EXE仍是上一稳定Release，本轮源码不是已发布包。

剩余：effect61／67无现有库存或原商店样本，仍缺自然调用证据（已实现按原字段，不造物品）；角色详情与页输入边界还需完整核对及整app SDL。原475970为轮廓矩形，已补六类别选择框；44DC93／44F4DB的472EF1(-4)新增16呈现阶梯、淡入输入锁和use后重建淡入，SDL专项已逐present验证（仅draw不推进）。实核菜单完整有限清单后才进入战斗AI及死亡剧情，尚未宣告这些类别收口。

<a id="2026-11-10-2323续作：菜单当前实现边界"></a>
<a id="history-v-011"></a>
### 2026-11-10 23:23续作：菜单当前实现边界

新增旅店8原41104F→43183B：原坐标(300,200)、默认肯定、原确认面板／字符串51；键盘Escape／左右切换／Enter，右键取消／鼠标确认，肯定后app_runtime清理旅店与当前局面返回标题。第65关《龍穴》`menus-inn.log`正常原rect鼠标进入／绘制、Escape取消留旅店、再进入Enter肯定、SAVE逐字节不变通过。此为菜单类中的一个生产分支，尚无新增真实SDL输入专项，**不是菜单类完成**。5／7仍blocker，已核原44D439筛control WORD8!=1、status33&70==0名单；mode0→44DC93及44EA2D／44EC85装备交易，mode1→44F4DB物品页；没有用只读roster冒充它们。战斗AI／死亡剧情未开始新功能。

保留当前可构建状态：`menus-current-{Debug,Release,verify,package}.log`及独立`out\\menus-current-independent`verify／opening SDL均通过，根最新Release与包当前SHA256 `1B2BAEA3F127C4FBB63674DABE6ABB50DD0CC1BEAF2CB5F109440C081BECBFC1`。这只是稳定检查点，不是四类最终交付。

<a id="2026-11-10-聚焦续作：共享箭头初始化、报价刷新、自动装备及ai6回退"></a>
<a id="history-v-024"></a>
### 2026-11-10 聚焦续作：共享箭头初始化、报价刷新、自动装备及AI6回退

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

<a id="2026-11-10-同轮续作：对象跨页与旅店存读档／设置"></a>
<a id="history-v-025"></a>
### 2026-11-10 同轮续作：对象跨页与旅店存读档／设置

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

<a id="2026-11-10-装备购买生产事务与有限原比较页接线"></a>
<a id="history-v-026"></a>
### 2026-11-10 装备购买生产事务与有限原比较页接线

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

<a id="2026-11-10-教会复活生产接线阶段交付（装备与后续仍未收口）"></a>
<a id="history-v-027"></a>
### 2026-11-10 教会复活生产接线阶段交付（装备与后续仍未收口）

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

<a id="2026-11-10-同轮菜单／ai续作阶段交付（未全收口）"></a>
<a id="history-v-028"></a>
### 2026-11-10 同轮菜单／AI续作阶段交付（未全收口）

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

<a id="2026-11-10-同轮续作：消耗品买入、旅店传闻、教会有限捐献生产接线"></a>
<a id="history-v-029"></a>
### 2026-11-10 同轮续作：消耗品买入、旅店传闻、教会有限捐献生产接线

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

<a id="save"></a>
## 存档与初始化

原格式往返、真实fresh新游戏与正常1→2跨关有证据；低层bridge拒绝未知baseline／非法跨stage仍正确。半事务锁、IO与备份边界不因初始化修正而取消。

> **历史区域**：下方各原标题仅表示当时状态，不能覆盖[最新状态](#current-status)；误判按[撤销表](#corrections)阅读。

#### 历史原证据区域（保留原标题便于追溯）

<a id="2026-11-10-存档／回归有限清单与统一验收（2244调整后，初始化缺口判断由上节纠正）"></a>
<a id="history-v-014"></a>
### 2026-11-10 存档／回归有限清单与统一验收（22:44调整后，初始化缺口判断由上节纠正）

本轮只核实／修补存档回归，不再扩画面、AI、死亡剧情或技能。以下“通过”仅指已实现的有限路径；新游戏缺完整raw baseline是实际存档入口缺口，不能以fixture替代并宣称全类完成。

|有限项|当前实现与证据|未覆盖／边界|
|---|---|---|
|原格式／全持久字节|4362B4写、4367CB读；`original_save_test`五份磁盘SAVE完整encode/decode逐字节相等；两原档每个截断、签名／尾随／人数／事件数量／库存错误均拒绝；保留未知raw区。`original_save_bridge_test`全22／42角色、角色WORD／关系BYTE／货币／行动值／事件状态往返通过|不意味着未知raw字段均已赋予运行语义；未完成技能／剧情不列为本类前提|
|读写失败／损坏拒绝|上述codec拒绝、missing-file；store新增slots0/54、库存不匹配、目录位置为普通文件、pending冲突及真实Windows目标共享锁拒绝|不模拟物理断电、磁盘写满或FlushFileBuffers硬件失败|
|原子目标／备份|43619F原码先以35h=53备份，再4362B4写；现CREATE_NEW pending、循环WriteFile、Flush、ReplaceFile／MoveFileEx。新共享锁测试失败目标逐字节不变、自有pending清理；解锁后成功、`.s2ne-backup`精确保存旧字节；原slot53与slots51/52测试通过|53号备份和目标是顺序两次单文件替换；目标写失败时53可已更新为目标旧内容，并非跨文件事务。未承诺OS崩溃后两文件同时回滚|
|已实现事务兼容|转职40合法／22材料／6装备退还分支，缺RNG时钟返回空optional且原RNG／库存不变；传送第60关《返鄉的遊子》生产侧末受控72帧保存锁／原action恢复；技能94既有隔离事务survivor／revival／reverse／award失败rollback及64固定RNG；shop出售拒绝隔离／精确raw delta；church取消／确认／完整SAVE delta往返|技能94是既有受控fixture兼容，不新增技能／不声称生产E2E；商店算法通过不代表尚未接入的全部UI完成|
|战场／关间菜单|`original_save_menu_test`原几何／metadata／键鼠／覆盖取消／失败不成功返回；`original_inn_test`第65关《龍穴》关间正常save／cancel／load；最新Release `--smoke-load`标题正常读取第65关《龍穴》turn9，隔离slot52写／重读，42呈现|并非每个slot所有UI组合；移动动画时新增`!animating_movement()`边界锁已编译，未新增菜单级移动保存输入专项；原bridge已有movement_pose/frame拒绝|
|现有支持功能回归|save／bridge／store／menu／promotion／teleport／skill94／inn／church／shop专项通过，未重跑强化战役|**无baseline的新游戏capture及stage/difficulty变化仍明确拒绝**；未知原始globals／inventory初始化与完整关卡转移raw状态尚未实现，本轮未修，不以捏造默认值解锁|

本轮修复两旧测试`.stack.count`为`std::stacktrace::empty()`；转职旧异常测试改按当前prepare失败optional断言，未恢复throw；生产仅新增移动时保存锁。所有改动C++已clang-format。证据`out\\save-scope-<test>{-build,}.log`，包括`original_save_test`、`original_save_store_test`、`original_save_bridge_test`、`original_save_menu_test`、`original_promotion_transaction_test`、`original_teleport_transaction_test`、`original_skill94_transaction_test`、`original_inn_test`、`original_church_test`、`original_shop_test`。原SAVE保留。

停止C++写入后`save-scope-Debug.log`→`save-scope-Release.log`均exit0，`save-scope-verify.log`资源exit0，`save-scope-load-sdl.log`真实load SDL exit0。package-game生成14运行／资源文件+launcher，独立解包`out\\save-scope-independent`；`save-scope-independent-verify.log`exit0，`save-scope-independent-sdl.log`真实SDL开场exit0／92呈现。独立库存1DAT／1PRG／0TXT／4DLL。根、Release、独立包EXE SHA256均`5766DA0D79C407C8D2AEE553B3AD9ED679888D2A5D9DF69B913801E1F3061121`。该包是当前有限路径稳定检查点，不是整个存档类别或六类总完成；不自动推进下一类别。

<a id="历史状态：存档初始化／正常跨关重新实核（2026-11-10，2257续作）"></a>
<a id="history-p-004"></a>
### 历史状态：存档初始化／正常跨关重新实核（2026-11-10，22:57续作）

- **纠正上批错误缺口判断**：`OriginalSaveBridge::try_capture`拒绝无baseline和跨stage是正确的低层契约，不等于生产入口未实现。磁盘实际`OriginalInitialization::try_fresh`及`try_transition_stage`已分别接新游戏首关autosave／关间option4；本次沿实际调用核实并将这两个生产调用从fatal便利wrapper改为显式Result检查，未拷贝原SAVE初始化、未捏造flags／库存。
- 原初始化依据44367C／44382B army清零及11 triples、443C3E／4443B3原101字节source展开、427645／427870本回合到场、原BIN opening货币／首关与stage字段、实际140开场完成flags及用户settings；跨关依据40FEE9／410478→44A4A9清临时角色／44A79C重建敌军，保留我军持久raw与库存、货币、设置，再装目标事件／动态链／原到场角色。
- 最小初始化专项通过：真实140开场两分支、1《第一神劍》／71《惡兆》难度1..5、正常到场、设置、1→2《攔路虎》及71→72《青色草原》格式往返；非法跳关／库存失败按Result拒绝，未扩大技能。
- 新`--smoke-fresh-save`从正常标题／设置／140对话进入第1关《第一神劍》，生产初始化后保存／重载通过。**无SAVE的独立发布包**同一真实SDL通过439呈现，证明不是随便拷原档。该专项使用正常开场选择，不注入flags／物品／强化属性。
- 最新Release原`--smoke-campaign`正常第1关《第一神劍》turn10获胜→奖励→关间→第2关《攔路虎》，原autosave重载并全部持久字节再capture相等，通过3400呈现；不强制胜利／不强化／不跑全战役。先前Debug尝试600s到turn8超时，Release正常通过，失败日志保留。
- 停止源码写入后Debug→Release、资源verify、package及独立解包verify／fresh-save SDL通过。根／Release／包SHA256 `53A44E0CE0B04A2FB5BDAD2EB214F17C8FCC92EE130BC5A7AD77BDB872E12ADC`。本次两个正常路径的旧阻碍判断已消除；低层未知模式／非法跳关仍failclosed。仍不等于所有剧情／技能存档语义全覆盖，不自动移下一类。

<a id="历史状态：仅存档／回归类别定向验收（2026-11-10，2244任务调整后）"></a>
<a id="history-p-005"></a>
### 历史状态：仅存档／回归类别定向验收（2026-11-10，22:44任务调整后）

- 已停止画面／AI／死亡新功能；移动保留上批安全实现，仅补战场移动动画期间保存边界锁。没有继续effect39或扩大技能范围。
- 本轮有限清单：①原4362B4/4367CB格式及原档全字节往返；②损坏／IO拒绝；③原43619F覆盖前53号备份与单目标原子替换；④既有转职／传送／技能94、商店／教会事务兼容；⑤已支持战场／关间存读档菜单、取消／失败与半事务锁。逐项证据和未覆盖见[分项验证导航](#validation)。
- 当前五份磁盘SAVE codec往返、两份原档完整22／42角色bridge往返、所有截断、菜单、store、promotion、teleport、skill94、inn、church、shop专项通过。新测真实Windows共享锁导致ReplaceFile失败后旧目标不变且pending清理；解锁后成功且系统backup逐字节正确。原槽53先备份后目标写入是顺序操作，不承诺跨文件原子事务。
- 最新Release真实SDL从标题正常读取第65关《龍穴》turn9，隔离slot52写入／重读通过（42呈现）；第60关《返鄉的遊子》受控原传送格生产侧末72帧、保存锁／action恢复通过，非自然走格存档。未重跑强化战役。
- **有限已支持路径已通过，但不宣称整个存档类别无缺口**：无raw baseline的新游戏保存仍明确拒绝（未知globals／inventory不可臆造），跨stage完整原始状态转移也仍拒绝；本轮未补此原始状态初始化链。未模拟断电／磁盘写满／Flush失败；移动锁新增条件仅源码实核，逐present路线由既有battle测试覆盖，尚无新增正常菜单移动时保存专项。下一类不自动启动。
- 停止C++写入后统一Debug→Release、资源verify、package-game和独立解包verify／真实SDL开场92呈现通过。根／Release／独立包EXE SHA256 `5766DA0D79C407C8D2AEE553B3AD9ED679888D2A5D9DF69B913801E1F3061121`一致；包1DAT／1PRG／0TXT。细节日志`out\\save-scope-*`。

<a id="新游戏进度"></a>
<a id="history-p-028"></a>
### 新游戏进度

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

<a id="visual"></a>
## 画面、特殊移动与城镇

最终窗口与箭头按获准差异；原逻辑帧不变。保留选框前景／人物AD、地形+5、190ms门控、class3/4/5、世界图DAT11与传送的原地址／测试／受控边界。

> **历史区域**：下方各原标题仅表示当时状态，不能覆盖[最新状态](#current-status)；误判按[撤销表](#corrections)阅读。

#### 历史原证据区域（保留原标题便于追溯）

<a id="2026-11-11-对话箭头归因与dosbox输出滤镜复核"></a>
<a id="history-v-002"></a>
### 2026-11-11 对话箭头归因与DOSBox输出滤镜复核

本次只读追踪原程序并补充记录，未修改游戏源码或重新打包。此前将箭头交换直接称为“原版恢复”不够严谨：当前修正是针对所持Windows EXE／CEL组合的可见方向纠正。

- `41CA6E` 从 `4EA890` 顺序加载五项DWORD `{233,234,235,237,238}`；输出指针从栈 `var_5C` 起依次占 `var_5C,var_58,var_54,var_50,var_4C`。`426CEC` 保持输入顺序，资源编号减142后读取CEL第2组，未交换后两项。
- `41CBD5` 判断人物屏幕y>310时使用上框，`41CBFF` 选 `var_50`，即原资源238，锚点为框y+215、人物x+12；否则 `41CC44` 选 `var_4C`，即237，锚点为框y−32、人物x+33。
- `41CD56` 先绘主框，随后以 `push 0` 调用 `474E3E` 绘箭头。该参数决定零色是否写入，并非翻转参数；绘制器按正向行序解码，不存在这里暗含上下翻转的证据。
- 现有实际解码测试证据：237尖端在右下，238尖端在左上。因此原Windows程序的这组资源选择确实与“上框向下、下框向上”相反。当前 `OriginalDialogueLayout` 交换这两项属于有证据的可见修正，**不能称为逐指令一致复刻**。不能由此推定所有Windows发行版均如此，也没有证明DOS EXE使用相同分支。此前未完整串联资源表、栈指针顺序和绘制器，是分析不充分。
- DOSBox整合版 `D:\project\S2NE-reference-dos\SUPERSP.conf` 明确配置 `output=opengl`、`scaler=super2xsai`、`aspect=true`、`windowresolution=912x684`；原启动BAT为 `DOSBox -noconsole -conf "SUPERSP.conf"`，程序版本0.74.0.0。这不是无滤镜基准：已经选择Super2xSaI增强及OpenGL输出。`scaler` 无 `forced`，实际是否应用Super2xSaI还受游戏模式尺寸与DOSBox自动选择限制，不能仅凭配置声称640×480每一帧均经过该算法；也不能把SDL最终LINEAR称为与Super2xSaI等价。

<a id="2026-11-10-2323后画面定向实核（尚不宣称完整类别收口）"></a>
<a id="history-v-012"></a>
### 2026-11-10 23:23后画面定向实核（尚不宣称完整类别收口）

有限清单见PROGRESS顶部。新重编`visual-map-frame.log`：第1关《第一神劍》／71《惡兆》122原PRG帧signed displacement正负各61、clipped10052、drawn431262；indexed B1反应、silhouette独立及RGBA等价通过。`visual-special.log`第65关《龍穴》原actor14原AD上移64精确2613像素、脚印570像素、class4/5共享phase／19cs／camera每present门控通过；新增**已进入绘制事务后**无效animation_action失败，RGBA／indexed不变、先前pending presentation保留，不仅测试早期buffer尺寸拒绝；这仍是受控class3/4/5 renderer fixture，不称自然状态来源。

`visual-worldmap.log`正常旅店option1第65关《龍穴》入口／DAT11世界图／DAT7 markers、边缘持续四像素滚动／原百分秒delay／64+64淡入淡出／返回不改SAVE，真实SDL通过。`visual-movement.log`仅第42关《小丑獻媚》原图裁剪脚印图元及全256索引，不冒充自然路线。原teleport图元测试旧输出“dispatcher not wired”已更正，因为生产侧末链早已接，图元测试不应发布全局错误结论。

普通伤害真实SDL首次失败`visual-feedback-sdl.log`是smokePhase>1000的入口检查误用于已在战场；修正仅未入战场检查。第二次`visual-feedback-retry-sdl.log`turn2卡住，发现smoke多回合复测未复位autoMenuOpened，修正每轮打开状态，不修改战斗规则。`visual-feedback-final-sdl.log`第1关《第一神劍》正常开场／原自动战斗菜单，多回合真实friendly／hostile、原indexed reaction／silhouette／浮动伤害／逐步扣血，exit0／1547呈现，无强化／flags／物品注入。两个失败日志保留。

`visual-teleport-sdl.log`第60关《返鄉的遊子》52呈现、生产侧末／保存锁／原action恢复；`visual-evacuation-sdl.log`第42关《小丑獻媚》29敌军170呈现／Escape保存锁／恢复一次。二者均明确原SAVE派生受控pad／隔离records14/17，非自然触发。源码停写Debug→Release `visual-final-{Debug,Release}.log`exit0、verify、package与独立解包`out\\visual-final-independent`verify／正常feedback SDL通过。根SHA256 `E8581980051E72AAFE7F218E760A5F962D3024C38819539F7547629141E77F48`。尚需补完清单正常选中合成／遮挡的全面调用审计与未覆盖分支，不以本批测试直接进入下一类。

<a id="2026-11-10-class45共享相位门控续作（尚未解除生产拒绝）"></a>
<a id="history-v-017"></a>
### 2026-11-10 class4/5共享相位门控续作（尚未解除生产拒绝）

新增503464共享相位与每actor时间戳：45CA1D先绘当前slot，camera/movement mode1每次原调用切换；其它mode由473192以百分秒abs差>=19切换并更新actor+A8。renderer用局部staged phase保证同帧多actor共享顺序，重复draw不发布，Prologue::frame_presented只提交一次；输出错误恢复pending，不推进phase。加载新map清actor时间戳/pending而保留全局phase。`out/class45-render.log`原第65关《龍穴》actor14受控class4/5图像fixture、19cs、camera、重复present、错误输出、真实SDL通过；`movement-bitmap.log`补189/190ms和时钟回退绝对差专项通过。这里class4/5是显式renderer fixture，**非原Unit状态/装备来源**；撤离/death preflight拒绝仍保留，正常路线不传播class4/5 visual class。

同时补生产movement呈现锁：启用presentation timing后，原120ms路线到时但未present不能丢弃pose；每pose只能ack一次，新movement/attack起点清ack。未启用presentation timing的算法测试保持旧计时。原battle_test含此边界与取消回滚通过（`out/class45-battle.log`），special AI6/7既有fixture通过（`class45-ai.log`）。原第42关《小丑獻媚》隔离records14/17死亡→撤离生产SDL通过，29敌人170呈现，输入/save锁/恢复一次（`class45-evacuation-sdl.log`）；此仍明确隔离脚本fixture，不冒充自然死亡或特殊class4/5角色。

**45DC37具体入口新增证据**：唯一caller466544，按BIN10每37字节技能record+19的effect==0x27进入（4666E3→4666B0→45DC37），不是普通菜单“起飞”可自行构造。先置status33 bit4、保存坐标/方向/AP12E/130→476707路表→42B37A/476A78→452B64选择→452AD5移动→447E59/4483CB→4535A1确认；否定恢复坐标/方向重选；肯定恢复AP、重绘、xor bit4、476905清路表。现有OriginalSkills仅32/88/94有限执行，effect39仍unsupported_execution。新增只读`original_movement_save_coverage_test`检查原SAVE五个battle文件：class4/5可见角色0、正常技能菜单可用effect39为0；原1..158关开场可控/到场角色菜单effect39也0（`out/movement-save-coverage.log`）。未注入已学位/装备/flag；这些结果只限定现有入口样本，不证明后续合法学习不能触发。完整45DC37依赖的技能支付/选择与route事务尚缺，不能仅动画/共享时钟通过就解除拒绝。六类余项未完成；本批尚未重新打包。源码格式化后Debug→Release统一构建、最新Release撤离SDL与verify通过；根/Release SHA256一致5EC080724E0B267F085F7057CA7F135E37BFAA83BB5DD886E1973DC641B276F7，日志`class45-production-{Debug,Release,verify}.log`。

<a id="2026-11-10-class3-buffer投影阻碍定位与最小闭环（替代下节失败结论）"></a>
<a id="history-v-018"></a>
### 2026-11-10 class3 buffer/投影阻碍定位与最小闭环（替代下节失败结论）

继续同一trace：42927B将4F7DF0静态world按6C/70镜头复制到4F7E18；42B327经42C50A在**当前4F7E18绘图destination**截脚下64×32（不是永远从静态world截）；47566E保持请求尺寸并在负边界补零。448B99在该indexed crop套DAT5 index21及map5；4468DF以原AD非零清mask，两军团在本实现统一完整roster，x±2、y严格后方到+24、排除state1/3/5；42AFBA清地形/物件；42B1D2影子arg0=0，随后角色原AD arg0=4抬高64像素，**不使用已经地形清过的ordinary display AD再次抬高**。frame+6 signed displacement只加角色，不加脚印，遮挡源4468DF不加该displacement。

旧exit4诊断：65《龍穴》首个class3 actor9 (10,33) ordinary相对静态图就没有一个差异，原测试未建立“源角色可见”前置条件；不能以该候选宣称整个投影链无效。新`tests/original_class3_render_test.cpp`跳过ordinary_delta=0，原actor14 (20,44) ordinary_delta=7670；控制action4测试2613个非零原AD像素逐个符合原palette与上移64坐标，570个脚印像素符合map5且不同静态；ordinary/special/static三图不同、重复一致、恢复原Unit后ordinary完全一致。镜头+32后重叠608×480所有RGBA逐像素相同。真实SDL window/texture/render/present通过（非dummy）。`out/class3-render-build.log`及`class3-render.log`保存证据。此为**第65关《龍穴》原角色+受控action4 renderer fixture**，不是自然移动/死亡E2E；没有注入原SAVE装备或旗标。

生产接线：Prologue撤离target传递原movement_class；OriginalEvacuation及death preflight仅解除class3拒绝，class4/5继续拒绝。Battle的452450 class3路线pose传递visual class，保留原120ms门控/原状态，不把presentation字段写入SAVE。`out/class3-battle.log`的既有battle_test通过（含新class3 dispatch断言、120ms及取消回滚）；图元回归`out/movement-bitmap.log`通过，关卡42《小丑獻媚》仍仅静态脚印fixture。

45CADA下一层resource/projection fixture也通过：45CA1D的4EAD9C=slots23/24（各64×32）；45CAA0的4F70FC是shared base+39*4，方向slots39..42（128×160/121×63/121×63/121×63）；原raw长度及方向&3、交替slot重现一致，effect先mode3/4后原AD mode4，renderer差异断言通过。尚未把全局503464和19cs/每present交替门控混成draw次数，因此class4/5生产仍拒绝，不宣称该类完成。45DC37确证需合法路表选择/验证/回滚而非动画，仍未接；六类、AI/死亡剩余未完成。

补验：显式提供不同indexed destination，确认42B327脚印确实读取当前destination而非静态world；half-tile镜头平移逐像素通过。生产Debug→Release及`--verify`通过，`out/class3-production-{Debug,Release,verify}.log`；根与Release SHA256同为E582C20BF85DE135BDA02719CF29DC1422FF1FC2FC88A8061E33AC6854B20D84。未重打包，旧包不含本批，不将其称最新交付。

另定向运行旧`original_death_continuation_test`失败于其故意破坏event index的末尾fixture，当前fatal策略进程退出而其旧try/catch无法捕获；日志`out/class3-death.log`。未为通过它恢复throw，也未宣称死亡全回归通过。

<a id="2026-11-10-城镇连续滚动与原淡入淡出续补（进行中，未最终交付）"></a>
<a id="history-v-020"></a>
### 2026-11-10 城镇连续滚动与原淡入淡出续补（进行中，未最终交付）

**纠错：首次城镇闭环误读41020B的FFFFFFF5为-12，实际是-11（DAT11），先前DAT12是640x480旅店等静态图，导致滚动测试条件允许不移动而虚通过。现已改DAT11真正大世界图，并强制断言实移动：第65关《龍穴》原SAVE视口(0,56)，首滚(8,56)，重复(12,56)，每次四像素。原SAVE delay0；另隔离副本仅delay10验证109不滚／110滚，明确非自然配置。旧PCX12闭环证据不再有效，不隐藏此问题。**

DAT11纠正及小图元最终停写后重新Debug→Release（同`worldmap-timing-{Debug,Release}.log`），重新资源／传送SDL及package；独立`out/worldmap-corrected-package-check`，`worldmap-corrected-package-{verify,sdl}.log`均0，开场右键87帧（silent fallback）。根／Release／独立包EXE现均`A8BE401BD0E92888390E52415FC0FC4BC7CAEB8AB72B431D5C7CA40AA4FABA4D`，替代下方旧AF60检查点。此仍非六类完成。

第65关《龍穴》原SAVE，`tests/original_worldmap_test.cpp`→`out/worldmap-timing-build.log`、`out/worldmap-timing.log`当前通过。新增生产`app_runtime`正常旅店入口启用presentation；每次SDL present才推进世界图和重绘，不仅fixture API。原415415边缘x<5/x>635/y<5/y>475连续四像素滚动；右键仅60..580、40..440内返回。4157C6边界逐指令核实：到宽-640／高-480时原回退宽-641／高-481，不继续使用先前通用clamp推测。414E62→47308B→415084(1)→473111使用SAVE globals+32原百分秒步进延时与abs(start-now)，不做模差回绕；每帧未present不可推进。

414D5B入口472EF1(palette,0,0)是64级0..63、出口(palette,1,0)为63..0，**不是商店-4的16级**；无delay_。当前64+64次真实SDL呈现逐阶验收、淡入输入锁及淡出返回通过，直接原palette DAC乘法，不从RGBA反推调色板。出口473006先47FA40读当前已淡入palette，再逐阶47F5D2，因此出口使用入场63末值二次乘法，不错误复用原DAC直接淡出。仍未实现音乐36→4，键盘按住连续时序与原系统调色板同频不能称全等价。渲染／受控时钟断言与未改SAVE正常旅店入口分开陈述；没有重跑强化战役或技能。

新增`original_movement_bitmap.hpp::try_shadow`仅实现448B99已确证的64x32索引脚印mask/5045B8映射小图元；`out/movement-bitmap-build.log`／`out/movement-bitmap.log`原DAT5 index21/2052字节及palette map5、256索引与错长拒绝专项通过。另新增423D76非棋盘mode0的负偏移裁剪非零清mask、`OriginalMap::try_movement_shadow`原静态索引64x32取图，严格按地图height*2半行坐标而不是height；第42关《小丑獻媚》原角色(4,68)资源隔离专项通过。它尚未生产接线，不计任何自然关卡流程或class3完成。曾尝试用“移除目标后重画整幅背景”衔接，发现不等价于42B327从当前buffer取图及4468DF双军团清mask，已删除该未验算法，保留严格拒绝，不将图元测试包装成流程。

正在继续448B99／45CADA：已核42B327先抓当前buffer的64x32脚下索引，经4F70B0 mask及5045B8变换，随后4468DF双军团叠图、42AFBA遮挡、42B1D2投影；不能直接套现有反应bitmap或把普通角色叠脚印当完成。class3/4/5拒绝仍保留，45DC37起降状态/路表仍未接。本批源码停写后`out/worldmap-timing-{Debug,Release}.log`按序构建通过；`worldmap-timing-verify.log`、第60关《返鄉的遊子》`worldmap-timing-teleport.log`52帧SDL回归通过。`worldmap-timing-package.log`打包、独立`out/worldmap-timing-package-check`的verify及真实SDL右键跳开场89帧均0（无默认音频，silent fallback，不冒称音频通过）。根／Release／独立包EXE SHA256一致`AF60865DF35B580D33866FEADDD50079384AACFE2153185389211E5D2A0D0089`。仍只是本批闭环，不是剩余六类全部收口。

<a id="2026-11-10-画面续作：411587原调色板阶梯／撤离普通移动姿态／到达图元"></a>
<a id="history-v-022"></a>
### 2026-11-10 画面续作：411587原调色板阶梯／撤离普通移动姿态／到达图元

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

<a id="2026-11-11-渲染固定场景第一批：整数缩放"></a>
<a id="history-v-061"></a>
### 2026-11-11 渲染固定场景第一批：整数缩放

- 实证代码差异：app_runtime默认960×720、逻辑640×480，先前LETTERBOX=1.5倍，NEAREST不保证均匀像素宽。改用checked configure_original_presentation(INTEGER_SCALE/NEAREST/BLENDMODE_NONE)，保持逻辑640×480。此为输出缩放修复，不推定其它原效果。
- 新tests/original_presentation_test.cpp/CMake original-integer-presentation：真实DAT19标题资源，SDL软件renderer输出960×720、1280×960、1000×800。对每个RGB像素独立计算来源像素或黑边并逐一相等；视口分别160,120,640,480；0,0,1280,960；180,160,640,480。测试退出0。
- 截图：out/presentation-title-960x720.bmp、presentation-title-1280x960.bmp、presentation-title-1000x800.bmp。
- 原版边界：当前工作区未找到DOSBox reference BMP/PNG，本批没有操作/截图DOSBox，不声称DOSBox一致。标题场景无关卡编号；战场/对话场景未改变亦未验收。
- cl /std:c++latest /EHsc /utf-8 /MD targeted test编译执行通过；同配置/bigobj单独编译src/app_runtime.cpp通过。全部本批C++已clang-format。未全量构建、未发布、未打包；根Release保留。

<a id="2026-11-11-对话箭头第二批（真实生产修复）"></a>
<a id="history-v-062"></a>
### 2026-11-11 对话箭头第二批（真实生产修复）

- ASM41CBD5/41CBFF：screenY>310选择表第五项238，上方框Y=max(32,Y-100-H)、箭头X=clamp(X+12,0,591)、Y=框Y+215；41CC44选择237，下方框Y=min(Y-2,479-H)、箭头X=clamp(X+33,0,591)、Y=框Y-32。生产原资源选择反置，已修复并抽取共享OriginalDialogueLayout。
- original_dialogue_layout_test.cpp编译运行exit0，断言screenY=400/310与边界X=639，真实资源生成out/dialogue-stage140-layout.bmp、out/menu-resource19-layout.bmp，均640×480。场景为原开场序章（内部140，不等于第1《第一神劍》）及无关卡标题菜单。截图是复刻输出，不是DOSBox黄金帧；没有像素相等证据。
- 474E3E loc475009/475096：零opcode/重复零在arg0非零时写PAL0，否则保留背景；literal分支另行拷贝。4753A4 loc475555参数0时跳过零，参数非零rep movs。未发现足够证据改现有头像type1参数0路径；不擅改透明规则。
- 重新编译运行现有original_menu_test（out/dialogue-menu-regression.exe）exit0，覆盖字体定位、DAT19/20真实资源指纹、setup零opcode/重复零保留背景。此测试不覆盖所有菜单半透明，也不覆盖对话全部像素。
- 生产src/app_runtime.cpp以C++23 /bigobj单编译成功（out/dialogue-app-runtime.obj），新改C++已clang-format。CMake注册original-dialogue-layout；本次手工cl验证，未执行CMake配置或完整链接。未打包，根Release未替换。

<a id="2026-11-11-菜单透明合同与对话层顺序"></a>
<a id="history-v-063"></a>
### 2026-11-11 菜单透明合同与对话层顺序

- 40DF6D先4753A4(push1)拷贝DAT19，然后422402/4239AD绘文字；422402逐字解析，未见标题半透明框调用。战场431EFx先42927B逐行拷贝地图，再474E3E(push0)绘CEL194。不能因视觉描述在标题路径新增RGBA矩形。
- 431F2x第六项补片196调用474E3E(push1)；生产错误传透明true，改false。独立资源renderer同步按opaque=true期望。新增实读断言确认当前CEL196没有零样本，透明/不透明输出相同，故此处是合同修正而不是截图可见差异。首次尝试要求不同输出断言失败，已按资源事实修正后重编通过。
- 41CD56主框235先画，之后箭头push0，最后头像/姓名。生产原来箭头先画，现移至主框后、头像前，保持原坐标和纹理。未改头像或姓名未证实的几何。
- original_battle_menu_test重编执行exit0；original_dialogue_layout_test重编执行exit0，新增菜单叠层全帧alpha==255断言，输出out/battle-menu-indexed-overlay.bmp（640×480；开场序章内部140背景上的UI合成fixture，不是合法战场菜单流程验收）。对话/title截图亦重生成。
- src/app_runtime.cpp C++23单编译通过；所有修改C++已clang-format。当前Computer Use开关返回already on，但工具目录未提供桌面读取接口；未读取新的DOSBox像素，不否定用户已截参考帧。未技能、未打包、未替换根EXE；战场地面栏/遮挡尚未专项验证。

<a id="2026-11-11-选择范围立体框前景裁切专项"></a>
<a id="history-v-064"></a>
### 2026-11-11 选择范围/立体框前景裁切专项

- ASM447AFE两个ICONSO框均先42AFBA，再4468DF人物裁切，然后42B0C6绘制。生产Battle::draw_selection先前完全绕过map遮挡；新增可选indexed mask回调，Prologue前后两层均传map.mask_selection，范围同样裁切。沿用原资源与既有前景十二圈实现，不新增自创RGBA混合。
- tests/original_selection_occlusion_test.cpp读取第1《第一神劍》PRG地图和DAT5 ICONSO slots26/27/35/36/37，709合法格，真实资源被裁非零像素333366；断言只清零、重复一致，合成137行满色基准裁切382319。测试exit0，640×480 SDL BMP在out/selection-stage1-occlusion.bmp。此截图未自动选择角色或打开范围，范围证据来自逐格indexed资源断言，不称截图已覆盖完整交互。
- original_map_frame_test重编执行exit0：第1《第一神劍》/71《惡兆》122原帧，positive61/negative61，clipped10052/drawn431262；生产indexed反应与B1、RGBA已有回归通过。
- original_ui_layout_test重编执行exit0：开场序章内部140、第1《第一神劍》/71《惡兆》全角色地形栏上下位置和买入数量布局通过。42BEED坐标核对：缩图526,+40；X/Y595,+48/+64；高低数531,+14；名称595-len*4,+16；攻防图527/577,+91，符号548/599,+92及数字556/607,+92，与现实现一致。未虚称已验证DOSBox草地+5具体像素。
- src/app_runtime.cpp C++23单编译通过；所有本批C++ clang-format，CMake注册original-selection-occlusion（本次手工cl验证）。尚缺4468DF人物对框裁切、42F4D7补裁以及动画节奏专项；不将当前fixture外推整类完成。未全量构建/打包或替换根Release。

<a id="2026-11-11-目标框人物ad裁切与原地形加成续验"></a>
<a id="history-v-065"></a>
### 2026-11-11 目标框人物AD裁切与原地形加成续验

生产新增try_mask_selection_units：按4468DF/447AFE读取真实当前PRG帧AD，按前后框模式、当前格包含规则、可见状态、横向±2/纵向24格及高差进行清零裁切；选中目标前框不含当前格，431347空闲鼠标框仅友方当前格mode2。失败不提交输入bitmap，非法格/appearance拒绝回滚测试通过。第1《第一神劍》人物清零40380，第71《惡兆》56040，重复幂等；生产已接原框两层回调。尚未覆盖原action_state1的类别3/4额外高差与特殊隐藏类别，不称4468DF全部分支完成。

特殊补裁42F4D7并非完全缺失：既有mask_unit_ground尾部已实现scenery depth、边界、4000封印跳过及423F6F忽略0/32/33，当前选择回调也执行该段；其独立原结构重建断言仍待补。原DAT6地形表+5/+5记录实际存在；生产42C8D1等效计算在第1《第一神劍》(8,5)、第71《惡兆》(8,34)均输出+5/+5，按42C3C6..42C4B8坐标对上下状态栏真实字体ink像素断言通过，非DOS图硬编码。

动画核查发现剩余问题：43A51B原19百分秒到期只推进一次并43A60B把A8重置当前时间，当前idle_frame(elapsed/190)会在长帧追赶/跨帧；本轮未冒然加入draw副作用时钟，尚未修复。既有movement bitmap时钟专项通过，不外推idle节奏。当前画面批次尚未收口，因此尚未执行用户指定最终Debug→Release/根同步/package；不宣称已交付新版。新增专项、movement bitmap、生产单编译通过，C++已clang-format。技能33恢复时应使用真实CEL34/21帧；撤销历史外置spellsp缺失结论，不再把它作为阻碍。

<a id="2026-11-11-190ms呈现门控修复与画面阶段发布"></a>
<a id="history-v-066"></a>
### 2026-11-11 190ms呈现门控修复与画面阶段发布

新增OriginalIdleClock，生产Prologue帧呈现后授予一次姿态推进；到期>=190ms只前进一次并重置本次时间基准，长帧不按elapsed/190连跳。map接受独立pose时间，人物与人物对框mask共用，movement_pose仍使用原动作帧，不改动作输入锁/逐字等计时。无present不推进，非presentation驱动测试保留tick授予。暂停时宿主不tick；重复draw/镜头移动不推进clock。第1《第一神劍》/第71《惡兆》真实资源stall、重复draw、镜头往返、无present暂停/恢复clock专项通过；不是所有角色独立A8重置时机均已恢复，切换方向/换关局部时钟仍需后续精化。

特殊补裁未无证据重写：重新编译original_map_test实际执行既有独立PRG fixture，42F2A7旗标控制15例、42F4D7 scenery真实像素353034通过；含depth/封印4000条件筛选及忽略0/32/33，未将未触发的每一分支称全覆盖。

本次停止源码写入后按Debug→Release完整构建exit0，日志visual-batch-debug/release.log；根EXE已同步Release。根--verify、正常第1《第一神劍》自动友敌动作→玩家恢复1381帧、第71《惡兆》对应alternate1796帧均exit0。package-game生成15项运行包（14文件+启动器），单DAT/PRG、零txt/CDS，无原CD目录。独立out/visual-batch-independent解包--verify及第1《第一神劍》SDL1392帧exit0；未复制SAVE，原SAVE保留。根/Release/独立EXE SHA256一致6EF2654EC7645C4F8E9F0B892DCD24A092DE8BA6BCCB04DAEA9D2C9A7C5B49AB。包S2NE-game-win64.zip已更新。

交付边界：本轮明确修复整数呈现、对话资源/层序、选择前景/人物裁切及190ms长帧跳帧；DOSBox逐帧目视一致、所有特殊隐藏/动作类别与独立A8生命周期仍不能称全部完成。此包为当前画面改进版，不是115技能完成版。技能33未在本轮改动；后续必须真实CEL34/21帧，既往外置缺失说法撤销。新C++已clang-format；不以发布掩盖这些剩余边界。

<a id="2026-11-11-本轮最终对话显示验证（替代旧整数与箭头结论）"></a>
<a id="history-v-067"></a>
### 2026-11-11 本轮最终对话/显示验证（替代旧整数与箭头结论）

- 独立MSVC /std:c++latest重新编译original_dialogue_layout_test、original_presentation_test；日志out/display-dialogue-build.log、out/display-dialogue-results.log，均exit0。没有以源图哈希代替显示测试。
- CEL原索引RLE逐行解码237/238：尺寸22×41；237非零首/末行20/2（右下tip），238为2/20（左上tip）。locate(200,400,226)→frame74、237@(212,289)、tip_y329；locate(200,310,226)→frame253、238@(233,221)、tip_y221；边界clamp保留。纠正实际几何方向，而非声称ASM表选择本身如此。
- 实际Prologue自然推进捕获，不将briefing冒充对白。下框frame_y158、238@(353,126)，上框frame_y32、237@(76,247)；分别335/299个露出框外的尾部像素与原PAL直接展开RGB逐个匹配，证明方向、anchor及最后绘制未遮掉尾尖。640×480截图：out/dialogue-upper-640x480.png、out/dialogue-lower-640x480.png（另有BMP）。男女原DOS参考不同高度仅作参考，本次无DOSBox运行/截图/像素等价断言。
- SDL真实隐藏窗口direct3d11 renderer、streaming RGBA纹理，RenderTexture后读取完整backbuffer（含黑边），不是软件重采样假装SDL。960×720 viewport(0,0,960,720)，1280×960(0,0,1280,960)，1000×800(0,25,1000,750)，1280×720(160,0,960,720)。检查居中4:3、输出尺寸、框外全黑及框内非空。每个尺寸LINEAR/NEAREST实际输出不同RGB通道数依次1319540/2374154/1353732/1319540。双模式BMP位于out/presentation-title-{尺寸}-{linear|nearest}.bmp；960×720另有对应PNG方便查看。相同原游戏输入上传两次，柔化仅发生最后SDL采样。
- 源码冻结后Debug exit0：out/display-final-debug.log；Release首轮因根EXE锁定发布exit1，后重试exit0：out/display-final-release-retry.log。保留原有sscanf弃用warning，不掩盖首轮失败。未更改依赖和资源规则。
- 根--verify exit0：out/display-root-verify.log；根--smoke-battle exit0：out/display-root-sdl.log，2032呈现帧、1529 hostile帧，玩家移动/敌移动/两次side-banner。tools/package-game.ps1完成：out/display-package.log。
- 最新包S2NE-game-win64.zip独立解包out/display-final-independent。独立--verify、--smoke-battle均exit0：该目录verify.log/sdl.log，2054呈现帧、1559 hostile帧；没有复制用户SAVE。ZIP恰15项、1 DAT/1 PRG/0 txt/0 CDS，四DLL、EXE与根和Release逐个hash相同。
- EXE SHA256：699C97C0400B150648754FF9858C45BFEDB1503A889C7DAE25B51BE176E895D3。根和包均为最新Release，不是旧Debug。非全部玩法/技能验收，未宣称原DOS逐帧一致。

<a id="历史状态：恢复系统补全／传送实核与城镇探索首闭环（2026-11-10-2115）"></a>
<a id="history-p-006"></a>
### 历史状态：恢复系统补全／传送实核与城镇探索首闭环（2026-11-10 21:15）

- 最新批末统一Debug→Release/verify/42《小丑獻媚》隔离撤离SDL与package-game独立解包verify/SDL91帧通过；根/Release/包EXE SHA256 73EC7898D00873BDBE5906C6D7903D68B441F5979A8B18C6C0973A921645B547一致，1DAT/1PRG/0TXT。只代表本批有限接线交付，以下菜单/AI/死亡具体缺口未完，不是六类完成。
- 最新非技能收口：45DC37已依466544/effect39明确延期下一轮技能，不再阻塞本轮。42B461/42B6C3/452450/4535A1有非技能class4/5调用，已接route pose与撤离/death preflight；原资源与受控输入/计时/回滚测试通过，仍无自然class4/5状态样本，不外推完整自然流程。菜单实核缺旅店5/7的44D439装备/物品子页、8的43183B返回标题；AI撤退受控HP/保存通过；6《沙盜營地》与82《惡德商人》完整原表死亡dispatcher正常对话交互通过但非自然kill。完整type4..16 handler/completion1/3、全events回归等仍未完，具体原关/record见本文件分项历史证据最新节。
- 历史class4/5续作：503464共享phase、每actor19cs与camera mode1逐present提交已实现，重复draw不推进，错误返回保留pending；65《龍穴》原资源/受控class4/5 renderer与真实SDL通过。普通movement生产补每pose呈现锁，battle/AI6/7针对测试及42《小丑獻媚》29敌人170呈现隔离死亡撤离SDL通过。自然class4/5状态及45DC37仍未闭环，拒绝保留。
- 45DC37已追到唯一466544的技能effect39入口，不是普通移动命令；原SAVE五个battle文件以及1..158开场可控角色均无正常菜单可用effect39，未注入学习/装备/flag；当前技能39执行/合法选择route/确认事务仍缺。此为具体阻碍，不宣称不可达，详本文件分项历史证据最新节。本批Debug→Release/verify/最新Release撤离SDL通过，根EXE SHA256 5EC080724E0B267F085F7057CA7F135E37BFAA83BB5DD886E1973DC641B276F7，未重打包。
- 按用户恢复系统补全；下方暂停文字为历史，不再表示当前任务暂停。最新架构和两项UI不重复改造。
- 磁盘实核：45DB10侧末→454ED5生产事务及rollback、输入／保存锁已存在，不沿用本文件分项历史证据旧“只有图元”结论。重编当前合并资源版本专项通过，39次呈现完成坐标及action恢复；实际SDL通过52次呈现，来源为第60关《返鄉的遊子》原SAVE派生受控传送格／耗尽行动，不是自然走格E2E。
- 新接旅店option1→414D5B正常城镇探索：原DAT12世界PCX、DAT7 other-cell原5/6标记（不是PRG7）、BIN4城镇坐标／名字、当前与目的地、四像素方向滚动、左键重定位、悬停名称、Escape／右键返回；不改SAVE。第65关《龍穴》原SAVE未删表、未注入物品／旗标，入口／交互／返回及真实SDL专项通过，原旅店专项回归通过。
- 纠正上批城镇背景误读：41020B的FFFFFFF5是-11而非-12，已改真正DAT11世界图；旧DAT12验收不能算世界图等价。纠正后本批最终根／Release／独立包EXE一致`A8BE401BD0E92888390E52415FC0FC4BC7CAEB8AB72B431D5C7CA40AA4FABA4D`，Debug→Release／资源／传送SDL／独立包verify与SDL87帧通过，替代下方AF60阶段哈希。新专项强制实际视口移动(0,56)→(8,56)→(12,56)，不再允许静止虚通过；另delay10副本仅测计时边界，原第65关《龍穴》SAVE未改。
- 城镇续补已接生产边缘持续滚动（415415→4157C6）、原globals+32百分秒绝对差等待473111、每present推进，以及472EF1入口0..63／出口63..0各64级；第65关《龍穴》未改原SAVE和真实SDL专项通过，详见[分项验证导航](#validation)。本批停写后Debug→Release、资源／传送SDL及独立包verify／SDL通过；根与Release／包EXE一致`AF60865DF35B580D33866FEADDD50079384AACFE2153185389211E5D2A0D0089`。后续仍需继续特殊遮罩／起降等剩余范围，不能当六类总收口。
- 城镇键盘八方向／系统repeat已接同一present与原delay门控；尚非原Win32键消息频率逐帧对照。右键原内部区域门控已恢复。
- 后续继续定位已突破旧失败：65《龍穴》actor9 ordinary本就不可见；改原可见actor14，2613 AD像素精确上移64、570脚印变换、镜头+32全重叠像素、重复/恢复及真实SDL通过。class3撤离与452450路线pose已接，原120ms/取消回滚battle_test通过；详见本文件分项历史证据最新节。class4/5仅原slot及45CADA投影fixture，503464全局present/19cs门控未接，生产仍拒绝；45DC37及其余六类仍未完。本批后续已Debug→Release构建、verify并确认根Release同步，SHA256 E582C20BF85DE135BDA02719CF29DC1422FF1FC2FC88A8061E33AC6854B20D84；尚未打包/独立解包，不是六类最终发布。
- 历史后续生产接线尝试失败已回滚：42《小丑獻媚》原开场无class3/4/5；改65《龍穴》原class3受控action4 renderer仍与ordinary同图（exit4），当前buffer/可见/投影契约未闭环，未发布假通过。具体原地址、两次尝试和日志见[分项验证导航](#validation)；未重复打包、未改原档。
- 特殊移动续作新增448B99脚印索引mask小图元及原资源专项（DAT5 index21、palette map5），尚未生产接线；不算class3完成。42B327当前buffer取图／4468DF双军团遮挡／42B1D2投影仍需收口，class3/4/5严格拒绝保留，不用重画整幅背景替代。
- 边界：城镇音乐36→4尚未恢复；本次专项不是全部关间菜单或六类完成。特殊遮罩class3/4/5及起降仍待实现；技能／剑法及双线全流程不提前扩展。阶段停写后Debug→Release构建、资源核验、传送SDL及独立解包核验／SDL已完成；根EXE为本阶段最新Release（SHA256 `967AC31FDC743668FD913D047AE2CEAAE9001B5709CEDF5D76D9F65CF664929B`），包仅一DAT／一PRG／零TXT。此为有限闭环检查点，不是六类总验收；后续改动仍需重新最终统一验收。

<a id="本轮画面反馈复核（2026-11-08）"></a>
<a id="history-p-029"></a>
### 本轮画面反馈复核（2026-11-08）

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

UI timing correction: SDL edge scrolling now uses configured screen_delay centiseconds, one step per due poll with no backlog across modal/edge exits; frame sleep only uses the remaining 10ms budget. SDL banners acknowledge successful RenderPresent and show all nine original poses, including pose0 and pose8, without an invented settled hold. Headless tick callers retain logical timing. Standalone original_ui_timing_test covers cadence, stalls, reset, nine-pose presentation and budget. Prologue feedback/disappearance now use roster-index map identity and full-roster silhouette rendering.

Focused SDL --smoke-battle failure was production easiest-mode AI, not banner acknowledgement: difficulty1 sideDA1 dispatches 40AFE6 -> 43939D -> 47736A(reserve0), previously marked unsupported and returned without a path. Added nearest approachable target selection and original select_toward route execution. Release dummy SDL smoke now passes (1461 action frames, 1974 presented); standalone original_easiest_goal_test passes. Existing broad battle_test reaches a failing stage3 automatic-routing assertion after the changed enemy behavior; not claimed passing, no assertion relaxed.

Stage3 regression resolved as an isolated native-stat visibility encounter fixture: restored hostile pursuit changes encounters, leaving hidden77 at (1,45) after every visible enemy is defeated; a scriptless automatic run cannot guarantee discovery. Test now places hidden77 on a checked unoccupied/equal-height traversable tile beside a real observer, proves hidden before placement, revealed with observers, hidden again with observers removed, and retains the original all-enemies/death/bounded-turn assertions (passes turn12). No stats, invisibility bits or production visibility rules changed. Full rebuilt battle_test and easiest_goal test PASS; Release and Debug native builds PASS; final Release dummy --smoke-battle PASS (1461 hostile frames,1961 presented). No campaign autoplay launched.

### 打包清单追加更正（2026-11-10）

按用户要求移除新增的PACKAGE.txt和SHA256SUMS.txt；打包脚本及现有ZIP均已更新，17项=16运行/资源文件+启动器，不再包含这两个文本文件。前述含三辅助项的记录为旧包历史。

<a id="2026-11-11-画面第二批：正常对话定位箭头"></a>
<a id="history-p-031"></a>
### 2026-11-11 画面第二批：正常对话定位箭头

已实际修复生产对话箭头上下资源反置。原41CBFF在screenY>310选择238，41CC44在其余分支选择237；原移植相反。抽取OriginalDialogueLayout供生产绘制与专项断言共享，保留原框位置、X裁剪和CEL group2 merged-142映射。主框235、头像2*(speaker-1)、姓名中心80的现有选择未擅改。生成原开场序章（内部140，非第1《第一神劍》）640×480对话与标题菜单截图；没有DOSBox参考帧，不能声称纹理/头像/姓名或菜单半透明已对照完成。

已再次核对474E3E的零opcode/重复零由push参数控制，literal零仍写入；4753A4在参数0时跳过零。现有对话实现与这些已读分支一致，本批不把PAL0一律透明，不新增RGBA半透明。专项布局/截图、现有菜单字体与零run回归、生产app_runtime编译通过；未打包。战场地面栏/高低差遮挡/present节奏尚未处理。

<a id="2026-11-11-菜单与对话层合成续修"></a>
<a id="history-p-032"></a>
### 2026-11-11 菜单与对话层合成续修

核对40DEAF标题入口：DAT19先不透明拷贝，再文字，无半透明矩形调用；422402是文字函数，不是框合成。战场主菜单431DE8则先42927B拷贝地图，再CEL194透明repeat模式；合作第六项补片196按原push1应不透明零，现已纠正。实际当前CEL196无零样本，两模式画面一致，不能将此合同修正称为可见半透明改善。子菜单仍按原PAL映射6变暗，无新RGBA alpha效果。

另实修正常对话图层顺序：原41CD56先画主框235，再画定位箭头，最后头像/姓名；此前箭头先画被主框覆盖，现恢复原顺序。复刻640×480截图已重生成；全帧alpha255断言通过，未引入半透明RGBA framebuffer。仍待原DOSBox具体帧目视对照和战场地形/遮挡专项；本轮不宣称这些验收完成。

<a id="2026-11-11-第1《第一神劍》范围目标框前景遮挡"></a>
<a id="history-p-033"></a>
### 2026-11-11 第1《第一神劍》范围/目标框前景遮挡

发现具体生产差异：Battle::draw_selection直接把ICONSO范围与立体框拷至RGBA，没有经过原42AFBA→42F027的前方十二圈地形/物件裁切。447AFE明确两个目标框均先42AFBA，再4468DF人物裁切，最后42B0C6绘制。现复用现有已恢复的map indexed前景裁切，在范围/框转换RGB前接入生产；不改变既有框后层→人物→框前层顺序，不将RGBA转换本身判错。人物4468DF对框逐像素裁切与42F4D7特殊补裁尚未补齐，不称全部遮挡完成。

第1《第一神劍》709个合法格扫描：真实ICONSO27/28及36/37/38共333366个非零样本被前景遮住；裁切只清零且可重复/幂等。生成640×480 SDL截图out/selection-stage1-occlusion.bmp。现有第1《第一神劍》/71《惡兆》122原帧带正负偏移遮挡回归及状态栏上下边界专项通过。状态栏草地名称、地形缩图、X/Y及加成坐标与42BEED读段核对未发现足够证据更改；+5具体原帧目视与蓝绿选择完整流程、动画节奏仍待验收。无技能扩展、无打包、根Release未替换。

<a id="history-p-034"></a>
### 2026-11-11 目标框人物AD裁切与原地形加成续验

（重复证据已合并，见[同一原记录](#history-v-065)。）

（重复证据已合并，见[同一原记录](#history-v-065)。）

（重复证据已合并，见[同一原记录](#history-v-065)。）

<a id="history-p-035"></a>
### 2026-11-11 190ms呈现门控修复与画面阶段发布

（重复证据已合并，见[同一原记录](#history-v-066)。）

（重复证据已合并，见[同一原记录](#history-v-066)。）

（重复证据已合并，见[同一原记录](#history-v-066)。）

（重复证据已合并，见[同一原记录](#history-v-066)。）

<a id="2026-11-11-对话实际尖端纠正与最终输出线性采样（本轮最终发布）"></a>
<a id="history-p-036"></a>
### 2026-11-11 对话实际尖端纠正与最终输出线性采样（本轮最终发布）

撤销此前“整数呈现修复默认画面”和“238/237方向已正确”的交付结论。实际app_runtime默认960×720；INTEGER_SCALE只能放640×480，造成多余黑边。现恢复640×480逻辑LETTERBOX比例铺满，默认最终texture LINEAR；1280×960也满幅，非4:3仅必要黑边，不裁切，不修改PAL/透明遮挡/原游戏像素帧或资源。未新增设置菜单或CLI。

重新核对opening.idb.asm：4EA890表为233/234/235/237/238，426CEC直接减142；41CBFF原选择238、frame_y+215/x+12；41CC44原选择237、frame_y-32/x+33；41CD56先框后尾。不能再把资源名当方向证据。实际解码CEL：237为22×41，上行20个非零/末行2个，尖端在右下；238为22×41，上行2个/末行20个，尖端在左上。为落实指向屏幕人物的要求，本次保留上述anchor和框后尾层序，明确纠正为上框237向下、下框238向上，不虚称与该Windows ASM选择逐字相同；不翻转整幅图或人物。人物screen_position沿用原格left/top；原框高226，男女对白仍随人物screen_y产生不同高度，不新增性别固定高度规则。真实上下对白截图及尾部PAL像素匹配已验证。未操作或新截图DOSBox，不宣称DOSBox逐像素一致。

C++修改均clang-format，未新增throw/try-catch，保留C++23/std::stacktrace和现有Result错误边界；新增只读dialogue_layout查询供实际绘制和位置回归共用，并排除briefing等非对白阶段。专项通过后停止源码写入，Debug→Release构建发布。首轮Release链接成功但根EXE短暂占用导致发布失败，确认占用后重试Release成功（未实际终止任何进程，PID已退出）。根/Release/独立包EXE及四DLL逐个SHA256一致。根与独立包verify/SDL战斗均exit0，包15项、单DAT/PRG、无txt/CDS/个人SAVE。技能暂停，无其它画面改造。

最终EXE SHA256：699C97C0400B150648754FF9858C45BFEDB1503A889C7DAE25B51BE176E895D3。证据详见本文件分项历史证据本轮条目。

<a id="campaign"></a>
## 双线流程与终局

两线强化视觉终局有证据，正常属性前段与失败也保留。阶段编号不是连续战斗序号；同一日志中的旧阻断与后续修正均归档，不能把每个checkpoint当胜利后存档。

> **历史区域**：下方各原标题仅表示当时状态，不能覆盖[最新状态](#current-status)；误判按[撤销表](#corrections)阅读。

#### 历史原证据区域（保留原标题便于追溯）

<a id="2026-11-10-original58--158-late-flow-verification"></a>
<a id="history-v-032"></a>
### 2026-11-10 original58 ->158 late-flow verification

- Unified VS18 `S2NE.sln` Release/x64 build PASS (`out\stage158-build.log`), using D:\project\vcpkg. Existing warnings remain.
- Fixed story-entry omission: 410478+24 calls44A4A9(group0,mode0), just as40FEE9 calls it with mode1. Both entry paths now remove only original temporary class1 records (death20 or absent40), preserve retained original-slot correspondence, renumber, and clear the first bytes of11 group triples. Story mode0 applies45DDA6 pool/status/presence reset before427870 arrivals. No army truncation or arbitrary dead-role removal.
- Genuine isolated run-86412-stage58-start.sav regression:31 allied slots ->14 retained ->35 after original158 arrivals; retained growth bytes40..9F, identity/order, input state, and original save roundtrip checked.
- Original158 grouped movement has header flag13 but terminator flag0. 422074 ends at event_count when no matching terminator; the decoder now permits that and completion is bounds-safe. 4619AB drops only actors whose461EF8 route failed, not the entire parallel operation; regression checks failed actors do not teleport. Inert no-trigger group terminators do not block exhausted-story header routing.
- Actual SDL dummy strengthened run (`out\stage158-flow.log`):58 **內鬨** victory turn4 ->158 **休戰儀式** story executed ->59 **奚族聖地** original autosave reload and battle victory turn3 ->60 **返鄉的遊子** original autosave reload, then original defeat turn100 (16 enemies still alive).61 **超時空傳說** NOT reached or claimed complete. No forced victory, enemy removal, tactic experiment, or normal difficulty-rule change.
- Strengthened stats are NON-ORIGINAL flow-only evidence, not balance/legitimate clear evidence. All saves isolated under `out\strengthened-flow-acceptance`; user SAVE untouched. Replay checkpoint: `run-32276-stage60-start.sav`.
- Focused freshly rebuilt native regressions PASS: original_campaign_stage158_test (cleanup, bounded group scan, per-actor failed-route preservation, inert terminator route); original_initialization_test; original_campaign_victory_route_test (both12->15/14 raw entries); original_scripted_movement_test (141 single and both parallel groups); original_campaign_story_test (4->154 choices5/8). The older story test expected temporary actors to survive; updated to the original44A4A9 retained identity/growth contract. Changed-source editor diagnostics clear. SDL helper exited after genuine turn100 defeat; no helper/writer left running by this task.

<a id="2026-11-10-stage60-teleport-chain---genuine-stage61-entry-strengthened-only"></a>
<a id="history-v-033"></a>
### 2026-11-10: stage60 teleport chain -> genuine stage61 entry (strengthened only)

- Inspected the cancelled agent's actual `tests\original_campaign_stage60_test.cpp` / exe: it was a resource/visibility dump, not a passing quest regression. No SDL helper was running on entry. README/PROGRESS untouched.
- Original trace: `4296BF` caches first 20 category35 map cells in row order; `40DB58` / `40DC07` invoke `4608B2` at allied/enemy side end; case35 calls `42FFA2`, rejecting occupied pads in BOTH armies, selects with native bounded RNG, then `45DB10` relocates while preserving actor action state. Implemented production pad loading/side-end settlement; does not alter AP/HP/learned bits/flags or remove enemies. No destinations are hardcoded. Original teleport effect194 animation remains unimplemented (coordinate/state execution only).
- Acceptance-only manual mouse/BFS pad policy is restricted to stages60/61 and `--smoke-campaign-strengthened`. It routes to a real reachable pad only while no living enemy can be approached in the component, exits the side normally on the pad, and returns to existing combat once enemies are approachable. An initial probe unnecessarily stayed on pads even with reachable enemies; corrected before the successful second probe. No normal AI/stat changes.
- Actual SDL dummy command, from game root: `S2NE\out\x64\Release\envoy-opening.exe --smoke-campaign-strengthened . S2NE\out\strengthened-flow-acceptance\run-32276-stage60-start.sav` with SDL_VIDEODRIVER/SDL_AUDIODRIVER=dummy. `out\stage60-pad-flow2.log`: stage60 original scripts/victory report T50 with8 remaining enemies -> stage61 **超時空傳說** -> original autosave reload verified. Persistent entry: `out\strengthened-flow-acceptance\run-69588-stage61-start.sav`. This is NOT a stage61 ending or normal-balance clear. Stage60 is NOT final. Existing script routing, not forced victory or enemy deletion, selected61.
- Final-policy SDL bounded probe from that61 entry: `out\stage61-pad-flow.log`, manual input onto (9,81), later real pad relocations and normal combat; stopped at T43 with21 enemies. Do not repeat blind auto as a final-quest solution. All three owned SDL helper trees explicitly stopped using their own shellIds; subsequent process query found no envoy-opening.exe.
- Exact next missing coherent executor: category12 map item pickup -> persisted inventory -> item USE -> native learned-skill transaction -> hostile targeted spell effect34 -> boss equipment removal. Original resource item301 **不明礦石**: type12 (+14), consumable1 (+15), operand3 (+17), operand12 (+19), effect65 (+25). `450550` case65 at450FE1 calls `451DDA`; do not treat pickup as learned magic or set flags. Spell32 **高溫氣化**, group2/bit11, effect34 (+19), native record raw `03-0C-B0-AA-B7-C5-AE-F0-A4-C6-00-00-00-28-00-40-01-04-01-22-00-00-00-46-00-00-00-00-00-00-00-02-00-01-00-00-00`. Existing OriginalSkills executor deliberately only supports self-heal88, so32 is NOT usable yet. Stage61 resource boss role20 at(14,45), equipment14/125/183/165/199/237; item237 **邪神護身符** special25 (+25). Walkthrough locates301 at(3,65); category12 pickup native455324 ->42D200 clears the collected chest and its four candidate items; this has NOT been wired. Stage61 victory33, death dialogue group completion13 -> original flags42/43 and ending flags73/75; no synthetic flags added.
- Verified fresh MSVC tests: `cl /nologo /std:c++20 /EHsc /utf-8 /bigobj /I S2NE\src S2NE\tests\original_campaign_stage60_test.cpp /Fo:S2NE\out\original_campaign_stage60_test.obj /Fe:S2NE\out\original_campaign_stage60_test.exe`, then exe >`out\stage60-pad-test.log`: PASS real60/61 pad lists, occupied-source exclusion, both-army/side guards, all-full/dead/inactive rejection, AP/HP/learning/flags preservation, original save bridge coordinate roundtrip, actual60 entry manual-pad request. Fixture checks do not claim campaign victory. `battle_test.cpp` freshly compiled with same flags: PASS existing movement, combat, AI, first-route tests. Native `MSBuild S2NE\S2NE.sln /p:Configuration=Release /p:Platform=x64 /v:quiet /nologo` and Debug equivalent: exit0 (existing C4996 warnings); logs `out\stage60-release-build.log` / `out\stage60-debug-build.log`. Environment: VS18 Community vcvars64.bat, D:\project\vcpkg. Changed production editor diagnostics clear.

<a id="2026-11-10-stage61-mineral-pickup---use---learned32-bounded-real-sdl-proof-partial-quest"></a>
<a id="history-v-034"></a>
### 2026-11-10: stage61 mineral pickup -> USE -> learned32, bounded real SDL proof (partial quest)

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
- Precise outstanding native presentation:41F011 role illustrations are tall PCX scrolls (height-479,3-row steps/10ms), not implemented by the current640x480 decoder;41EDD4 full campaign summary and41F167 class/stat panels are not fully reproduced;41ED8A calls45EDAB movie3 tail after report cleanup. Those are known local resources/executors needing expansion, not missing data. Current host explicitly fails at this boundary instead of inventing a return-to-title or declaring completion. Full spell compositor remains separate. No Natalia work. Source/tests/本文件分项历史证据 only; all owned SDL helpers exited, writes stopped.

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
- Next required gameplay step is legitimate caster MP capacity/cost reduction (or another eligible learned caster), followed by a legal boss approach and actual cast; full effect animation and real ending dispatcher remain unverified. No successful SDL cast,237 consumption,boss death or ending is claimed. Source/tests/本文件分项历史证据 only; README/PROGRESS untouched. Owned SDL processes stopped by their exact tool shell IDs; no running helper remains.

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

<a id="后续验收顺序与终盘提示（2026-11-10，历史计划，终局／换盘已达成）"></a>
<a id="history-p-011"></a>
### 后续验收顺序与终盘提示（2026-11-10，历史计划，终局／换盘已达成）

1. 白志超路线必须继续到《超時空傳說》（资源61）及其真实结束流程，不把60的header后继0当作本轮路线完成证明。用户指出61为最后一关，需结合原条件转场核对。
2. 用户提示后期存在“手动进入传送点逐步接敌”及“关内拾取物品→使用→习得特殊魔法→对敌方BOSS施放”的必要链。具体关卡、道具、法术、BOSS及触发条件必须读取原资源／反汇编确认，不能凭提示直接注入物品、习得状态、剧情标志或胜利。先实现并验证该链，避免反复自动战斗至超时；强化只作为数值流程辅助。
3. 白志超路线终局验证后，测试娜塔莉娅路线全程，包含实际换盘节点的自动资源切换及存读档。
4. 两路线剧情流程验证后，专项审计法术、剑术等习得条件、使用限制、效果与动画；不得以剧情跑通宣称这些系统全部完成。音频暂缓、双人对战排除的既定范围不变。
5. 流程摘要必须包含原关卡名称及资源阶段号，区分实际通过、独立专项和未到达阶段。完整验证证据归本文件分项历史证据，不写回README。

<a id="最新验收边界：娜塔莉娅路线131（2026-11-10）"></a>
<a id="history-p-014"></a>
### 最新验收边界：娜塔莉娅路线131（2026-11-10）

- 100《草原逐惡》事件3的opcode12／condition19及20已按原逻辑实现通用移除，复用原存档事务及消失动画，保留固定军队槽位和死亡／未激活无操作语义。真实100及白线26／158回归、Release／Debug通过。
- 最新实际强化SDL通过资源阶段100→103→104→107→109→113→114→115→116→117→118→120→124→126→121→168剧情→129；各关原名与回合见本文件分项历史证据最新记录。
- 130《返鄉的遊子》的原超时失败已追查：验收驱动把传送台输入限于白线60／61。已移除该关卡限制，按原地图资源复用合法鼠标／BFS及回合结束输入，未修改地图、AI、剧情或胜利条件。实际强化SDL在第64回合原生胜利，进入131《超時空傳說》；真实130及白线60相关回归、Release／Debug通过。
- 131《超時空傳說》已实际第78回合原生胜利。验收输入已按131原资源及娜线主角接通拾取301《不明礦石》→正常USE习得32《高溫氣化》→原40MP／命中概率／随机装备槽施法→移除237《邪神護身符》→正常击败首领；未注入物品、习得、熟练度或旗标。先前第100回合失败为修复前历史，不再是当前边界。
- 娜线完整视觉结局已通过：原44阶段、CD2影片4→5→3、25003呈现帧、原清理分支正常exit0；另修职业图帧索引下溢。持久结果`out/strengthened-flow-acceptance/run-61460-natalia-ending-viewed.sav`字节一致重载通过，日志`out/natalia131-ending-complete-sdl.log`。Release／Debug、131真档／技能／白线60相关回归通过，父级停止写入后的统一Release／Debug复核通过。
- 白志超与娜塔莉娅两条独立强化剧情路线及完整视觉结局现均已有实际证据；这不是正常属性平衡通关认证，也不代表原版音画逐帧完全一致。音频暂缓、淡入淡出及完整特效合成仍有缺口。
- 法术／剑术专项首轮已审计原目录11类115技能（96非合作、19合作），逐类条件、原函数地址和差距见本文件分项历史证据技能专项。确证修复45C8CA喚術联动学习条件反转，新增全部115记录门槛及关系真值表测试；Release／Debug和五项相关回归通过，父级复跑全技能审计与真实转职事务回归通过。
- 实际实现仍有重大缺口：96条非合作技能中94条无技能专用执行器；32《高溫氣化》仅有限生产实现，88仅有限模块事务。现有菜单／目录／通用习得算法不等于这些技能均可在可玩流程合法获得或施放；完整技能动画尚未恢复。19条合作技能需单独记录边界，不与用户排除的双人对战混淆。
- 技能续轮已扫描168阶段原资源，确认63／74的剧情授予记录及104相关物品／原始角色记录入口，补原451DDA范围学习、opcode12条件27／28角色事务及effect66物品门控；入口可达性和学习弹框仍有单独边界，未以事务fixture冒充实际重走剧情。修复32在熟练15000封顶时整次施放被错误撤销，原支付／经验／RNG继续保留。
- 32已恢复原46C078角色准备子链，原PRG姿态、朝向及454E43的12百分秒呈现节拍，真实SDL专项通过，父级复跑范围学习／封顶施法／SDL专项通过。完整BIN10／SPELLSP调色、投射／目标合成及恢复仍未完成，不能把角色准备动作记为完整法术动画。
- 156额外回归失败已归因为旧fixture覆盖持久队伍：测试改走真实部署，两份实际档均通过，生产身份校验未放宽。最新Release／Debug及10项相关回归通过，旧“未归因”记录已由本轮证据取代。
- 32已接通原SPELLSP第33项mode2动画：原16字节头／8字节帧目录、调色映射140／45、23帧透明合成、原50ms节拍与结束画面恢复，损坏数据拒绝执行。真实Windows SDL专项呈现23帧／21个不同画面，恢复逐字节一致；父级已复跑视觉专项通过。证据`out/skill33-sdl-frame4.png`。这证明原合成子链，不冒充正常鼠标选择到效果结算的完整新端到端施法录像。
- 32正常输入端到端已通过：真实Windows SDL从原档走选人→原菜单→32→目标→确认→准备→23帧合成→原效果／结算→恢复。固定娜131存档自然未命中（chance6／roll66），未筛RNG；MP406→366、AP71→26、熟练3932→4054、等级18→19。取消前后原档一致，恢复后500ms无重复支付。修复攻击子菜单键盘路由及首次合成时钟重复计时，Release／Debug与相关专项通过；父级重跑持久证据专项通过。帧序列／前后存档见`out/skill32-input/`，日志`out/skill32-input-run.log`。这不是全部状态／遮挡及失败分支均已验证。
- 88《療傷心法》已接原后分区菜单、自身目标、type12准备、ST25及AP／熟练／经验结算；独立恢复原SPELLSP79的mode3、六色混合与35帧动画，非套用32。Release／Debug与相关专项通过，父级复跑Windows SDL专项35个不同画面、结束精确恢复通过。当前是隔离资源fixture验证，尚无真实可操作存档的正常输入E2E证据，不计完整验收。
- 32旧支付断言误报已确定性复现：漏算原升级MP增长，64案例中34例触发旧断言；已修证据与诊断按实际增长核对40MP支付，4种确定性呈现／停顿调度与真实SDL23帧通过。首次失败现场数值缺失，不能声称完全重建该次现场；父级复跑64案例／4调度通过。
- 88已从原始`SAVE/UJ02.SAV`角色3经正常回合推进恢复AP，真实Windows SDL走完正常菜单→自身→准备→35帧→支付／熟练→恢复：ST70→45、AP93→48、熟练3635→3650；取消及恢复后500ms存档一致，父级证据回归通过。原角色满血，疗伤0／XP0符合原规则，正疗伤／正XP仍仅fixture通过，不冒称实际受伤疗愈验收。修复88错误拒绝difficulty4／5，按原配置范围1..5，0／6仍拒绝。最新Release／Debug及六项相关回归通过。
- 32已独立核对原46D496→407E2D→4080E7，确证难度1..5有效并修错误门控；1..5进入原事务、0／6无支付回归通过，非从88盲推。
- 88正疗伤真实输入已通过：从原UJ02正常移动／回合推进接敌，经原AI使HP878→814，再正常自身施放。HP814→878、ST70→45、AP88→43、熟练3635→3655、XP0→12；准备与35帧、取消及恢复后500ms无重复支付均验证。未强化、未改HP／ST／技能／熟练或筛随机；真实受伤沿途档与帧证据在`out/skill88-positive-input/`。父级复跑正疗伤持久证据和32调度回归通过；最新Release／Debug及七项专项通过。
- 下一项已选94《虛空劍法》：真实131档角色32已学，原阈值0、ST5、范围3、effect1／formula44。已恢复独立SPELLSP85原mode2合成、着色233／35、15帧及结束恢复，Windows SDL15个不同画面通过，父级已复跑。Release／Debug及七项相关回归通过。
- 94命中入口已纠正：真实flags=0xC2，实际走46D748→46DD11，不走上一轮46D874。旧D874算术只是已核实的辅助分支，不能当94基础命中；未将错误入口接入生产。formula44原始伤害46F89E／4704AC证据保留。
- 94新增无状态后修正子链：按实际flags裁剪不触发的分支，核实职业抗性／弱点、特殊与状态减伤、随机与难度、最低伤害、护盾及最终清零次序；父级复跑resolution专项通过。最新Release／Debug、94真实Windows SDL15帧及32／88紧耦合六项回归通过。
- 94已完成42C88F地形三字节→46DD11算术；纠正此前“朝向”措辞，实际为地面／物件属性，不是面向。真实131档角色32／敌20只读DD11=45。实际effect1入口已核定4654A6→466082→46618D→471F51，按原命中随机→伤害随机→当前HP封顶／零升1→最终roll次序，64固定案例通过；父级复跑通过，最新Release／Debug及六项相关专项通过。
- 94隔离部分事务已恢复HP／+132反馈、常驻special25复活、special29反疗、存活奖励、ST5／AP支付及请求防重复提交；64固定随机案例覆盖命中／未命中／复活／反疗与回调失败回滚，父级复跑通过。紧耦合修复mode1战力快照及+132原档写回，生产身份校验未放宽。最新Release／Debug、七项相关专项及真实Windows SDL15帧恢复通过。
- 94新增special28首匹配装备消费／属性扣除、普通无脚本死亡／kill与loss计数、原掉落合并与99封顶，以及原档往返；奖励失败时装备／库存／HP／计数／RNG均回滚。纠正94真正击杀的rawXP为46913D提前返回1，未套通用尾公式。父级复跑64案例事务通过，最新Release／Debug、九项紧耦合回归及Windows SDL15帧恢复通过。
- 94死亡派发新增原41C02A有序扫描：首个匹配空／负行数记录直接返回0，不能继续后续同角色记录；正行数实际返回12h（十进制18）并要求同步剧情，仍拒绝执行。纠正此前把其它不可达条件返回11h／13h列为此入口产出的说法。特殊移除／召唤槽分支仍原子拒绝。父级复跑事务通过，最新Release／Debug及九项相关专项通过。
- 94有限生产路径已接通：真实army库存、运行态39及职业抗性来源、后分区菜单／范围3／准备／15帧／事务／保存；未知运行态、同步死亡脚本、特殊召唤／移除目标在呈现与随机前拒绝，不批量开放effect1。
- 重要资格纠正：此前131角色32虽已学94但原为未激活reserve，不能正常选中，未改旗标激活。实际E2E由同一原档原已学且激活的角色11完成，合法普通敌role0，首次施放自然MISS（chance50／roll53），ST219→214、AP71→26、熟练2800→2826、XP2→3；15帧、取消与恢复后500ms无重复支付通过。父级复跑原存档持久证据专项通过，证据`out/skill94-input/`及`out/skill94-production-input.log`。
- 94普通命中数字反馈已接原468BAC呈现链：逐present分块HP显示、至少8帧、无物攻flash，数字结束一次提交HP／费用／奖励／库存／RNG，输入期间锁定，不重复调用物攻结算。父级复跑反馈专项通过。
- 新实际HIT证据从上一轮MISS结果继续一次正常输入，改由下一位原激活已学角色16施放，未筛随机：chance50／roll32，普通敌HP减少1，ST230→225、AP72→27、熟练2800→2830、XP60→61。独立15帧后数字恢复、取消与500ms无重复提交均通过，证据`out/skill94-feedback-input/`；不能与角色11 MISS混称同次结果，也不是击杀验收。
- 94普通无脚本HP0已接数字→455048(3C,0)原死亡模式→单次发布保存：原独立十帧、调色3C／80%、30ms节拍，非旧故事五帧消失。结束前输入锁及延后尸体／费用／奖励提交。Windows SDL隔离整链23次present、一次提交通过，父级复跑通过；必杀／loot／支付／存档由64案例隔离事务覆盖。未取得正常输入自然击杀E2E，不把隔离整链称作实际击杀验收。
- 94新增受限同步死亡对白：completion先置位→原逐字分页／输入推进→可证明无副作用的41FF30扫描→死亡十帧→单次结算；数字／对白／死亡期间禁止保存与加载半事务。真实资源stage6角色104／stage82角色147专项通过，父级复跑通过；后者核实旗标13先于对白生效、返回仅一次。此为原资源fixture，不是自然击杀E2E。
- 最新Release／Debug及11项紧耦合专项、原事件finish-facing专项通过。完整original_events_test在branch0第4回合campaign_modal仍失败；隔离禁用本轮同步路径后同点复现，不能称完整原事件套件全绿，也未为通过放宽门控。
- 同步dispatcher已新增41C33E→41B9A3逐记录调用栈，支持type9／condition0嵌套扫描与原返回值重扫，不预展平对白。原stage11三条保留资源记录的隔离fixture验证五层嵌套、旗标顺序、单次返回、保存／取消锁及异常全回滚，父级复跑专项exit0。完整stage11仍有未支持handler，生产预检继续拒绝，不能把删去其它事件的fixture称完整原表／真实E2E。
- 最新Release／Debug及12项紧耦合专项通过；完整events已有对照失败边界保留，未冒称全套通过。其余type4..16及41FF6E的1／3／7仍严格拒绝。当前继续挑选实际阻塞原表且已有确证事务的一类handler，目标使一张未经删改事件的原死亡链可执行；自然击杀E2E仍待验。
- 其它93条非合作、19合作及94特殊移除／召唤、多段复活反疗呈现仍缺；不称全94完成。
- 其它94条技能专用执行器及19项单人合作技能等缺口继续保留。音频暂缓、仅双人对战排除。技能专项仍未完成，不因双线剧情跑通而计为技能系统完成。下节为累计历史，出现早期阻塞时以上述最新边界为准。

<a id="当前流程验证（2026-11-10，恢复推进）"></a>
<a id="history-p-015"></a>
### 当前流程验证（2026-11-10，恢复推进）

按最新要求恢复强化模式流程验证，报告原关卡名＋资源阶段编号，简要总结结算与转场；以下旧“暂停”仅指当时安排。保留日志确认此前已50《提那南橋》→54《力量與魔法》→56《和平會談》→58《內鬨》。本次从独立强化58入口实际SDL续测，第4回合完成原胜利报告，随后转场容量超限已按410478→44A4A9恢复剧情入口原临时角色清理与状态重置，31槽保留14、原158入场后35槽，保留普通角色身份／成长及顺序，未截断队伍。另修原并行脚本无匹配终结标志时扫描到事件末尾、逐角色失败路径处理，未瞬移失败角色。实际强化SDL已58《內鬨》T4获胜→158《休戰儀式》剧情完成→59《奚族聖地》T3获胜→60《返鄉的遊子》自动档重载；60仍16敌至T100触发原超时失败，61《超時空傳說》尚未到达。统一Release／Debug及158清理／入场专项复核通过，五项相关回归通过。60阻塞现已按原4296BF／4608B2／42FFA2恢复传送台侧末结算，并以合法鼠标／BFS进入原地图传送台；排除两军已占格，保留原随机选择和AP／HP／学习／剧情状态，未直接置胜。实际强化SDL《返鄉的遊子》（60）T50经原事件转入《超時空傳說》（61）并验证自动档重载，独立入口run-69588-stage61-start.sav；终局未完成。传送特效194动画仍缺。终盘资源已确认道具301《不明礦石》→使用效果65／451DDA→法术32《高溫氣化》→敌对效果34→首领20装备237《邪神護身符》的移除链，原矿石拾取、库存及动态宝箱链持久化、真实物品指令／USE确认→学习32《高溫氣化》已接通。实际强化SDL《超時空傳說》（61）T4经正常鼠标／命令输入完成拾取、使用、原档重载，独立入口run-84592-stage61-mineral-learned.sav；该次exit0只代表学习阶段专项，不代表结局。Release／Debug、拾取／学习／MP门控／渲染／存档专项通过。敌对法术32的原选择／目标／范围、施法姿态、effect34命中及随机装备消耗、经验／熟练度／AP／MP支付次序现已接通，Release／Debug及相关原档回归通过。实际SDL原学习检查点角色MP20／上限20，低于法术40MP成本，已正确拒绝且不损失敌装备；未成功实机施法或移除237。按“强化我方优先验流程”的当前要求，下一轮将只在显式强化模式提高并维持友方MP，明确记录非原成长；普通模式仍保留40MP门控，不增加学习、物品或结局标志。显式强化模式现补足并维持400MP，正常模式20MP仍拒绝40MP施法。实际SDL正常选法／选敌／确认已随机消耗165、183，继而237《邪神護身符》，未指定消耗槽或重置随机流；继续正常物理攻击使首领20由4000HP降至0，T14执行原死亡对白及胜利报告。持久检查点run-24456-stage61-boss-outcome.sav，此前旗标日志索引有误，现纠正为原一基旗标42=0、43=1、73=0、75=1。已补原opcode9／condition1条件执行，实际SDL重放死亡脚本→奖励→原终局入口，持久入口run-79908-stage61-terminal-pending.sav，并移除终局错误进入关间的路径。Release／Debug及终局条件专项通过，resource3原结局文字、背景、肖像与角色分页已接通；实际SDL正常输入显示41页，保存／回读run-60956-white-ending-viewed.sav，原42=0／43=1／73=0／75=1保持不变，未添加完成旗标。Release／Debug与专项通过，该早期部分展示exit1现已被后续完整视觉验收替代：41F011原长图滚动、职业／战绩／总结面板及CD2 movie4→5→3已实现。最终实际SDL按原计时完成57阶段、25555呈现帧，movie3完整1142帧，执行原返回WinMain清理退出分支，正常exit0；run-49056-white-ending-viewed.sav回读通过，未伪造返回标题或完成旗标。父级统一Release／Debug及终盘专项复核通过，日志out/white-ending-final-native-sdl.log。白志超强化剧情＋完整视觉收尾已验收，不代表正常属性连续通关或音画逐帧完全一致；原音效调度、淡入淡出及完整法术合成仍缺。娜塔莉娅已独立新档正常选择原难度1，显式强化模式首批实际SDL验证71《惡兆》T11→72《青色草原》T12→73《神秘的氣息》T10获胜，原对白／奖励／关间／部署与自动档重载通过；均CD1，不计实际换盘。161《返回奚族》已复用原461535事务统一剧情／战斗转换，并修正mode3原档恢复被误作战斗的派发。实际SDL继续161剧情完成→74《邊城提那》T5胜→75《驅離守衛》T4胜→77《撤退保護戰》入场；原路线1／进度7在77实际自动选择CD2，原资源读取及自动档重载通过，不能再将72当换盘证明。该批在明确CD2入口边界exit0，未计77胜利或全线结局；下一入口run-83796-stage77-start.sav。161／26相关回归、Release／Debug通过；日志out/natalia161-batch2-sdl.log，显式续跑参数`--smoke-campaign-strengthened-alternate-continue`保留换盘／存档验证并解除首CD2入口停止限制。实际《撤退保護戰》（77）至T4时出口路径被当前单位占位阻断，地形自身仍连通，未通过本关；已持久保存stage77-route-blocked-103152.sav，新增复现及161／换盘回归、Release／Debug通过。已修验收策略：77原目标仅出口，动态敌占位时正常自动战斗后重试出口，未改碰撞。实际强化SDL已77《撤退保護戰》T7胜→78《突圍的選擇》T10胜→79《暹族部落》T4胜→162《罕泉》剧情完成→81《暹族的伏擊》T3胜→83《會面》T4胜→84《丑子軍團》T6胜→89《孤零的寶兒》T10胜利报告。89转场歧义已按原顺序执行语义修正：后续旗标链把原90转场覆盖为108，并非任意选首／末分支；白线双分支回归通过。实际强化SDL续测89《孤零的寶兒》T8→108《殺之宴》T6→91《領主府內》T10→96《義助刑天》T3→97《賢者歐西德》T9胜→165《賢者歐西德》剧情完成→100《草原逐惡》入场。100单角色移动已按45DEE9／45E107修正：找到角色后原事件正常消费，寻路失败仅保持原坐标，不应锁死剧情；继续使用原动画执行器，未瞬移。真实100及白线141／157／158回归、Release／Debug通过。实际SDL越过移动事件后停于100事件3 opcode12／condition19缺原执行器，保留run-104788-stage100-blocked.sav，继续还原；尚未通过100或全线结局。独立89入口run-97452-stage89-start.sav，日志out/natalia77-retry-sdl.log，Release／Debug及77目标回归通过；全线结局仍未到达。完整法术／传送特效仍未验收，娜塔莉娅全线尚未开始。未注入物品、学习标志或结局状态。日志：`out/stage158-flow.log`，独立强化60入口`run-32276-stage60-start.sav`；详细证据见验证文档。强化数据不计正常成长通关。

<a id="当前验收策略：强化我方先验证流程"></a>
<a id="history-p-017"></a>
### 当前验收策略：强化我方先验证流程

按最新要求，暂停各种战术尝试，优先推进流程。新增显式参数 `--smoke-campaign-strengthened`，沿用连续SDL验收与隔离临时存档，仅在该模式把存活且活跃的玩家角色／behavior1友军NPC的HP上限、当前HP、攻击、防御设为30000，并在流程中维持。不会复活死亡角色、直接设置胜利、跳过事件或修改普通启动／原难度规则。该模式属于人为强化验收，不代表原版平衡、合法成长或正常通关。

用法：`envoy-opening.exe --smoke-campaign-strengthened <资源目录> <原存档>`。入口原存档只读；导出的强化检查点单独保存在 `out/strengthened-flow-acceptance`，不能混作 `out/campaign-acceptance` 的正常战役证据。首次Release构建、编辑器检查通过，实际SDL dummy渲染／控制流程从真实第15关入口在强化模式第4回合获胜，经奖励及后续转场进入第16关并验证自动档重载（两个入口检查点已独立导出）。第16关推进到59回合仍为2名可控队员、8名敌人，已主动停止该次辅助进程。核查原438656–43867C／4620AA及真实资源确认：本关目标为条件31（主角到达原出口且flag1成立），不是歼敌；真实第12回合flag1已成立，主角仍在(11,34)，原出口为(1,19)、(1,20)、(2,20)、(1,21)、(1,22)。当前烟测输入只对第10关且清敌后寻出口，导致第16关持续自动打斗；已补验收驱动基于原条件31／object6元数据的合法出口输入，第10关保留清敌后寻出口策略，第16关flag1就绪即可寻出口，不要求清敌；原胜利条件未改。Release／Debug构建及资源目标回归通过。实际SDL从强化第16关检查点续测至第2回合，主角(10,32)、13敌，寻路报告不可达并持久保存stage16-route-blocked-92932.sav；严格占位及允许友军让路的诊断均无出口路径，回归报告起点可通行邻边为0。进一步核实原477DE4–477E6E允许路径经过友军、仅禁止落点被占，现已修正本体与验收BFS此前把经过／落点一起禁止的问题。真实检查点地形可连通五个出口，NPC2在(2,21)造成此前假断路，并非缺桥；新增真实检查点通行回归及battle通过。实际强化SDL已第16关第4回合获胜，继续经17、20进入156；统一Release／Debug构建及battle通过。156剧情非活跃角色变更事务已按原461149–461173修复：死亡／非活跃角色无修改地返回成功，由41BA1E–41BA56消费事件；不复活、不创建或强制激活。实际强化SDL已20→156→23关第2回合获胜→146，146事件1 opcode11已按4211A3恢复条件0–6的原角色谓词，condition2使用440D4C的身份／状态语义，不是菜单；不匹配记录保持休眠，未强制执行。统一Release／Debug及146身份谓词、两分支对话、休眠记录与存档往返回归通过。实际强化SDL继续23→146→25→30→31→37→38→39→26；26入口已查明并非空场，而是原NPC专用段落：22活跃、32第2回合登场，持久主角队伍保持非活跃符合原部署。已修正烟测只统计可控队员导致误判失败的前提，并把战斗可见脚本接入已有执行器；未激活主角队伍。统一Release／Debug与真实26入口、NPC部署、可见脚本及原增援插入回归通过。实际强化SDL推进至26关第7回合，原461535控制归属转换及condition1撤离已接入，未附加激活、复活、阵营搬迁或强制标志；26／146／156回归及统一Release／Debug通过。实际强化SDL已26关第7回合获胜→40关第6回合获胜→41，自动档重载通过。41出口驱动的误报已修复：原条件31仍选友方身份11（flag41时为1），不是22／32；本关另有胜利组33，缺少可选出口主角时不应中断正常自动行动。仅修验收策略，未激活角色或改胜利条件。真实41及既有16／10出口回归通过，统一Release／Debug通过。实际强化SDL已41第3回合获胜→42第4回合获胜→150→46，46剩2敌到第100回合触发原超时失败后，核实本关为歼敌目标，幸存者均带abilities80h隐形，原441EFD要求附近观察者。验收输入已补合法BFS／鼠标接近揭示，再交回未改的原自动AI；没有直接取消隐形或设置胜利。真实46及16／41回归、统一Release／Debug通过。实际强化SDL已46第27回合获胜→47第5回合获胜→157，157事件3脚本移动已修复：角色6的movement_type为4，原45E107／45E450允许该类型，scripted_route却误限为1；现移除错误限制，保留原地形代价、朝向、80ms等待及HP／AP／状态。真实157及141单人／并行移动专项通过，统一Release／Debug与157回归复核通过。实际强化SDL已47→157→48第15回合获胜→50；阶段50第12回合已无活跃敌军，到第24回合仍未结算，辅助进程已停，正在核查原目标／脚本，不直接置胜。新增强化入口run-50492-stage50-start.sav，日志out/stage157-flow.log。资源阶段号不等于连续战斗序号；剩余战斗数量需按原后续转场及分支核算，尚未确认。证据out/stage46-reveal-flow.log，独立强化入口run-102056-stage47-start.sav；不计正常属性通关。证据out/stage146-flow.log及stage146-next-boundary.log，独立检查点run-72852-stage26-start.sav。统一Release／Debug与156活跃／死亡／非活跃完成及存档往返回归通过。日志见out/stage16-transit-flow.log及out/stage156-flow.log，新增独立强化23关检查点；该结果不是正常属性通关，也尚未全流程完成。日志见 `out/strengthened-flow.log`。音频仍暂缓。

<a id="单人全流程验收推进（进行中）"></a>
<a id="history-p-020"></a>
### 单人全流程验收推进（进行中）

验收标准是依据原规则自行完成单人战役并到达原终局；首战胜利、进入下一关或独立规则测试均不能替代全通关。双人对战不纳入范围。

已新增连续战役启动测试 `--smoke-campaign-continue`，不在第2关提前退出，每次进入战场核对原自动档。第4关后的154阶段已从原事件确认为模式3剧情中间阶段，不应按普通战斗或再次结算奖励处理；原选择事件指向第5或第8阶段，正在恢复该转场并接入实际运行。此段记录为进行中，不是验收通过声明。

连续 SDL 实跑已完成第1关（第13回合）与第2关（第14回合）的胜利和自动档核对，第3关在第100回合触发原超时败北；结束时主角仍存活，日志中的敌军均已死亡。后续查明并非特殊目标移动：首领77仍隐形存活，AI 用移动前位置评估未来攻击点的可见性，导致漏选目标；已按原 `40B0BA` 修复候选观察者坐标。最新连续 SDL 实跑通过第1关第12回合、第2关第11回合、第3关第11回合、第4关第8回合，并正常经过154剧情进入第5关、第25回合胜利。再次实跑仍通过第1至5关，随后定位到具体错误：第5关胜利报告后尝试存档时仍有原生移动动画状态，触发 `Cannot save during a native movement animation`。该问题已修复：仅在原胜败结果确认、移动和攻击反馈均结束后清理残留显示状态，不移除存档安全检查。后续实际 SDL 连续运行已完成第1至7关及154剧情分支，每关均验证自动档重载；第7关胜利后进入第10阶段时报 `Invalid original event index`，已定位并修复入口：第10阶段没有首回合对话，旧流程将未找到事件的末尾索引交给对话准备函数。现按正常战场初始化、自动档与横幅流程进入，保留第2回合对话的原触发时机。真实资源入口及第2回合触发回归通过，连续 SDL 全链复测进行中。该关第二回合选择、剧情角色加入适配及原特殊胜利条件已补入并通过专项回归。最新实际连续 SDL 已进入第10关并在第27回合消灭敌军，但该关要求原出口条件，不能以敌军清零代替胜利；已修正连续测试对此的提前失败判断。原普通自动战斗没有已证实的自动寻出口策略，因此测试在歼敌后通过正常选人、移动命令与实际AP逐步走向原出口（选择索引1、主角11、出口21/4或20/5或21/6），不改生产AI、不直接设坐标／旗标；连续验收再次到达第10关；现已保留真实战后检查点，主角(5,37)的34边出口路线被(6,36)的角色14阻挡，该角色HP仍正但状态十进制32即死亡位0x20、无活跃位0x80，是此前阵亡角色在部署时补满HP池但保留死亡状态，并非该关剧情离场或复活。原空间查询排除非活跃角色，正在修复占位门控，不通过选择／移动阵亡角色绕过。此前在剧情执行期间触发移动动画状态存档保护，已补齐角色加入／增援事务前的动作排空和残留姿态清理，保留原存档保护；真实 NPC 移动中触发事务的专项回归通过，Release／Debug 构建通过。真实战后检查点的实际 SDL 出口操作已通过：按原地形命中查询校验选人和目的格点击像素，五次正常移动于第17回合触发原出口事件。此前名义投影中心被高差地形遮挡，属于测试点击错误，未修改生产命中规则。出口后的错误阻塞已按原 `41BC44` 修复：胜利仅分发匹配胜利组或条件0的type2事件，不应因未选择的休眠事件阻塞。实际 SDL 已于第17回合完成出口→原胜利剧情／报告→奖励→关间出征→第141剧情阶段，Release／Debug通过。第141阶段的场景人物初始化与原镜头指令已通过专项回归，正在接入原脚本移动及镜头跟随；该移动不同于玩家耗AP移动，不能用瞬移或移动后返还AP替代。娜塔莉娅首战→72及自动档重载的共享逻辑回归再次通过。最新实际 SDL 已从真实第10关战后检查点通过出口胜利→奖励／关间→第141完整剧情→原选择第11关，并验证第11关自动档重载。第141的另一原分支第12关仅专项回归通过，未作该次实际分支通关。后续同次实际 SDL 第11关于第49回合胜利并进入第12关、验证自动档；第12关推进至第64回合时触发测试每阶段600秒的墙钟超时，并非原游戏判败。已保留真实第12关入口存档。续档暴露的opcode12条件0角色重置分发遗漏已修复；完整原开场、角色重置及第7回合增援的原生诊断通过执行，但固定seed73在第12回合主角阵亡，并非AI停滞。该存档不保存随机流，不能将固定种子原生结果当作SDL相同结果。后续实际SDL完整日志已确认一次第10回合主角阵亡，与固定种子原生诊断同属真实败北，不是隐藏敌人卡死。已从真实第12关入口存档验证主角实际等级19／经验11（不沿用此前等级推测），通过SDL原命令4完成81→83转职及保存重载；独立持久续玩档只执行原转职事务，职业83、等级1／经验0，输入原档未改写。该合法转职档实际自动作战仍在第9回合主角阵亡；不能把转职验证通过视为战役通关。正在核对全队合法准备及手动战术，不修改生产AI以强行保送，不以更换随机种子筛选胜利；不能削弱敌军或伪造胜利以通过，全队检查仅主角11和角色15符合合法转职条件；独立准备档已执行81→83及10→22，并验证全部库存、装备、能力与其他记录不变。转职并非直接强化：主角防御101→97，不能假定转职即获胜。原第12关另有合法战术条件：在第7回合增援链执行前击败首领72，可通过原旗标事件阻止四名敌援到达；正在以正常战斗行动验证，未直接设置旗标。仍未全通关。

第155剧情的原群体／单人动作、150ms等待、角色五帧消失及延后去活跃、后续对话与状态重绘已接入，真实资源脚本回归走到原头部指定的第16阶段；这只是专项流程验证，不代表已从真实战役通关进入该阶段。不能宣称全通关；不回改回合数规避失败。

<a id="resources"></a>
## 资源、发布与历史包

当前为单DAT／单PRG的S2NEPK03去重封装；历史CDS与双PRG包记录只证明当时产物。源归档、字节等价、哈希与独立包证据保留；33后根Release未打新包。

> **历史区域**：下方各原标题仅表示当时状态，不能覆盖[最新状态](#current-status)；误判按[撤销表](#corrections)阅读。

#### 历史原证据区域（保留原标题便于追溯）

<a id="2026-11-10-本批统一构建独立包证据（不是六类总完成）"></a>
<a id="history-v-015"></a>
### 2026-11-10 本批统一构建/独立包证据（不是六类总完成）

停止本批C++写入后Debug→Release均通过，最新Release --verify与第42关《小丑獻媚》隔离撤离SDL再次通过。package-game生成14运行/资源文件+launcher；独立解包`out/systems-batch-independent`，verify exit0、真实SDL opening-right-skip exit0/91呈现，当前远程音频设备实际输出330246非静音样本且位置推进（只证明该开场片段，不是音频完整恢复）。根/Release/独立包EXE SHA256 73EC7898D00873BDBE5906C6D7903D68B441F5979A8B18C6C0973A921645B547一致。独立库存1DAT/1PRG/0TXT/4DLL，无源CD路径。日志`out/systems-batch-{Debug,Release,verify,evacuation,package,independent-verify,independent-sdl}.log`。保留原SAVE及证据；不重跑强化全战役。

<a id="原程序与分析资料"></a>
<a id="history-v-031"></a>
### 原程序与分析资料

分析目标为原 Windows x86 程序 `WSUPERSP.EXE`，SHA-256：

```text
58A9808E090DE51F56059F965D313BB274A08B82A3526E3C8991C4D3F978AAD2
```

本地 IDA 数据库及完整汇编位于 `analysis\ida\`。运行需要用户本地已有的原游戏资源，本项目介绍不包含原游戏程序或资源的分发授权。

<a id="2026-11-10-本地检查副本与精简打包验收"></a>
<a id="history-v-054"></a>
### 2026-11-10 本地检查副本与精简打包验收

新增 tools/publish-runtime.ps1，原生vcxproj AfterTargets=Build同步EXE及四个实际运行DLL；Debug与Release均构建exit0，最后Release覆盖根目录，SHA256与out/x64/Release一致：9EEA4EF3EDFF2187907B9439C5C4E06500149DE19285463D528AD65577B19F2E。备用CMake POST_BUILD亦配置同步，但本轮未运行CMake，不能称该入口已验证。

新增 tools/package-game.ps1，用显式运行清单而非整目录复制：EXE、SDL3/SDL3_mixer/fluidsynth/mpg123四DLL、PAL/BIN/CEL/PRG/CFG/ITEMSP/KINDSP七文件、当前main实际读取的MP3/SUPERSP_A02.mp3，以及运行时首选windows-disc1和windows-disc2各一个DAT；加run-game.cmd、PACKAGE.txt、SHA256SUMS.txt，总18个ZIP项。无个人存档、旧EXE、重复光盘、调试符号、源码/测试/分析产物、攻略或修改器。不可裁剪原索引归档内部内容，避免破坏偏移。目标Windows x64，仍需Microsoft VC++ x64系统运行库。

打包命令：powershell -NoProfile -ExecutionPolicy Bypass -File S2NE\tools\package-game.ps1。最终S2NE-game-win64.zip为220993658字节。曾发现遗漏当前引用MP3的初版包，已重打替代，最终独立解压后全部15项SHA256核对通过；包内--verify原标题与双开场解码exit0，--smoke-opening-right-skip实际SDL dummy呈现105帧exit0，混音322773个非零样本。临时解压目录已清理，未触碰原SAVE或结局证据。DLL直接依赖经dumpbin核查，包内运行未依赖工程目录。该验证不冒称完整战役重新跑通或全新机器VC运行库安装测试。

<a id="2026-11-10-交付更正：s2neexe及不同cd2-prg"></a>
<a id="history-v-055"></a>
### 2026-11-10 交付更正：S2NE.exe及不同CD2 PRG

用户要求EXE与sln同名：原生TargetName改为ProjectName、备用CMake OUTPUT_NAME=S2NE，同步publish/package/run-opening/README。Debug与Release实际MSBuild均exit0，最终资源根S2NE.exe与Release SHA256同为999D1E9650A7056572B1FA2AF150A521CBF299363DD543264A07BC0AD1DC4183。历史envoy-opening文件不再发布且不入新包，未批量改写历史验证命令。

确认根SUPERSP.PRG与windows-disc1相同，SHA256 F3B3EF5587536991062879782209C7E104A19BD4DC568169C1ADAD7A4A69498A；windows-disc2不同，SHA256 C6E36E77DEC30C3FEC80E156096B8F6DFBB2DF35A97CC07FF5061815C0E9A62F。上包只带根PRG，确有遗漏，现追加cd/windows-disc2/SUPERSP/SUPERSP.PRG，CD1不再重复放cd子目录。最终ZIP248254008字节，16项运行资源加三辅助项；独立临时解压全部哈希通过，包内S2NE.exe --verify和--smoke-opening-right-skip退出0，103帧，当前音乐非零样本322773，临时目录清理。

源码核实仍有独立运行缺口：OriginalMap构造及其它PRG消费者直接读根SUPERSP.PRG，set_disc仅改disc字段和reaction缓存，不重载PRG。此次修正没有未经原证据盲改所有PRG索引，包含双PRG不代表生产已正确换PRG；原双路线记录也不能当作该项验收。后续必须独立核原PAL与CD2 PRG索引/加载调用并补回归。

<a id="2026-11-10-prg用途核实及上一结论纠正"></a>
<a id="history-v-056"></a>
### 2026-11-10 PRG用途核实及上一结论纠正

直接核原473A55 case2（473B26..473B56）：4F7D68非零用安装目录supersp.prg，为0才4737AD解析光盘路径。原42800C在69066附近清默认值、随后fscanf DEVICE首值写4F7D68；本地SUPERSP.CFG实际DEVICE=1,0,F。因此上一节将“固定读取安装PRG”定性为换盘缺陷证据不足，现明确撤回，不应盲目替换CD2 PRG。

按同一原PAL目录比较：第1组136角色记录、第2组4417动作帧、第3组4284地面图、第4组896物件图、第5组360场景图、第6组132地图、第7组属性表、第8组50背景图，共8组全部SHA256一致。第9组（原PAL offset100703123/length11086）不同，CD2余下尾部也不同；第9组用途与CD2内置布局尚未完整核实，不能称全文件只有尾部填充差异。两文件头SuperSp_P，均9组；CD1长度100714209、CD2长度103481337。PRG是原图形/地图资源归档，不是待运行的游戏EXE，也不是DAT影片/换盘资源的同义替代。当前恢复所用前8组在双盘实字节相同，故现证据不支持必须按关卡切换PRG。

<a id="2026-11-10-包内容精简"></a>
<a id="history-v-057"></a>
### 2026-11-10 包内容精简

按用户要求，package-game.ps1不再生成或打入PACKAGE.txt、SHA256SUMS.txt，已重建资源根S2NE-game-win64.zip并检查ZIP目录：16个运行/资源文件加run-game.cmd，共17项，无上述两个TXT。其余资源与程序清单不变；上文含说明/哈希清单的包记录为历史。

<a id="2026-11-11-资源来源纠正：撤销“缺失外置-spellsp”阻碍"></a>
<a id="history-p-002"></a>
### 2026-11-11 资源来源纠正：撤销“缺失外置 spellsp”阻碍

**此前“技能33依赖缺失外置 `spellsp`”结论撤销，属于资源来源误判，不再作为当前阻碍。** `spellsp` 是 `SUPERSP.CEL` 第一个资源组的读取错误标签，不是外部文件名；技能33仍未完成，当前缺口是原资源与运行时对象字段的实现／契约映射，不是缺文件。

- 原ASM `4638E6 → 463BCA` 初始化全局对象 `5055D8`；`463D6D` 调用 `473CDB(EAX=1, EDX=1, EBX=&组二元组)`，读取CEL1内层数量与八字节索引至对象 `+2Ch/+30h`。`473D43/473D6A` 明确选文件 `supersp.cel`；`473EB3` 打开文件，`473EDF` 返回组偏移／大小并定位。`463EA6` 按 `+34h` 选择 `+30h` 索引，seek到“组基址＋记录相对偏移”，读入 `+15Ch`，头指针为 `+40h`，不是打开独立spellsp。
- 字符串 `004E497D`（原WSUPERSP.EXE文件偏移 `0xE2F7D`）经 `off_4EAE14` 引用；`463D6D+3Bh/+B5h` 等失败分支传入 `4729F3(EDX=2)`，由 `472A87` 格式化 `Read %s error!`。字符串和lseek不能证明外置文件。
- PAL读取／解码在 `425BF9/425CD2`；当前loader `src\spell_resource.hpp` 已按 `PAL[808]−56` 定位CEL1，实测基址28、133项。`OriginalSkill32Visual` 使用CEL第33项。**技能33 → visual33 → CEL第34项，21帧；此前23帧是技能32 → visual32 → CEL第33项的编号错位，不能计作技能33视觉验收。**
- Windows整合RAR、DOS整合RAR与当前工作区的CEL／PAL大小及SHA-256完全一致：CEL 41,570,946字节，`f7c5168af2d90001020a17df4811142b9152c7676a4532412ef6a80dfd54918c`；PAL 2,488字节，`773859f26194a2718dc1c0b688fe0fd0dad3c832ab75078ab94dc0e1b4cf3db9`。不存在需要另补DOS版spellsp的证据。

旧失败与错误推断作为历史保留，但不再支持缺资源结论；原严格拒绝只说明实现未完成。本次仅纠正文档，未改生产源码、未继续技能实现、未打包。完整只读核查边界见[分项验证导航](#validation)。

<a id="交付更正：s2neexe与双盘prg（2026-11-10）"></a>
<a id="history-p-012"></a>
### 交付更正：S2NE.exe与双盘PRG（2026-11-10）

- 游戏EXE已改为与方案同名的`S2NE.exe`，原生两配置／备用CMake输出名／根目录发布／启动器／打包清单均同步。Debug与Release构建通过，最终根目录Release哈希一致；旧envoy-opening.exe保留为历史文件，不再发布或打入新包。
- 用户指出双盘PRG不同，经哈希确认根目录PRG与CD1一致，CD2不同。上一包确实只带CD1 PRG，已更正：根目录保留CD1，`cd/windows-disc2/SUPERSP/SUPERSP.PRG`保留CD2，各只一份；DAT也保持各盘一份。新包248254008字节（约236.8MiB），独立解压16项哈希及改名EXE校验／SDL开场通过。
- PRG用途进一步核实后纠正上一结论：原42800C从CFG的DEVICE首值读4F7D68，当前DEVICE=1；473B26..473B56在其非零时始终使用安装目录PRG，仅为0才经4737AD查光盘路径。因此“未随盘切PRG就是缺陷”的判断不成立，不应盲目换成CD2。按原PAL逐组哈希，两个文件前8组（角色／动作／瓦片／物件／场景／地图／属性／背景）全部相同；第9组及余下尾部不同，尚未完整解析其用途。打包目前保留不同CD2 PRG作为独立原资源，不意味着运行必须加载它。

<a id="最新本地交付：根目录检查程序与精简游戏包（2026-11-10，以下包名／体积为更正前历史）"></a>
<a id="history-p-013"></a>
### 最新本地交付：根目录检查程序与精简游戏包（2026-11-10，以下包名／体积为更正前历史）

- 原生MSBuild Debug／Release完成后自动同步游戏EXE及四个运行库到资源根目录；最新构建配置会覆盖检查副本。本次最终保留Release，根目录与Release EXE SHA256一致。备用CMake入口也已配置构建后同步，当前实际验收使用原生sln，未宣称备用入口已构建验证。
- 已生成资源根目录`S2NE-game-win64.zip`，220993658字节（约210.8MiB）。工具为`tools/package-game.ps1`，固定使用Release；包含15个运行／资源文件及启动器、包说明、SHA256清单。
- 双盘仅保留运行时优先选用的windows-disc1／windows-disc2各一份DAT，不带重复disc1／disc2；仅带当前实际读取的MP3曲目，不带原EXE、修改器、攻略、备份、源码、测试、日志、个人SAVE和其余未引用独立资源。原索引归档BIN／CEL／DAT必须完整保留，不能删除其内部条目破坏偏移。
- 已在独立临时目录解压验证15项哈希，运行包内程序原资源校验及SDL开场右键跳过，均exit0，当前引用曲目混音正常；临时目录已清理。此为本地检查包，不是全游戏完成或独立机器VC运行库兼容认证。
- 剧情现状：两条独立强化路线及完整视觉结局均通过；技能为32／88既有验收路径及94受限生产，另93非合作／19合作仍缺。94最新嵌套死亡调用栈仅隔离子集验证，完整原表复杂handler仍拒绝；自然击杀E2E未验。完整events第4回合历史失败边界保留。
- 用户本次打包期间已停止继续派发技能实现；未丢弃既有源码。后续从现有磁盘继续，先完成完整原表的一类dispatcher支持及对应验证。

<a id="来源"></a>
<a id="history-p-026"></a>
### 来源

目标：WSUPERSP.EXE，x86 PE，IDA 自动分析识别 Watcom 运行库。
SHA-256：`58A9808E090DE51F56059F965D313BB274A08B82A3526E3C8991C4D3F978AAD2`。
原分析数据库及汇编导出已迁移到 `analysis\ida\`，其中数据库为 `opening.idb`。该目录仅作本地分析缓存，由 Git 忽略；仓库不包含游戏二进制、资源、IDA 授权文件或反编译代码。旧工程目录已移除。

<a id="engineering"></a>
## 工程、诊断、复现方法与音频

现行Windows x64／C++23／SDL3及VS入口见README。旧C++20、旧EXE名和构建命令保留历史，不执行；音频完整还原暂缓，非永久排除。

> **历史区域**：下方各原标题仅表示当时状态，不能覆盖[最新状态](#current-status)；误判按[撤销表](#corrections)阅读。

#### 历史原证据区域（保留原标题便于追溯）

<a id="2026-11-10-构建目录清理"></a>
<a id="history-v-058"></a>
### 2026-11-10 构建目录清理

检查无活跃MSBuild/cl/link/游戏进程后，仅清理可再生及明显重复产物：旧CMake build整个目录（当前原生sln不依赖它）、out/obj中间目录、out根82个obj，以及out/x64两配置旧envoy-opening产物、重复旧测试EXE/PDB和ILK。共535文件，回收554.12MiB。保留当前S2NE.exe/PDB与四运行DLL、out根专项测试EXE、全部日志/截图/序列CSV/存档及战役/技能证据；未删SAVE、源码、原资源或游戏包。

清理后资源根S2NE.exe --verify退出0，原标题与两开场解码成功。Release与根EXE哈希仍一致，Debug/Release及两结局、94实际输入与88正疗伤关键存档存在核对通过。下次构建需重建obj；旧build如需备用CMake可重新配置生成，未改IDE配置或工程源码。

<a id="2026-11-10-1802-暂停补全：main职责重构／格式／无模态诊断"></a>
<a id="history-v-059"></a>
### 2026-11-10 18:02 暂停补全：main职责重构／格式／无模态诊断

main拆为app_options.cpp/hpp（参数与资源根）、app_campaign.cpp/hpp（原档盘号、关间adapter和转职）、app_smoke.cpp/hpp（驱动状态与前后输入），原生vcxproj/filters与CMake同步。相同格式基线4250行→2478行，约42%；压缩旧源1467行与格式化新行数不可直接比较。agent词法对照smoke/转职提取保持等价。所有9个本次C++文件clang-format --dry-run --Werror通过，父已复核；新增format-code.ps1及VS Code C/C++保存格式设置，沿原_clang-format，不批量改无关代码。

runtime_diagnostics.hpp在main首行调用：CRT debug报告file/stderr，SetErrorMode抑制系统错误窗、abort/参数/terminate/purecall/SEH给stderr并致命退出。tests/runtime_diagnostics_test.cpp Debug独立编译，crt report退出0，assert/abort/parameter退出1，未处理C++/SEH非零，均无等待窗口且有stderr。一次Start-Process初始化abort进程返回0xC0000142无日志，直接运行与后续子进程通过；未隐瞒该系统初始化异常。诊断日志out/runtime-diagnostics-*.log；不承诺能拦截外部调试器或第三方显式MessageBox。

Release/Debug MSBuild exit0，日志out/refactor-{release,debug}-build.log。Release verify/menu/new-game/battle/shop/promotion通过；Debug verify/menu通过。Debug battle/shop触发MSVC span下标越界stderr断言；旧main独立对照同点复现（out/refactor-baseline-smoke-{battle,shop}.err与新debug对应.err），不是重构引入证据，但仍是未修缺陷，不能列全绿。断言现在终端退出，不忽略继续执行。未重跑完整战役或技能专项。

最终根Release SHA256=9DED3DCC606E3FA740AA9392EFD8C1B98AF193C7C3BC3ABB08B9E4A419BFC916，与out/x64/Release/S2NE.exe一致；父重打共享资源ZIP189423232字节，独立临时解包--verify及--smoke-opening-right-skip通过，104帧，非零混音361650；临时目录清理。包不附两个TXT。补全任务暂停，原档与旧证据未撤销。CDS仍为82映射40块去重内容，无须用户手工执行。

<a id="ssd重构、统一资源去重及点名ui交付（2026-11-10）"></a>
<a id="history-v-060"></a>
### SSD重构、统一资源去重及点名UI交付（2026-11-10）

本节覆盖18:02重构记录的旧状态。开发工作目录为D:\project\S2NE-workspace；原目录复制4986文件后包含隐藏文件逐项SHA256一致。未删除旧E盘资源、SAVE或历史证据。

### 架构与错误回归

- 全部本次C++按现有clang-format规则整理。原生MSBuild x64统一Debug后Release构建均退出0；两配置--verify、--smoke真实SDL均退出0。日志out/all-errors-{Debug,Release}-{build,verify,smoke}.log。
- src剥离注释和字面量后的扫描out/all-errors-lexical-scan.json只剩main中的try/catch；main外显式throw/try/catch为0，Windows自定义栈API/VEH/DbgHelp及longjmp为0。工程采用C++23标准调用栈，不再自实现平台栈展开。
- out/runtime_error-completion-test.log显示Error创建点tests/runtime_error_test.cpp:12及标准库栈条目；可恢复battle/event/map/skill失败回滚通过。未知库异常的main兜底明确不伪称拥有原始throw现场。
- 菜单、商店、转职、教会、装备、设置、存档菜单、确认及报告专项见out/*-all-errors-test.log；剧情移动结果见original_campaign_motion-completion-test.log，娜线结局所有原生页面回归见original_natalia_ending-completion-test.log。此处是专项，不是双线完整战役重新通关。
- 存档bridge原settings_4f7cf0读取修复，避免从29字节globals块offset32越界；Debug存档专项、战斗smoke已通过。

### 资源与UI

- 合并档逐字节去重后DAT=180661337字节、PRG=100706959字节；两档281368296字节，较完整双源节省193231888字节(40.7147%)。100项源资源字节等价、132地图、原CD目录不存在时读取、损坏拒绝、片头结局及地图回归通过。原始归档保留，不以去重后较小误判丢失CD2内容。
- UI原依据42BEED地形面板；4224F2购买小字、411F5B价格、411FA1/4138F7数量页。original_ui_layout专项及原商店交易测试通过；渲染证据out/ui-*.png。未覆盖全部城镇、长装备提示及原程序逐像素对照。

### 发布与独立包验收

- 根S2NE.exe与out/x64/Release/S2NE.exe及out/dedupe-final-package内EXE SHA256一致：78438462FDE99B4246B4C8AEFC51BD70EA791B360F4CBB7024CA61E2E49C0A90。
- S2NE-game-win64.zip使用最终Release重新生成，仅一DAT、一PRG，无CDS、源CD目录及额外新增TXT。
- 独立解包out/dedupe-final-package后--verify、真实SDL --smoke均退出0，呈现91帧。音频设备可输出不代表完整原音频恢复已完成。
- 游戏系统补全保持暂停；本次仅重构、资源及用户点名UI问题，不宣告技能/剑法、AI、死亡剧情及全流程全部完成。

<a id="历史状态：ssd重构与点名ui修复验收（2026-11-10）"></a>
<a id="history-p-007"></a>
### 历史状态：SSD重构与点名UI修复验收（2026-11-10）

- 开发目录已迁至D盘SSD；旧E盘副本不再作为开发入口。游戏系统补全继续暂停，本次仅工程重构及用户点名的两项UI修复。
- 原生Debug→Release统一构建、两配置资源校验与真实SDL smoke通过；根目录S2NE.exe已恢复为最新Release，最终独立游戏包重新生成。源码已停止写入。
- 语言标准升级C++23；main仅调用run_application并保留顶层catch(...)。剥离注释/字面量的全src词法扫描：main之外throw/try/catch为0。错误通过Result及原有事务回滚传播；未实现原函数桩仍明确终止，不冒充已还原。
- 调用栈仅使用std::stacktrace，Error创建点保存栈；不再自行调用Windows栈API/VEH/DbgHelp。未知标准库异常仅main兜底，明确无法凭catch恢复原抛出点调用栈。CRT致命错误终端输出，不弹诊断窗。
- 两源DAT/PRG各合为一档并逐字节去重：DAT 180,661,337字节，PRG 100,706,959字节，共节省193,231,888字节。运行时仅一套资源，旧SAVE字段兼容逻辑剧本域，不再换物理CD；源归档保留。
- 修复选人简略状态旁地形面板（42BEED），购买小字/价格/数量越框与叠印（4224F2、411F5B、411FA1/4138F7）；专项与渲染证据通过，未声称全城镇逐像素验收。
- 已修复存档恢复从错误全局块读offset32造成的Debug span越界，补充回归；下方18:02旧状态中的“越界未修”“CDS”“main2478行”“旧发布哈希”不再代表当前版本。
- 详细证据见[分项历史证据](#validation)。本轮未重跑双线完整战役，未继续技能、剑法、AI、死亡剧情系统补全，完整音频还原仍暂缓。

<a id="历史暂停与工程整理（2026-11-10-1802，已由上方状态更新）"></a>
<a id="history-p-008"></a>
### 历史暂停与工程整理（2026-11-10 18:02，已由上方状态更新）

按用户要求暂停系统补全，先重构与诊断：

- main按职责提取app_options（CLI／资源根）、app_campaign（存档盘号／关间adapter／转职）、app_smoke（测试驱动状态与事件前后处理），不是整段搬家。相同clang-format基线main4250行降为2478行，约减42%；原1467行是未展开压缩格式，不以原始行数误算。程序规则未故意改变。
- 所有本次C++改动按现有_clang-format格式化并dry-run通过；新增tools/format-code.ps1，VS Code C/C++保存格式化已配置。
- 启动首行配置runtime_diagnostics：CRT断言／参数／abort／未处理C++及Windows异常输出stderr、致命错误非零退出，不弹MSVC诊断窗。独立子进程专项通过，不影响外部调试器主动暂停。
- Debug／Release构建通过；Release资源／菜单／新游戏／战斗／商店／转职smoke通过，Debug资源／菜单通过。Debug战斗／商店仍span越界断言，重构前main对照复现同点，当前转为终端错误但根因未修，不称全回归通过。
- 根S2NE.exe最终为Release且哈希一致，ZIP已重打；独立解包资源校验与SDL开场通过。系统补全保持暂停，未重跑全战役／技能。
- SUPERSP.CDS为双盘DAT去重共享档，82映射／40内容块，约节省85.6MiB载荷；不是存档、不需要手动执行，源CD不删除。

<a id="构建与验证"></a>
<a id="history-p-024"></a>
### 构建与验证

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

<a id="重新生成"></a>
<a id="history-p-025"></a>
### 重新生成

[tools/export_functions.idc](tools/export_functions.idc) 可通过 IDA 原生 IDC 导出当前数据库中的函数，无需 IDAPython。脚本会在数据库旁生成 `.functions.tsv`，人工审阅后更新仓库的函数清单。

```powershell
.\tools\generate-stubs.ps1
```

[生成代码](src/generated/) 会整体覆盖，暂不在这里直接实现业务逻辑；确定签名和功能后应调整生成策略或移出对应实现，避免覆盖人工修改。

<a id="声音排查"></a>
<a id="history-p-027"></a>
### 声音排查

`test-audio.cmd` 会打印 SDL 音频后端、系统播放设备及实际非静音混音样本数；不再仅凭播放时间推进判断声音正常。当前验证环境的 WASAPI 唯一设备为“远程音频”，非静音样本检测通过，但不能证明远程客户端扬声器已出声。远程桌面应设置“远程音频播放：在此计算机上播放”，并检查客户端及 Windows 音量混合器中程序的静音/音量设置；程序不会擅自修改系统音量。

可设置环境变量 `S2NE_AUDIO_DEVICE` 为目标设备名的一部分以指定输出。不存在的设备会明确报错，不会悄悄使用其他输出。默认继续跟随系统默认播放设备，轨道与混音器增益显式设为 1.0。

<a id="cross"></a>
## 跨分项调查与历史独有证据

本区专门保留无法拆开而不损失调用／阶段语境的跨项原记录、旧进度清单和阶段计划。所有“当前／最新／暂停／完成”均限当时；当前未完成以顶部为准。

> **历史区域**：下方各原标题仅表示当时状态，不能覆盖[最新状态](#current-status)；误判按[撤销表](#corrections)阅读。

#### 历史原证据区域（保留原标题便于追溯）

<a id="2026-11-11-只读复核纠正：撤销“缺失外置-spellsp”阻碍"></a>
<a id="history-v-003"></a>
### 2026-11-11 只读复核纠正：撤销“缺失外置 spellsp”阻碍

**旧结论“463EA6打开外置spellsp，资源缺失”错误，现明确撤销。** 旧失败／拒绝仍保留为历史，不代表当前缺资源；当前技能33缺口是实现／契约映射，尚未完成生产施法验收。本次只改PROGRESS／本文件分项历史证据，没有改生产源码、继续实现技能或打包。

### 原地址与句柄来源（opening.idb.asm）

- `4638E6 → 463BCA` 构造 `5055D8`，清零资源字段；`463D6D` 以 `EAX=1, EDX=1, EBX=&组二元组` 调用 `473CDB`。对象 `+2Ch` 为内层数量，`+30h` 为八字节 `{offset,size}` 索引，`+34h` 为当前记录号。
- `473CDB` 的 `473D43/473D6A` 选择CEL域第1组，实际文件名为 `supersp.cel`，索引来自 `4F7D9C`。`473EB3` 打开该文件，`473EDF` 输出组二元组并seek到组起点，返回文件句柄；不是从5055D8取得独立spellsp文件句柄。
- `463EA6` 再用同样的CEL1调用取句柄；按 `+30h + 8*(+34h−1)` 取记录偏移／大小，seek位置为“CEL组基址＋记录相对偏移”，读入 `+15Ch`，设置 `+40h` 头指针并构造 `+148h/+14Ch/+144h/+150h/+154h/+158h` 等字段。旧文把 `473CDB` 说成分配缓冲、把该seek说成独立文件读取，均不成立；实际内存分配调用为 `430330`。
- `spellsp` 原字符串VA `004E497D`、WSUPERSP.EXE文件偏移 `0xE2F7D`（929661），经 `off_4EAE14` 引用。`463D6D+3Bh/+B5h`、`463EA6`、`46421C`、`46B003` 的读失败分支将其传入 `4729F3(EDX=2)`；`472A87` 格式化 `Read %s error!`。它是资源读取错误标签；附近Open／Write字符串以及lseek均不能证明独立文件。
- `425BF9` 读取PAL，`425CD2` 起建立并解码组表，CEL计数扣56、索引逐项扣56+i。`src\spell_resource.hpp` 已读取 `PAL[808]−56` 的CEL1基址28，内层133项，组长度32,735,578。

### 编号纠正与复核边界

**技能33 → BIN10 visual33 → CEL1第34项：绝对偏移7,769,315，长度209,750，21帧。此前23帧是技能32 → visual32 → CEL1第33项：绝对偏移7,568,349，长度200,966。** `OriginalSkill32Visual` 的既有23帧证据保留，但不能归入技能33完成度。此次仅读取表／头核对来源，没有重做技能33连续trace或施法测试。

### Windows／DOS归档与哈希

通过系统tar直接列举两个整合RAR并以 `tar -xOf` 在内存读取成员；DOS两张ISO用Python标准库解析ISO9660，没有安装依赖、提取或覆盖游戏文件。E目录递归仅13个文件，旧展开目录不再存在。归档来源：`E:\game\超时空英雄传说\北方密使Windows整合版\北方密使Windows版2023整合版.rar` 与 `E:\game\超时空英雄传说\Dosbox北方密使.rar`。

| 成员 | 两版共同大小（字节） | SHA-256 |
|---|---:|---|
| SUPERSP.CEL | 41,570,946 | `f7c5168af2d90001020a17df4811142b9152c7676a4532412ef6a80dfd54918c` |
| SUPERSP.PAL | 2,488 | `773859f26194a2718dc1c0b688fe0fd0dad3c832ab75078ab94dc0e1b4cf3db9` |
| SUPERSP.PRG | 100,714,209 | `f3b3ef5587536991062879782209c7e104a19bd4dc568169c1adad7a4a69498a` |

当前根CEL／PAL与两版同哈希；根PRG／DAT为合并资源，不能拿其大小差异判断缺spellsp。DOS RAR另有UVCONFIG.DAT（21,307字节），没有独立spellsp。

| DOS ISO内成员 | 大小（字节） | SHA-256 |
|---|---:|---|
| CD1 SUPERSP\SUPERSP.DAT | 89,599,904 | `0d4815a004f6ca41b954bab59e83041451d7031801c63c460ee9240feb4d0cf3` |
| CD2 SUPERSP\SUPERSP.DAT | 180,804,734 | `18950416558c6fe9bc1645b3969da24e8cbe12ebf2629d79b2f734fd185fa2f6` |
| CD2 SUPERSP\SUPERSP.PRG | 103,481,337 | `c6e36e77dec30c3fec80e156096b8f6dfbb2df35a97cc07ff5061815c0e9a62f` |

两盘各另有3字节DISK.ID，未见独立spellsp；上表与已有 `cd\windows-disc1`／`cd\windows-disc2` 展开文件大小及哈希一致。Windows光盘RAR目录仅有GCD003-1.mdf/.mds及GCD003_2.mdf/.mds，本次没有再次独立解码RAR内MDF，不将既有展开目录冒称为本次MDF读取证据。结论是CEL1资源已存在，不能再以“缺失外置spellsp”阻断实现调查。

自动化启动测试使用隔离写档目录，避免覆盖用户存档。算法测试、实际游戏流程测试和原版画面对照应分别记录，不互相替代。

<a id="2026-11-10-2257续作：正常新游戏与跨关两路径实核／纠正旧结论"></a>
<a id="history-v-013"></a>
### 2026-11-10 22:57续作：正常新游戏与跨关两路径实核／纠正旧结论

上节把bridge低层拒绝误认成生产初始化缺失，结论错误，现明确替代：`app_runtime.cpp`新游戏首关autosave前已调用`OriginalInitialization::fresh`，关间option4部署后已调用`transition_stage`；不是任取UJ档作baseline。本次这两调用改`try_fresh`／`try_transition_stage`并显式检查Result，新游戏restore也改try_restore；失败不发布rawSave，原failclosed保留。其余规则未臆造或扩展。

原证据：44367C army结构与inventory清零、44382B十一组三元状态；443C3E／4443B3角色raw362字节清零后按原101字节source与KIND／ITEM资源赋值；427645／427870原turn到场；opening配置给两线首关、初始货币，实际140对话完成flags及settings映射至4F7CF0。40FEE9入口先427609、44A4A9(group0,mode1)、44A79C(group1)，turn4F7CDC=1、next4F7CE4→stage4F7CD8，再4274B5／436C06；410478对应mode0故事入口。当前transition保留原我军raw／inventory／metadata／货币／settings，按44A4A9去原临时角色、清11状态首字节／重建敌军，再装目标stage／event states／dynamic chains／到场记录，未用跨stage capture强改globals绕锁。

- `save-init-build.log`／`save-init.log`：重编`original_initialization_test`exit0；旧两处期待runtime_error的测试改当前try API失败断言。真实140开场两选择；第1关《第一神劍》17名turn1、第71关《惡兆》22名turn1，难度1..5、原设置／到场／摆位／raw codec及bridge往返；1→2《攔路虎》库存／metadata／progress与全部字节往返、71→72《青色草原》原部署及stage恢复；非法73跳关拒绝。受控flags边界测试与真实140开场测试分别存在，不混称均为自然输入。
- 新`--smoke-fresh-save`只复用正常标题／设置／对话输入，进入第1关《第一神劍》后核实生产autosave已完成，capture→隔离slot52 write/load→bridge restore，未读源SAVE或注入flags／物品／强化属性。`save-init-fresh-final-sdl.log`exit0；独立包本无SAVE，同专项`save-init-independent-fresh-sdl.log`exit0／439呈现，这是可复现非拷档初始化证据。仍非手动点击战场保存菜单专项；菜单另由上批原资源键鼠测试覆盖。
- 正常跨关真实SDL命令`S2NE.exe --smoke-campaign`：先Debug `save-init-transition-sdl.log`600s到第1关turn8超时，未推进／伪造胜利；改优化Release `save-init-transition-release-sdl.log`正常turn10获胜。最终源码`save-init-transition-final-sdl.log`exit0，正常第1关《第一神劍》turn10胜利→原奖励→关间option4→第2关《攔路虎》，目标slot52原autosave load→bridge restore→capture全部编码字节相等，3400呈现；无HP/MP强化、无物品flags注入、无强制胜利。只跑这一跨关，不重跑全战役。
- 最终停止C++写入，`save-init-Debug.log`→`save-init-Release.log`exit0；`save-init-verify.log`exit0；`save-init-package.log`package-game；独立解包`out\\save-init-independent`，verify与上述真实fresh SDL exit0。根／Release／独立包EXE SHA256 `53A44E0CE0B04A2FB5BDAD2EB214F17C8FCC92EE130BC5A7AD77BDB872E12ADC`一致。包仍单DAT／PRG、无多余TXT，不含源SAVE。原档和全部失败证据保留。

边界：这消除“正常新游戏无baseline、正常跨关全状态缺失”的两个误报，不开放任意stage跳转／未知模式，也不宣称完整剧情／未来技能所有持久语义已实现。上节断电／写满等未覆盖仍有效；不自动移画面类别。

<a id="2026-11-10-生产特殊移动继续尝试及阻碍（历史失败，已由上节继续定位）"></a>
<a id="history-v-019"></a>
### 2026-11-10 生产特殊移动继续尝试及阻碍（历史失败，已由上节继续定位）

`tests/original_movement_coverage_test.cpp`重编运行：`out/movement-coverage-build.log`／`movement-coverage.log`列原1..158可读关卡开场名册。第42关《小丑獻媚》class3/4/5均0；第65关《龍穴》class3=11、4/5=0。不能为42注入移动类型再称真实入口，也不能以开场名册4/5=0认定自然后续不可触发（状态／装备动态门控仍需追踪）。

实际两次尝试生产renderer：新增显式渲染class选择→448B99静态64x32脚印→4468DF邻人mask清除→42AFBA→原AD mode4抬高64px；第42关测试找不到原class3角色，exit2，记录`out/class3.log`。转第65关《龍穴》实际class3角色，控制action4并强制present谓词筛选后，画面与ordinary仍相同，`out/class3-render.log`exit4；尚未定位bitmap可见／投影与控制入口分歧，因此删除该实验与对应短期测试源码、恢复生产class3/4/5严格拒绝。没有把失败分支发布为已实现，也没有修改原SAVE／flags来制造自然流程。证据日志保留。

确证原链：42B461在action4才440F56分派；class3先448B99再42B1D2(mode4，即elevation+4)，class4→45CA1D强制交替shared23/24，class5→45CAA0方向4F70FC；45CADA先脚印再mode3/4特效、复制AD最后mode4。45DC37还需476707/476A78路表、452B64→452AD5→447E59→4483CB→4535A1合法移动结果，成功才恢复12E/130并xor status4；当前未形成可验收实现。无缺失工具证明，属于原生状态／buffer契约尚未补齐，不声称无法继续或六类完成。本次未重复整工程打包，根Release仍为上一有效批。

<a id="2026-11-10-2115-恢复系统补全：传送生产实核／原城镇探索首闭环"></a>
<a id="history-v-021"></a>
### 2026-11-10 21:15 恢复系统补全：传送生产实核／原城镇探索首闭环

下方旧“传送尚未生产接线”已由磁盘实现及本次重编验证替代，不删除历史失败。`out/teleport-current-build.log`重编当前头文件，`teleport-current.log`：第60关《返鄉的遊子》原地图、受控传送格和行动耗尽，生产end-side 39次present、原action恢复、坐标提交和保存锁通过。根程序`--smoke-teleport D:\project\S2NE-workspace`真实SDL通过52次present，保留原来源SAVE及`teleport-controlled-before/after.sav`；仍非自然走格E2E。最初直接运行旧测试二进制因旧PRG读取报Invalid battle map ID／sprite index，重编当前合并资源读取后通过，未将旧exe失败隐藏。

新`OriginalWorldmap`接OriginalInn正常option1，不删城镇表也不以战场背景替代：`41020B`调用`425092(-12, palette+0xC00,3)`解原DAT12 PCX；`426971(5/6)`实际`473CDB(3,7)`，因此资源是**DAT7 other cell，不是PRG7**。单元头有一字节type，原样RLE解码；当前两个标记type0，原当前位置标记偏移(-7,-25)，目的地(-7,-30)，透明0。BIN4 town WORD+6/+8坐标及DWORD名字偏移、原40像素命中、左键414F5D居中裁界、键盘4157C6四像素滚动、Escape/Return/右键原退出保留。

`out/worldmap-build.log`及`worldmap-test.log`：第65关《龍穴》用户原SAVE正常旅店option1进入、完整原表、原世界图与标记、方向滚动／重定位、两次返回、640x480真实SDL present及键事件、逐字节SAVE不变通过。`out/inn-current-build.log`／`inn-current.log`原传闻／自动装备／存读档等既有旅店专项回归通过。新增/改动C++均已clang-format。

边界：未称城镇全画面时序等价；边缘持续滚动、百分秒计时／按住重复、原淡入淡出与音乐36→4尚缺，本次无整套强化战役。特殊遮罩及起降未改；阶段停写后实际执行Debug→Release：`out/system-current-{Debug,Release}.log`均0；`system-current-verify.log`原资源核验通过；第60关《返鄉的遊子》`system-current-teleport-sdl.log`生产传送52帧通过。`system-current-package.log`打包成功，独立解包`out/system-current-package-check`仅1 DAT／1 PRG／0 TXT，独立`system-current-package-verify.log`与`system-current-package-sdl.log`资源及真实SDL右键跳开场91帧均exit0。当前设备无默认音频，SDL明确silent fallback通过，不冒称音频播放通过。根EXE／Release／独立包EXE SHA256全部`967AC31FDC743668FD913D047AE2CEAAE9001B5709CEDF5D76D9F65CF664929B`。这是有限阶段闭环，不是六类最终收口；后续写入后必须重新统一验收。

<a id="2026-11-10-本轮收口：411587计时缓存与非空completion7挂起恢复"></a>
<a id="history-v-023"></a>
### 2026-11-10 本轮收口：411587计时缓存与非空completion7挂起恢复

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

<a id="2026-11-10-按序系统补全：本轮实际交付与未完成边界"></a>
<a id="history-v-030"></a>
### 2026-11-10 按序系统补全：本轮实际交付与未完成边界

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

<a id="本轮系统补全顺序（2026-11-10-1315，当前暂停）"></a>
<a id="history-p-009"></a>
### 本轮系统补全顺序（2026-11-10 13:15，当前暂停）

用户指定本轮依次处理：**资源 → 存档／回归 → 画面 → 菜单／关间 → 战斗／AI → 死亡及剧情**。六类处理后统一报告；技能／剑法及全流程验收留下一轮，不提前扩展。每项必须依据原反汇编／资源核实实际实现、补齐确证缺陷并验收，不以审计、算法fixture或某个子链通过冒充全类别完成。

资源首先核实双盘重复项及PRG第9组差异；考虑将字节完全相同的内容共享存储以节省空间，但必须保持原资源读取等价、索引／版本／边界校验，不覆盖或删除原归档。合并前后须独立验证双盘资源及包内运行，不能仅凭文件名相同合并。

本轮统一交付仍使用S2NE.exe，构建后复制到资源根目录；游戏包不附加两个TXT。停止全部写入后统一构建／相关专项／SDL／打包验收，详细证据归本文件分项历史证据。两条强化结局与现有实际技能验收证据保留；音频暂缓、双人对战排除，单人合作不排除。

阶段执行报告（尚非六类完成）：

- 资源：双盘82项DAT映射归并40块，共享载荷从270404582降为180653377字节，逐字节等价／版本与损坏拒绝／原归档回退通过，原资源未删。PRG的CD1尾部等于属性块、CD2尾部等于完整地图块，保持原安装PRG读取；详细证据已写[分项验证导航](#validation)。
- 存档／回归：旧fixture未消费原剧情导致第4回合失败，现用正常对白输入修复；完整两分支事件回归通过。续补曾暴露AI新调用战力mode5..8未接，已按原组件补齐并重跑完整events通过，不能将首次失败隐藏。
- 画面／菜单／AI／死亡已续补：消失动画逐帧呈现栅栏、真实原档出售事务、低血非撤退路径、死亡嵌套身份谓词。仅事务不是实际商店UI；真正撤退、behavior6／7及特殊死亡完成仍缺。
- 新续补已完成生产商店出售UI：原关间入口／图字／库存／数量／确认／返回，真实SAVE副本精确差异和往返、正常SDL输入通过。真正低血撤退接入原候选及可见移动，原+0E=5持久字段往返通过；不等于behavior6／7全接。
- 死亡completion7仅空敌军原立即返回分支已接，非空撤离和completion1／3仍拒绝；该项是隔离资源fixture，不是自然击杀E2E。
- 菜单最新新增：原城镇门控的消耗品买入、旅店原传闻页面、教会小额捐献确认／取消／返回已接生产，并通过原档事务／SDL。旅店不是自创住宿恢复：原option10是自动装备，恢复在原关间进入重置路径，不将其误称付费休息。
- 菜单续补新增大额捐献生产事务，已按原44ACAD纠正为固定难度经验基值＋费用增量，调用原成长／共享RNG而非随机选经验；确认取消与原档事务／SDL通过，但大额资格为受控原档fixture，非自然合资格流程。旅店新增原名册／人物详情／返回，真实SAVE来源SDL通过。
- AI6／7字段已核正：40AED5读原WORD+0A，不是阵营behavior或control+8；新增持久字段，接原物件／route目标选择与kind7可见移动，kind6仅本回合精确可达分支。kind6不可达40AEB3 fallback仍拒绝，不能称6／7全量完成。
- 教会复活已接生产41430B(0)：原死者人物页／费用／确认取消／不足提示／扣钱清死亡标志与原档同步；严格按原不补HP，库存装备不变。当前验证为真实SAVE派生受控死者／钱fixture及SDL，不冒称自然死亡进城E2E；人物页全部像素／滚动条及第三业务仍未完整验收。
- 装备购买已形成生产买入／替换闭环：原资格44159E→440A89→443351、人物选择、不装备买入、旧槽退库、原非对称属性与军团flags重算、费用库存副本事务及原档往返。真实SAVE原报价item7／人物0的SDL正常取消／替换／两级返回通过；未冒称全部画面等价。
- 装备页新增对象首行／选中行分离与跨页、原滚动命中区域及取消保真，受控12对象跨页专项／SDL通过；原按住重复计时与完整图元仍未恢复，不声称原交互全等价。411587上级刷新／面板恢复与44EE0E原箭头仍需收口，不能猜资源ID。
- 旅店3／4已接原mode1存读档、翻页选择、覆盖确认与原槽备份／原子落盘，隔离原SAVE往返通过；旅店9原设置页返回SDL通过。旅店10已接451ABC原自动装备资格／排序／退库／属性／全军flag及取消提交返回保存；城镇探索等仍未完成。
- 原共享数组初始化4262EC已确证44EE0E箭头ID147、拇指ID146，接入专项／SDL，非猜图元。411587新增悬停／水平滚动／报价库存刷新及钱不足提示；原光标、重复计时与缓存时序尚缺，当前继续。
- AI6精确目标不可达的原普通决策回退已补。411587新增原百分秒计时／按住重复／光标定位与CPU报价缓存，原淡入双buffer呈现时序仍未收口。
- 非空completion7已接死亡栈挂起／逐敌镜头与呈现撤离、保存／恢复原控制值及异常回滚，真实stage42原表隔离fixture29敌军与SDL通过；这不是自然流程证据，撤离移动姿态分支仍待补，不外推全撤离画面等价。
- 411587新增原472EF1的16阶调色淡入／逐present门控／输入锁；纠正474A7C／474827只是buffer复制／选择，本身无淡入帧。参数-4无delay、末阶60，不猜额外帧。专项／SDL通过。
- completion7已开放class0／1／2原PRG移动姿态撤离，class3／4／5仍因原遮罩合成未接而拒绝。传送454ED5到达十帧仅图元／计时专项完成，尚未接原侧末生产事务；城镇探索仍只有原调用资源清单，无正常入口链。
- 最新Debug／Release、完整events及九项专项、四项SDL与独立无DAT包通过，根S2NE.exe和ZIP已同步。当前优先传送十帧生产接线及城镇正常入口／交互／返回，再特殊遮罩与起降；仍不称六类全部完成。
- 最新Debug／Release、完整events与八项相关专项、双线自动SDL及共享资源无DAT独立包通过，根S2NE.exe／DLL／ZIP已更新。未到六类全量完成，不提前最终报告。历史证据以[分项验证导航](#validation)最新条目优先。

<a id="阅读说明"></a>
<a id="history-p-018"></a>
### 阅读说明

- 以最新实际回归结果为准，算法或资源测试通过不等于对应的完整游戏流程已经恢复。
- 下方保留历史记录；其中“新游戏不能存档”“奖励未接”“转职未提交”等旧限制已被最新结果取代，不应作为当前结论。
- 已验证两路线首战胜利、奖励、关间出征、下一关及自动档重载；另已合法续玩白志超至第4关并完成实际转职与重载。这不代表后续全战役完成。

<a id="历史主要缺口（2026-11-09，现状以顶部总表为准）"></a>
<a id="history-p-019"></a>
### 历史主要缺口（2026-11-09，现状以顶部总表为准）

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

<a id="当前状态"></a>
<a id="history-p-022"></a>
### 当前状态

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

<a id="待完成列表与已知缺陷"></a>
<a id="history-p-023"></a>
### 待完成列表与已知缺陷

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

<a id="merge-integrity"></a>
## 合并完整性与本次验证边界

- 已完整分段阅读两份原文；按九个专题重排，不按原两文件顺序拼接。每个原二级标题保留为历史记录标题，并显式保留可用标题锚点；最新状态、待办、撤销误判及获准差异独立集中。
- 完全相同长段仅保留一份，其余以同一原记录锚点替代；不同日期、受控限制、原函数／命令、日志、存档、数字／哈希或失败现场的独有内容仍保留。部分跨项历史含相似阶段摘要，为避免损失语境未强行压缩为统一成功结论。
- 重要独有证据去向：技能专题保存115项目录／63与74与104合法学习入口、33 CEL偏移及跨归档哈希、32支付误断言／调度、88 UJ02真实正疗伤、94 reserve与MISS／HIT／同步栈；流程专题保存两线逐阶段日志、终局影片／flags及结果存档；其它专题保存IO失败、原子备份、class4合法装备141、DAT11误读、前景／AD／输出像素、DAT／PRG尾部与历史包哈希。跨项历史保留不可拆分的调查上下文。
- 本次仅修改本文件、README并删除原验证文档；不改源码、测试、构建配置、代理日志，不构建／打包。新增33状态按用户转述的停写交付记录，不将其专项升级为真实鼠标链或全技能完成。
