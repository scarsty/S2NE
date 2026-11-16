# 图片导出与kys-cpp式目录

参考本机 `D:\kys-all\_cpp\kys-cpp\src\TextureManager.cpp` 和工具说明：同组图片用 `编号.png`，多帧可用 `编号_帧.png`，`index.txt`每行是 `编号: dx, dy`。本工程采用相同的数字图片／文本锚点组织方式，**不是直接把北方密使数据转换为kys游戏规则或可直接运行的资源包**。

## 使用

### 工作目录自动选择（迁移中，尚非全纹理）

紧凑资源已移动到资源工作目录`D:\project\S2NE-workspace\texture`。普通启动检测当前工作目录的texture；不存在时沿旧规则读取，存在但不是目录则报错。发现目录时向stderr输出路径，不污染stdio JSON协议；ui组首次实际加载时输出一次组日志。当前只接入共用draw_original_cel的UI PNG路径，按编号缓存Surface与SDL纹理，通过SDL软件renderer融合到兼容画布；仍需旧调色板效果的索引适配。缺图直接报告，不以旧CEL静默替代。

这不等于全盘纹理迁移：直接draw_original_sprite、头像、地图、人物、法术正常生产路径、字体及最终整帧上传仍未替换，原数据与事件解析保留。不能将检测到texture的日志解释为所有画面均已使用PNG。后续需要迁移这些生产调用及剪裁／遮挡，最后取消兼容画布和RGB回索引。标题／新游戏启动两种目录选择已验证；旧detail smoke在进入战场时超时，尚未作为纹理UI认证通过。

### SDL纹理实验版（当前简化输出）

最新格式（2026-11-16，优先于下述历史完整周期格式）：输出目录为`out/textures-sdl-compact`，共19634张PNG。每个战场背景编号只保留一张静态图（50个编号，不是删除其它不同背景）。流水统一100ms节拍，取图片所含循环色段的最长帧数，不求公倍数：262组各8帧、46组各13帧，每帧都是完整图片。短色段按自身长度取模，整组在最长帧数处重置；接受混合8/13色段的短色段重置差异，不恢复联合周期。无cycle覆盖层、indices或JSON。SDL实验已用该目录验证；下面textures-sdl-full的67235张为旧版历史输出，不再推荐使用。

```powershell
python .\tools\simplify-resource-images.py D:\project\S2NE-workspace .\out\resource-images-kys-v4 .\out\textures-sdl-compact
.\out\x64\Release\S2NE.exe --texture-experiment .\out\textures-sdl-compact 78
```

先运行下述原始审计导出，再执行：

```powershell
python .\tools\simplify-resource-images.py D:\project\S2NE-workspace .\out\resource-images-kys-v4 .\out\textures-sdl-full
.\out\x64\Release\S2NE.exe --texture-experiment .\out\textures-sdl-full 78
```

当前输出textures-sdl-full共67235张PNG，仅附TXT，不含JSON、indices、cycle子目录或重复不透明版本。旧textures-sdl是已废弃的覆盖层实验格式，保留旧目录不覆盖用户文件。ui保留零游程透明版，合法literal黑色不删除；人物在units/外观号内按序连续编号，frames.txt为“动作,方向: 起点,帧数”，index.txt保留逐帧锚点。法术在spells/图组号内按实际播放顺序展开，frames.txt记录数量、间隔及普通锚点；混色比例直接写Alpha，原调色板量化不再应用。skills.txt保留技能分派及背景变色参数，palette.txt保留原基础颜色。

流水直接导出完整图片编号_帧号.png，不挖空基图、不叠加色段层；静态图仍为编号.png。各图片组frames.txt每行“图片号: 第0帧毫秒, 第1帧毫秒, ...”。多色段按各自原速度的联合周期，在任一色段变化的时间点生成完整帧，因而帧间隔可能不同，图片数量较多；所有图片从同一场景时间取相位。转换时仍从原PRG取索引，不能根据RGB反推，因为原调色板存在同色索引。

游戏新增显式实验入口，直接加载导出PNG，通过SDL纹理融合将指定法术叠到第1张战场大图，背景按完整帧切换，每次只画一张背景纹理；Esc退出，追加--smoke播放一个法术周期后退出。此入口是纹理合成实验，不是正常战场已全部迁移：尚未接生产人物／地形遮挡、特殊handler锚点覆盖、整屏变色与正常施法状态机。普通启动和原资源读取保持不变，影片未处理。下文v4为转换器输入与历史审计格式，不是简化版的使用要求。

仅需Python3.10+标准库，不额外安装Pillow。输出目录必须不存在，防止覆盖人工整理结果：

```powershell
python .\tools\export-resource-images.py D:\project\S2NE-workspace D:\project\S2NE-workspace\S2NE\out\resource-images-kys-v4
```

源资源只读；支持本工程S2NEPK03合并PRG，按原始逻辑域1重建目录，不将容器物理偏移误认成原PRG偏移。输出manifest记录输入SHA256、原逻辑偏移、长度、尺寸、锚点和RGBA校验值。生成图片只供本地研究和检查，不纳入发布包，不上传或分发原版权素材。

## 当前导出范围

