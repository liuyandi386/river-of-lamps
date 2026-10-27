# River of Lamps — C++ 独立版

《灯河》的独立 C++20 + SFML 2.6.1 移植。自绘 UI，不需要 Godot。

- 产物：`build\RiverOfLamps.exe`（静态链接，双击即玩）
- 测试：`build\river_tests.exe`（40 条原断言 + 星级窗口/边界/进度断言）
- 旧 GDScript 工程已于 2026-10-27 移除；历史见 [`../docs/migration_cpp.md`](../docs/migration_cpp.md)。
- 数据在项目根目录 `data/levels/`；运行时会优先读外部 JSON，找不到再用 exe 内嵌数据。

## 构建

在项目根目录双击：

```text
build_cpp.bat
```

或手动：

```powershell
$env:PATH = "C:\Program Files\RedPanda-Cpp\mingw64\bin;$env:PATH"
cmake -S cpp -B build -G "MinGW Makefiles" -DCMAKE_BUILD_TYPE=Release
cmake --build build -j 8
```

运行：

```text
build\RiverOfLamps.exe
```

测试：

```powershell
.\build\river_tests.exe
ctest --test-dir build --output-on-failure
```

依赖全部在 `cpp/third_party/`：SFML 2.6.1 静态库、nlohmann/json 单头、Noto Sans SC 子集。
不需要 vcpkg、网络或 Godot。详细说明见 [`../docs/migration_cpp.md`](../docs/migration_cpp.md)。

## 目录

```text
cpp/
├─ CMakeLists.txt
├─ src/
│  ├─ main.cpp
│  ├─ app/           窗口、开始/关卡选择、HUD、画布、波形、音效、存档
│  ├─ core/          网格、模拟器、关卡构建、关卡仓库、元件规则
│  └─ generated/     嵌入的关卡 JSON 与字体（生成文件）
├─ tests/main.cpp    40 条原断言 + 星级窗口/边界断言
├─ tools/            重新生成嵌入资源的 Python 脚本
├─ third_party/      SFML 2.6.1 / nlohmann / Noto 子集 / 许可证
└─ screenshots/      7 张界面截图
```

## 操作

| 操作 | 按键 |
|---|---|
| 放置/拖拽灯槽 | 左键 |
| 删除整条灯槽/元件 | 右键 |
| 运行/暂停 | `Space` |
| 重试 | `Backspace` |
| 提示 | `H` |
| 撤销 | `Ctrl+Z` |
| 暂停/返回 | `Esc` |
| 静音 | `M` |

## 截图

![菜单](screenshots/01_menu.png)

![第 1 关运行中](screenshots/02_game.png)

![第 2 关：非门](screenshots/06_level2_not.png)

![通过](screenshots/03_pass.png)

![2 星与差拍提示](screenshots/07_two_star.png)

![失败](screenshots/04_fail.png)

![第 5 关](screenshots/05_level5.png)

## 重新生成嵌入资源

关卡/元件 JSON 或 UI 文案改动后：

```powershell
python cpp/tools/make_embedded_assets.py
python cpp/tools/make_font.py
cmake --build build -j 8
```

> `make_font.py` 需要 fontTools；来源字体是 `C:/Windows/Fonts/NotoSansSC-VF.ttf`。
> 构建本身不需要 Python，生成文件已随项目提供。

## 截图模式（开发用）

```powershell
build\RiverOfLamps.exe --capture cpp\screenshots\01_menu.png menu
build\RiverOfLamps.exe --capture cpp\screenshots\02_game.png game
build\RiverOfLamps.exe --capture cpp\screenshots\06_level2_not.png level2game
build\RiverOfLamps.exe --capture cpp\screenshots\03_pass.png pass
build\RiverOfLamps.exe --capture cpp\screenshots\04_fail.png fail
build\RiverOfLamps.exe --capture cpp\screenshots\05_level5.png level5game
```

## 未实现（当前边界）

- 第二批时序元件、第三批干扰/屏蔽/衰减/中继；
- 关卡编辑器、手柄、设置菜单、音乐。

C5（中继 + 确定性阈值衰减）在 `docs/design/decisions.md` 中标记为第二批，当前 5 关不使用；
C++ 保持现有行为，不在没有关卡/测试的情况下提前实现。
