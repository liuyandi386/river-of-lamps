#include "app/app.h"

#include "app/imgui_ui.h"
#include "app/theme.h"
#include "app/ui_draw.h"
#include "core/component_rules.h"
#include "core/level_builder.h"
#include "generated/embedded_assets.h"

#include "imgui-SFML.h"
#include "imgui.h"

#include <SFML/System/Sleep.hpp>

#ifdef _WIN32
#include <windows.h>
#endif

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <string>
#include <utility>

namespace river {

namespace {

constexpr float kW = 1280.f;
constexpr float kH = 720.f;
constexpr float kTickInterval = 0.55f;

std::string number2(int value) {
    char buffer[8];
    std::snprintf(buffer, sizeof(buffer), "%02d", value);
    return buffer;
}

sf::String window_title() {
    const std::string text = "River of Lamps / 灯河 — 即时逻辑布线解谜";
    return sf::String::fromUtf8(text.begin(), text.end());
}

}  // namespace

App::App(std::filesystem::path exe_dir, std::string capture_path, std::string capture_mode,
         int forced_width, int forced_height)
    : exe_dir_(std::move(exe_dir)),
      repo_(exe_dir_),
      capture_path_(std::move(capture_path)),
      capture_mode_(std::move(capture_mode)) {
    settings_ = SettingsStore::load();
    if (forced_width > 0 && forced_height > 0) {
        settings_.display_mode = DisplayMode::Windowed;
        settings_.window_width = forced_width;
        settings_.window_height = forced_height;
    }
    create_window();

    font_ok_ = font_.loadFromMemory(embedded::kFontRegularData,
                                    embedded::kFontRegularSize);
    if (!font_ok_) {
        const char* fallbacks[] = {
            "C:/Windows/Fonts/msyh.ttc",
            "C:/Windows/Fonts/simhei.ttf",
        };
        for (const char* path : fallbacks) {
            if (font_.loadFromFile(path)) {
                font_ok_ = true;
                break;
            }
        }
    }

    progress_ = ProgressStore::load();
    const int count = static_cast<int>(repo_.entries().size());
    if (count > 0) {
        progress_.unlocked = std::clamp(progress_.unlocked, 1, count);
    }

    build_ui();
    refresh_level_cards();
    refresh_button_labels();
    menu_message_ = repo_.entries().empty()
                        ? "找不到关卡数据：请把 data/levels 放在 exe 旁边。"
                        : "选择关卡，或直接开始游戏。";

    if (arrow_cursor_.loadFromSystem(sf::Cursor::Arrow)) {
        window_.setMouseCursor(arrow_cursor_);
    }
    hand_cursor_.loadFromSystem(sf::Cursor::Hand);

    imgui_ok_ = ImGui::SFML::Init(window_, false);
    if (imgui_ok_) {
        ImGuiIO& io = ImGui::GetIO();
        io.IniFilename = nullptr;
        io.LogFilename = nullptr;
        io.MouseDrawCursor = false;
        io.Fonts->TexDesiredWidth = 2048;

        ImGuiStyle& style = ImGui::GetStyle();
        style.WindowPadding = ImVec2(0.f, 0.f);
        style.FramePadding = ImVec2(12.f, 8.f);
        style.ItemSpacing = ImVec2(8.f, 8.f);
        style.WindowRounding = theme::radius_md;
        style.FrameRounding = theme::radius_sm;
        style.PopupRounding = theme::radius_sm;
        style.ScrollbarRounding = theme::radius_sm;
        style.GrabRounding = theme::radius_sm;
        style.WindowBorderSize = 1.f;
        style.FrameBorderSize = 1.f;
        style.AntiAliasedLines = true;
        style.AntiAliasedFill = true;

        auto set_color = [&style](ImGuiCol idx, sf::Color c) {
            style.Colors[idx] = ImVec4(c.r / 255.f, c.g / 255.f, c.b / 255.f, c.a / 255.f);
        };
        set_color(ImGuiCol_Text, theme::text);
        set_color(ImGuiCol_TextDisabled, theme::text_dim);
        set_color(ImGuiCol_WindowBg, theme::bg);
        set_color(ImGuiCol_ChildBg, theme::surface);
        set_color(ImGuiCol_PopupBg, theme::surface);
        set_color(ImGuiCol_Border, theme::border);
        set_color(ImGuiCol_FrameBg, theme::surface_alt);
        set_color(ImGuiCol_FrameBgHovered, theme::surface_hover);
        set_color(ImGuiCol_FrameBgActive, theme::surface_hover);
        set_color(ImGuiCol_TitleBg, theme::surface);
        set_color(ImGuiCol_TitleBgActive, theme::surface_alt);
        set_color(ImGuiCol_Button, theme::surface_alt);
        set_color(ImGuiCol_ButtonHovered, theme::surface_hover);
        set_color(ImGuiCol_ButtonActive, theme::border_strong);
        set_color(ImGuiCol_Header, theme::surface_alt);
        set_color(ImGuiCol_HeaderHovered, theme::surface_hover);
        set_color(ImGuiCol_HeaderActive, theme::border_strong);
        set_color(ImGuiCol_Separator, theme::border);
        set_color(ImGuiCol_CheckMark, theme::gold);
        set_color(ImGuiCol_SliderGrab, theme::gold);
        set_color(ImGuiCol_SliderGrabActive, theme::gold_bright);
        set_color(ImGuiCol_ScrollbarBg, theme::bg_elevated);
        set_color(ImGuiCol_ScrollbarGrab, theme::border);

        // 字体按物理像素加载；draw list 坐标仍使用 1280x720 设计坐标，
        // 由 DisplayFramebufferScale 做最终缩放，避免 1920 下放大发虚。
        io.Fonts->Clear();
        const float body_px = 20.f * ui_scale_y_;
        const float heading_px = 34.f * ui_scale_y_;
        const float display_px = 76.f * ui_scale_y_;
        ImFontGlyphRangesBuilder range_builder;
        range_builder.AddRanges(io.Fonts->GetGlyphRangesChineseFull());
        range_builder.AddText("≤≥×→←↑↓");
        ImVector<ImWchar> glyph_ranges;
        range_builder.BuildRanges(&glyph_ranges);
        auto add_font = [&io, &glyph_ranges](const unsigned char* data, std::size_t size,
                                            float pixels) -> ImFont* {
            ImFontConfig cfg;
            cfg.FontDataOwnedByAtlas = false;
            cfg.OversampleH = 2;
            cfg.OversampleV = 2;
            cfg.PixelSnapH = true;
            return io.Fonts->AddFontFromMemoryTTF(
                const_cast<unsigned char*>(data), static_cast<int>(size), pixels,
                &cfg, glyph_ranges.Data);
        };
        ImFont* body_font = add_font(embedded::kFontRegularData,
                                     embedded::kFontRegularSize, body_px);
        ImFont* heading_font = add_font(embedded::kFontHeadingData,
                                       embedded::kFontHeadingSize, heading_px);
        ImFont* display_font = add_font(embedded::kFontDisplayData,
                                       embedded::kFontDisplaySize, display_px);
        if (body_font == nullptr) {
            body_font = io.Fonts->AddFontDefault();
        }
        io.FontDefault = body_font;
        imgui_ui::set_fonts(body_font, heading_font ? heading_font : body_font,
                            display_font ? display_font : body_font);
        if (!ImGui::SFML::UpdateFontTexture()) {
            io.Fonts->Clear();
            body_font = io.Fonts->AddFontDefault();
            io.FontDefault = body_font;
            imgui_ui::set_fonts(body_font, body_font, body_font);
            const bool fallback_ok = ImGui::SFML::UpdateFontTexture();
            (void)fallback_ok;
        }
        apply_imgui_scale();
    }

    splash_time_ = 0.f;
    splash_active_ = settings_.splash_enabled && capture_mode_.empty();
    if (capture_mode_ == "splash") {
        splash_active_ = true;
        splash_time_ = 0.9f;
    } else if (capture_mode_ == "settings") {
        screen_ = Screen::Menu;
        settings_open_ = true;
    }
}

void App::create_window() {
    const sf::VideoMode desktop = sf::VideoMode::getDesktopMode();
    sf::VideoMode video_mode = desktop;
    sf::Uint32 style = sf::Style::Titlebar | sf::Style::Close;
    borderless_window_ = settings_.display_mode != DisplayMode::Windowed;
    if (settings_.display_mode == DisplayMode::Windowed) {
        video_mode = sf::VideoMode(static_cast<unsigned>(settings_.window_width),
                                   static_cast<unsigned>(settings_.window_height));
    } else {
        // 无边框窗口全屏：不调用 SFML 的 Style::Fullscreen，也就不会触发
        // ChangeDisplaySettings / 桌面显示模式切换。
        // 关键：不要恰好铺满桌面。2560x1600 的 WS_POPUP 会被 Windows 当成
        // 全屏应用交给 DirectFlip/MPO；本机这条路径会定格旧帧并闪黑。
        // 比桌面少 2px 即可保持 DWM 合成。
        const unsigned width = desktop.width > 2 ? desktop.width - 2 : desktop.width;
        const unsigned height = desktop.height > 2 ? desktop.height - 2 : desktop.height;
        video_mode = sf::VideoMode(width, height);
        style = sf::Style::None;
    }

    window_.create(video_mode, window_title(), style);
    if (!window_.isOpen()) {
        borderless_window_ = false;
        window_.create(sf::VideoMode(1280, 720), window_title(),
                       sf::Style::Titlebar | sf::Style::Close);
    }
    window_size_ = window_.getSize();
    ui_scale_x_ = static_cast<float>(window_size_.x) / kW;
    ui_scale_y_ = static_cast<float>(window_size_.y) / kH;

    ui_view_ = sf::View(sf::FloatRect(0.f, 0.f, kW, kH));
    window_.setView(ui_view_);
    window_.setVerticalSyncEnabled(true);
    window_.setKeyRepeatEnabled(false);

    if (borderless_window_) {
        window_.setPosition({0, 0});
    } else {
        const int x = std::max(0, static_cast<int>(desktop.width) / 2 -
                                      static_cast<int>(window_size_.x) / 2);
        const int y = std::max(0, static_cast<int>(desktop.height) / 2 -
                                      static_cast<int>(window_size_.y) / 2);
        window_.setPosition({x, y});
    }
    build_window_icon();
}

sf::Vector2f App::to_ui(const sf::Vector2f& point) const {
    if (window_size_.x == 0 || window_size_.y == 0) return point;
    return {point.x * kW / static_cast<float>(window_size_.x),
            point.y * kH / static_cast<float>(window_size_.y)};
}

void App::apply_imgui_scale() {
    if (!imgui_ok_) return;
    ImGuiIO& io = ImGui::GetIO();
    io.DisplaySize = ImVec2(kW, kH);
    io.DisplayFramebufferScale = ImVec2(ui_scale_x_, ui_scale_y_);
}

void App::handle_window_resize(unsigned width, unsigned height) {
    if (width == 0 || height == 0) return;
    window_size_ = {width, height};
    ui_scale_x_ = static_cast<float>(width) / kW;
    ui_scale_y_ = static_cast<float>(height) / kH;
    ui_view_ = sf::View(sf::FloatRect(0.f, 0.f, kW, kH));
    window_.setView(ui_view_);
    apply_imgui_scale();
}

void App::update_borderless_window_state() {
#ifdef _WIN32
    if (!borderless_window_ || !window_.isOpen()) return;
    const bool focused = window_.hasFocus();
    if (focused == window_focused_ && borderless_topmost_ == focused) return;
    HWND hwnd = reinterpret_cast<HWND>(window_.getSystemHandle());
    if (hwnd != nullptr) {
        SetWindowPos(hwnd, focused ? HWND_TOPMOST : HWND_NOTOPMOST, 0, 0, 0, 0,
                     SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE | SWP_NOOWNERZORDER);
    }
    window_focused_ = focused;
    borderless_topmost_ = focused;
#endif
}

void App::save_settings() {
    SettingsStore::save(settings_);
}

App::~App() {
    if (imgui_ok_) {
        ImGui::SFML::Shutdown(window_);
    }
}

int App::run() {
    if (!font_ok_) {
        return 1;
    }
    if (!capture_mode_.empty()) {
        run_capture();
        return 0;
    }
    sf::Clock clock;
    while (window_.isOpen()) {
        sf::Event event{};
        while (window_.pollEvent(event)) {
            if (imgui_ok_) {
                ImGui::SFML::ProcessEvent(window_, event);
            }
            handle_event(event);
        }
        update_borderless_window_state();
        const sf::Time delta = clock.restart();
        if (imgui_ok_) {
            ImGui::SFML::SetCurrentWindow(window_);
            const sf::Vector2i mouse = sf::Mouse::getPosition(window_);
            const sf::Vector2f virtual_mouse = to_ui(
                {static_cast<float>(mouse.x), static_cast<float>(mouse.y)});
            ImGui::SFML::Update(sf::Vector2i(static_cast<int>(virtual_mouse.x),
                                             static_cast<int>(virtual_mouse.y)),
                                sf::Vector2f(kW, kH), delta);
            apply_imgui_scale();
        }
        update(delta.asSeconds());
        window_.clear(theme::bg);
        draw();
        window_.display();
    }
    return 0;
}

void App::apply_reference_for_capture() {
    graph_ = LevelBuilder::build(level_.raw, true);
    board_.set_graph(&graph_);
    reset_sim();
}

void App::build_fail_demo() {
    graph_ = LevelBuilder::build(level_.raw, false);
    const Vec2i cells[] = {
        {1, 2}, {1, 3}, {1, 4}, {1, 5}, {1, 6}, {2, 6}, {3, 6},
        {3, 5}, {3, 4}, {3, 3}, {4, 3}, {5, 3}, {6, 3}, {6, 2}, {7, 2},
    };
    nlohmann::json path = nlohmann::json::array();
    for (const Vec2i& cell : cells) {
        path.push_back(nlohmann::json::array({cell.x, cell.y}));
    }
    LevelBuilder::apply_path(graph_, path);
    board_.set_graph(&graph_);
    reset_sim();
}

void App::run_capture() {
    progress_ = Progress{};
    if (!capture_path_.empty()) {
        std::error_code ec;
        std::filesystem::create_directories(
            std::filesystem::path(capture_path_).parent_path(), ec);
    }

    if (capture_mode_ == "menu") {
        screen_ = Screen::Menu;
    } else if (capture_mode_ == "game") {
        load_level(0);
        apply_reference_for_capture();
        set_phase(Phase::Editing);
        for (int i = 0; i < 4 && !sim_.finished(); ++i) {
            const TickFrame& frame = sim_.step();
            board_.set_frame(&frame);
            waveform_.set_state(expect_sequence(), target_start_tick(), level_.tick_budget,
                                sim_.target_actual(target_uid_), sim_.first_error(),
                                sim_.glitch_history());
        }
        set_phase(Phase::Running);
        message_ = "运行中……灯槽正把信号一格格送向目标。";
    } else if (capture_mode_ == "level2game") {
        progress_.unlocked = 2;
        load_level(1);
        apply_reference_for_capture();
        set_phase(Phase::Editing);
        for (int i = 0; i < 3 && !sim_.finished(); ++i) {
            const TickFrame& frame = sim_.step();
            board_.set_frame(&frame);
            waveform_.set_state(expect_sequence(), target_start_tick(), level_.tick_budget,
                                sim_.target_actual(target_uid_), sim_.first_error(),
                                sim_.glitch_history());
        }
        set_phase(Phase::Running);
        message_ = "运行中……脉冲进入非门的那一拍，目标会先灭。";
    } else if (capture_mode_ == "level3game") {
        progress_.unlocked = 3;
        load_level(2);
        apply_reference_for_capture();
        set_phase(Phase::Editing);
        for (int i = 0; i < 2 && !sim_.finished(); ++i) {
            const TickFrame& frame = sim_.step();
            board_.set_frame(&frame);
            waveform_.set_state(expect_sequence(), target_start_tick(), level_.tick_budget,
                                sim_.target_actual(target_uid_), sim_.first_error(),
                                sim_.glitch_history());
        }
        set_phase(Phase::Running);
        message_ = "运行中……两级非门把脉冲翻回亮，目标只看到一盏短灯。";
    } else if (capture_mode_ == "level5game") {
        progress_.unlocked = 5;
        load_level(4);
        apply_reference_for_capture();
        set_phase(Phase::Editing);
        for (int i = 0; i < 6 && !sim_.finished(); ++i) {
            const TickFrame& frame = sim_.step();
            board_.set_frame(&frame);
            waveform_.set_state(expect_sequence(), target_start_tick(), level_.tick_budget,
                                sim_.target_actual(target_uid_), sim_.first_error(),
                                sim_.glitch_history());
        }
        set_phase(Phase::Running);
        message_ = "运行中……或门正在把两条支路汇成一条河。";
    } else if (capture_mode_ == "twostar") {
        load_level(0);
        graph_ = LevelBuilder::build(level_.raw, false);
        const Vec2i cells[] = {
            {1, 2}, {1, 3}, {2, 3}, {3, 3}, {4, 3},
            {5, 3}, {6, 3}, {6, 2}, {7, 2},
        };
        nlohmann::json path = nlohmann::json::array();
        for (const Vec2i& cell : cells) {
            path.push_back(nlohmann::json::array({cell.x, cell.y}));
        }
        LevelBuilder::apply_path(graph_, path);
        board_.set_graph(&graph_);
        reset_sim();
        set_phase(Phase::Editing);
        start_run();
        for (int i = 0; i < 100 && !sim_.finished(); ++i) {
            step_once();
        }
    } else if (capture_mode_ == "pass") {
        load_level(0);
        apply_reference_for_capture();
        set_phase(Phase::Editing);
        start_run();
        for (int i = 0; i < 100 && !sim_.finished(); ++i) {
            step_once();
        }
    } else if (capture_mode_ == "fail") {
        load_level(0);
        build_fail_demo();
        set_phase(Phase::Editing);
        start_run();
        for (int i = 0; i < 100 && !sim_.finished(); ++i) {
            step_once();
        }
    } else if (capture_mode_ == "level5") {
        progress_.unlocked = 5;
        load_level(4);
        apply_reference_for_capture();
        set_phase(Phase::Editing);
        start_run();
        for (int i = 0; i < 100 && !sim_.finished(); ++i) {
            step_once();
        }
    } else {
        screen_ = Screen::Menu;
    }

    refresh_level_cards();
    refresh_button_labels();
    fade_ = 1.f;
    overlay_anim_ = 1.f;
    capture_window(capture_path_);
}

bool App::capture_window(const std::string& path) {
    window_.setActive(true);
    for (int i = 0; i < 4; ++i) {
        window_.clear(theme::bg);
        if (imgui_ok_) {
            ImGui::SFML::SetCurrentWindow(window_);
            ImGui::SFML::Update(sf::Vector2i(640, 360), sf::Vector2f(kW, kH),
                                sf::seconds(1.f / 60.f));
            apply_imgui_scale();
        }
        draw();
        window_.display();
        sf::sleep(sf::milliseconds(40));
    }
    window_.clear(theme::bg);
    if (imgui_ok_) {
        ImGui::SFML::SetCurrentWindow(window_);
        ImGui::SFML::Update(sf::Vector2i(640, 360), sf::Vector2f(kW, kH),
                            sf::seconds(1.f / 60.f));
        apply_imgui_scale();
    }
    draw();
    window_.display();

    sf::Texture texture;
    if (!texture.create(window_.getSize().x, window_.getSize().y)) {
        return false;
    }
    texture.update(window_);
    const sf::Image image = texture.copyToImage();
    return image.saveToFile(path);
}

void App::build_window_icon() {
    sf::Image icon;
    icon.create(32, 32, sf::Color(8, 12, 20));
    for (int y = 0; y < 32; ++y) {
        for (int x = 0; x < 32; ++x) {
            const int dx = x - 15;
            const int dy = y - 13;
            if (dx * dx + dy * dy <= 49) {
                icon.setPixel(x, y, sf::Color(255, 196, 80));
            } else if (x >= 12 && x <= 19 && y >= 20 && y <= 28) {
                icon.setPixel(x, y, sf::Color(55, 92, 150));
            } else if (y >= 27 && y <= 29) {
                icon.setPixel(x, y, sf::Color(35, 62, 105));
            }
        }
    }
    window_.setIcon(icon.getSize().x, icon.getSize().y, icon.getPixelsPtr());
}

void App::build_ui() {
    menu_buttons_.clear();
    menu_buttons_.push_back(Button{{72.f, 360.f, 232.f, 46.f}, "开始游戏", true, true, false, false, false, false, static_cast<unsigned>(theme::text_title)});
    menu_buttons_.push_back(Button{{72.f, 418.f, 232.f, 38.f}, "重置进度", true, false, false, false, false, false, static_cast<unsigned>(theme::text_body)});
    menu_buttons_.push_back(Button{{72.f, 466.f, 232.f, 38.f}, "音效：开", true, false, false, true, false, false, static_cast<unsigned>(theme::text_body)});
    menu_buttons_.push_back(Button{{72.f, 514.f, 232.f, 38.f}, "设置", true, false, false, false, false, false, static_cast<unsigned>(theme::text_body)});
    menu_buttons_.push_back(Button{{72.f, 562.f, 232.f, 38.f}, "退出", true, false, false, false, false, false, static_cast<unsigned>(theme::text_body)});

    back_button_ = Button{{16.f, 16.f, 76.f, 32.f}, "返回", true, false, false, false, false, false, static_cast<unsigned>(theme::text_label)};
    run_button_ = Button{{330.f, 604.f, 84.f, 34.f}, "运行", true, true, false, false, false, false, static_cast<unsigned>(theme::text_body)};
    retry_button_ = Button{{422.f, 604.f, 84.f, 34.f}, "重试", true, false, false, false, false, false, static_cast<unsigned>(theme::text_small)};
    hint_button_ = Button{{514.f, 604.f, 84.f, 34.f}, "提示", true, false, false, false, false, false, static_cast<unsigned>(theme::text_small)};
    speed_button_ = Button{{606.f, 604.f, 84.f, 34.f}, "1.0x", true, false, false, true, false, false, static_cast<unsigned>(theme::text_small)};
    next_button_ = Button{{698.f, 604.f, 84.f, 34.f}, "下一关", false, false, false, false, false, false, static_cast<unsigned>(theme::text_small)};

    board_.set_origin({342.f, 116.f});
    board_.set_tool("wire");
    board_.on_edit_started = [this]() { on_edit_started(); };
    board_.on_layout_changed = [this]() { on_layout_changed(); };

    overlay_buttons_.clear();
    rebuild_settings_buttons();
}

void App::rebuild_settings_buttons() {
    settings_buttons_.clear();
    const DisplayMode modes[2] = {DisplayMode::Windowed, DisplayMode::Borderless};
    for (int i = 0; i < 2; ++i) {
        Button button;
        button.rect = {368.f + i * 230.f, 208.f, 220.f, 38.f};
        button.label = display_mode_name(modes[i]);
        button.active = settings_.display_mode == modes[i];
        button.font_size = static_cast<unsigned>(theme::text_small);
        settings_buttons_.push_back(button);
    }
    const int sizes[4][2] = {{1280, 720}, {1600, 900}, {1920, 1080}, {2560, 1440}};
    for (int i = 0; i < 4; ++i) {
        Button button;
        button.rect = {368.f + i * 138.f, 284.f, 126.f, 34.f};
        button.label = std::to_string(sizes[i][0]) + " × " +
                       std::to_string(sizes[i][1]);
        button.enabled = settings_.display_mode == DisplayMode::Windowed;
        button.active = button.enabled && settings_.window_width == sizes[i][0] &&
                        settings_.window_height == sizes[i][1];
        button.font_size = static_cast<unsigned>(theme::text_small);
        settings_buttons_.push_back(button);
    }
    Button splash;
    splash.rect = {368.f, 348.f, 220.f, 36.f};
    splash.label = settings_.splash_enabled ? "启动动画：开" : "启动动画：关";
    splash.active = settings_.splash_enabled;
    splash.font_size = static_cast<unsigned>(theme::text_small);
    settings_buttons_.push_back(splash);

    Button reset;
    reset.rect = {368.f, 486.f, 126.f, 36.f};
    reset.label = "恢复默认";
    reset.font_size = static_cast<unsigned>(theme::text_small);
    settings_buttons_.push_back(reset);

    Button close;
    close.rect = {790.f, 486.f, 126.f, 36.f};
    close.label = "关闭";
    close.font_size = static_cast<unsigned>(theme::text_small);
    settings_buttons_.push_back(close);
}

void App::save_progress() {
    ProgressStore::save(progress_);
}

void App::refresh_level_cards() {
    level_cards_.clear();
    const auto& entries = repo_.entries();
    for (int i = 0; i < static_cast<int>(entries.size()) && i < 5; ++i) {
        Button card;
        card.rect = {560.f, 150.f + i * 78.f, 648.f, 68.f};
        card.label = entries[i].name;
        card.enabled = i < progress_.unlocked;
        card.font_size = static_cast<unsigned>(theme::text_h3);
        card.hovered = false;
        level_cards_.push_back(card);
    }
}

void App::rebuild_game_level_buttons() {
    game_level_buttons_.clear();
    const auto& entries = repo_.entries();
    for (int i = 0; i < static_cast<int>(entries.size()) && i < 5; ++i) {
        Button button;
        button.rect = {26.f, 108.f + i * 32.f, 182.f, 28.f};
        button.label = entries[i].name;
        button.enabled = i < progress_.unlocked;
        button.active = i == current_index_;
        button.font_size = static_cast<unsigned>(theme::text_small);
        game_level_buttons_.push_back(button);
    }
}

void App::rebuild_palette() {
    palette_buttons_.clear();
    nlohmann::json available = level_.raw.value("available", nlohmann::json::array({"wire"}));
    if (!available.is_array() || available.empty()) {
        available = nlohmann::json::array({"wire"});
    }
    bool has_wire = false;
    for (const auto& item : available) {
        if (item.is_string() && item.get<std::string>() == "wire") {
            has_wire = true;
        }
    }
    selected_tool_ = has_wire ? "wire" : available[0].get<std::string>();
    board_.set_tool(selected_tool_);

    int i = 0;
    for (const auto& item : available) {
        if (!item.is_string()) {
            continue;
        }
        const std::string type = item.get<std::string>();
        Button button;
        button.rect = {26.f, 306.f + i * 34.f, 182.f, 30.f};
        button.label = ComponentRules::display_name(type);
        button.toggle = true;
        button.active = type == selected_tool_;
        button.font_size = static_cast<unsigned>(theme::text_small);
        palette_buttons_.push_back(button);
        ++i;
    }
}

void App::rebuild_overlay_buttons() {
    overlay_buttons_.clear();
    const bool passed = phase_ == Phase::Passed;
    Button primary;
    primary.rect = {370.f, 458.f, 150.f, 44.f};
    primary.primary = true;
    Button secondary;
    secondary.rect = {536.f, 458.f, 150.f, 44.f};
    Button third;
    third.rect = {702.f, 458.f, 150.f, 44.f};
    primary.font_size = static_cast<unsigned>(theme::text_subhead);
    secondary.font_size = static_cast<unsigned>(theme::text_subhead);
    third.font_size = static_cast<unsigned>(theme::text_subhead);

    if (passed) {
        primary.label = current_index_ + 1 < static_cast<int>(repo_.entries().size())
                            ? "下一关"
                            : "返回关卡";
        secondary.label = "重玩";
        third.label = "选关";
    } else {
        primary.label = "重试";
        secondary.label = "改布局";
        third.label = "选关";
    }
    overlay_buttons_.push_back(primary);
    overlay_buttons_.push_back(secondary);
    overlay_buttons_.push_back(third);
}

void App::refresh_button_labels() {
    if (menu_buttons_.size() >= 5) {
        menu_buttons_[0].label = "开始游戏";
        menu_buttons_[1].label = reset_confirm_ ? "再点一次确认" : "重置进度";
        menu_buttons_[2].label = sound_.muted() ? "音效：关" : "音效：开";
        menu_buttons_[3].label = "设置";
        menu_buttons_[4].label = "退出";
    }

    if (phase_ == Phase::Running) {
        run_button_.label = "暂停";
    } else if (phase_ == Phase::Paused) {
        run_button_.label = "继续";
    } else if (phase_ == Phase::Failed) {
        run_button_.label = "重新运行";
    } else if (phase_ == Phase::Passed) {
        run_button_.label = "重玩";
    } else {
        run_button_.label = "运行";
    }
    retry_button_.label = "重试";
    hint_button_.label = hint_visible_ ? "隐藏提示" : "提示";
    speed_button_.label = speed_ == 1.f ? "1.0x" : (speed_ == 2.f ? "2.0x" : "0.5x");
    next_button_.label = "下一关";
    next_button_.enabled = phase_ == Phase::Passed && !overlay_visible_ &&
                           current_index_ + 1 < static_cast<int>(repo_.entries().size());
}

void App::load_level(int index) {
    const auto& entries = repo_.entries();
    if (index < 0 || index >= static_cast<int>(entries.size())) {
        return;
    }
    LevelData data = repo_.load_level(entries[index].id);
    if (!data.valid) {
        menu_message_ = "关卡加载失败：" + entries[index].id;
        return;
    }
    current_index_ = index;
    level_ = std::move(data);
    graph_ = LevelBuilder::build(level_.raw, false);
    board_.set_graph(&graph_);
    undo_stack_.clear();
    overlay_visible_ = false;
    hint_visible_ = false;
    phase_ = Phase::Editing;
    selected_tool_ = "wire";
    rebuild_game_level_buttons();
    rebuild_palette();
    reset_sim();
    set_phase(Phase::Editing);
    refresh_button_labels();
    screen_ = Screen::Game;
    fade_ = 0.f;
    menu_message_.clear();
    message_ = "从信号源开始，按住左键拖出灯槽，接到目标端口。";
    board_.set_editable(true);
}

void App::reset_sim() {
    sim_.setup(graph_, level_.tick_budget, level_.max_components,
               level_.stars.three, level_.stars.two, level_.stars.one);
    target_uid_ = sim_.first_target_uid();
    board_.set_frame(nullptr);
    board_.clear_error();
    waveform_.set_state(expect_sequence(), target_start_tick(), level_.tick_budget, {}, std::nullopt, {});
}

void App::set_phase(Phase phase) {
    phase_ = phase;
    board_.set_editable(phase_ != Phase::Running && !overlay_visible_);
    refresh_button_labels();
}

void App::start_run() {
    if (!level_.valid) {
        return;
    }
    const auto errors = sim_.validation_errors();
    if (!errors.empty()) {
        overlay_visible_ = false;
        set_phase(Phase::Editing);
        message_ = errors.front();
        return;
    }
    sim_.reset();
    board_.set_frame(nullptr);
    board_.clear_error();
    waveform_.set_state(expect_sequence(), target_start_tick(), level_.tick_budget, {}, std::nullopt, {});
    accumulator_ = 0.f;
    overlay_visible_ = false;
    set_phase(Phase::Running);
    message_ = "运行中……看灯槽一格一格亮起来。";
    sound_.play_run();
}

void App::step_once() {
    if (sim_.finished()) {
        return;
    }
    const TickFrame& frame = sim_.step();
    board_.set_frame(&frame);
    waveform_.set_state(expect_sequence(), target_start_tick(), level_.tick_budget,
                        sim_.target_actual(target_uid_), sim_.first_error(), sim_.glitch_history());

    if (!sim_.finished()) {
        return;
    }

    if (sim_.passed()) {
        unlock_next();
        set_phase(Phase::Passed);
        message_ = "通过！第 " + std::to_string(sim_.completion_tick()) + " 拍完成 · " +
                   std::to_string(sim_.stars()) + " 星。";
        sound_.play_pass();
    } else {
        set_phase(Phase::Failed);
        board_.set_error(sim_.first_error());
        message_ = error_text();
        sound_.play_fail();
    }
    show_result_overlay();
}

void App::show_result_overlay() {
    overlay_visible_ = true;
    overlay_anim_ = 0.f;
    overlay_title_ = phase_ == Phase::Passed ? "通过！" : "差一点";
    if (phase_ == Phase::Passed) {
        overlay_message_ = "第 " + std::to_string(sim_.completion_tick()) + " 拍完成。";
        const std::string gap = star_gap_text();
        if (!gap.empty()) {
            overlay_message_ += "\n" + gap;
        }
    } else {
        overlay_message_ = error_text();
    }
    rebuild_overlay_buttons();
    board_.set_editable(false);
    refresh_button_labels();
}

void App::unlock_next() {
    const int count = static_cast<int>(repo_.entries().size());
    if (current_index_ + 1 < count) {
        progress_.unlocked = std::max(progress_.unlocked, current_index_ + 2);
    }
    progress_.unlocked = std::clamp(progress_.unlocked, 1, std::max(1, count));
    const std::string id = level_.id;
    if (!id.empty() && !progress_.is_completed(id)) {
        progress_.completed.push_back(id);
    }
    if (!id.empty()) {
        progress_.record_stars(id, sim_.stars());
    }
    if (capture_mode_.empty()) {
        save_progress();
    }
    refresh_level_cards();
    rebuild_game_level_buttons();
    next_button_.enabled = phase_ == Phase::Passed && !overlay_visible_ &&
                           current_index_ + 1 < count;
}

void App::toggle_run() {
    if (overlay_visible_) {
        return;
    }
    switch (phase_) {
        case Phase::Running:
            set_phase(Phase::Paused);
            message_ = "已暂停。按空格继续；也可以修改布局（修改会重置本次运行）。";
            break;
        case Phase::Paused:
            set_phase(Phase::Running);
            message_ = "继续运行……";
            break;
        case Phase::Editing:
            start_run();
            break;
        case Phase::Failed:
        case Phase::Passed:
            overlay_visible_ = false;
            reset_sim();
            set_phase(Phase::Editing);
            start_run();
            break;
    }
}

void App::retry() {
    overlay_visible_ = false;
    reset_sim();
    set_phase(Phase::Editing);
    message_ = "已重置。布局保留，可以重新运行。";
}

void App::undo() {
    if (undo_stack_.empty()) {
        message_ = "没有可撤销的操作。";
        return;
    }
    graph_ = undo_stack_.back();
    undo_stack_.pop_back();
    board_.set_graph(&graph_);
    overlay_visible_ = false;
    reset_sim();
    set_phase(Phase::Editing);
    message_ = "已撤销上一步。";
}

void App::next_level() {
    if (current_index_ + 1 < static_cast<int>(repo_.entries().size())) {
        overlay_visible_ = false;
        load_level(current_index_ + 1);
    } else {
        back_to_menu();
    }
}

void App::back_to_menu() {
    screen_ = Screen::Menu;
    fade_ = 0.f;
    overlay_visible_ = false;
    refresh_level_cards();
    refresh_button_labels();
    menu_message_ = "选择关卡，或直接开始游戏。";
}

void App::start_game() {
    const auto& entries = repo_.entries();
    if (entries.empty()) {
        menu_message_ = "找不到关卡数据。";
        return;
    }
    int index = progress_.unlocked - 1;
    for (int i = 0; i < static_cast<int>(entries.size()) && i < progress_.unlocked; ++i) {
        if (!progress_.is_completed(entries[i].id)) {
            index = i;
            break;
        }
    }
    index = std::clamp(index, 0, static_cast<int>(entries.size()) - 1);
    load_level(index);
}

void App::reset_progress() {
    if (!reset_confirm_) {
        reset_confirm_ = true;
        menu_message_ = "再点一次“重置进度”确认。";
        refresh_button_labels();
        return;
    }
    ProgressStore::reset();
    progress_ = Progress{};
    reset_confirm_ = false;
    refresh_level_cards();
    refresh_button_labels();
    menu_message_ = "进度已重置。";
}

void App::on_edit_started() {
    if (undo_stack_.size() >= 50) {
        undo_stack_.erase(undo_stack_.begin());
    }
    undo_stack_.push_back(graph_);
}

void App::on_layout_changed() {
    overlay_visible_ = false;
    reset_sim();
    set_phase(Phase::Editing);
    next_button_.enabled = false;
    message_ = "布局已修改。点“运行”验证；按 Ctrl+Z 撤销。";
    board_.set_editable(true);
}

void App::update(float dt) {
    if (splash_active_) {
        if (capture_mode_.empty()) {
            splash_time_ += dt;
            if (splash_time_ >= 2.2f) {
                splash_active_ = false;
                fade_ = 0.f;
            }
        }
        return;
    }
    time_ += dt;
    board_.update(dt);
    fade_ = std::min(1.f, fade_ + dt / theme::duration_normal);
    overlay_anim_ = std::min(1.f, overlay_anim_ + dt / theme::duration_slow);
    if (screen_ != Screen::Game || phase_ != Phase::Running || overlay_visible_) {
        return;
    }
    accumulator_ += dt * speed_;
    int safety = 0;
    while (accumulator_ >= kTickInterval && phase_ == Phase::Running && !overlay_visible_ &&
           safety < 8) {
        accumulator_ -= kTickInterval;
        step_once();
        ++safety;
    }
}

void App::draw() {
    if (splash_active_) {
        draw_splash();
    } else if (screen_ == Screen::Menu) {
        draw_menu();
    } else {
        draw_game();
        if (overlay_visible_) {
            draw_overlay();
        }
    }
    if (imgui_ok_) {
        ImGui::SFML::Render(window_);
    }
}

namespace {

std::string level_tagline(int index) {
    static const char* tags[] = {
        "灯槽 · 每格 1 拍",
        "非门 · 先灭后亮",
        "双非门 · 偶次翻转",
        "与门 · 同拍对齐",
        "或门 · 合流",
    };
    if (index < 0 || index >= 5) return {};
    return tags[index];
}

void draw_fade_overlay(ImDrawList* draw, float fade) {
    if (draw == nullptr || fade >= 1.f) return;
    const int alpha = static_cast<int>(std::clamp(1.f - fade, 0.f, 1.f) * 255.f);
    draw->AddRectFilled({0.f, 0.f}, {1280.f, 720.f}, IM_COL32(10, 14, 20, alpha));
}

void draw_waveform_panel(ImDrawList* draw, const std::vector<int>& expect, int start_tick,
                         int budget, const std::vector<int>& actual,
                         const std::optional<ErrorInfo>& error,
                         const std::vector<GlitchMark>& glitches, ImVec2 origin, float width) {
    if (draw == nullptr) return;
    budget = std::max(1, budget);
    const float label_w = 38.f;
    const float cell_w = std::clamp((width - label_w - 6.f) / static_cast<float>(budget),
                                    5.f, 11.f);
    const float y_target = origin.y + 16.f;
    const float y_actual = y_target + 34.f;
    const float bar_h = 15.f;
    const float x0 = origin.x + label_w;

    imgui_ui::text(draw, "目标", theme::text_micro, {origin.x, y_target}, theme::text_dim);
    imgui_ui::text(draw, "实际", theme::text_micro, {origin.x, y_target + 18.f}, theme::text_dim);

    for (int i = 0; i < budget; ++i) {
        const float x = x0 + i * cell_w;
        int expected = -1;
        const int idx = i - start_tick;
        if (idx >= 0 && idx < static_cast<int>(expect.size())) expected = expect[idx];
        int observed = -1;
        if (i < static_cast<int>(actual.size())) observed = actual[i];
        const sf::Color expected_color = expected == 1 ? theme::gold
                                        : expected == 0 ? theme::surface_hover
                                                        : theme::bg_elevated;
        const sf::Color observed_color = observed == 1 ? theme::teal
                                        : observed == 0 ? theme::wire_low
                                                        : theme::bg_elevated;
        imgui_ui::panel(draw, {x, y_target}, {x + cell_w - 1.f, y_target + bar_h},
                        expected_color, sf::Color(0, 0, 0, 0), 2.f, 0.f);
        imgui_ui::panel(draw, {x, y_actual}, {x + cell_w - 1.f, y_actual + bar_h},
                        observed_color, sf::Color(0, 0, 0, 0), 2.f, 0.f);
    }

    const float sx = x0 + start_tick * cell_w;
    imgui_ui::vline(draw, sx, y_target - 3.f, y_actual + bar_h + 3.f,
                    sf::Color(theme::gold.r, theme::gold.g, theme::gold.b, 150), 1.f);

    if (error.has_value() && error->tick >= 0) {
        const float ex = x0 + error->tick * cell_w;
        draw->AddRect({ex - 1.f, y_target - 3.f}, {ex + cell_w, y_actual + bar_h + 3.f},
                      imgui_ui::color(theme::danger), 2.f, ImDrawFlags_RoundCornersAll, 1.5f);
    }

    for (const GlitchMark& glitch : glitches) {
        if (glitch.tick < 0) continue;
        const float gx = x0 + glitch.tick * cell_w + cell_w * 0.5f;
        imgui_ui::dot(draw, {gx, y_actual + bar_h + 9.f}, 2.5f, theme::danger);
    }

    const int step = budget <= 32 ? 4 : 8;
    for (int i = 0; i < budget; i += step) {
        imgui_ui::text(draw, std::to_string(i), theme::text_nano, {x0 + i * cell_w, origin.y + 72.f}, theme::text_dim);
    }
}

}  // namespace

void App::draw_background() {
    ui::fill_rect(window_, {0.f, 0.f, kW, kH}, theme::bg);
    // 棋盘外框由 SFML 绘制，ImGui 只负责其上的界面层，避免整屏 overdraw。
    ui::draw_rounded_rect(window_, {330.f, 104.f, 472.f, 472.f}, theme::radius_lg,
                          theme::bg_elevated, theme::border, 1.f);
}

void App::draw_button(const Button& button) {
    if (!imgui_ok_ || button.rect.width <= 0.f || button.rect.height <= 0.f) return;
    ImDrawList* draw = ui_draw_ != nullptr ? ui_draw_ : ImGui::GetForegroundDrawList();
    sf::FloatRect rect = button.rect;
    const bool pressed = button.enabled && button.hovered &&
                         sf::Mouse::isButtonPressed(sf::Mouse::Left);
    if (pressed) {
        rect.top += 1.f;
    } else if (button.enabled && button.hovered) {
        rect.top -= 1.f;
    }

    sf::Color fill = theme::surface_alt;
    sf::Color border = theme::border;
    sf::Color label = theme::text;
    if (!button.enabled) {
        fill = theme::bg_elevated;
        border = theme::border;
        label = theme::text_dim;
    } else if (button.primary) {
        fill = pressed ? theme::gold : (button.hovered ? theme::gold_bright : theme::gold);
        border = theme::gold_bright;
        label = theme::bg;
    } else if (button.danger) {
        fill = button.hovered ? sf::Color(55, 32, 38) : theme::surface_alt;
        border = theme::danger;
        label = theme::danger;
    } else if (button.active) {
        fill = theme::surface_hover;
        border = theme::gold;
        label = theme::gold;
    } else if (button.hovered) {
        fill = theme::surface_hover;
        border = theme::border_strong;
    }

    imgui_ui::panel(draw, {rect.left, rect.top}, {rect.left + rect.width, rect.top + rect.height},
                    fill, border, theme::radius_md, button.active ? theme::border_active
                                                                  : theme::border_thin);
    imgui_ui::text_centered(draw, button.label, static_cast<float>(button.font_size),
                            {rect.left + rect.width * 0.5f,
                             rect.top + rect.height * 0.5f},
                            label);
}

void App::draw_panel(const sf::FloatRect& rect) {
    if (!imgui_ok_) return;
    ImDrawList* draw = ui_draw_ != nullptr ? ui_draw_ : ImGui::GetForegroundDrawList();
    imgui_ui::panel(draw, {rect.left, rect.top},
                    {rect.left + rect.width, rect.top + rect.height}, theme::surface,
                    theme::border, theme::radius_lg, theme::border_thin);
}

void App::draw_level_card(const Button& button, int index) {
    if (!imgui_ok_) return;
    ImDrawList* draw = ui_draw_ != nullptr ? ui_draw_ : ImGui::GetForegroundDrawList();
    sf::FloatRect rect = button.rect;
    if (button.enabled && button.hovered) rect.top -= 1.f;

    const std::string id = repo_.entries()[index].id;
    const bool completed = progress_.is_completed(id);
    const int best = progress_.stars_for(id);
    const bool enabled = button.enabled;
    int recommended = -1;
    for (int i = 0; i < static_cast<int>(level_cards_.size()) && i < progress_.unlocked; ++i) {
        if (!progress_.is_completed(repo_.entries()[i].id)) {
            recommended = i;
            break;
        }
    }
    const bool is_recommended = enabled && index == recommended;
    const sf::Color fill = !enabled ? theme::bg_elevated
                          : button.hovered ? theme::surface_hover
                                           : theme::surface;
    const sf::Color border = !enabled ? theme::border
                           : button.hovered ? theme::gold
                           : is_recommended ? theme::teal
                                            : theme::border_strong;
    imgui_ui::panel(draw, {rect.left, rect.top}, {rect.left + rect.width, rect.top + rect.height},
                    fill, border, theme::radius_md,
                    button.hovered || is_recommended ? theme::border_active
                                                     : theme::border_thin);

    const sf::Color accent = !enabled ? theme::border
                            : is_recommended ? theme::teal
                            : (completed ? theme::text_muted : theme::gold);
    draw->AddRectFilled({rect.left + 1.f, rect.top + 12.f},
                        {rect.left + 4.f, rect.top + rect.height - 12.f},
                        imgui_ui::color(accent), 2.f);
    imgui_ui::text(draw, number2(index + 1), theme::text_number, {rect.left + 18.f, rect.top + 17.f}, enabled ? (completed ? theme::teal : theme::gold) : theme::text_dim);
    imgui_ui::text(draw, button.label, theme::text_h3, {rect.left + 64.f, rect.top + 13.f}, enabled ? theme::text : theme::text_dim);
    imgui_ui::text(draw, level_tagline(index), theme::text_label, {rect.left + 64.f, rect.top + 40.f}, enabled ? theme::text_muted : theme::text_dim);

    const std::string status = completed ? "已通过"
                                : is_recommended ? "继续"
                                : (enabled ? "可挑战" : "未解锁");
    const sf::Color status_color = completed ? theme::teal
                                  : is_recommended ? theme::teal
                                  : (enabled ? theme::text_muted : theme::text_dim);
    imgui_ui::text_right(draw, status, theme::text_caption, rect.left + rect.width - 18.f, rect.top + 16.f, status_color);
    imgui_ui::stars(draw, rect.left + rect.width - 46.f, rect.top + 46.f, best, 3,
                    5.5f, 14.f, theme::gold, theme::border);
}

void App::draw_game_level_button(const Button& button, int index) {
    if (!imgui_ok_) return;
    ImDrawList* draw = ui_draw_ != nullptr ? ui_draw_ : ImGui::GetForegroundDrawList();
    sf::FloatRect rect = button.rect;
    if (button.enabled && button.hovered) rect.top -= 1.f;
    const sf::Color fill = !button.enabled ? theme::bg_elevated
                          : button.active ? theme::surface_hover
                          : button.hovered ? theme::surface_alt
                                           : theme::surface;
    const sf::Color border = button.active ? theme::gold
                             : button.hovered ? theme::border_strong
                                              : theme::border;
    imgui_ui::panel(draw, {rect.left, rect.top}, {rect.left + rect.width, rect.top + rect.height},
                    fill, border, theme::radius_sm, button.active ? theme::border_active
                                                                  : theme::border_thin);
    if (button.active) {
        draw->AddRectFilled({rect.left + 1.f, rect.top + 7.f},
                            {rect.left + 3.f, rect.top + rect.height - 7.f},
                            imgui_ui::color(theme::gold), 1.f);
    }
    const std::string number = number2(index + 1);
    imgui_ui::text(draw, number, theme::text_micro, {rect.left + 8.f, rect.top + 8.f}, button.enabled ? theme::text_dim : theme::text_dim);
    imgui_ui::text(draw, button.label, theme::text_label, {rect.left + 28.f, rect.top + 7.f}, button.enabled ? (button.active ? theme::gold : theme::text)
                                  : theme::text_dim);
    const int best = progress_.stars_for(repo_.entries()[index].id);
    imgui_ui::stars(draw, rect.left + rect.width - 32.f, rect.top + rect.height * 0.5f,
                    best, 3, 3.5f, 9.f, theme::gold, theme::border);
}

void App::draw_menu() {
    if (!imgui_ok_) return;
    ui_draw_ = ImGui::GetBackgroundDrawList();
    ImDrawList* draw = ui_draw_;
    draw->AddRectFilled({0.f, 0.f}, {kW, kH}, imgui_ui::color(theme::bg));
    draw->AddRectFilled({520.f, 0.f}, {kW, kH}, imgui_ui::color(theme::bg_elevated));
    imgui_ui::vline(draw, 520.f, 0.f, kH, theme::border, 1.f);

    // 极轻的氛围线：一条流动的金色细线，不叠粒子，不与文字抢层级。
    const float wave = std::sin(time_ * 0.45f) * 18.f;
    draw->AddBezierCubic({278.f, 120.f}, {352.f, 250.f + wave}, {232.f, 440.f - wave},
                         {300.f, 650.f}, imgui_ui::color(sf::Color(230, 184, 92, 44)), 1.5f);
    for (int i = 0; i < 5; ++i) {
        const float t = 0.16f + i * 0.17f;
        const float u = 1.f - t;
        const float x = u * u * u * 278.f + 3.f * u * u * t * 352.f +
                        3.f * u * t * t * 232.f + t * t * t * 300.f;
        const float y = u * u * u * 120.f + 3.f * u * u * t * (250.f + wave) +
                        3.f * u * t * t * (440.f - wave) + t * t * t * 650.f;
        imgui_ui::dot(draw, {x, y}, 2.5f, sf::Color(230, 184, 92, 105));
    }

    imgui_ui::text(draw, "灯河", theme::text_display, {72.f, 92.f}, theme::gold);
    imgui_ui::text(draw, "RIVER OF LAMPS", theme::text_body_lg, {78.f, 184.f}, theme::text_muted);
    imgui_ui::hline(draw, 78.f, 246.f, 212.f, theme::gold, 2.f);
    imgui_ui::text(draw, "即时逻辑布线解谜", theme::text_h3_lg, {78.f, 232.f}, theme::text);
    imgui_ui::text(draw, "拖动灯槽，让信号按拍抵达目标。", theme::text_body, {78.f, 282.f}, theme::text_muted);
    imgui_ui::text(draw, "每格延迟一拍，门零延迟。", theme::text_body, {78.f, 304.f}, theme::text_muted);

    for (const Button& button : menu_buttons_) draw_button(button);

    imgui_ui::text(draw, "选择关卡", theme::text_h2, {560.f, 96.f}, theme::text);
    imgui_ui::text(draw, "LEVELS  /  01—05", theme::text_caption, {560.f, 128.f}, theme::text_dim);
    imgui_ui::hline(draw, 560.f, 1208.f, 146.f, theme::border, 1.f);
    for (int i = 0; i < static_cast<int>(level_cards_.size()); ++i) {
        draw_level_card(level_cards_[i], i);
    }

    const int unlocked = std::min(static_cast<int>(level_cards_.size()), progress_.unlocked);
    int total_stars = 0;
    for (const auto& entry : repo_.entries()) total_stars += progress_.stars_for(entry.id);
    imgui_ui::text(draw, "已通过  " + std::to_string(progress_.completed.size()) +
                            " / " + std::to_string(level_cards_.size()), theme::text_caption, {560.f, 552.f}, theme::text_dim);
    imgui_ui::text_right(draw, "已解锁 " + std::to_string(unlocked) + "  ·  星级 " +
                                   std::to_string(total_stars) + " / " +
                                   std::to_string(level_cards_.size() * 3), theme::text_caption, 1208.f, 552.f, theme::text_dim);

    if (!menu_message_.empty()) {
        imgui_ui::wrapped_text(draw, menu_message_, theme::text_label, {72.f, 616.f}, 420.f, 34.f, theme::text_dim);
    }
    imgui_ui::text(draw, "v1.0.1  ·  WINDOWS x64", theme::text_caption, {72.f, 674.f}, theme::text_dim);
    imgui_ui::text_right(draw, "C++20 / SFML 2.6.1", theme::text_caption, 1208.f, 674.f, theme::text_dim);
    if (settings_open_) {
        draw_settings_panel();
    }
    draw_fade_overlay(draw, fade_);
}

void App::draw_settings_panel() {
    if (!imgui_ok_) return;
    ImDrawList* draw = ui_draw_ != nullptr ? ui_draw_ : ImGui::GetBackgroundDrawList();
    draw->AddRectFilled({0.f, 0.f}, {kW, kH}, imgui_ui::color(theme::overlay));
    imgui_ui::panel(draw, {340.f, 94.f}, {940.f, 564.f}, theme::surface,
                    theme::border_strong, theme::radius_lg, 1.f);
    draw->AddRectFilled({340.f, 94.f}, {343.f, 564.f}, imgui_ui::color(theme::gold), 2.f);

    imgui_ui::text(draw, "显示与启动", theme::text_h3, {368.f, 118.f}, theme::gold);
    imgui_ui::text(draw, "DISPLAY / STARTUP", theme::text_caption, {370.f, 146.f},
                   theme::text_dim);
    imgui_ui::hline(draw, 368.f, 912.f, 174.f, theme::border, 1.f);

    imgui_ui::text(draw, "显示模式", theme::text_label, {368.f, 186.f}, theme::text_muted);
    for (int i = 0; i < 2; ++i) draw_button(settings_buttons_[i]);

    imgui_ui::text(draw, "窗口分辨率", theme::text_label, {368.f, 264.f}, theme::text_muted);
    for (int i = 2; i < 6; ++i) draw_button(settings_buttons_[i]);
    if (settings_.display_mode != DisplayMode::Windowed) {
        const sf::VideoMode desktop = sf::VideoMode::getDesktopMode();
        imgui_ui::text(draw, "按桌面分辨率：" + std::to_string(desktop.width) + " × " +
                                std::to_string(desktop.height),
                       theme::text_caption, {368.f, 324.f}, theme::text_dim);
    }

    draw_button(settings_buttons_[6]);
    imgui_ui::text(draw, "全屏使用无边框窗口，不切换桌面显示模式。", theme::text_caption,
                   {368.f, 392.f}, theme::text_dim);
    imgui_ui::text(draw, "显示模式与分辨率在下一次启动时生效。", theme::text_caption,
                   {368.f, 410.f}, theme::text_dim);
    std::string current_resolution;
    if (settings_.display_mode == DisplayMode::Windowed) {
        current_resolution = std::to_string(settings_.window_width) + " × " +
                             std::to_string(settings_.window_height);
    } else {
        const sf::VideoMode desktop = sf::VideoMode::getDesktopMode();
        current_resolution = std::to_string(desktop.width) + " × " +
                             std::to_string(desktop.height);
    }
    imgui_ui::text(draw, "当前：" + std::string(display_mode_name(settings_.display_mode)) +
                            "  ·  " + current_resolution,
                   theme::text_body, {368.f, 438.f}, theme::text_muted);
    if (restart_required_) {
        imgui_ui::text(draw, "设置已保存，重启后应用。", theme::text_label,
                       {368.f, 460.f}, theme::gold);
    }
    draw_button(settings_buttons_[7]);
    draw_button(settings_buttons_[8]);
}

void App::draw_splash() {
    if (!imgui_ok_) return;
    ui_draw_ = ImGui::GetForegroundDrawList();
    ImDrawList* draw = ui_draw_;
    draw->AddRectFilled({0.f, 0.f}, {kW, kH}, imgui_ui::color(theme::bg));
    const float t = std::clamp(splash_time_ / 2.2f, 0.f, 1.f);
    const float in = theme::ease_out(std::min(1.f, splash_time_ / 0.55f));
    const int alpha = static_cast<int>(255.f * in);
    const float rise = (1.f - in) * 14.f;

    draw->AddRectFilled({80.f, 80.f}, {1200.f, 82.f},
                        imgui_ui::color(sf::Color(theme::border.r, theme::border.g,
                                                  theme::border.b, alpha / 3)));
    imgui_ui::text(draw, "RIVER OF LAMPS", theme::text_label, {80.f, 96.f}, sf::Color(theme::text_dim.r, theme::text_dim.g, theme::text_dim.b,
                             static_cast<std::uint8_t>(alpha)));
    imgui_ui::text(draw, "灯河", theme::text_display, {78.f, 210.f + rise},
                   sf::Color(theme::gold.r, theme::gold.g, theme::gold.b,
                             static_cast<std::uint8_t>(alpha)));
    imgui_ui::hline(draw, 80.f, 80.f + 360.f * theme::ease_out(t), 360.f,
                    sf::Color(theme::gold.r, theme::gold.g, theme::gold.b,
                              static_cast<std::uint8_t>(alpha)), 2.f);
    imgui_ui::text(draw, "即时逻辑布线解谜", theme::text_h3, {82.f, 380.f}, sf::Color(theme::text.r, theme::text.g, theme::text.b,
                             static_cast<std::uint8_t>(alpha)));
    imgui_ui::text(draw, "灯槽 · 门 · 拍", theme::text_label, {82.f, 414.f}, sf::Color(theme::text_dim.r, theme::text_dim.g, theme::text_dim.b,
                             static_cast<std::uint8_t>(alpha)));

    const sf::Color line_color(theme::border.r, theme::border.g, theme::border.b,
                               static_cast<std::uint8_t>(alpha / 2));
    for (int i = 0; i < 8; ++i) {
        const float x = 760.f + i * 48.f;
        const float h = 18.f + 26.f * (0.5f + 0.5f * std::sin(t * 6.f + i));
        imgui_ui::line(draw, {x, 560.f - h}, {x, 560.f}, line_color, 2.f);
    }
    imgui_ui::progress(draw, {80.f, 650.f}, {360.f, 653.f}, t,
                       theme::surface_hover, theme::gold);
    imgui_ui::text_right(draw, "点击或任意键跳过", theme::text_caption, 1200.f, 648.f, theme::text_dim);
}

void App::draw_game() {
    draw_background();
    board_.draw(window_, font_);
    if (!imgui_ok_) return;
    ui_draw_ = ImGui::GetForegroundDrawList();
    ImDrawList* draw = ui_draw_;

    draw->AddRectFilled({0.f, 0.f}, {kW, 64.f}, imgui_ui::color(theme::surface));
    imgui_ui::hline(draw, 0.f, kW, 64.f, theme::border, 1.f);
    draw_button(back_button_);
    imgui_ui::text(draw, "RIVER OF LAMPS", theme::text_micro, {112.f, 11.f}, theme::text_dim);
    imgui_ui::text(draw, "第 " + number2(current_index_ + 1) + " 关  ·  " + level_name(), theme::text_title_lg, {112.f, 27.f}, theme::text);

    const int tick = sim_.tick() >= 0 ? sim_.tick() : 0;
    imgui_ui::text(draw, "拍  " + std::to_string(tick) + " / " +
                            std::to_string(level_.tick_budget), theme::text_caption, {430.f, 12.f}, theme::text_muted);
    imgui_ui::progress(draw, {430.f, 37.f}, {730.f, 41.f},
                       static_cast<float>(tick + 1) / std::max(1, level_.tick_budget),
                       theme::surface_hover, theme::gold);
    imgui_ui::text(draw, "TICK", theme::text_nano, {430.f, 48.f}, theme::text_dim);
    imgui_ui::text(draw, "元件", theme::text_micro, {790.f, 12.f}, theme::text_dim);
    imgui_ui::text(draw, std::to_string(graph_.count_player_functional()) + " / " +
                            std::to_string(level_.max_components), theme::text_small, {790.f, 25.f}, theme::text);
    imgui_ui::text_right(draw, "v1.0.1", theme::text_micro, 1248.f, 28.f, theme::text_dim);

    const std::string teach = teach_text();
    if (!teach.empty()) {
        imgui_ui::panel(draw, {244.f, 66.f}, {888.f, 102.f}, theme::surface_alt, theme::border,
                        theme::radius_sm, 1.f);
        draw->AddRectFilled({244.f, 66.f}, {247.f, 102.f}, imgui_ui::color(theme::gold), 1.f);
        imgui_ui::wrapped_text(draw, teach, theme::text_caption, {256.f, 71.f}, 620.f, 28.f, theme::text_muted);
    }

    draw_panel({16.f, 72.f, 206.f, 632.f});
    draw_panel({906.f, 72.f, 358.f, 632.f});

    imgui_ui::text(draw, "关卡", theme::text_label, {28.f, 84.f}, theme::text_dim);
    for (int i = 0; i < static_cast<int>(game_level_buttons_.size()); ++i) {
        draw_game_level_button(game_level_buttons_[i], i);
    }
    imgui_ui::hline(draw, 28.f, 206.f, 278.f, theme::border, 1.f);
    imgui_ui::text(draw, "工具", theme::text_label, {28.f, 292.f}, theme::text_dim);
    for (const Button& button : palette_buttons_) draw_button(button);
    imgui_ui::text(draw, "提示", theme::text_caption, {28.f, 360.f}, theme::text_dim);
    if (hint_visible_) {
        imgui_ui::wrapped_text(draw, hint_text(), theme::text_caption, {28.f, 378.f}, 172.f, 160.f, theme::text_muted);
    } else {
        imgui_ui::text(draw, "按 H 查看提示", theme::text_caption, {28.f, 378.f}, theme::text_dim);
    }
    imgui_ui::hline(draw, 28.f, 206.f, 556.f, theme::border, 1.f);
    imgui_ui::text(draw, "操作", theme::text_micro, {28.f, 570.f}, theme::text_dim);
    imgui_ui::text(draw, "空格   运行 / 暂停", theme::text_caption, {28.f, 590.f}, theme::text_muted);
    imgui_ui::text(draw, "H       提示", theme::text_caption, {28.f, 610.f}, theme::text_muted);
    imgui_ui::text(draw, "Ctrl+Z  撤销", theme::text_caption, {28.f, 630.f}, theme::text_muted);
    imgui_ui::text(draw, "Esc     返回 / 暂停", theme::text_caption, {28.f, 650.f}, theme::text_muted);
    imgui_ui::text(draw, "灯槽不占元件", theme::text_micro, {28.f, 682.f}, theme::text_dim);

    draw_button(run_button_);
    draw_button(retry_button_);
    draw_button(hint_button_);
    draw_button(speed_button_);
    draw_button(next_button_);

    imgui_ui::text(draw, "目标", theme::text_title, {920.f, 84.f}, theme::text);
    imgui_ui::text_right(draw, "TARGET", theme::text_micro, 1248.f, 90.f, theme::text_dim);
    const std::vector<int> expect = expect_sequence();
    imgui_ui::text(draw, "第 " + std::to_string(target_start_tick()) + " 拍起  ·  " +
                            std::to_string(expect.size()) + " 拍", theme::text_caption, {920.f, 110.f}, theme::text_muted);
    for (int i = 0; i < static_cast<int>(expect.size()) && i < 6; ++i) {
        const sf::Color fill = expect[i] == 1 ? theme::gold : theme::surface_alt;
        const sf::Color text_color = expect[i] == 1 ? theme::bg : theme::text_muted;
        imgui_ui::chip(draw, {920.f + i * 34.f, 132.f}, {946.f + i * 34.f, 154.f}, fill, theme::border, expect[i] == 1 ? "亮" : "灭", theme::text_caption, text_color);
    }
    imgui_ui::hline(draw, 920.f, 1248.f, 174.f, theme::border, 1.f);

    draw_waveform_panel(draw, expect, target_start_tick(), level_.tick_budget,
                        sim_.target_actual(target_uid_), sim_.first_error(),
                        sim_.glitch_history(), {920.f, 186.f}, 328.f);

    if (level_.stars.enabled()) {
        imgui_ui::text(draw, "星级窗口", theme::text_micro, {920.f, 292.f}, theme::text_dim);
        const int thresholds[3] = {level_.stars.three, level_.stars.two, level_.stars.one};
        const int filled[3] = {3, 2, 1};
        const sf::Color colors[3] = {theme::gold, theme::teal, theme::text_muted};
        for (int i = 0; i < 3; ++i) {
            const float y = 312.f + i * 22.f;
            imgui_ui::stars(draw, 936.f, y, filled[i], 3, 3.5f, 9.f, colors[i],
                            theme::border);
            imgui_ui::text(draw, std::to_string(3 - i) + " 星", theme::text_caption, {966.f, y - 6.f}, colors[i]);
            imgui_ui::text_right(draw, "完成 ≤ " + std::to_string(thresholds[i]) + " 拍", theme::text_caption, 1248.f, y - 6.f, theme::text_muted);
        }
    }
    imgui_ui::hline(draw, 920.f, 1248.f, 380.f, theme::border, 1.f);
    imgui_ui::text(draw, "反馈", theme::text_label, {920.f, 394.f}, theme::text_dim);
    imgui_ui::wrapped_text(draw, message_, theme::text_label, {920.f, 416.f}, 328.f, 190.f, theme::text);
    imgui_ui::hline(draw, 920.f, 1248.f, 626.f, theme::border, 1.f);
    const int done = static_cast<int>(progress_.completed.size());
    const int total = static_cast<int>(repo_.entries().size());
    imgui_ui::text(draw, "已通过 " + std::to_string(done) + " / " + std::to_string(total), theme::text_caption, {920.f, 642.f}, theme::text_dim);
    const int best = progress_.stars_for(level_.id);
    imgui_ui::text(draw, "本关最佳", theme::text_micro, {920.f, 668.f}, theme::text_dim);
    imgui_ui::stars(draw, 1002.f, 672.f, best, 3, 4.f, 11.f, theme::gold, theme::border);

    draw_fade_overlay(draw, fade_);
}

void App::draw_overlay() {
    if (!imgui_ok_) return;
    ui_draw_ = ImGui::GetForegroundDrawList();
    ImDrawList* draw = ui_draw_;
    draw->AddRectFilled({0.f, 0.f}, {kW, kH}, imgui_ui::color(theme::overlay));
    const bool passed = phase_ == Phase::Passed;
    const float anim = theme::ease_out(overlay_anim_);
    const ImVec2 min{340.f, 160.f};
    const ImVec2 max{940.f, 550.f};
    imgui_ui::panel(draw, min, max, theme::surface, theme::border_strong,
                    theme::radius_lg, 1.f);
    draw->AddRectFilled({min.x, min.y}, {max.x, min.y + 3.f},
                        imgui_ui::color(passed ? theme::gold : theme::danger), 2.f);
    imgui_ui::text(draw, "第 " + number2(current_index_ + 1) + " 关  ·  " + level_name(), theme::text_micro, {min.x + 22.f, min.y + 16.f}, theme::text_dim);
    imgui_ui::text_right(draw, passed ? "CLEAR" : "RETRY", theme::text_micro, max.x - 22.f, min.y + 16.f, passed ? theme::gold : theme::danger);
    imgui_ui::text_centered(draw, overlay_title_, theme::text_h1, {(min.x + max.x) * 0.5f, min.y + 50.f}, passed ? theme::gold : theme::danger);
    if (passed) {
        const float radius = 14.f + 7.f * anim;
        imgui_ui::stars(draw, (min.x + max.x) * 0.5f, min.y + 100.f, sim_.stars(), 3,
                        radius, 48.f, theme::gold, theme::border_strong);
        imgui_ui::wrapped_text(draw, overlay_message_, theme::text_body_lg, {min.x + 52.f, min.y + 138.f}, 496.f, 110.f, theme::text);
    } else {
        imgui_ui::hline(draw, min.x + 52.f, max.x - 52.f, min.y + 86.f, theme::border, 1.f);
        imgui_ui::wrapped_text(draw, overlay_message_, theme::text_subhead, {min.x + 52.f, min.y + 112.f}, 496.f, 150.f, theme::text);
    }

    const std::string stats_labels[3] = {passed ? "完成拍" : "首错拍",
                                         passed ? "评级" : "1 星窗口",
                                         passed ? "参考解" : "状态"};
    std::string stats_values[3];
    if (passed) {
        stats_values[0] = std::to_string(sim_.completion_tick());
        stats_values[1] = std::to_string(sim_.stars()) + " 星";
        stats_values[2] = std::to_string(level_.reference_finish) + " 拍";
    } else {
        stats_values[0] = sim_.first_error().has_value()
                              ? std::to_string(sim_.first_error()->tick)
                              : "--";
        stats_values[1] = "≤ " + std::to_string(level_.stars.one);
        stats_values[2] = "未完成";
    }
    for (int i = 0; i < 3; ++i) {
        const float x = min.x + 52.f + i * 168.f;
        imgui_ui::text(draw, stats_labels[i], theme::text_caption, {x, min.y + 202.f}, theme::text_dim);
        imgui_ui::text(draw, stats_values[i], theme::text_title, {x, min.y + 218.f}, theme::text);
    }
    imgui_ui::hline(draw, min.x + 32.f, max.x - 32.f, 430.f, theme::border, 1.f);
    for (const Button& button : overlay_buttons_) draw_button(button);
}


void App::handle_event(const sf::Event& event) {
    if (event.type == sf::Event::Closed) {
        window_.close();
        return;
    }
    if (event.type == sf::Event::Resized) {
        handle_window_resize(event.size.width, event.size.height);
        return;
    }
    if (event.type == sf::Event::GainedFocus || event.type == sf::Event::LostFocus) {
        update_borderless_window_state();
        return;
    }
    if (splash_active_ &&
        (event.type == sf::Event::KeyPressed ||
         event.type == sf::Event::MouseButtonPressed)) {
        splash_active_ = false;
        splash_time_ = 2.2f;
        fade_ = 0.f;
        return;
    }
    if (event.type == sf::Event::KeyPressed) {
        handle_key(event.key);
        return;
    }
    if (event.type == sf::Event::MouseMoved) {
        handle_mouse_move(to_ui({static_cast<float>(event.mouseMove.x),
                                 static_cast<float>(event.mouseMove.y)}));
        return;
    }
    if (event.type == sf::Event::MouseButtonPressed) {
        const sf::Vector2f point = to_ui({static_cast<float>(event.mouseButton.x),
                                          static_cast<float>(event.mouseButton.y)});
        if (event.mouseButton.button == sf::Mouse::Left) {
            handle_mouse_click(point);
        } else if (event.mouseButton.button == sf::Mouse::Right && screen_ == Screen::Game &&
                   !overlay_visible_ && !settings_open_) {
            board_.handle_mouse_press(point, sf::Mouse::Right);
        }
        return;
    }
    if (event.type == sf::Event::MouseButtonReleased) {
        handle_mouse_release();
        return;
    }
}

void App::handle_key(const sf::Event::KeyEvent& key) {
    if (settings_open_) {
        if (key.code == sf::Keyboard::Escape) {
            settings_open_ = false;
        }
        return;
    }
    if (key.code == sf::Keyboard::M) {
        sound_.toggle_mute();
        refresh_button_labels();
        return;
    }
    if (screen_ == Screen::Menu) {
        if (key.code == sf::Keyboard::Enter) {
            sound_.play_click();
            start_game();
        } else if (key.code == sf::Keyboard::Escape) {
            window_.close();
        }
        return;
    }

    if (overlay_visible_) {
        if (key.code == sf::Keyboard::Enter) {
            activate_overlay(0);
        } else if (key.code == sf::Keyboard::Escape) {
            if (phase_ == Phase::Failed || phase_ == Phase::Passed) {
                overlay_visible_ = false;
                set_phase(Phase::Editing);
                board_.set_editable(true);
                refresh_button_labels();
            }
        }
        return;
    }

    if (key.code == sf::Keyboard::Escape) {
        if (phase_ == Phase::Running) {
            set_phase(Phase::Paused);
            message_ = "已暂停。按空格继续。";
        } else {
            back_to_menu();
        }
    } else if (key.code == sf::Keyboard::Space) {
        toggle_run();
    } else if (key.code == sf::Keyboard::Backspace) {
        retry();
    } else if (key.code == sf::Keyboard::H) {
        hint_visible_ = !hint_visible_;
        refresh_button_labels();
    } else if (key.code == sf::Keyboard::Z && key.control) {
        undo();
    }
}

void App::handle_mouse_move(const sf::Vector2f& point) {
    if (screen_ == Screen::Menu) {
        if (settings_open_) {
            for (Button& button : settings_buttons_) {
                button.hovered = button.enabled && button.rect.contains(point);
            }
            bool hand = false;
            for (const Button& button : settings_buttons_) hand = hand || button.hovered;
            update_cursor(hand);
            return;
        }
        for (Button& button : menu_buttons_) {
            button.hovered = button.enabled && button.rect.contains(point);
        }
        for (Button& card : level_cards_) {
            card.hovered = card.enabled && card.rect.contains(point);
        }
        bool hand = false;
        for (const Button& button : menu_buttons_) hand = hand || button.hovered;
        for (const Button& card : level_cards_) hand = hand || card.hovered;
        update_cursor(hand);
        return;
    }

    if (overlay_visible_) {
        for (Button& button : overlay_buttons_) {
            button.hovered = button.enabled && button.rect.contains(point);
        }
        bool hand = false;
        for (const Button& button : overlay_buttons_) hand = hand || button.hovered;
        update_cursor(hand);
        return;
    }

    auto hover_all = [&point](std::vector<Button>& buttons) {
        for (Button& button : buttons) {
            button.hovered = button.enabled && button.rect.contains(point);
        }
    };
    hover_all(game_level_buttons_);
    hover_all(palette_buttons_);
    back_button_.hovered = back_button_.rect.contains(point);
    run_button_.hovered = run_button_.rect.contains(point);
    retry_button_.hovered = retry_button_.rect.contains(point);
    hint_button_.hovered = hint_button_.rect.contains(point);
    speed_button_.hovered = speed_button_.rect.contains(point);
    next_button_.hovered = next_button_.enabled && next_button_.rect.contains(point);

    board_.handle_mouse_move(point, sf::Mouse::isButtonPressed(sf::Mouse::Left));

    bool hand = back_button_.hovered || run_button_.hovered || retry_button_.hovered ||
                hint_button_.hovered || speed_button_.hovered || next_button_.hovered;
    for (const Button& button : game_level_buttons_) hand = hand || button.hovered;
    for (const Button& button : palette_buttons_) hand = hand || button.hovered;
    if (board_.is_editable() && board_.board_rect().contains(point)) hand = true;
    update_cursor(hand);
}

void App::handle_mouse_click(const sf::Vector2f& point) {
    if (screen_ == Screen::Menu) {
        if (settings_open_) {
            for (int i = 0; i < static_cast<int>(settings_buttons_.size()); ++i) {
                if (!settings_buttons_[i].hit(point)) {
                    continue;
                }
                sound_.play_click();
                if (i < 2) {
                    const DisplayMode modes[2] = {DisplayMode::Windowed,
                                                  DisplayMode::Borderless};
                    settings_.display_mode = modes[i];
                    restart_required_ = true;
                    save_settings();
                } else if (i < 6) {
                    const int sizes[4][2] = {{1280, 720}, {1600, 900},
                                             {1920, 1080}, {2560, 1440}};
                    const int idx = i - 2;
                    settings_.window_width = sizes[idx][0];
                    settings_.window_height = sizes[idx][1];
                    restart_required_ = true;
                    save_settings();
                } else if (i == 6) {
                    settings_.splash_enabled = !settings_.splash_enabled;
                    save_settings();
                } else if (i == 7) {
                    settings_ = AppSettings{};
                    restart_required_ = true;
                    save_settings();
                } else {
                    settings_open_ = false;
                }
                rebuild_settings_buttons();
                return;
            }
            return;
        }
        for (int i = 0; i < static_cast<int>(menu_buttons_.size()); ++i) {
            if (!menu_buttons_[i].hit(point)) {
                continue;
            }
            if (reset_confirm_ && i != 1) {
                reset_confirm_ = false;
                refresh_button_labels();
            }
            sound_.play_click();
            if (i == 0) {
                start_game();
            } else if (i == 1) {
                reset_progress();
            } else if (i == 2) {
                sound_.toggle_mute();
                refresh_button_labels();
            } else if (i == 3) {
                settings_open_ = true;
            } else if (i == 4) {
                window_.close();
            }
            return;
        }
        for (int i = 0; i < static_cast<int>(level_cards_.size()); ++i) {
            if (!level_cards_[i].hit(point)) {
                continue;
            }
            if (i < progress_.unlocked) {
                sound_.play_click();
                load_level(i);
            } else {
                menu_message_ = "先通过前面的关卡，才能进入这一关。";
            }
            return;
        }
        return;
    }

    if (overlay_visible_) {
        for (int i = 0; i < static_cast<int>(overlay_buttons_.size()); ++i) {
            if (overlay_buttons_[i].hit(point)) {
                activate_overlay(i);
                return;
            }
        }
        return;
    }

    if (back_button_.hit(point)) {
        sound_.play_click();
        back_to_menu();
        return;
    }
    for (int i = 0; i < static_cast<int>(game_level_buttons_.size()); ++i) {
        if (game_level_buttons_[i].hit(point)) {
            if (i < progress_.unlocked) {
                sound_.play_click();
                load_level(i);
            } else {
                message_ = "先通过前面的关卡，才能进入这一关。";
            }
            return;
        }
    }
    for (int i = 0; i < static_cast<int>(palette_buttons_.size()); ++i) {
        if (palette_buttons_[i].hit(point)) {
            sound_.play_click();
            const int pal_index = i;
            int type_index = 0;
            nlohmann::json available = level_.raw.value("available", nlohmann::json::array({"wire"}));
            for (const auto& item : available) {
                if (!item.is_string()) continue;
                if (type_index == pal_index) {
                    selected_tool_ = item.get<std::string>();
                    break;
                }
                ++type_index;
            }
            board_.set_tool(selected_tool_);
            for (int j = 0; j < static_cast<int>(palette_buttons_.size()); ++j) {
                palette_buttons_[j].active = j == i;
            }
            message_ = "当前工具：" + ComponentRules::display_name(selected_tool_) +
                       "。按住左键在网格上放置/拖拽。";
            return;
        }
    }

    if (run_button_.hit(point)) {
        sound_.play_click();
        toggle_run();
        return;
    }
    if (retry_button_.hit(point)) {
        sound_.play_click();
        retry();
        return;
    }
    if (hint_button_.hit(point)) {
        sound_.play_click();
        hint_visible_ = !hint_visible_;
        refresh_button_labels();
        return;
    }
    if (speed_button_.hit(point)) {
        sound_.play_click();
        if (speed_ == 1.f) speed_ = 2.f;
        else if (speed_ == 2.f) speed_ = 0.5f;
        else speed_ = 1.f;
        refresh_button_labels();
        message_ = "播放速度：" + speed_button_.label;
        return;
    }
    if (next_button_.hit(point)) {
        sound_.play_click();
        next_level();
        return;
    }

    if (board_.board_rect().contains(point)) {
        board_.handle_mouse_press(point, sf::Mouse::Left);
    }
}

void App::handle_mouse_release() {
    if (settings_open_) {
        return;
    }
    board_.handle_mouse_release();
}

void App::activate_overlay(int index) {
    sound_.play_click();
    if (phase_ == Phase::Passed) {
        if (index == 0) {
            overlay_visible_ = false;
            next_level();
        } else if (index == 1) {
            overlay_visible_ = false;
            reset_sim();
            set_phase(Phase::Editing);
            message_ = "重新来一次。";
        } else {
            back_to_menu();
        }
    } else {
        if (index == 0) {
            retry();
        } else if (index == 1) {
            overlay_visible_ = false;
            set_phase(Phase::Editing);
            board_.set_editable(true);
            message_ = "改布局后重新运行。";
        } else {
            back_to_menu();
        }
    }
    refresh_button_labels();
}

void App::update_cursor(bool hand) {
    if (hand == hand_cursor_active_) {
        return;
    }
    hand_cursor_active_ = hand;
    if (hand) {
        window_.setMouseCursor(hand_cursor_);
    } else {
        window_.setMouseCursor(arrow_cursor_);
    }
}

std::vector<int> App::expect_sequence() const {
    for (const auto& [uid, inst] : graph_.instances) {
        (void)uid;
        if (inst.type == "target") {
            return inst.expect;
        }
    }
    return {};
}

int App::target_start_tick() const {
    for (const auto& [uid, inst] : graph_.instances) {
        (void)uid;
        if (inst.type == "target") {
            return inst.start_tick;
        }
    }
    return 0;
}

std::string App::target_label() const {
    const std::vector<int> expect = expect_sequence();
    if (expect.empty()) {
        return "无目标";
    }
    std::string parts;
    for (int value : expect) {
        if (!parts.empty()) {
            parts += " / ";
        }
        parts += value == 1 ? "亮" : "灭";
    }
    std::string label = "第 " + std::to_string(target_start_tick()) + " 拍起：" + parts +
                        "（共 " + std::to_string(expect.size()) + " 拍）";
    if (level_.stars.enabled()) {
        label += "\n3星≤" + std::to_string(level_.stars.three) +
                 " · 2星≤" + std::to_string(level_.stars.two) +
                 " · 1星≤" + std::to_string(level_.stars.one);
    }
    return label;
}

std::string App::teach_text() const {
    return level_.teach;
}

std::string App::star_gap_text() const {
    if (!level_.stars.enabled() || sim_.stars() <= 0) {
        return {};
    }
    const int finish = sim_.completion_tick();
    if (sim_.stars() >= 3) {
        return "满星：已经压到参考解附近。";
    }
    if (sim_.stars() == 2) {
        return "再快 " + std::to_string(std::max(1, finish - level_.stars.three)) +
               " 拍可拿 3 星。";
    }
    return "再快 " + std::to_string(std::max(1, finish - level_.stars.two)) +
           " 拍可拿 2 星。";
}

std::string App::hint_text() const {
    if (level_.raw.contains("hint") && level_.raw["hint"].is_object()) {
        const auto& hint = level_.raw["hint"];
        return hint.value("zh", hint.value("en", "本关没有提示。"));
    }
    return "本关没有提示。";
}

std::string App::error_text() const {
    if (sim_.fail_reason() == "timeout") {
        return "超时：" + std::to_string(level_.tick_budget) +
               " 拍内没有完成目标序列。检查灯槽总长度和目标起始拍。";
    }
    if (!sim_.first_error().has_value()) {
        return "失败。";
    }
    const ErrorInfo& error = *sim_.first_error();
    const std::string expected = error.expected == 1 ? "亮" : "灭";
    const std::string got = error.got == 1 ? "亮" : "灭";
    return "第 " + std::to_string(error.tick) + " 拍出错：期望" + expected + "，实际" + got + "。";
}

std::string App::status_text() const {
    const int tick = sim_.tick() >= 0 ? sim_.tick() : 0;
    return "第 " + std::to_string(current_index_ + 1) + " 关 · " + level_name() +
           "    拍 " + std::to_string(tick) + " / " + std::to_string(level_.tick_budget) +
           "    元件 " + std::to_string(graph_.count_player_functional()) + " / " +
           std::to_string(level_.max_components);
}

std::string App::level_name() const {
    return level_.name.empty() ? level_.id : level_.name;
}

}  // namespace river