| 组 | 编号与含义 | 锚点 |
|---|---|---|
| cel-transparent | CEL第2组，保留原全局号142起；零游程透明 | 0,0 |
| cel-opaque | 同一组的零游程不透明版本，避免误将参数0/1的用途合并 | 0,0 |
| portraits | CEL第3组typed头像，保留目录0基编号；对白角色通常引用 `(speaker-1)*2` | 0,0 |
| ground | PRG第3组地面，1基编号 | 0,0 |
| objects | PRG第4组物件，1基编号 | 0,0 |
| scenery | PRG第5组景物，1基编号 | 0,0 |
| unit-frames | PRG第2组全部角色图帧，0基目录号 | 宽/2,高（脚底中心） |
| unit-animations/职业外观号/action-动作号/direction-方向号 | 按原序列表复制帧，文件名为序列内0基帧号 | 每帧index.txt，不强制统一尺寸 |
| spells/法术图组号 | CEL第1组133个SPELLSP图组，各目录独立保存原帧 | animation.json保存原锚点，index.txt仅为原始图帧原点 |
| battle-backdrops/opaque、transparent | PRG第8组50张战场背景／前景大图，保留1基编号及两种合成模式 | 0,0；地图摆放位置另由地图记录决定 |

`appearances.json`保存PRG第1组各职业的原8字节元数据、200字节序列表及各序列引用的全局帧号；序列索引为 `action*4+direction`。不按文件排序猜动作或删除原重复帧。各帧尺寸不一，所以先保留全局帧组而不是强行转换成只有一个锚点的职业动画组。

新增人物分组按原职业外观号、动作号、方向号组织，各子目录的animation.json保存全局帧引用。重复帧与顺序不去重；动作和方向暂保留数字，不猜测尚未逐项认证的中文动作名或方向名。136种外观中共2277个非空序列、7109个序列帧引用；它们是4417个全局帧的分组副本，并非新增原画。各动作的实际计时由生产调用者决定，不统一强设帧率。

### CEL为什么有两组导出

cel-transparent与cel-opaque不是原档案中两份不同的图，而是同一份CEL第2组的两种调用语义。原474E3E的参数决定零游程是跳过背景，还是覆盖为调色板索引0；literal中的索引0仍正常绘制。生产确认框、商店等确有不透明调用，因此不透明组有实际意义，不应直接删除或把黑色全部抠掉。两组都导出是为了保留可检查的合成差异，不表示每张图片在原游戏中都使用两种方式。

### 流水效果

原472D78按表4EAF48轮转调色板：索引3–10共8色、间隔严格大于16cs；11–18共8色及19–31共13色、间隔严格大于9cs。图像像素的索引不变，索引对应的RGB随时间轮转，产生水流等动态；不是需要逐帧重画整张水面，也不应自行用位移贴图代替。生产实现见[original_palette.hpp](../src/original_palette.hpp)。普通PNG只呈现基础调色板时刻，不含这种动态。

### 法术与战场大图的元数据

法术每组animation.json保留8个原头字段、混色索引／百分比、正顺序表或负重复次数展开后的播放序列、原延时及普通锚点。skills.json保留115条原技能到图组、dispatch和背景变色参数的映射；图组号不等于技能号，部分handler还会引用相邻图组。特殊handler可能覆盖锚点或添加抖动等演出，不能仅凭PNG及普通锚点重放全部法术。

法术PNG按原基础PAL给出透明预览，但半透明像素依赖原背景和调色板最近色混合，并非普通PNG alpha就能等价；同步导出逐像素1字节的`.indices`文件，结合animation.json保留混色所需的原索引。战场大图也保留索引，方便还原调色板循环。`.indices`按逐行连续像素保存，无额外头，宽高见manifest。

战场大图位于PRG第8组，并非影片。原4293AF→42FD9F先不透明绘制背景，再绘制地形等，最后透明绘制前景；地图记录+16为背景号、+18/+20为位置，+22为前景号、+24/+26为位置。导出的是这50张原大图，不是已经拼接好地形、物件和遮挡的完整关卡地图。

CEL的literal零色仍可为不透明黑色，不能全局将索引0抠掉；typed原图和PRG角色／地面按各自透明规则处理。RGBA为原基础PAL的6位通道展开，不包含战场遮挡、阴影、行动后变暗、调色板循环、镜像及法术背景混色。这些仍由生产渲染器执行。

## 不包含的内容

当前工具不导出完整拼接地图、DAT的PCX背景、全部字体图元或PRG其它非图数据组。影片按用户要求暂不处理。不能把本次图片数称为“所有原资源已解完”，也不把混色特效的普通透明PNG预览宣称为等价演出。

## 验证

导出需检查目录边界、尺寸及RLE总像素数，异常直接失败；保留源哈希及manifest，输出不替换原资源。对照生产解码器抽样验证RGBA与透明规则；PNG使用标准RGBA无损编码。游戏仍读取原资源，尚未切换到这些PNG。

本次实际输出在 `out/resource-images-kys-verified/`：共11597张PNG（头像334、UI两种透明模式各653、地面4284、物件896、景物360、人物帧4417），不是11597张互异原画，UI两种模式有重复。987张UI透明版／头像与当前C++生产解码器逐像素RGBA校验一致；四项Python回归覆盖literal黑色、零游程、typed原图、角色空literal、行界限、截断与PNG无损／CRC。PAL／CEL／PRG源SHA256导出前后不变。人物帧和地形尚未逐张对照原程序截图，不能将导出成功作为全部显示等价证明。

2026-11-16新增输出 `out/resource-images-kys-v4/`：主manifest共14651张PNG，新增法术133组2954帧、战场大图50张两种模式共100张；另有7109张人物分组副本，总计21760张PNG。六项回归通过，新增PNG全部通过CRC／解压后RGBA哈希校验、索引文件长度校验，人物分组逐帧与全局帧字节一致，PAL／CEL／PRG／BIN源哈希不变。未逐帧与原程序动态画面认证，不修改生产游戏读取器或发布包。
