#pragma once

#include "core/circuit_graph.h"
#include "core/types.h"

#include <SFML/Graphics.hpp>

#include <functional>
#include <map>
#include <optional>
#include <string>
#include <vector>

namespace river {

// 画布：网格、灯槽、元件、信号颜色；输入只改 CircuitGraph。
class BoardView {
public:
    std::function<void()> on_edit_started;
    std::function<void()> on_layout_changed;

    void set_graph(CircuitGraph* graph);
    void set_frame(const TickFrame* frame);
    void set_error(const std::optional<ErrorInfo>& error);
    void clear_error();
    void set_tool(const std::string& tool);
    void set_editable(bool editable);
    void set_origin(const sf::Vector2f& origin);
    void update(float dt);

    int cell_size = 56;
    sf::FloatRect board_rect() const;
    Vec2i cell_at(const sf::Vector2f& point) const;

    void draw(sf::RenderTarget& target, const sf::Font& font) const;

    void handle_mouse_move(const sf::Vector2f& point, bool left_down);
    bool handle_mouse_press(const sf::Vector2f& point, sf::Mouse::Button button);
    void handle_mouse_release();

    bool is_editable() const { return editable_; }
    bool is_drawing() const { return drawing_; }

private:
    void on_left_press(const Vec2i& cell);
    void try_draw_to(const Vec2i& cell);
    void on_right_click(const Vec2i& cell);

    void draw_wire(sf::RenderTarget& target, const sf::Font& font, const Instance& inst) const;
    void draw_component(sf::RenderTarget& target, const sf::Font& font, const Instance& inst) const;
    void draw_ports(sf::RenderTarget& target, const Instance& inst, const sf::FloatRect& rect) const;

    CircuitGraph* graph_ = nullptr;
    std::string tool_ = "wire";
    bool editable_ = true;
    bool drawing_ = false;
    Vec2i last_cell_{-1, -1};
    bool last_is_component_ = false;
    Vec2i hover_{-1, -1};

    std::map<int, int> frame_values_;
    std::vector<int> frame_glitched_;
    int error_target_uid_ = -1;
    int error_tick_ = -1;
    float pulse_ = 0.f;
    int last_frame_tick_ = -2;

    sf::Vector2f origin_{0.f, 0.f};
};

}  // namespace river
