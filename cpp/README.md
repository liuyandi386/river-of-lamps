# River of Lamps — C++ 独立版

《灯河》的独立 C++20 + SFML 2.6.1 + Dear ImGui 1.89.9 移植。界面由 ImGui-SFML 渲染，棋盘由 SFML 绘制，不需要 Godot。

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

依赖全部在 `cpp/third_party/`：SFML 2.6.1 静态库、Dear ImGui 1.89.9、ImGui-SFML 2.6.1、nlohmann/json 单头、Sarasa UI SC 子集。
不需要 vcpkg、网络或 Godot。详细说明见 [`../docs/migration_cpp.md`](../docs/migration_cpp.md)。

## 目录

```text
cpp/
├─ CMakeLists.txt
├─ src/
│  ├─ main.cpp
│  ├─ app/           窗口、ImGui UI、开始/关卡选择、设置、启动动画、HUD、画布、波形、音效、存档
│  ├─ core/          网格、模拟器、关卡构建、关卡仓库、元件规则
│  └─ generated/     嵌入的关卡 JSON 与字体（生成文件）
├─ tests/main.cpp    40 条原断言 + 星级窗口/边界 + 进度/设置存档断言
├─ tools/            重新生成嵌入资源的 Python 脚本
├─ third_party/      SFML 2.6.1 / Dear ImGui / ImGui-SFML / nlohmann / Sarasa UI SC 子集 / 许可证
└─ screenshots/      14 张界面截图（含 1920、设置、开场动画、两级非门）
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
| 设置 | 主菜单“设置”按钮 |
| 静音 | `M` |

## 截图

![菜单](screenshots/01_menu.png)

![第 1 关运行中](screenshots/02_game.png)

![第 2 关：非门](screenshots/06_level2_not.png)

![通过](screenshots/03_pass.png)

![2 星与差拍提示](screenshots/07_two_star.png)

![失败](screenshots/04_fail.png)

![第 5 关](screenshots/05_level5.png)

![1920×1080 主菜单](screenshots/08_menu_1920.png)

![1920×1080 游戏内](screenshots/09_game_1920.png)

![1920×1080 通关](screenshots/10_pass_1920.png)

![1920×1080 开场动画](screenshots/11_splash_1920.png)

![1280×720 开场动画](screenshots/12_splash.png)

![设置](screenshots/13_settings.png)

![第 3 关：两级非门](screenshots/14_level3.png)

> 改前基线在 `screenshots/before_ui/`。

## 重新生成嵌入资源

关卡/元件 JSON 或 UI 文案改动后：

```powershell
python cpp/tools/make_embedded_assets.py
python cpp/tools/make_font.py
cmake --build build -j 8
```

> `make_font.py` 需要 fontTools 和 Sarasa UI SC 源字体（下载地址见 `third_party/fonts/README.md`）。
> 构建本身不需要 Python，生成文件已随项目提供。

## 截图模式（开发用）

```powershell
build\RiverOfLamps.exe --capture cpp\screenshots\01_menu.png menu
build\RiverOfLamps.exe --capture cpp\screenshots\02_game.png game
build\RiverOfLamps.exe --capture cpp\screenshots\06_level2_not.png level2game
build\RiverOfLamps.exe --capture cpp\screenshots\03_pass.png pass
build\RiverOfLamps.exe --capture cpp\screenshots\04_fail.png fail
build\RiverOfLamps.exe --capture cpp\screenshots\05_level5.png level5game
build\RiverOfLamps.exe --capture cpp\screenshots\08_menu_1920.png menu --size 1920x1080
build\RiverOfLamps.exe --capture cpp\screenshots\09_game_1920.png game --size 1920x1080
build\RiverOfLamps.exe --capture cpp\screenshots\10_pass_1920.png pass --size 1920x1080
build\RiverOfLamps.exe --capture cpp\screenshots\11_splash_1920.png splash --size 1920x1080
build\RiverOfLamps.exe --capture cpp\screenshots\12_splash.png splash
build\RiverOfLamps.exe --capture cpp\screenshots\13_settings.png settings
build\RiverOfLamps.exe --capture cpp\screenshots\14_level3.png level3game
```

显示设置保存在 `%APPDATA%\RiverOfLamps\settings.json`；`--size WxH` 只用于本次调试截图，
不会改写用户设置。

## 未实现（当前边界）

- 第二批时序元件、第三批干扰/屏蔽/衰减/中继；
- 关卡编辑器、手柄、音乐。

C5（中继 + 确定性阈值衰减）在 `docs/design/decisions.md` 中标记为第二批，当前 5 关不使用；
C++ 保持现有行为，不在没有关卡/测试的情况下提前实现。
