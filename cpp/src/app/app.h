#pragma once

#include "app/board_view.h"
#include "app/progress.h"
#include "app/sound.h"
#include "app/waveform_view.h"
#include "core/circuit_graph.h"
#include "core/level_data.h"
#include "core/level_repository.h"
#include "core/simulator.h"

#include <SFML/Graphics.hpp>

#include <filesystem>
#include <string>
#include <vector>

namespace river {

class App {
public:
    explicit App(std::filesystem::path exe_dir, std::string capture_path = {},
                 std::string capture_mode = {});
    int run();

private:
    enum class Screen { Menu, Game };
    enum class Phase { Editing, Running, Paused, Failed, Passed };

    struct Button {
        sf::FloatRect rect;
        std::string label;
        bool enabled = true;
        bool primary = false;
        bool danger = false;
        bool toggle = false;
        bool active = false;
        bool hovered = false;
        unsigned font_size = 15;

        bool hit(const sf::Vector2f& point) const {
            return enabled && rect.contains(point);
        }
    };

    void build_ui();
    void run_capture();
    bool capture_window(const std::string& path);
    void apply_reference_for_capture();
    void build_fail_demo();
    void build_menu_buttons();
    void refresh_level_cards();
    void rebuild_game_level_buttons();
    void rebuild_palette();
    void rebuild_overlay_buttons();
    void refresh_button_labels();

    void load_level(int index);
    void reset_sim();
    void set_phase(Phase phase);
    void start_run();
    void step_once();
    void toggle_run();
    void retry();
    void undo();
    void next_level();
    void back_to_menu();
    void start_game();
    void reset_progress();
    void on_edit_started();
    void on_layout_changed();
    void unlock_next();
    void show_result_overlay();
    void activate_overlay(int index);

    void update(float dt);
    void draw();
    void draw_menu();
    void draw_game();
    void draw_overlay();
    void draw_background();
    void draw_lamp_river();
    void draw_button(const Button& button);
    void draw_panel(const sf::FloatRect& rect);
    void draw_level_card(const Button& button, int index);
    void draw_game_level_button(const Button& button, int index);

    void handle_event(const sf::Event& event);
    void handle_key(const sf::Event::KeyEvent& key);
    void handle_mouse_move(const sf::Vector2f& point);
    void handle_mouse_click(const sf::Vector2f& point);
    void handle_mouse_release();
    void update_cursor(bool hand);

    std::vector<int> expect_sequence() const;
    int target_start_tick() const;
    std::string target_label() const;
    std::string teach_text() const;
    std::string star_gap_text() const;
    std::string hint_text() const;
    std::string error_text() const;
    std::string status_text() const;
    std::string level_name() const;

    void save_progress();
    void build_window_icon();

    sf::RenderWindow window_;
    sf::Font font_;
    bool font_ok_ = false;
    sf::Cursor arrow_cursor_;
    sf::Cursor hand_cursor_;
    bool hand_cursor_active_ = false;

    std::filesystem::path exe_dir_;
    LevelRepository repo_;
    Progress progress_;
    SoundPlayer sound_;
    std::string capture_path_;
    std::string capture_mode_;

    Screen screen_ = Screen::Menu;
    Phase phase_ = Phase::Editing;
    float time_ = 0.f;
    float accumulator_ = 0.f;
    float speed_ = 1.f;

    std::vector<Button> menu_buttons_;
    std::vector<Button> level_cards_;
    std::vector<Button> game_level_buttons_;
    std::vector<Button> palette_buttons_;
    std::vector<Button> overlay_buttons_;
    Button back_button_;
    Button run_button_;
    Button retry_button_;
    Button hint_button_;
    Button speed_button_;
    Button next_button_;

    bool reset_confirm_ = false;
    bool hint_visible_ = false;
    bool overlay_visible_ = false;
    std::string menu_message_;
    std::string message_;
    std::string overlay_title_;
    std::string overlay_message_;

    int current_index_ = 0;
    int target_uid_ = -1;
    LevelData level_;
    CircuitGraph graph_;
    Simulator sim_;
    BoardView board_;
    WaveformView waveform_;
    std::vector<CircuitGraph> undo_stack_;
    std::string selected_tool_ = "wire";
};

}  // namespace river
