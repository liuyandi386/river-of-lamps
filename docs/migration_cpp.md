# 《灯河》GDScript → 独立 C++ 迁移说明

> 迁移历史文档。旧 GDScript 工程已于 **2026-10-27** 移除；文中 `game/...` 路径只作为迁移对照保留，当前数据在 `data/levels`。

## 1. 结论

- 新版本：**独立 C++20 + SFML 2.6.1 + Dear ImGui 1.89.9（ImGui-SFML 2.6.1）程序**，界面由 ImGui 渲染，棋盘由 SFML 绘制。
- 产物：`build\RiverOfLamps.exe`（约 6.1 MB），双击即可运行。
- **不依赖 Godot 编辑器、Godot 运行时或 GDExtension**。
- 静态链接 SFML 与 MinGW 运行库；exe 只导入 Windows 系统 DLL（KERNEL32 / USER32 / GDI32 / OPENGL32 / WINMM / ADVAPI32 / msvcrt）。
- 旧 GDScript 工程已移除；保留 `docs/`、`概要.txt`、关卡数据与 `docs/screenshots/` 作为历史参照。

## 2. 为什么是独立 C++ + SFML，而不是 GDExtension

GDExtension 的产物是 Godot 加载的动态库，仍然需要一个 Godot 可执行文件来启动，和“双击一个 exe 就能玩”冲突，因此排除。

在独立程序前提下选 **SFML 2.6.1**，直接对齐本机已有的 `jyfactorio` 工程：

- `D:\JYGAME\jyfactorio\factory-td\CMakeLists.txt`
  - C++20；
  - `find_package(SFML 2.6 COMPONENTS graphics window system)`；
  - 本地预编译依赖目录 `deps/local2/SFML-2.6.1`；
  - MinGW/GCC 分支使用 `-O3`、`-flto`；
  - 构建后拷贝 `assets` 与 SFML DLL。
- `D:\JYGAME\jyfactorio\factory-td\build.bat`
  - `PATH=C:\Program Files\RedPanda-Cpp\mingw64\bin`（GCC 11.5）；
  - CMake `D:\miniconda\envs\lerobot\Scripts\cmake.exe`；
  - `-G "MinGW Makefiles"`。
- 本项目沿用同一套编译器/CMake/SFML，并从同一本地依赖包取 **静态库**，这样不需要带一排 SFML DLL，能做成单 exe。
- 没有引入 EnTT：本作规模不需要 ECS。
- UI 层使用 Dear ImGui 1.89.9 + ImGui-SFML 2.6.1（源码内置、MIT、静态编译），用于 draw list、字体图集和统一主题；
  棋盘/灯槽等游戏特定视觉仍由 SFML 绘制，交互命中仍走原有 SFML 层。选用原因见 `docs/design/decisions.md` C22。

取舍：

| 选择 | 好处 | 代价 |
|---|---|---|
| SFML 2.6.1 静态链接 | 与 jyfactorio 同栈；单 exe；2D 图形/字体/窗口足够 | SFML 2.6 较旧；静态链接不可再替换 DLL；exe 约 6 MB |
| Dear ImGui + ImGui-SFML | 成熟控件/字体/抗锯齿绘制；源码内置、MIT；无运行时 DLL | 增加约 1 MB 静态库；需在 CMake 增加 `river_imgui` |
| MinGW-w64 | 本机现成；静态编译简单 | 不能用 vendored 的 MinGW `.a` 直接喂给 MSVC |
| 嵌入 JSON/字体 | 单 exe 可直接跑 | 关卡/文案改动后需要重新生成嵌入资源 |

## 3. 构建（可直接复制）

### 3.1 前置

- Windows 10/11 x64
- MinGW-w64 GCC（本机实测：RedPanda-C++ 自带 **GCC 11.5.0**）
- CMake ≥ 3.20（本机实测 **4.3.4**）
- 不需要 vcpkg、不需要网络、不需要 Godot

### 3.2 一键

在项目根目录双击：

```text
build_cpp.bat
```

它会配置并构建：

```text
build\RiverOfLamps.exe
build\river_tests.exe
```

双击 `run_cpp.bat` 启动游戏。也可以直接双击 `build\RiverOfLamps.exe`。

### 3.3 命令行

```powershell
# 在 D:\JYGAME\River of Lamps 下
$env:PATH = "C:\Program Files\RedPanda-Cpp\mingw64\bin;$env:PATH"

cmake -S cpp -B build -G "MinGW Makefiles" -DCMAKE_BUILD_TYPE=Release
cmake --build build -j 8

# 运行
.\build\RiverOfLamps.exe

# 测试
.\build\river_tests.exe
# 或
ctest --test-dir build --output-on-failure
```

