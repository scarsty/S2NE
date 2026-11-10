# S2NE —《超时空英雄传说：北方密使》复刻工程

## 游戏介绍

《超时空英雄传说：北方密使》是一款以剧情推进、队伍培养和回合制战棋战斗为核心的角色扮演游戏。游戏包含白志超、娜塔莉娅等路线，以及角色成长、职业转换、装备物品和关卡事件等系统。

## 项目介绍

S2NE 是面向 Windows x64 的 C++23／SDL3 原生重写工程，以原 Windows 程序 `WSUPERSP.EXE` 的反汇编和原游戏资源为依据，逐步恢复界面、操作、剧情与战斗逻辑，而非重新设计规则的同类游戏。

项目仍在开发中，不代表原版全部功能已经实现。本页仅介绍游戏与项目；实现状态见 [进度与缺口](PROGRESS.md)，测试方法与记录见 [验证文档](VALIDATION.md)。

### 还原原则

- 菜单、剧情、操作与战斗规则以原程序和原资源为准，不用自创行为替代未查明的功能。
- 以兼容原格式存档为目标。
- 原资源放在工程上一级游戏目录，不复制进源码目录；资源读取不改写原档案。
- 双人对战按当前约定不处理，不纳入本轮实现与通关验收范围；音频完整还原暂缓。

## 开发环境

- Windows x64
- Visual Studio，支持 C++23 的 MSVC 工具链
- SDL3／SDL3_mixer，通过 vcpkg 的 MSBuild 集成使用
- 默认 vcpkg 路径：`D:\project\vcpkg`，可通过 MSBuild 属性 `VcpkgRoot` 覆盖

日常开发入口为原生单工程方案 [S2NE.sln](S2NE.sln)，工程文件为 [S2NE.vcxproj](S2NE.vcxproj)。旧 [CMakeLists.txt](CMakeLists.txt) 和构建目录仅保留作兼容用途，不用于生成日常 VS 工程。

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

原生 Debug／Release 构建完成后，会自动将当前游戏程序及运行库同步到资源根目录，可直接运行根目录的 `S2NE.exe`。Debug 构建也会覆盖该检查副本；日常交付包固定使用 Release。

运行 `powershell -NoProfile -ExecutionPolicy Bypass -File S2NE\tools\package-game.ps1` 可在资源根目录生成 `S2NE-game-win64.zip`。解压后使用 `run-game.cmd` 启动；需要 Windows x64 及 Microsoft Visual C++ x64 运行库。包中仅保留运行依赖与当前使用资源，不含个人存档、源码或验证产物。

### 单 DAT／单 PRG 资源格式

打包调用 `tools\pack-disc-resources.ps1`，从两套原 Windows 资源生成独立的 `SUPERSP.DAT` 与 `SUPERSP.PRG` 容器；包内各一个，不含 CDS、个人存档或源光盘目录。源光盘不改写、不删除；首次替换原安装根文件前保存在 `cd\archived-installed`。

两个容器采用相同 v2 小端格式：40 字节头（magic `S2NEPK02`、版本 uint32=2、类型 uint32=1/DAT或2/PRG、PAL长度 uint32、源数量 uint32=2、映射数量 uint32、保留 uint32=0、载荷长度 uint64），完整原 PAL，两个32字节源描述符（逻辑资源域 uint32、源格式版本 uint32=1、载荷偏移 uint64、完整源长度 uint64、FNV-1a-64 uint64），32字节映射（域 uint32、ID uint32、源内偏移 uint64、长度 uint64、FNV-1a-64 uint64），最后为两份完整源字节。DAT 保存82项、PRG保存18组；各自有独立索引、版本、长度和校验。校验用于检测意外损坏，不是认证机制。

PRG 并非单根文件冒充合并：两盘前8组相同，CD1第9组与第7组属性逐字节相同；CD2第9组保留直到实际文件末尾（原PAL第9项仅声明CD1长度），与第6组完整132张地图逐字节相同。生成器检查每张尾部地图的边界，完整保存原文件头、索引间隙和尾部，并在发布前逐字节回读两份原文件。

当前选择无去重的无损布局，因此两容器均大于任一单源；以后若做字节相同块去重，唯一内容总量不保证严格大于单源，不能以填充数据伪造合并体积。运行时仅访问这两个根容器，不选择物理光盘、不回退光盘目录。原存档0/1字段兼容映射为逻辑剧本资源域；剧情域选择不执行换盘。旧PRG偏移读取由资源层解包首套原字节图像后兼容，第二套及其完整尾部可通过独立域API读取。

### 代码格式与运行诊断

C/C++修改后使用工程现有 `_clang-format` 格式化改动文件；可运行 `tools\format-code.ps1 -Files <文件列表>`。工作区已为 C/C++ 配置保存时格式化（需要 VS Code C/C++ 扩展）。不批量格式化无关源文件。

程序启动时将 MSVC CRT断言、运行库参数错误与未处理异常输出到标准错误，不弹出运行库调试对话框；致命错误保持非零退出，不忽略断言继续运行。需要查看错误时从终端执行 `run-opening.cmd`，也可用 `2>error.log` 重定向标准错误。外部调试器主动中断不受此设置控制。

## 工程导航

| 位置 | 用途 |
|---|---|
| [src](src/) | 游戏入口、资源解码、界面、剧情与战斗实现 |
| [tests](tests/) | 原资源、规则、存档与流程回归测试 |
| [analysis](analysis/) | 原函数地址、还原证据与功能审计 |
| [tools](tools/) | 分析导出、资源核查与代码生成工具 |
| [PROGRESS.md](PROGRESS.md) | 当前进度、已知缺口及待完成事项 |
| [VALIDATION.md](VALIDATION.md) | 验证方法、过程及历史记录 |
