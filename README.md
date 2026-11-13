# S2NE —《超时空英雄传说：北方密使》复刻工程

## 游戏介绍

《超时空英雄传说：北方密使》是一款以剧情推进、队伍培养和回合制战棋战斗为核心的角色扮演游戏。游戏包含白志超、娜塔莉娅等路线，以及角色成长、职业转换、装备物品和关卡事件等系统。

## 项目介绍

S2NE 是面向 Windows x64 的 C++23／SDL3 原生重写工程，以原 Windows 程序 `WSUPERSP.EXE` 的反汇编和原游戏资源为依据，逐步恢复界面、操作、剧情与战斗逻辑，而非重新设计规则的同类游戏。

项目仍在开发中，不代表原版全部功能已经实现。本页仅介绍游戏与项目；实现状态、未完成项与原函数／实现／测试／历史证据统一见 [PROGRESS](PROGRESS.md#current-status)；[分项验证导航](PROGRESS.md#validation)区分正常输入、受控专项与未验证边界。

效果音首批恢复：原soundsp的245项WAV由资源对象持有，SDL_mixer使用最多16个效果通道，显式通道复用、满不抢占、独立停止及CFG第三项效果音量；当前仅接有原址证据的战场菜单选择100／确认Space与手动选人126，不代表移动、攻击、技能帧音效已恢复。固定A02背景音乐仍保留。已完成dummy解码／输入触发验证，**尚未验证真实扬声器听感**；设备、资源或解码失败经Result报告，不以静默假成功。

后续动作批已接452167普通移动122／131／124／103分支与返回通道复用、43E9C0物攻record声音／未命中111，CFG及设置页效果音量即时更新活动通道。dummy原输入已观察移动103及敌方攻击110／118／111；原已有94亦正常完成，但**尚未接45EDAB的ANI帧声音表，不能称技能声音已恢复**。452450特殊移动、ANI停止及场景停止时序仍是明确缺口；不凭猜测统一补音。

最新ANI批覆盖上段部分历史缺口：BIN7实际只有5个电影脚本，原开场21／22使用脚本1／2，现按真实解码帧（非CEL帧）推进正声音、-1停止和flags400负ID显式通道复用，结束／Esc按原flags停止；原452450 class4／5为132、class3为133，战场root菜单431DE8入口停止效果音。已有94不是ANI脚本，不能把开场ANI声音冒称为技能音效；其余原技能专用声音入口仍待核。固定A02及真实扬声器未验口径不变。

ANI开场调度已按原45EDAB将AF12速度先整数截断为百分秒，再换算毫秒（速度33对应60ms，不是33ms），只在真实解码成功帧推进一次cue；结束／跳过停止，暂停不推进，无额外EOF声音。245最后合法资源实际解码通过。电影3–5及其它技能／脚本专用音效仍未全部接入，详见PROGRESS，不将本批闭环扩大为全音频完成。

后续源码已覆盖电影3–5共享ANI、脚本type13声音／0停止、多阶段技能cue、原准备98／99与动作record声音、召唤136、条件命中118、普通脚本移动的独有通道复用及战场子菜单／技能选择／转职声音。以上覆盖旧段落的对应缺口；仍缺特殊脚本移动／传送、死亡事件独有分支、动态通知／拾取对象及附加视觉的完整生产入口，精确原址与验证范围见PROGRESS顶部清单。最新独立Release为`out\x64\Release\S2NE-audio.exe`，尚未将本未收口批次同步资源根。固定A02不是原BGM映射恢复，dummy测试不是扬声器听感证明。

固定六类最终核对已补46618D真实命中条件的118／闪光→数字、452FB1可达默认死亡141及失败回滚；原死亡过滤下返回17／19不可达，不再虚列必缺分支。剩余特殊脚本移动／传送、动态通知、拾取对象、附加视觉属于整个生产展示尚未实现的跨阶段依赖，精确函数与恢复契约已固定于PROGRESS，不靠伪播认定完成。现有入口音效批结束，后续按既定顺序恢复BGM；尚不宣称全部原音效完成。

BGM首个源码节点已取消固定A02，按原两盘曲号表／CD1、CD2文件列表恢复关卡音乐、对话前后曲、脚本type13/condition2及电影ANI音乐cue；音乐独立换曲／停止、同曲不重启与CFG音量即时更新已有dummy聚焦证据。正常Esc新游戏stage140播放原曲2/A03，合法stage4档播放原曲8/A09。**仍未完成原XMI合成及其余特定场景音乐调用**：原曲14／17／33／35无任何CD录音映射，不能猜替代音，电影4／5及强制MIDI目前明确有缺口。完整清单见PROGRESS顶部。本次仅独立Release，尚未同步资源根；真实扬声器未验。

后续XMI节点已覆盖上段合成缺口：原乐谱在内存转换为标准MIDI，由已安装SDL_mixer／FluidSynth合成；曲14／17／33／35与电影4真实handler已有dummy证据，ANI按原强制MIDI，CD/MIDI设置和各自音量即时生效。新增标题25、关间hub4、教会37、世界地图36及统计页MIDI13。音色使用`assets\audio\GeneralUser-GS.sf2`（附原许可证，可通过`S2NE_SOUNDFONT`覆盖），**并非原声卡音色**；原channel10 program49缺GS鼓组时回落标准鼓组的差异已明确记录，未声称完全一致。credits／临时音乐脚本／部署完整展示及分散stop入口仍待收口，详见PROGRESS，尚未发布覆盖根EXE或宣称BGM全部完成。

**最新可试玩BGM节点覆盖上述旧待办口径**：41F5DD实为现有角色结局序列，已补统一强制曲20→统计13，并非缺一个credits页面；确认读档／同关卡恢复及场景构建前停止已接。原XMI的channel／program未错位，program49鼓组回落属于现代SoundFont缺该未定义预置的音色差异，未擅映射其它patch。当前可达已实现生产入口BGM收口，仍有两项整个展示跨阶段依赖：42162B condition3临时结局播放器的保存／恢复曲，44AED2完整部署页的25／stop，恢复这些功能时必须回接。Release已显式同步资源根，新增`audio\GeneralUser-GS.sf2`和`audio\GeneralUser-GS-LICENSE.txt`为运行资产；不含于尚未重建的旧ZIP，未重建DAT／PRG或更改个人SAVE。真实扬声器听感未验，后续按既定顺序进入自然习得。

### 还原原则

自然习得新增实际原输入证据：既有合法stage4转职训练档熟练860，经原菜单使用已学96达到915，跨900取得99，显示原学习通知后才提交；隔离SAVE往返后正常下一回合从菜单施放99并再次保存。已补此前缺失的原458B03学习名通知／等待，不新增“升级自动学技”规则。具体原址、日志及合法转职旧证据见PROGRESS顶部；dummy不代表桌面，正常双线通关尚未开始。

- **全项目以逆向重现为原则**：所有系统的行为、数据含义、绘制与透明规则、执行顺序、状态机及代码共享／独立边界，均以原反汇编和原资源证据为依据；不仅适用于技能，也适用于已完成和后续实现。
- 工作批次、功能相似性或现代架构偏好不能替代原程序的契约。原本共用的机制按证据复用，原本不同的语义保持区别；不要求机械照抄机器指令，但不得改变原有行为。未查明的功能明确列为缺口，不自行设计替代。
- 以兼容原格式存档为目标。
- 原资源放在工程上一级游戏目录，不复制进源码目录；资源读取不改写原档案。
- 仅以下“准许保留的差异”为明确例外；其它未还原项仍是缺口，不因记录而视为获准差异。

### 准许保留的差异

统一范围与约束见 [PROGRESS：准许保留的差异](PROGRESS.md#approved-differences)，不在本页重复维护。双人对战排除不影响单人合作技能待办；音视频后端／编解码／播放实现可现代化，但不得自行更换内容或删除事件。资源合并去重与免换盘已获准；经证据确认的明显原版错误可修，须逐项记录原行为、修正原因与影响，不得泛化为任意改规则。箭头纠正、最终输出柔化及上述例外均以集中差异表为准。音频完整还原仅暂缓，与现代后端许可不矛盾，非永久省略。

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

原生 Debug／Release 构建默认**不向资源根目录同步**游戏程序及运行库（`PublishGameRuntime=false`）。日常开发保持关闭，Debug绝不发布。重大可试玩节点在相关验证通过后，仅一次同步已验证的Release EXE至资源根目录；运行DLL仅必要时更新，保留个人SAVE。不因节点交付重打ZIP或重建DAT／PRG。显式发布开关为 `/p:PublishGameRuntime=true`，仅用于Release；CMake同样默认关闭根目录发布。

### JSONL实操接口

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
{"cmd":"wait","ms":1000}
{"cmd":"screenshot"}
{"cmd":"quit"}
```

`mouse`坐标为640×480逻辑帧，button为left/right/move；`key`使用SDL键名。`wait`允许0–10000毫秒并持续正常tick/present。截图为隔离目录frame.bmp。observe含场景／模态／输入锁、人物HP/AP/XP/级别、原菜单可用掩码及技能资源拒绝原因；selected_skill_failure另按生产合作伙伴／召唤条件探测。`screen_x/y`为绘制中心，`input_x/y`及input_hittable为原地表pick判定的可点击位置；occupant与status_flags可区分HP尚有数值但原状态已死亡／不占地的角色。自动检查`tools\check-stdio.py`显式使用dummy，**不证明用户看见窗口或硬件音频**。控制器持有子进程直至quit/EOF/Ctrl+C；不留无人控制后台自动战斗。

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
