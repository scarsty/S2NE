# S2NE —《超时空英雄传说：北方密使》复刻工程

## 游戏介绍

《超时空英雄传说：北方密使》是一款以剧情推进、队伍培养和回合制战棋战斗为核心的角色扮演游戏。游戏包含白志超、娜塔莉娅等路线，以及角色成长、职业转换、装备物品和关卡事件等系统。

## 项目介绍

S2NE 是面向 Windows x64 的 C++20／SDL3 原生重写工程，以原 Windows 程序 `WSUPERSP.EXE` 的反汇编和原游戏资源为依据，逐步恢复界面、操作、剧情与战斗逻辑，而非重新设计规则的同类游戏。

项目仍在开发中，不代表原版全部功能已经实现。**实现进度、验证范围、已知缺陷和待完成事项统一见 [进度与缺口](PROGRESS.md)**，本页只介绍游戏、工程及使用方法。

### 还原原则

- 菜单、剧情、操作与战斗规则以原程序和原资源为准，不用自创行为替代未查明的功能。
- 保留原格式存档兼容目标，区分算法测试、实际游戏流程测试和原版画面对照。
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
- Release 输出：`out\x64\Release\envoy-opening.exe`；Debug 输出在对应 Debug 目录。
- 默认资源位置为工程上一级游戏目录；Visual Studio 的调试工作目录也指向该目录。
- 正常游戏使用游戏目录下的存档；自动化启动测试使用隔离写档目录，避免覆盖用户存档。

## 工程导航

| 位置 | 用途 |
|---|---|
| [src](src/) | 游戏入口、资源解码、界面、剧情与战斗实现 |
| [tests](tests/) | 原资源、规则、存档与流程回归测试 |
| [analysis](analysis/) | 原函数地址、还原证据与功能审计 |
| [tools](tools/) | 分析导出、资源核查与代码生成工具 |
| [PROGRESS.md](PROGRESS.md) | 当前进度、缺口、验证边界及历史记录 |

## 原程序与分析资料

分析目标为原 Windows x86 程序 `WSUPERSP.EXE`，SHA-256：

```text
58A9808E090DE51F56059F965D313BB274A08B82A3526E3C8991C4D3F978AAD2
```

本地 IDA 数据库及完整汇编位于 `analysis\ida\`。运行需要用户本地已有的原游戏资源，本项目介绍不包含原游戏程序或资源的分发授权。