> 如果你的 MinGW 不在 RedPanda 目录：把它的 `bin` 放进 PATH，保留同样的
> `cmake -S cpp -B build -G "MinGW Makefiles"` 即可。
> MSVC 不能直接链接 `cpp/third_party/SFML-2.6.1/lib/*.a`（这些是 MinGW 静态库）。
> 如果必须用 MSVC，请用 vcpkg 安装 SFML 2.6.1，并把 `CMakeLists.txt` 的静态库列表改成
> `find_package(SFML 2.6 COMPONENTS graphics window system)` 的导入目标；这一路未在本机验证。

### 3.4 依赖来源

| 依赖 | 位置 | 说明 |
|---|---|---|
| SFML 2.6.1（graphics/window/system + freetype + sfml-main） | `cpp/third_party/SFML-2.6.1/{include,lib}` | 从 `jyfactorio/factory-td/deps/local2/SFML-2.6.1` 复制；静态库 `.a`；保留 `license.md` |
| Dear ImGui 1.89.9 | `cpp/third_party/imgui/` | 源码内置；MIT；UI draw list/字体图集 |
| ImGui-SFML 2.6.1 | `cpp/third_party/imgui-sfml/` | 源码内置；MIT；SFML 2.5+ 后端 |
| nlohmann/json | `cpp/third_party/nlohmann/json.hpp` | 单头文件；MIT，见同目录 `LICENSE.MIT` |
| Sarasa UI SC 子集（Regular/SemiBold/Bold） | `cpp/third_party/fonts/SarasaUiSC-*-subset.ttf` | 从 Sarasa UI SC 1.0.43 子集化；SIL OFL 1.1，见 `OFL-SarasaGothic.txt`；嵌入 exe |
| MinGW 运行库 | 静态链接 | `-static -static-libgcc -static-libstdc++` |

没有使用 vcpkg manifest；`jyfactorio` 虽然带 `vcpkg.json`，但实际构建走 `deps/local2` 本地包，本项目采用同样策略，保证离线可构建。

## 4. 资源与数据策略

- **关卡 JSON / 元件 JSON 仍以现有文件为准**，构建时生成嵌入 C++：
  - `data/levels/index.json`
  - `data/levels/level_01.json` … `level_05.json`
  - `data/levels/elements.json`
  - 生成器：`python cpp/tools/make_embedded_assets.py`
  - 生成结果：`cpp/src/generated/embedded_json.cpp`
- 字体：
  - `python cpp/tools/make_font.py`
  - 从 Sarasa UI SC 静态字重取字符集，分别生成正文/标题/展示子集，
  - 生成 `cpp/src/generated/embedded_font.cpp` 与 `cpp/third_party/fonts/SarasaUiSC-*-subset.ttf`。
- 运行时优先读取**外部数据**，找不到再用嵌入数据：
  - 外部目录候选：`data/levels`（相对 exe 目录、exe 上级、当前目录、当前目录上级）；
  - 元件 JSON：`data/levels/elements.json`、`data/elements/elements.json`。
- `elements.json` 会被解析并保留（端口/批次/名称等元数据）；当前 5 关的模拟仍和迁移前一样按 `type_id` 分支实现，不让 JSON 规则表反向驱动模拟，避免行为漂移；字段结构未改。
- 因为数据是“构建时嵌入 + 运行时外部优先”，既满足“继续读取 data/levels”的要求，也能做到把 exe 单独拷走就能运行。

**进度存档**：`%APPDATA%\RiverOfLamps\progress.json`（与旧 Godot 的 `user://` 分开，互不污染）。

**显示设置**：`%APPDATA%\RiverOfLamps\settings.json`（窗口/无边框全屏、窗口分辨率、启动动画开关）；
显示模式与分辨率在设置面板中立即保存，但重启后应用。界面保持 1280×720 设计坐标，
按实际窗口做 `DisplayFramebufferScale` 缩放，鼠标坐标反算，见 `docs/design/decisions.md` C23/C24/C25。
独占全屏已移除：SFML 2.6.1 的 `WindowImplWin32::switchToFullscreen` 使用
`ChangeDisplaySettingsW(..., CDS_FULLSCREEN)`，在 Windows/DWM 下会闪黑屏；无边框全屏不切换显示模式。
无边框窗口取桌面尺寸 − 2px，避免恰好铺满触发 DirectFlip/MPO；有焦点时临时 TOPMOST，失焦恢复，
并在进程启动时 `ImmDisableIME`。

> 实际仓库路径说明：你消息里写的是 `scripts/main.gd`、`data/levels`、`docs/outline.md`；
> 迁移时旧代码实际在 `game/main.gd`、`game/data/levels`、`docs/design/outline.md`。
> 旧 GDScript 已按 2026-10-27 的决定移除；下表 `game/...` 只作为历史对照保留，当前数据在 `data/levels`。

## 5. 功能对齐表

