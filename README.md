# S2NE —《超时空英雄传说：北方密使》复刻工程

## 游戏介绍

《超时空英雄传说：北方密使》是一款以剧情推进、队伍培养和回合制战棋战斗为核心的角色扮演游戏。游戏包含白志超、娜塔莉娅等路线，以及角色成长、职业转换、装备物品和关卡事件等系统。

## 项目介绍

项目仍在开发中，不代表原版全部功能已经实现。本页仅介绍游戏与项目；实现状态、未完成项与原函数／实现／测试／历史证据统一见 [PROGRESS](PROGRESS.md#current-status)；[分项验证导航](PROGRESS.md#validation)区分正常输入、受控专项与未验证边界。

音频现状：已接原资源效果音、ANI声音cue、关卡及关间BGM与XMI合成；现代SoundFont不等同原声卡音色，但听感差异不作为缺口。跨阶段未实现展示所需的音乐／音效入口仍是缺口，不宣称全音频完成。自然习得已验证原技能菜单施放、熟练度跨阈值、学习通知与隔离存读档；不代表双线正常通关。详细证据与边界见[进度报告](PROGRESS.md)。

现代设备原声卡听感不作为验收条件（2026-11-16）。生产补齐按PROGRESS第1至6条顺序推进，第7条暂不推进。当前[剩余缺口摘要](PROGRESS.md#当前剩余缺口摘要)区分用户报告待定位问题、已有实现的等价核对及工具／证据缺口；用户已确认全关卡AI流程跑通，后续以汇编／原资源及局部回归验收，不要求重复全关卡。

普通启动仅保留错误／异常诊断，不逐次打印音乐、音效、招式帧及结算流水；`--stdio`与专项smoke保留协议及验收证据输出。

纹理迁移例外：工作目录优先读取`texture.zip`，不存在时回退读取`texture`目录；两者都缺失才走旧路径。当前外部模式已接入UI、头像、地图地面／物件／场景／战场背景、普通人物动作、代表法术及原字形；原索引数据仍用于遮挡、规则和状态计算。正常输出仍经过CPU framebuffer与SDL render-target的过渡桥，特殊人物状态、部分内部演出及其它直接sprite路径尚未迁完，不能据启动日志认定已完成无读回的全盘GPU渲染。大字外部字体使用资源根`chinese.ttf`生成SDL_ttf纹理缓存，仅texture模式启用；`SUPERSP.CFG`的`SIMPLIFIED_CHINESE=0`为默认关闭，设为1后读取资源根`cc\TSCharacters.txt`和`cc\TSPhrases.txt`进行繁转简。SimpleCC来源及许可见[说明](doc/simplecc-license.txt)。关卡数据导出建议见[关卡数据导出方案](doc/level-data-export.md)。

### 还原原则

- **全项目以汇编重现为原则**：所有系统的行为、数据含义、绘制与透明规则、执行顺序、状态机及代码共享／独立边界，均以原反汇编和原资源证据为依据；不仅适用于技能，也适用于已完成和后续实现。
- 工作批次、功能相似性或现代架构偏好不能替代原程序的契约。原本共用的机制按证据复用，原本不同的语义保持区别；不要求机械照抄机器指令，但不得改变原有行为。未查明的功能明确列为缺口，不自行设计替代。
- 以兼容原格式存档为目标。
- 原资源放在工程上一级游戏目录，不复制进源码目录；资源读取不改写原档案。
- 运行时大型原字节经 `read_shared_resource` 按规范文件路径常驻共享，缓存命中不再读取；PRG另区分逻辑域及PAL身份。地图、剧情、菜单及事务快照复制共享句柄，不复制整个PRG／CEL／BIN。事件完成状态单独保存，不写原BIN；调色板修改须显式COW，迭代器只读。解码图像、独立运行状态及截取的小资源仍各自持有，不宣称所有内存仅一份；底层返回vector的资源工具接口不属于共享读取接口。
- 仅以下“准许保留的差异”为明确例外；其它未还原项仍是缺口，不因记录而视为获准差异。

### 准许保留的差异

- 允许使用 SDL3／SDL3_ttf、现代纹理缓存与 renderer 纹理融合替代 DOS/VGA 直接绘图；允许将原资源导出为外部 `texture` PNG/TXT、将其打包为资源根 `texture.zip`（运行时优先读取 ZIP，缺失时读取目录）、使用资源根 `chinese.ttf` 生成字体纹理缓存，并使用 `cc` 目录词典做可选繁转简。上述差异仅改变显示后端、字体载入和发布介质，不得改变原剧情、菜单结构、事件、战斗规则、调色板语义或输入时序；未完成的特殊状态与无读回全盘纹理仍按缺口记录。

统一范围与约束见 [PROGRESS：准许保留的差异](PROGRESS.md#approved-differences)，不在本页重复维护。双人对战排除不影响单人合作技能待办；音视频后端／编解码／播放实现可现代化，但不得自行更换内容或删除事件。资源合并去重与免换盘已获准；经证据确认的明显原版错误可修，须逐项记录原行为、修正原因与影响，不得泛化为任意改规则。箭头纠正、最终输出柔化及上述例外均以集中差异表为准。音频完整还原仅暂缓，与现代后端许可不矛盾，非永久省略。

## 战斗核查状态

逆向参考：[事件指令与关卡格式](doc/event-instructions.md)、[图片导出与kys-cpp式目录](doc/resource-images.md)、[友伤与治疗刷经验问题记录](doc/friendly-fire-healing-exploit.md)。文档明确已知指令、未知子项与导出范围，不据目录存在宣称全量等价；友伤与治疗问题目前仅记录，暂不改变默认原版规则。

简单难度战斗核查：第16关原资源未启用入场强度调整（baseline/divisor均为0），敌军存档数值与资源一致，并非漏乘统一减益。简单×3／较易×2经验只适用于我方；已修正敌方显式技能经验误用倍率。休息已补HP／MP／ST数值恢复、AP门槛及原恢复数字展示，并有专项回归；详细原址、验证范围与历史纠偏见[进度记录](PROGRESS.md)。

## 显示

游戏保持原始 640×480 逻辑画面，默认窗口 960×720 按比例铺满，不裁切；宽屏仅添加保持 4:3 所必需的黑边。最终 SDL 纹理使用线性采样轻柔化，原 PAL 索引、透明遮挡与游戏像素帧不作滤镜改写。

菜单原资源 sprite 统一经 `original_sprite.hpp` 解码合成，调用点显式指定原零色游程透明／不透明；透明模式不等于删除黑色，literal 内索引0及其它索引的合法黑色仍保留。存读档页底部51／52／53为原版固定特殊槽，并非分页或裁切错误；原地址、调用分类与真实资源像素回归见 PROGRESS 最新记录。

## 开发环境

- Windows x64
- Visual Studio，支持 C++23 的 MSVC 工具链
- SDL3／SDL3_mixer，通过 vcpkg 的 MSBuild 集成使用
- 默认 vcpkg 路径：`D:\project\vcpkg`，可通过 MSBuild 属性 `VcpkgRoot` 覆盖

日常开发入口为原生单工程方案 [S2NE.sln](S2NE.sln)，工程文件为 [S2NE.vcxproj](S2NE.vcxproj)。旧 [CMakeLists.txt](CMakeLists.txt) 和构建目录仅保留作兼容用途，不用于生成日常 VS 工程。

CMake 兼容入口默认 `BUILD_TESTING=OFF`，不配置或编译测试。需要测试时显式配置 `-DBUILD_TESTING=ON`，再用 `cmake --build <构建目录> --config Release --target <测试目标>` 仅编译相关目标，并用 `ctest --test-dir <构建目录> -C Release -R <测试名称正则> --output-on-failure` 运行对应测试。测试可执行目标也排除在默认构建之外；`ctest` 不会自动编译它们，须先构建所选目标。已有缓存若启用了测试，须显式传入 `-DBUILD_TESTING=OFF` 才会关闭。

## 构建与运行

在 Visual Studio 中打开方案，选择 `Release | x64` 或 `Debug | x64` 构建。也可在 Visual Studio Developer PowerShell 中进入工程目录执行：

```powershell
msbuild .\S2NE.sln /p:Configuration=Release /p:Platform=x64 /m
.\run-opening.cmd
```

- 启动脚本：[run-opening.cmd](run-opening.cmd)。
- Release 输出：`out\x64\Release\S2NE.exe`；Debug 输出在对应 Debug 目录。
- 默认资源位置为工程上一级游戏目录；Visual Studio 的调试工作目录也指向该目录。
- 游戏存档位于游戏目录下。

## 本地检查与游戏包

CMake 兼容构建也默认不向资源根目录发布；仅显式配置 `-DS2NE_PUBLISH_RUNTIME=ON` 才启用该复制步骤。构建输出目录所需的 DLL 复制保留，不触发 DAT／PRG 重建。

原生 Release 构建完成后仅同步一份 `S2NE.exe` 到资源根目录，使用 `SkipUnchangedFiles`，不打包、不复制运行库；Debug 构建绝不执行此同步。完整运行库发布仍由用户显式使用 `/p:PublishGameRuntime=true` 或资源根目录 `publish-release.bat` 完成，避免把 Debug 程序错误放入游戏目录。完整打包还包括外部 `texture.zip`、`chinese.ttf` 和 `cc` 词典；开发目录仍保留 `texture` 文件夹作为 ZIP 缺失时的回退来源。

发布现由用户手动执行资源根目录 `publish-release.bat`，同步调用现有发布流程；结束直接退出，不暂停或另开终端，失败返回非零码。`publish-release.bat -SkipBuild`只同步并重打包。后续日常修改仅构建验证，不主动发布。工具维护清单见[tools/README](tools/README.md)。

### JSONL实操接口

观察`stage_name_bytes`为原关名Big5；可见`item_cells`额外只读报告原`marker`、`locked`（marker&0x100）和`item_candidates`原四项链，便于区分锁箱资格与普通拾取。不会改动态对象、随机流或物品；实际取得仍须原移动／拾取及库存变化确认。

旅馆根页`inn_auto_equipment`打开原自动装备确认，`inn_auto_confirm`按原默认确认调用451ABC自动装备及原库存／资格处理；不是住宿、治疗或付费。原出发44A4A9→45DDA6恢复下关当前HP／MP／ST，不应把旅馆自动装备当恢复服务。

商店`shop_commit`也支持已选择装备的原购买确认页：保持“不立即装备”选择，经原两级确认购买一件进库存；之后可用旅馆自动装备依法分配。未选择物品、动画未完成或资金不足仍拒绝，不模拟按键。

只读`observe_movement_graph`接受军表零基`index`及`occupied`（默认true），返回`state.movement_graph`边数组`[起x,起y,终x,终y,原成本]`。直接复用原交错邻接、地形step_cost及敌占格通行判定；false仅用于区分静态地形连通，不能据此穿敌。图不含当回合AP预算，执行落点仍必须由原reachable_cells及move_select批准，不修改地图或单位。

全新流程可在原标题菜单使用`new_game`打开原难度页，`setup_difficulty index=0..4`选择原难度（0为简单），`setup_confirm`调用原确认／序幕创建。观察setup的`difficulty`为原1..5值。入口不选择主角、关卡或剧情，娜塔莉雅分支仍须按序幕原选择文本用`story_choose`选择；不是加载旧SAV或跳过开场动画。

战场原道具菜单支持 `item_select`（`item_id` 为当前原菜单候选）、两次 `item_confirm`（先打开原确认框，再确认）及 `item_cancel`。消耗品目标页用 `item_target_select`（`index` 为完整军表下标），仍校验原装备资格、距离与登场／死亡状态；效果经共用原道具姿态、学习通知／数值展示后发布，不直接写库存或学习位。使用后须等待 `modal=none`、`input_locked=false`，不能把目标请求成功当作效果已提交。取消已打开的确认框只返回菜单；再次取消才关闭菜单。

结局开始后，`observe` 的 `ending` 给出原展示页、总页数和说明。`ending_advance` 仅调用原结局普通确认：文字／角色统计页继续，影片与滚动画面仍按原计时播放，不跳过自动展示。非结局状态拒绝该命令。

同步死亡共用原type8镜头事务：condition0定位并present，condition4按坐标或角色定位、present并等待原100时间单位；失败恢复镜头/事件。脚本显式消失已死亡角色仅准备独立绘制姿态，不发布HP或清死亡位。原420DC3写入的`script_route`在旗标消耗后仍须用于奖励后续关及原转换授权。控制器按stage刷新原目标解码，不沿用前关文本。

60关控制器使用既有`deployment_role/cell/confirm`逐人部署，保留角色1娜塔莉雅未选，再用`deployment_option index=2`按原规则完成；不调用会重新填满空位的`deployment_auto`。原部署栏没有该角色的强制标记；取消放置须恢复候选分配。观察`present`是到场回合条件，是否实际出场还须核`status_flags & 0x80`，不能仅看`present`。

原46618D范围伤害目标沿446E53的80h/20h登场死亡判定，不能把角色40h标志当作不支持死亡事件；59关《奚族聖地》技能62《譴責》的真实范围包含该类目标。事务仍先暂存所有目标、奖励、支付和RNG，任一拒绝均不发布；AI事务拒绝设置campaign blocker，须重载而非下帧重试。

FluidSynth各级原生日志统一送stderr（包括默认会输出stdout的INFO），不混入JSONL响应；控制器解析失败时保留`failed-request-N.json`、`failed-response-N.txt`和完整`failed-parse-N.txt`，不跳过非法响应或重发事务。

同步死亡续接支持原`461110`的type12/condition0角色重置：经既有raw军表事务执行`461215`，清DWORD+30h、写WORD+8=1、清WORD+2身份，保留军表槽位；与condition1撤退（40h置位、80h清除）不同。适配器拒绝时保留原死亡事务回滚，不消耗后续事件。

`state.capture_cells`只读列出原地图对象类别4（我方可占领红旗）的坐标及`object_category`，与`exit_cells`撤离口独立，不设置旗标或修改事件。原455828占旗资格为存活、已登场的我方且非普通NPC selector1；任意合资格角色均可触发。语义战役脚本在胜利说明含“紅旗”时优先驱动主角向最近红旗移动，仍由原移动和事件结算判胜，不改变既有撤离与战斗策略。

`modal=spirit_sight`是原灵视技能面板的确认边界。`{"cmd":"spirit_sight_confirm"}`复用原关闭面板事务，清除展示后提交暂存的技能支付、行动及随机流；没有面板时明确拒绝，不模拟键鼠。

经验辅助测试仅限`--stdio --headless-speed`：`{"cmd":"assist_experience","index":0,"amount":999}`对空闲战场中的存活我方角色增加经验（单次1..999，总经验按原上限999），进入原成长展示，以`growth_confirm`逐次确认；正常游戏奖励不变，不直接修改等级或能力。须使用存档副本并保留请求记录，结果标为经验辅助，不计自然练度通关。严格无键鼠实测入口为`tools\check-semantic-campaign.ps1 -Save <源档> -VerifyExperienceAssistance`，未整关覆盖时明确以退出码2报告blocked，不报成功。

`state.interaction.mode`区分`input`（可交给AI选择原键鼠操作）、`advancing`（原动画／电脑行动继续tick）、`blocked`（已有明确blocker）；`context`给出当前modal。不以`input_locked`单独判断是否应等待：成长确认、关间对话等可能锁战场输入但仍需要菜单响应。新增明确模态包括战报、道具使用／确认、覆盖存档确认和退出确认；AI应结合截图及状态契约处理，协议不替AI擅自选择剧情或交易。

状态契约：任何命令可附`expect_before`（执行前必须匹配，否则不发输入）和`expect_after`（响应时校验，失败返回`ok=false/postcondition_met=false`及实际state，不回滚已执行动作）。按顶层字段精确比较，仅比较指定字段；对象／数组值整体比较。异步动作使用`{"cmd":"await_state","expect_after":{"battle_state":1},"max_frames":200}`，逐原帧tick/present，达到条件或耗尽帧预算后返回；预算耗尽明确报错，不自动确认／跳过动作，不使用10秒卡死规则。可用`{"cmd":"transaction","steps":[...]}`串行提交带后置条件的步骤，失败即停止并返回完成步数，不回滚已执行动作。`ok=true`在未提供后置条件时仍只表示命令处理成功，不能推断剧情或行动已提交。可用`tools\check-stdio-contract.ps1 -Save <第16关真实开局SAV>`复验前置拒绝、等待成功／失败、剧情推进、选人、结束取消／重选／确认提交及后置失败；该验收限定此路径，不代表全部游戏菜单已通关覆盖。

每条命令（包括错误响应）均附带当前 `state`；不输出周期心跳。已取消10秒无输出报警及协议的`progress.suspected_stall`字段；`changed/unchanged_ms`仅提供观察数据，不据此判定卡死或跳过动作。续行驱动仍保留单请求60秒传输超时，超时只报告失败，不改变游戏状态。

普通序幕的`modal=none`不代表无需输入：`text_revealing`、`dialogue_scrolling`、`choosing`、`battle_intro`及`showing_objectives`分别报告文字展开、镜头滚动、剧情选择、回合提示及任务目标状态；原回车／空格推进非战斗文本，Alt展开文字，镜头滚动期间不抢输入。移动页`reachable_cells`为全地图原路径判定可达格，`movement_cells`仅为当前镜头内通过地表pick校验的可点击格；空屏幕列表不等于全地图无路，`camera_x/y`用于诊断镜头。所有字段只观察现有状态，不放宽规则。

经营子菜单分别报告 `modal=inn/church/shop`，不再被统称为 `campaign`；旅馆另含 `inn_saves/equipment/world/settings/roster`，教会／商店含当前页。驱动必须确认返回经营根页及出发后的关卡变化，不得仅凭 `interbattle=true` 重复结算事务。原战场根菜单索引3（返回代码6）为自动战斗，索引4（返回代码3）为结束回合。

自动验证可显式追加`--headless-speed`，仅接受`--stdio`且SDL视频和音频驱动均为`dummy`，否则报错退出。此选项按原循环逐帧推进统一16ms虚拟游戏时钟，取消真实帧sleep；`wait`使用相同游戏时钟，仍保留逐帧present确认、脚本顺序、输入锁和成长确认。不固定／注入RNG，实际秒数仍按原随机源读取；不同运行不保证相同随机结果。这是无界面验证加速，不授权改变正常游戏规则；默认可见游玩与默认发布速度不变。

`--stdio <资源根> --save-root <全新隔离目录> [--load <只读初始sav>]`保持正常可见SDL窗口、原音频及用户键鼠；不默认dummy，不绕过动画或输入锁。stdin必须为pipe或文件，逐行JSON对象，stdout仅JSON响应，诊断在stderr。EOF处理完待响应后退出；隔离存档保留，原始sav不改写。可见交互控制器：

```powershell
python .\tools\play-stdio.py .\out\manual-session .\out\growth-check\legal\eligible-input.sav
```

在控制器终端逐行输入（须清除人为设置的SDL dummy驱动环境变量；桌面须可用）：

```json
{"cmd":"observe","id":1}
{"cmd":"mouse","x":320,"y":240,"button":"move"}
{"cmd":"key","key":"Down"}
{"cmd":"key","key":"Down","modifiers":["Left Shift"]}
{"cmd":"wait","ms":1000}
{"cmd":"screenshot"}
{"cmd":"quit"}
```

`mouse`坐标为640×480逻辑帧，button为left/right/move；`key`使用SDL键名。`wait`允许0–10000毫秒并持续正常tick/present。截图为隔离目录frame.bmp。observe含场景／模态／输入锁、人物HP/AP/XP/级别、原菜单可用掩码及技能资源拒绝原因；selected_skill_failure另按生产合作伙伴／召唤条件探测。`screen_x/y`为绘制中心，`input_x/y`及input_hittable为原地表pick判定的可点击位置；occupant与status_flags可区分HP尚有数值但原状态已死亡／不占地的角色。自动检查`tools\check-stdio.py`显式使用dummy，**不证明用户看见窗口或硬件音频**。控制器持有子进程直至quit/EOF/Ctrl+C；不留无人控制后台自动战斗。

### CLI原文显示（UTF-8 JSON）

所有响应（包括事务嵌套对象）保留原`*_bytes`，新增去掉该后缀的UTF-8文字字段，按Windows CP950/Big5严格解码，不翻译／简繁转换。例如`name_bytes`→`name`、`text_bytes`→`text`、`stage_name_bytes`→`stage_name`。空／无法解码名称明确显示`unknown:<id|kind|index>`，不猜图标名称。索引保持各原接口定义，不统一改为1起算。

战斗菜单同时提供`index/id/name/return_code`：普通模式5项；模式2保留原6项规则；`index=3,id=124,name=自動作戰,return_code=6`，模式2撤离`index=5,id=126,return_code=15`。前三项进入子操作，`return_code=null`并标记`return_code_kind=operation`，不伪造固定返回。确认页另列原`confirmation.id/text`。

技能增加原记录`name_bytes/name/index`，保留技能ID、组、费用及拒绝原因。剧情选项、人物／职业、学习文字同样解码。城镇、旅店、教会、商店分类／物品、部署及设置增加对应`*_items`／`*_options`；目标保留原索引数组，附加`*_items`人物名称；物品保留原数组并增加`usable_item_options`。范围仍用原坐标／目标索引表示，不给格子自造名称。`semantic_actions`是当前场景的协议命令目录（未知原文字显示unknown），**不是可用性授权**，输入锁、enabled、模态及原命令拒绝仍有效。原确认O/X是图像，不伪造原文“确定／取消”。`tools\invoke-campaign002-actions.ps1`打印这些字段，但原JSONL及机器字段不删除。

`key.modifiers`为可选数组，支持SDL名称Left/Right Shift、Ctrl、Alt、GUI，只作用于当前按键事件，不留下全局按住状态。此前单独发送修饰键不能为后续按键提供`event.key.mod`，因此技能页Shift导航、设置页左右Ctrl分支及Alt+C无法完整表达；现可发送组合键。普通点击、右键取消、鼠标移动及原菜单键盘路径已有入口；本次输入分派检查不等于全游戏每个菜单均已实测，尚未实现的游戏功能不会因协议支持键鼠而自动补齐。

`tools\invoke-campaign002-actions.ps1 -SettleAutomaticRound -AutomaticRounds N`可在同一进程连续执行至多N次原自动战斗，每次回到空闲边界先原保存slot5并记录独立`round-N.json`，再打开原第4项及确认；选择、失败和奖励边界仍优先停止／处理，不以次数判断胜利。默认N=1，提交奖励仍须显式`-CommitAutomaticVictory`。原自动战斗确认沿既有`key Return`入口，不使用仅适用于战场选择的`battle_confirm`。

剧情选择页观察包含`dialogue`原提示与`choices`全部原选项（Big5字节、零基`index`）；`{"cmd":"story_choose","index":1}`通过原选择／确认入口提交第二项，非法下标拒绝。任意剧情对白、选择及经验／习得／升级响应页均可提前打断`await_state`，返回实际部分状态和未满足的原后置条件；控制器须优先处理该响应页，不重放上一动作或继续盲等战斗反馈。`experience`模态包含`experience_award`，使用`growth_confirm`确认后进入原习得／升级／事务续接。流程脚本会在终端输出对白、选项与经验奖励；这些提示不是强制跳过动画或自动改变游戏剧情。

运行 `powershell -NoProfile -ExecutionPolicy Bypass -File S2NE\tools\package-game.ps1` 可在资源根目录生成 `S2NE-game-win64.zip`。解压后使用 `run-game.cmd` 启动；需要 Windows x64 及 Microsoft Visual C++ x64 运行库。包中仅保留运行依赖与当前使用资源，不含个人存档、源码或验证产物。

### 单 DAT／单 PRG 资源格式

打包调用 `tools\pack-disc-resources.ps1`，从两套原 Windows 资源生成独立的 `SUPERSP.DAT` 与 `SUPERSP.PRG` 容器；包内各一个，不含 CDS、个人存档或源光盘目录。源光盘不改写、不删除；首次替换原安装根文件前保存在 `cd\archived-installed`。

两个容器采用相同 v3 小端格式：40 字节头（magic `S2NEPK03`、版本 uint32=3、类型 uint32=1/DAT或2/PRG、PAL长度 uint32、逻辑图像数量 uint32=2、映射数量 uint32、extent数量 uint32、载荷长度 uint64），完整原 PAL，两个32字节逻辑图像描述符（域 uint32、extent数 uint32、首extent序号 uint64、完整图像长度 uint64、FNV-1a-64 uint64），32字节PAL映射（域 uint32、ID uint32、逻辑偏移 uint64、长度 uint64、FNV-1a-64 uint64），32字节extent（逻辑偏移、长度、共享载荷偏移、FNV-1a-64，均 uint64），最后为唯一字节块池。DAT 保存82项、PRG保存18组；各自有独立索引、版本、长度和校验。校验用于检测意外损坏，不是认证机制。

PRG 并非单根文件冒充合并：两盘前8组相同，CD1第9组与第7组属性逐字节相同；CD2第9组保留直到实际文件末尾（原PAL第9项仅声明CD1长度），与第6组完整132张地图逐字节相同。生成器检查每张尾部地图的边界，完整保存原文件头、索引间隙和尾部，并在发布前逐字节回读两份原文件。

生成器按全部PAL起止边界切块，先以长度和SHA256建桶，再逐字节比较确认共享；不靠hash判等，不舍弃头部、间隙或尾部。DAT相同条目共享，PRG跨逻辑域相同组及组9与组6/7重叠尾部也共享，两个逻辑图像可完整逐字节重建；不再顺序存入两份完整档案。运行时仅访问这一套根资源，不选择物理光盘、不回退光盘目录。原存档0/1字段仅兼容映射为逻辑剧本资源域；剧情域选择不执行换盘。旧PRG偏移读取通过逻辑字节图像兼容，索引读取直接从共享块取数据。生成器输出原字节数、唯一载荷、容器总长与实际节省量。

### 代码格式与运行诊断

C/C++修改后使用工程现有 `_clang-format` 格式化改动文件；可运行 `tools\format-code.ps1 -Files <文件列表>`。工作区已为 C/C++ 配置保存时格式化（需要 VS Code C/C++ 扩展）。不批量格式化无关源文件。

程序启动时将 MSVC CRT断言、运行库参数错误与未处理异常输出到标准错误，不弹出运行库调试对话框；致命错误保持非零退出，不忽略断言继续运行。需要查看错误时从终端执行 `run-opening.cmd`，也可用 `2>error.log` 重定向标准错误。外部调试器主动中断不受此设置控制。

启动卡顿诊断可设置环境变量 `S2NE_STARTUP_TRACE=1`，标准错误输出单调时钟分阶段耗时、原资源实际读取字节数、窗口／原开场首帧／事件循环／首次输入分发；不设置时不输出这些记录。`tools\measure-startup.ps1` 默认执行三次有界 dummy 开场跳过检查，并在 `out` 保存阶段日志与调用启动至阶段日志被观察的上界时间（含轮询／日志刷新开销）；它不是实际桌面 renderer 或硬件音频性能证明，也不清理系统缓存。真实窗口验证须在可用且已解锁的桌面进行。原首帧在音频初始化和战斗数据准备之前提交；完整性检查和原视频／菜单顺序不删除。

## 工程导航

| 位置 | 用途 |
|---|---|
| [src](src/) | 游戏入口、资源解码、界面、剧情与战斗实现 |
| [tests](tests/) | 原资源、规则、存档与流程回归测试 |
| [analysis](analysis/) | 原函数地址、还原证据与功能审计 |
| [tools](tools/) | 分析导出、资源核查与代码生成工具 |
| [PROGRESS：最新状态](PROGRESS.md#current-status) | 当前实现、未完成与完整性边界 |
| [PROGRESS：分项验证](PROGRESS.md#validation) | 原函数、实现、测试与历史独有证据 |
| [PROGRESS：获准差异](PROGRESS.md#approved-differences) | 集中维护的获准范围，音频暂缓非省略 |
