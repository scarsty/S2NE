# S2NE —《超时空英雄传说：北方密使》复刻工程

## 游戏介绍

《超时空英雄传说：北方密使》是一款以剧情推进、队伍培养和回合制战棋战斗为核心的角色扮演游戏。游戏包含白志超、娜塔莉娅等路线，以及角色成长、职业转换、装备物品和关卡事件等系统。

## 项目介绍

S2NE 是面向 Windows x64 的 C++20／SDL3 原生重写工程，以原 Windows 程序 `WSUPERSP.EXE` 的反汇编和原游戏资源为依据，逐步恢复界面、操作、剧情与战斗逻辑，而非重新设计规则的同类游戏。

项目仍在开发中，不代表原版全部功能已经实现。本页仅介绍游戏与项目；实现状态见 [进度与缺口](PROGRESS.md)，测试方法与记录见 [验证文档](VALIDATION.md)。

### 还原原则

- 菜单、剧情、操作与战斗规则以原程序和原资源为准，不用自创行为替代未查明的功能。
- 以兼容原格式存档为目标。
- 原资源放在工程上一级游戏目录，不复制进源码目录；资源读取不改写原档案。
- 双人对战按当前约定不处理，不纳入本轮实现与通关验收范围；音频完整还原暂缓。

## 开发环境

- Windows x64
- Visual Studio，支持 C++20 的 MSVC 工具链
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

## 工程导航

| 位置 | 用途 |
|---|---|
| [src](src/) | 游戏入口、资源解码、界面、剧情与战斗实现 |
| [tests](tests/) | 原资源、规则、存档与流程回归测试 |
| [analysis](analysis/) | 原函数地址、还原证据与功能审计 |
| [tools](tools/) | 分析导出、资源核查与代码生成工具 |
| [PROGRESS.md](PROGRESS.md) | 当前进度、已知缺口及待完成事项 |
| [VALIDATION.md](VALIDATION.md) | 验证方法、过程及历史记录 |