| 原 GDScript（已移除，仅历史对照） | 原职责 | C++ 对应 |
|---|---|---|
| `game/main.gd` | 主界面、关卡切换、运行/暂停/重试、进度、撤销 | `cpp/src/app/app.cpp` `App`（`load_level`、`toggle_run`、`start_run`、`step_once`、`retry`、`undo`、`unlock_next`） |
| `game/board/board_view.gd` | 网格/灯槽/元件/信号颜色；拖拽布线、删除 | `cpp/src/app/board_view.cpp` `BoardView`（`handle_mouse_press/move`、`try_draw_to`、`remove_path_at`、`draw_*`） |
| `game/sim/simulator.gd` | 按拍推进、延迟调度、目标比对、校验 | `cpp/src/core/simulator.cpp` `Simulator`（`step`、`run_until_done`、`validation_errors`） |
| `game/sim/circuit_graph.gd` | 网格、元件、单元格、灯槽方向、删除整条路径 | `cpp/src/core/circuit_graph.cpp` `CircuitGraph` |
| `game/sim/sim_types.gd` | Signal/TickFrame/RunResult 等 | `cpp/src/core/types.h` |
| `game/sim/component_rules.gd` | 端口朝向、显示名 | `cpp/src/core/component_rules.cpp` |
| `game/sim/level_builder.gd` | JSON → 图、参考解铺线 | `cpp/src/core/level_builder.cpp` |
| `game/flow/level_repo.gd` | 读 index/level JSON | `cpp/src/core/level_repository.cpp` |
| `game/ui/waveform_panel.gd` | 目标/实际拍带 | `cpp/src/app/waveform_view.cpp` |
| `tests/run_tests.gd` | 40 条逻辑断言 | `cpp/tests/main.cpp`（40 条 + 星级窗口/边界/进度断言） |

### 行为清单

| 功能 | 状态 |
|---|---|
| 8×8 网格、预置源/门/目标、逐格画灯槽 | 完成 |
| 门 0 延迟；灯槽每格 1 拍；门链不误报 | 完成 |
| 组合环不收敛检测（迭代上限 16） | 完成（结构已移植；现有固定朝向关卡不产生门级环） |
| 目标逐拍比对、首错拍、期望/实际 | 完成 |
| 5 关加载、参考解、6/3/2/4/6 拍 3 星通关 | 完成 |
| 拍数窗口与 1–3 星评级、每关最高星级存档 | 完成 |
| 波形面板 | 完成 |
| 进度存档/重置 | 完成（新路径见下） |
| 撤销、暂停、重试、速度、提示 | 完成 |
| 失败/通关浮层 | 完成（C++ 版新增） |
| 开始界面/关卡选择 | 完成（C++ 版新增） |
| 鼠标悬停/点击反馈、窗口图标/标题 | 完成 |
| 基础占位音效 | 完成（WinMM 内存 WAV；可静音） |

### 关键函数对照

| GDScript | C++ |
|---|---|
| `CircuitGraph.setup` / `add_instance` / `add_wire` / `remove_path_at` / `count_player_functional` | 同名 `CircuitGraph` 方法 |
| `Simulator.setup` / `reset` / `step` / `run_until_done` / `get_validation_errors` | 同名 `Simulator` 方法（`validation_errors()`） |
| `LevelBuilder.build` / `_apply_path` | `LevelBuilder::build` / `apply_path` |
| `LevelRepository.load_index` / `load_level` / `read_json` | `LevelRepository` 构造 / `load_level` / `entries` |
| `BoardView._on_left_press` / `_try_draw_to` / `_on_right_click` | `BoardView::on_left_press` / `try_draw_to` / `on_right_click` |
| `BoardView._draw_wire` / `_draw_component` / `_draw_ports` | 同名 `BoardView` 绘制方法 |
| `Main._on_run_pressed` / `_start_run` / `_step_once` / `_on_retry_pressed` | `App::toggle_run` / `start_run` / `step_once` / `retry` |
| `Main._undo` / `_unlock_next` / `_load_progress` / `_save_progress` | `App::undo` / `unlock_next` / `ProgressStore` |
| `WaveformPanel.set_state` / `_draw` | `WaveformView::set_state` / `draw` |
| `tests/run_tests.gd` 的 8 组测试 | `cpp/tests/main.cpp` 同名测试函数（`test_wire_delay` … `test_levels`） |

**未迁移/仍延后**：

- 第二批：延迟元件、脉冲发生器、边沿触发器、计数器；
- 第三批：干扰源、屏蔽线、信号衰减 + 中继、真正的逐事件毛刺；
- 关卡编辑器、关卡验证 CLI、手柄、触屏、设置菜单、音乐。

> C5（中继 + 确定性阈值衰减）在 `docs/design/decisions.md` 中明确标为“第二批实现”；当前 C++ 与 5 关均未实现。保持当前行为，不提前重写规则。等做第二批时按决策文档实现，不重新发明。

