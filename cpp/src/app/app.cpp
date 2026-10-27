#include "app/app.h"

#include "app/ui_draw.h"
#include "core/component_rules.h"
#include "core/level_builder.h"
#include "generated/embedded_assets.h"

#include <SFML/System/Sleep.hpp>

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <string>
#include <utility>

namespace river {

namespace {

constexpr float kW = 1280.f;
constexpr float kH = 720.f;
constexpr float kTickInterval = 0.55f;

const sf::Color kText(220, 232, 255);
const sf::Color kTextDim(120, 138, 168);
const sf::Color kGold(255, 199, 90);
const sf::Color kRed(255, 96, 96);
const sf::Color kInPort(140, 191, 255);
const sf::Color kPanel(15, 20, 33);
const sf::Color kPanelBorder(38, 50, 74);
const sf::Color kButton(30, 38, 58);
const sf::Color kButtonHover(44, 56, 82);

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

App::App(std::filesystem::path exe_dir, std::string capture_path, std::string capture_mode)
    : window_(sf::VideoMode(static_cast<unsigned>(kW), static_cast<unsigned>(kH)),
              window_title(),
              sf::Style::Titlebar | sf::Style::Close),
      exe_dir_(std::move(exe_dir)),
      repo_(exe_dir_),
      capture_path_(std::move(capture_path)),
      capture_mode_(std::move(capture_mode)) {
    window_.setVerticalSyncEnabled(true);
    window_.setKeyRepeatEnabled(false);
    build_window_icon();

    font_ok_ = font_.loadFromMemory(embedded::kFontData, embedded::kFontSize);
    if (!font_ok_) {
        const char* fallbacks[] = {
            "C:/Windows/Fonts/NotoSansSC-VF.ttf",
            "C:/Windows/Fonts/simhei.ttf",
            "C:/Windows/Fonts/msyh.ttc",
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
                        ? "找不到关卡数据：请把 game/data/levels 放在 exe 旁边。"
                        : "选择关卡，或直接开始游戏。";

    if (arrow_cursor_.loadFromSystem(sf::Cursor::Arrow)) {
        window_.setMouseCursor(arrow_cursor_);
    }
    hand_cursor_.loadFromSystem(sf::Cursor::Hand);
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
            handle_event(event);
        }
        update(clock.restart().asSeconds());
        window_.clear(sf::Color(6, 9, 15));
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

    capture_window(capture_path_);
}

bool App::capture_window(const std::string& path) {
    window_.setActive(true);
    for (int i = 0; i < 4; ++i) {
        window_.clear(sf::Color(6, 9, 15));
        draw();
        window_.display();
        sf::sleep(sf::milliseconds(40));
    }
    window_.clear(sf::Color(6, 9, 15));
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
    menu_buttons_.push_back(Button{{80.f, 350.f, 210.f, 44.f}, "开始游戏", true, true});
    menu_buttons_.push_back(Button{{80.f, 406.f, 210.f, 40.f}, "重置进度"});
    menu_buttons_.push_back(Button{{80.f, 456.f, 210.f, 40.f}, "音效：开"});
    menu_buttons_.push_back(Button{{80.f, 506.f, 210.f, 40.f}, "退出"});

    back_button_ = Button{{16.f, 14.f, 82.f, 32.f}, "返回", true, false, false, false, false, false, 14};
    run_button_ = Button{{302.f, 600.f, 88.f, 34.f}, "运行", true, true};
    retry_button_ = Button{{398.f, 600.f, 88.f, 34.f}, "重试"};
    hint_button_ = Button{{494.f, 600.f, 88.f, 34.f}, "提示"};
    speed_button_ = Button{{590.f, 600.f, 88.f, 34.f}, "1.0x"};
    next_button_ = Button{{686.f, 600.f, 88.f, 34.f}, "下一关", false};

    board_.set_origin({342.f, 116.f});
    board_.set_tool("wire");
    board_.on_edit_started = [this]() { on_edit_started(); };
    board_.on_layout_changed = [this]() { on_layout_changed(); };

    overlay_buttons_.clear();
}

void App::save_progress() {
    ProgressStore::save(progress_);
}

void App::refresh_level_cards() {
    level_cards_.clear();
    const auto& entries = repo_.entries();
    for (int i = 0; i < static_cast<int>(entries.size()) && i < 5; ++i) {
        Button card;
        card.rect = {60.f + i * 232.f, 588.f, 216.f, 100.f};
        card.label = entries[i].name;
        card.enabled = i < progress_.unlocked;
        card.font_size = 18;
        card.hovered = false;
        level_cards_.push_back(card);
    }
}

void App::rebuild_game_level_buttons() {
    game_level_buttons_.clear();
    const auto& entries = repo_.entries();
    for (int i = 0; i < static_cast<int>(entries.size()) && i < 5; ++i) {
        Button button;
        button.rect = {28.f, 96.f + i * 32.f, 190.f, 28.f};
        button.label = entries[i].name;
        button.enabled = i < progress_.unlocked;
        button.active = i == current_index_;
        button.font_size = 13;
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
        button.rect = {28.f, 288.f + i * 34.f, 190.f, 30.f};
        button.label = ComponentRules::display_name(type);
        button.toggle = true;
        button.active = type == selected_tool_;
        button.font_size = 13;
        palette_buttons_.push_back(button);
        ++i;
    }
}

void App::rebuild_overlay_buttons() {
    overlay_buttons_.clear();
    const bool passed = phase_ == Phase::Passed;
    Button primary;
    primary.rect = {390.f, 440.f, 150.f, 44.f};
    primary.primary = true;
    Button secondary;
    secondary.rect = {565.f, 440.f, 150.f, 44.f};
    Button third;
    third.rect = {740.f, 440.f, 150.f, 44.f};

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
    if (menu_buttons_.size() >= 4) {
        menu_buttons_[0].label = "开始游戏";
        menu_buttons_[1].label = reset_confirm_ ? "再点一次确认" : "重置进度";
        menu_buttons_[2].label = sound_.muted() ? "音效：关" : "音效：开";
        menu_buttons_[3].label = "退出";
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
    time_ += dt;
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
    if (screen_ == Screen::Menu) {
        draw_menu();
    } else {
        draw_game();
        if (overlay_visible_) {
            draw_overlay();
        }
    }
}

void App::draw_background() {
    sf::VertexArray gradient(sf::Quads, 4);
    const sf::Color top(6, 9, 16);
    const sf::Color bottom(13, 20, 35);
    gradient[0] = sf::Vertex({0.f, 0.f}, top);
    gradient[1] = sf::Vertex({kW, 0.f}, top);
    gradient[2] = sf::Vertex({kW, kH}, bottom);
    gradient[3] = sf::Vertex({0.f, kH}, bottom);
    window_.draw(gradient);

    for (float x = 0.f; x < kW; x += 64.f) {
        ui::draw_line(window_, {x, 0.f}, {x, kH}, sf::Color(45, 70, 110, 18), 1.f);
    }
    for (float y = 0.f; y < kH; y += 64.f) {
        ui::draw_line(window_, {0.f, y}, {kW, y}, sf::Color(45, 70, 110, 18), 1.f);
    }

    if (screen_ == Screen::Menu) {
        draw_lamp_river();
    }
}

void App::draw_lamp_river() {
    for (int i = 0; i < 18; ++i) {
        const float x = 520.f + i * 40.f;
        const float y = 330.f + std::sin(time_ * 0.55f + i * 0.55f) * 56.f;
        const float pulse = 0.65f + 0.35f * std::sin(time_ * 1.8f + i * 0.7f);
        ui::fill_circle(window_, {x, y}, 3.8f * pulse, sf::Color(255, 210, 110, 210));
        if (i > 0) {
            const float px = 520.f + (i - 1) * 40.f;
            const float py = 330.f + std::sin(time_ * 0.55f + (i - 1) * 0.55f) * 56.f;
            ui::draw_line(window_, {px, py}, {x, y}, sf::Color(255, 190, 70, 26), 1.f);
        }
    }
}

void App::draw_button(const Button& button) {
    if (button.rect.width <= 0.f || button.rect.height <= 0.f) {
        return;
    }
    sf::Color fill = kButton;
    sf::Color border = sf::Color(64, 84, 120);
    sf::Color text = kText;
    if (!button.enabled) {
        fill = sf::Color(18, 23, 35);
        border = sf::Color(36, 44, 60);
        text = sf::Color(76, 86, 108);
    } else if (button.primary) {
        fill = button.hovered ? sf::Color(255, 216, 130) : sf::Color(255, 196, 80);
        border = sf::Color(255, 224, 160);
        text = sf::Color(24, 28, 38);
    } else if (button.danger) {
        fill = button.hovered ? sf::Color(92, 38, 44) : sf::Color(64, 30, 36);
        border = sf::Color(190, 75, 78);
        text = sf::Color(255, 190, 190);
    } else {
        if (button.hovered) {
            fill = kButtonHover;
        }
        if (button.active) {
            fill = sf::Color(48, 62, 92);
            border = kGold;
        }
    }
    ui::draw_rounded_rect(window_, button.rect, 8.f, fill, border, 1.5f);
    ui::draw_text_centered(window_, font_, button.label, button.font_size, button.rect, text);
}

void App::draw_panel(const sf::FloatRect& rect) {
    ui::draw_rounded_rect(window_, rect, 10.f, kPanel, kPanelBorder, 1.f);
}

void App::draw_level_card(const Button& button, int index) {
    sf::Color fill = sf::Color(20, 27, 43);
    sf::Color border = sf::Color(48, 63, 90);
    if (button.enabled) {
        fill = sf::Color(23, 31, 49);
        border = button.hovered ? sf::Color(255, 214, 120) : sf::Color(70, 92, 130);
    }
    ui::draw_rounded_rect(window_, button.rect, 12.f, fill, border, button.hovered ? 2.f : 1.5f);

    const std::string id = repo_.entries()[index].id;
    const bool completed = progress_.is_completed(id);
    const std::string status = completed ? "已通过" : (button.enabled ? "可挑战" : "未解锁");
    const sf::Color status_color = completed ? kGold : (button.enabled ? kInPort : kTextDim);

    ui::draw_text(window_, font_, number2(index + 1), 22,
                  {button.rect.left + 16.f, button.rect.top + 10.f},
                  button.enabled ? kGold : kTextDim);
    ui::draw_text(window_, font_, button.label, 19,
                  {button.rect.left + 58.f, button.rect.top + 12.f},
                  button.enabled ? kText : kTextDim);
    ui::draw_text(window_, font_, status, 12,
                  {button.rect.left + 16.f, button.rect.top + 68.f}, status_color);

    const int best = progress_.stars_for(id);
    ui::draw_star_row(window_, button.rect.left + button.rect.width - 38.f,
                      button.rect.top + 78.f, best, 3, 5.5f, 14.f,
                      kGold, sf::Color(76, 92, 118));
}

void App::draw_game_level_button(const Button& button, int index) {
    sf::Color fill = button.active ? sf::Color(44, 58, 88) : sf::Color(25, 33, 51);
    sf::Color border = button.active ? kGold : sf::Color(48, 63, 90);
    if (!button.enabled) {
        fill = sf::Color(18, 23, 35);
        border = sf::Color(36, 44, 60);
    } else if (button.hovered) {
        fill = sf::Color(36, 47, 71);
    }
    ui::draw_rounded_rect(window_, button.rect, 6.f, fill, border, button.active ? 2.f : 1.f);
    const std::string label = std::to_string(index + 1) + "  " + button.label;
    ui::draw_text(window_, font_, label, 13,
                  {button.rect.left + 10.f, button.rect.top + 5.f},
                  button.enabled ? (button.active ? kGold : kText) : kTextDim);
    const int best = progress_.stars_for(repo_.entries()[index].id);
    ui::draw_star_row(window_, button.rect.left + button.rect.width - 36.f,
                      button.rect.top + button.rect.height * 0.5f, best, 3, 4.5f, 11.f,
                      kGold, sf::Color(76, 92, 118));
}

void App::draw_menu() {
    draw_background();

    ui::draw_text(window_, font_, "灯河", 82, {78.f, 104.f}, kGold);
    ui::draw_text(window_, font_, "RIVER OF LAMPS", 24, {84.f, 212.f}, sf::Color(150, 186, 235));
    ui::draw_text(window_, font_, "即时逻辑布线解谜", 16, {84.f, 250.f}, sf::Color(180, 200, 228));
    ui::draw_text(window_, font_, "拖拽灯槽 · 空格运行", 13, {84.f, 300.f}, kTextDim);
    ui::draw_text(window_, font_, "H 提示 · Ctrl+Z 撤销", 13, {84.f, 320.f}, kTextDim);

    for (const Button& button : menu_buttons_) {
        draw_button(button);
    }

    for (int i = 0; i < static_cast<int>(level_cards_.size()); ++i) {
        draw_level_card(level_cards_[i], i);
    }

    if (!menu_message_.empty()) {
        ui::draw_wrapped_text(window_, font_, menu_message_, 12,
                              {60.f, 560.f, 1100.f, 20.f}, kTextDim);
    }
    ui::draw_text(window_, font_, "C++ / SFML 2.6 · 独立 Windows 版", 11,
                  {1030.f, 700.f}, sf::Color(82, 100, 128));
}

void App::draw_game() {
    draw_background();

    draw_button(back_button_);
    ui::draw_text(window_, font_, status_text(), 15, {112.f, 20.f}, kText);

    const std::string teach = teach_text();
    if (!teach.empty()) {
        draw_panel({244.f, 62.f, 644.f, 36.f});
        ui::draw_wrapped_text(window_, font_, teach, 12,
                              {256.f, 69.f, 620.f, 26.f}, sf::Color(214, 226, 248));
    }

    draw_panel({16.f, 60.f, 214.f, 648.f});
    draw_panel({902.f, 60.f, 362.f, 648.f});
    draw_panel({330.f, 104.f, 472.f, 472.f});

    ui::draw_text(window_, font_, "关卡", 15, {28.f, 70.f}, sf::Color(170, 190, 220));
    for (int i = 0; i < static_cast<int>(game_level_buttons_.size()); ++i) {
        draw_game_level_button(game_level_buttons_[i], i);
    }

    ui::draw_text(window_, font_, "元件", 15, {28.f, 262.f}, sf::Color(170, 190, 220));
    for (const Button& button : palette_buttons_) {
        draw_button(button);
    }

    if (hint_visible_) {
        ui::draw_wrapped_text(window_, font_, hint_text(), 12,
                              {28.f, 334.f, 190.f, 330.f}, sf::Color(150, 175, 215));
    } else {
        ui::draw_text(window_, font_, "按 H 查看提示", 12, {28.f, 338.f}, kTextDim);
    }

    board_.draw(window_, font_);

    draw_button(run_button_);
    draw_button(retry_button_);
    draw_button(hint_button_);
    draw_button(speed_button_);
    draw_button(next_button_);

    ui::draw_text(window_, font_, "目标序列", 15, {918.f, 74.f}, sf::Color(170, 190, 220));
    ui::draw_wrapped_text(window_, font_, target_label(), 13,
                          {918.f, 98.f, 330.f, 42.f}, sf::Color(245, 218, 150));
    waveform_.draw(window_, font_, {916.f, 142.f, 336.f, 210.f});

    ui::draw_text(window_, font_, "反馈", 15, {918.f, 378.f}, sf::Color(170, 190, 220));
    ui::draw_wrapped_text(window_, font_, message_, 13,
                          {918.f, 402.f, 330.f, 180.f}, sf::Color(180, 205, 240));

    const int done = static_cast<int>(progress_.completed.size());
    const int total = static_cast<int>(repo_.entries().size());
    ui::draw_text(window_, font_, "已通过 " + std::to_string(done) + " / " + std::to_string(total),
                  12, {918.f, 654.f}, kTextDim);
    const int best = progress_.stars_for(level_.id);
    ui::draw_text(window_, font_, "本关最佳 " + std::to_string(best) + " 星", 11,
                  {918.f, 674.f}, sf::Color(120, 138, 168));
}

void App::draw_overlay() {
    ui::fill_rect(window_, {0.f, 0.f, kW, kH}, sf::Color(2, 4, 8, 185));
    ui::draw_rounded_rect(window_, {360.f, 200.f, 560.f, 320.f}, 16.f,
                          sf::Color(15, 21, 35), sf::Color(70, 92, 130), 1.5f);
    const sf::Color title_color = phase_ == Phase::Passed ? kGold : kRed;
    ui::draw_text_centered(window_, font_, overlay_title_, 34,
                           {360.f, 216.f, 560.f, 50.f}, title_color);

    if (phase_ == Phase::Passed) {
        ui::draw_star_row(window_, 640.f, 282.f, sim_.stars(), 3, 16.f, 42.f,
                          kGold, sf::Color(102, 122, 155));
        ui::draw_wrapped_text(window_, font_, overlay_message_, 15,
                              {392.f, 314.f, 496.f, 96.f}, kText);
    } else {
        ui::draw_wrapped_text(window_, font_, overlay_message_, 16,
                              {392.f, 292.f, 496.f, 120.f}, kText);
    }
    for (const Button& button : overlay_buttons_) {
        draw_button(button);
    }
}

void App::handle_event(const sf::Event& event) {
    if (event.type == sf::Event::Closed) {
        window_.close();
        return;
    }
    if (event.type == sf::Event::KeyPressed) {
        handle_key(event.key);
        return;
    }
    if (event.type == sf::Event::MouseMoved) {
        handle_mouse_move({static_cast<float>(event.mouseMove.x),
                           static_cast<float>(event.mouseMove.y)});
        return;
    }
    if (event.type == sf::Event::MouseButtonPressed) {
        const sf::Vector2f point{static_cast<float>(event.mouseButton.x),
                                 static_cast<float>(event.mouseButton.y)};
        if (event.mouseButton.button == sf::Mouse::Left) {
            handle_mouse_click(point);
        } else if (event.mouseButton.button == sf::Mouse::Right && screen_ == Screen::Game &&
                   !overlay_visible_) {
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