## 6. 测试

```powershell
.\build\river_tests.exe
```

结果：

```text
ALL TESTS PASSED（40 条原断言 + 星级窗口/边界/进度断言）
```

- 原 40 条：灯槽延迟（3）、非门+脉冲（3）、与门对齐（3）、或门序列（5）、首错拍（5）、零延迟门链（4）、连线校验（2）、5 关加载/校验/通关（15）。
- 5 关参考解现在必须拿到 **3 星**，完成拍为 **6 / 3 / 2 / 4 / 6**；第 2 关「倒影」是非门教学，不再使用第 1 关的直连解法。
- 额外边界断言：刚好 3 星、刚好 2 星、超过 2 星线但仍在窗口内拿 1 星、窗口内无匹配失败且保留首错拍。
- 进度断言：最高星级保存/读档、重置删除存档。
- `ctest --test-dir build --output-on-failure` 同样通过。

## 7. “像个游戏”范围

### 本次必做且已完成

- 标题、窗口图标、开始/关卡选择界面；
- 在游戏内可见的 HUD：关卡、拍数、元件数；
- 失败/通关浮层与下一步按钮；
- 鼠标悬停/点击/拖拽反馈；
- 目标/实际波形与首错高亮；
- 进度存档、撤销、提示、速度；
- 占位点击/运行/成功/失败音效；
- 固定 1280×720 窗口（不缩放，保证 UI 坐标稳定）。

### 可以留到后续

- 设置菜单、键位重映射、音量滑条；
- 背景音乐、更完整的音效；
- 关卡过场/镜头/粒子；
- 完整中英文本地化；
- 手柄/触屏；
- 关卡编辑器/创意工坊；
- 更高分辨率/可缩放 UI。

## 8. 操作

| 操作 | 键鼠 |
|---|---|
| 放置/拖拽灯槽 | 左键 |
| 删除整条灯槽/元件 | 右键 |
| 运行/暂停 | `Space` |
| 重试（保留布局） | `Backspace` |
| 提示 | `H` |
| 撤销 | `Ctrl+Z` |
| 暂停/返回 | `Esc` |
| 静音 | `M` |

关卡选择/开始界面均可用鼠标；`Enter` 开始游戏。

## 9. 验证记录

- 迁移前 Godot GDScript 原版：`ALL TESTS PASSED`（Godot 4.7.2，历史记录）。
- C++ 版：`ALL TESTS PASSED（40 条原断言 + 星级窗口/边界/进度断言）`；5 关参考解均为 3 星，
  完成拍为 **6 / 3 / 2 / 4 / 6**（L2 从旧的 4 拍改为非门教学 3 拍）。
- 单文件检查：`objdump -p build\RiverOfLamps.exe` 只列出系统 DLL，无 `sfml-*.dll`。
- 把 exe 单独复制到空临时目录（无 `data/levels`）运行：菜单、字体、5 关卡片正常，说明嵌入 JSON/字体生效。
- 截图：`cpp/screenshots/01_menu.png`、`02_game.png`、`03_pass.png`、`04_fail.png`、`05_level5.png`、`06_level2_not.png`、`07_two_star.png`。

## 10. 已知限制

- 固定 1280×720；更小屏（如 1280×720 且带标题栏）可能显示不完整，建议 1366×768 以上。
- 快速甩动鼠标可能跳过中间格；当前要求鼠标经过每一格，慢速拖拽稳定。后续可做鼠标轨迹插值。
- 字体是子集；新增中文 UI 文案或关卡后，需重跑 `make_font.py`，否则可能缺字。
- 外部数据/嵌入数据二选一：改关卡后若不重跑嵌入生成器，单 exe 版本仍是旧关卡；在项目目录内运行时会优先读外部 JSON。
- SFML 2.6 静态库面向 MinGW；MSVC 需要换依赖方案（见 3.3）。

## 11. GDScript 工程已移除

- **删除日期：2026-10-27。**
- 已删除：`game/`（`main.gd`、`main.tscn`、`board/`、`sim/`、`flow/`、`ui/`）、
  `tests/`（`run_tests.gd` 及 `.uid`）、`tools/dev/`（Godot 截图脚本）、
  `project.godot`、`run_game.bat`、`run_tests.bat`、`.godot/`。
- 保留：`data/levels/*.json`、`data/levels/elements.json`、`docs/` 全部文档（含历史截图）、
  `概要.txt`、`cpp/`、`build/`、`build_cpp.bat`、`run_cpp.bat`、`README.md`。
- 迁移对照表里的 `game/...` 路径是历史记录，不再指向现存文件；当前行为以 C++ 和 `data/levels` 为准。
- 旧版窗口截图保留在 `docs/screenshots/`；C++ 版截图在 `cpp/screenshots/`。
