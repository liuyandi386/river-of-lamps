#include "app/board_view.h"

#include "app/theme.h"
#include "app/ui_draw.h"
#include "core/component_rules.h"

#include <algorithm>
#include <cmath>

namespace river {

namespace {

sf::FloatRect grow(const sf::FloatRect& rect, float amount) {
    return {rect.left + amount, rect.top + amount, rect.width - amount * 2.f,
            rect.height - amount * 2.f};
}

sf::Color gate_accent(const std::string& type) {
    if (type == "not") return theme::gold;
    if (type == "or") return theme::teal;
    return theme::blue;
}

}  // namespace

void BoardView::set_graph(CircuitGraph* graph) {
    graph_ = graph;
    drawing_ = false;
    last_is_component_ = false;
    hover_ = {-1, -1};
    pulse_ = 0.f;
    last_frame_tick_ = -2;
}

void BoardView::set_frame(const TickFrame* frame) {
    frame_values_.clear();
    frame_glitched_.clear();
    if (frame != nullptr) {
        frame_values_ = frame->values;
        frame_glitched_ = frame->glitched;
        if (frame->tick != last_frame_tick_) {
            last_frame_tick_ = frame->tick;
            pulse_ = 1.f;
        }
    } else {
        pulse_ = 0.f;
        last_frame_tick_ = -2;
    }
}

void BoardView::update(float dt) {
    pulse_ = std::max(0.f, pulse_ - dt * 5.5f);
}

void BoardView::set_error(const std::optional<ErrorInfo>& error) {
    if (!error.has_value()) {
        clear_error();
        return;
    }
    error_target_uid_ = error->target_uid;
    error_tick_ = error->tick;
}

void BoardView::clear_error() {
    error_target_uid_ = -1;
    error_tick_ = -1;
}

void BoardView::set_tool(const std::string& tool) {
    tool_ = tool;
}

void BoardView::set_editable(bool editable) {
    editable_ = editable;
    if (!editable_) {
        drawing_ = false;
    }
}

void BoardView::set_origin(const sf::Vector2f& origin) {
    origin_ = origin;
}

sf::FloatRect BoardView::board_rect() const {
    if (graph_ == nullptr) {
        return {origin_.x, origin_.y, 0.f, 0.f};
    }
    return {origin_.x, origin_.y, static_cast<float>(graph_->width * cell_size),
            static_cast<float>(graph_->height * cell_size)};
}

Vec2i BoardView::cell_at(const sf::Vector2f& point) const {
    return {static_cast<int>(std::floor((point.x - origin_.x) / cell_size)),
            static_cast<int>(std::floor((point.y - origin_.y) / cell_size))};
}

void BoardView::handle_mouse_move(const sf::Vector2f& point, bool left_down) {
    if (graph_ == nullptr) {
        return;
    }
    const Vec2i cell = cell_at(point);
    hover_ = cell;
    if (drawing_ && editable_ && left_down) {
        try_draw_to(cell);
    }
}

bool BoardView::handle_mouse_press(const sf::Vector2f& point, sf::Mouse::Button button) {
    if (graph_ == nullptr || !board_rect().contains(point)) {
        return false;
    }
    const Vec2i cell = cell_at(point);
    if (!graph_->in_bounds(cell)) {
        return false;
    }
    if (button == sf::Mouse::Left) {
        if (editable_) {
            on_left_press(cell);
        }
        return true;
    }
    if (button == sf::Mouse::Right) {
        if (editable_) {
            on_right_click(cell);
        }
        return true;
    }
    return false;
}

void BoardView::handle_mouse_release() {
    drawing_ = false;
    last_is_component_ = false;
}

void BoardView::on_left_press(const Vec2i& cell) {
    if (graph_ == nullptr || !graph_->in_bounds(cell)) {
        return;
    }

    if (tool_ != "wire") {
        if (graph_->is_empty(cell)) {
            if (on_edit_started) on_edit_started();
            graph_->add_instance(tool_, cell, false);
            if (on_layout_changed) on_layout_changed();
        }
        return;
    }

    const Instance* inst = graph_->get_instance_at(cell);
    if (inst == nullptr) {
        if (on_edit_started) on_edit_started();
        graph_->add_wire(cell);
        drawing_ = true;
        last_cell_ = cell;
        last_is_component_ = false;
        if (on_layout_changed) on_layout_changed();
        return;
    }

    if (inst->is_wire()) {
        if (graph_->get_wire_flow(cell) == Vec2i{0, 0}) {
            if (on_edit_started) on_edit_started();
            drawing_ = true;
            last_cell_ = cell;
            last_is_component_ = false;
        }
        return;
    }

    if (inst->type == "source" || ComponentRules::is_gate(*inst)) {
        if (on_edit_started) on_edit_started();
        drawing_ = true;
        last_cell_ = cell;
        last_is_component_ = true;
    }
}

void BoardView::try_draw_to(const Vec2i& cell) {
    if (graph_ == nullptr || !graph_->in_bounds(cell)) {
        return;
    }
    if (cell == last_cell_) {
        return;
    }
    const Vec2i diff = cell - last_cell_;
    if (std::abs(diff.x) + std::abs(diff.y) != 1) {
        return;
    }

    const Instance* target = graph_->get_instance_at(cell);
    if (target != nullptr) {
        if (target->is_wire()) {
            return;
        }
        if (last_is_component_) {
            return;
        }
        const int idx = ComponentRules::input_index_facing(*target, last_cell_);
        if (idx >= 0) {
            graph_->set_wire_flow(last_cell_, diff);
            drawing_ = false;
            last_is_component_ = false;
            if (on_layout_changed) on_layout_changed();
        }
        return;
    }

    if (last_is_component_) {
        const Instance* last = graph_->get_instance_at(last_cell_);
        if (last != nullptr && !last->is_wire() && diff != kRight) {
            return;
        }
        graph_->add_wire(cell);
        last_cell_ = cell;
        last_is_component_ = false;
        if (on_layout_changed) on_layout_changed();
        return;
    }

    graph_->set_wire_flow(last_cell_, diff);
    graph_->add_wire(cell);
    last_cell_ = cell;
    if (on_layout_changed) on_layout_changed();
}

void BoardView::on_right_click(const Vec2i& cell) {
    if (graph_ == nullptr || !graph_->in_bounds(cell)) {
        return;
    }
    const Instance* inst = graph_->get_instance_at(cell);
    if (inst == nullptr || inst->is_preset) {
        return;
    }
    if (on_edit_started) on_edit_started();
    if (inst->is_wire()) {
        graph_->remove_path_at(cell);
    } else {
        graph_->remove_instance_at(cell);
    }
    if (on_layout_changed) on_layout_changed();
}

void BoardView::draw(sf::RenderTarget& target, const sf::Font& font) const {
    if (graph_ == nullptr) {
        return;
    }
    const sf::FloatRect board = board_rect();
    ui::fill_rect(target, board, theme::board_bg);

    for (int y = 0; y < graph_->height; ++y) {
        for (int x = 0; x < graph_->width; ++x) {
            const sf::FloatRect cell_rect(origin_.x + x * cell_size,
                                           origin_.y + y * cell_size,
                                           static_cast<float>(cell_size),
                                           static_cast<float>(cell_size));
            const bool alternate = ((x + y) & 1) == 0;
            ui::fill_rect(target, cell_rect,
                          alternate ? theme::board_cell : theme::board_bg);
            ui::outline_rect(target, cell_rect, theme::board_grid, 1.f);
        }
    }

    if (hover_.x >= 0 && graph_->in_bounds(hover_) && editable_) {
        const sf::FloatRect hover_rect(origin_.x + hover_.x * cell_size,
                                       origin_.y + hover_.y * cell_size,
                                       static_cast<float>(cell_size),
                                       static_cast<float>(cell_size));
        if (graph_->is_empty(hover_)) {
            ui::fill_rect(target, grow(hover_rect, 2.f), sf::Color(theme::teal.r,
                                                                   theme::teal.g,
                                                                   theme::teal.b, 24));
            ui::outline_rect(target, grow(hover_rect, 2.f), theme::teal, 1.f);
        } else {
            ui::fill_rect(target, grow(hover_rect, 2.f), sf::Color(theme::danger.r,
                                                                   theme::danger.g,
                                                                   theme::danger.b, 18));
        }
    }

    for (const auto& [uid, inst] : graph_->instances) {
        (void)uid;
        if (inst.is_wire()) {
            draw_wire(target, font, inst);
        }
    }
    for (const auto& [uid, inst] : graph_->instances) {
        (void)uid;
        if (!inst.is_wire()) {
            draw_component(target, font, inst);
        }
    }

    if (drawing_ && graph_->in_bounds(last_cell_) && graph_->in_bounds(hover_)) {
        const sf::FloatRect last_rect(origin_.x + last_cell_.x * cell_size,
                                      origin_.y + last_cell_.y * cell_size,
                                      static_cast<float>(cell_size),
                                      static_cast<float>(cell_size));
        ui::outline_rect(target, grow(last_rect, 2.f), theme::teal, 1.f);
        const int distance = std::abs(hover_.x - last_cell_.x) +
                             std::abs(hover_.y - last_cell_.y);
        if (distance == 1) {
            const sf::Vector2f a{last_rect.left + last_rect.width * 0.5f,
                                 last_rect.top + last_rect.height * 0.5f};
            const sf::Vector2f b{origin_.x + hover_.x * cell_size +
                                     static_cast<float>(cell_size) * 0.5f,
                                 origin_.y + hover_.y * cell_size +
                                     static_cast<float>(cell_size) * 0.5f};
            ui::draw_line(target, a, b, sf::Color(theme::teal.r, theme::teal.g,
                                                   theme::teal.b, 140), 2.f);
        }
    }

    ui::outline_rect(target, board, theme::border_strong, 1.f);
}

void BoardView::draw_wire(sf::RenderTarget& target, const sf::Font& font,
                          const Instance& inst) const {
    (void)font;
    const sf::FloatRect cell_rect(origin_.x + inst.origin.x * cell_size,
                                  origin_.y + inst.origin.y * cell_size,
                                  static_cast<float>(cell_size),
                                  static_cast<float>(cell_size));
    const auto value_it = frame_values_.find(inst.uid);
    const int value = value_it == frame_values_.end() ? 0 : value_it->second;
    const bool glitch = std::find(frame_glitched_.begin(), frame_glitched_.end(), inst.uid) !=
                        frame_glitched_.end();

    sf::Color fill = value == 1 ? theme::wire_high : theme::wire_low;
    sf::Color border = value == 1 ? theme::gold_bright : theme::border_strong;
    if (glitch) {
        fill = theme::danger;
        border = sf::Color(255, 154, 154);
    } else if (value == 1 && pulse_ > 0.f) {
        fill = ui::mix(fill, sf::Color(255, 246, 220), 0.12f * pulse_);
    }

    const sf::FloatRect body = grow(cell_rect, 2.f);
    ui::draw_rounded_rect(target, body, theme::radius_sm, fill, border, 1.f);

    const sf::Vector2f center{cell_rect.left + cell_rect.width * 0.5f,
                              cell_rect.top + cell_rect.height * 0.5f};
    const sf::Color arrow = value == 1 ? sf::Color(84, 61, 22) : theme::text_dim;
    if (inst.flow_dir != Vec2i{0, 0}) {
        const sf::Vector2f end{center.x + inst.flow_dir.x * cell_size * 0.30f,
                               center.y + inst.flow_dir.y * cell_size * 0.30f};
        ui::draw_line(target, center, end, arrow, 3.f);
        ui::fill_circle(target, center, 2.2f, arrow);
    } else {
        ui::fill_circle(target, center, 2.2f, arrow);
    }
}

void BoardView::draw_component(sf::RenderTarget& target, const sf::Font& font,
                               const Instance& inst) const {
    const sf::FloatRect cell_rect(origin_.x + inst.origin.x * cell_size,
                                  origin_.y + inst.origin.y * cell_size,
                                  static_cast<float>(cell_size),
                                  static_cast<float>(cell_size));
    const auto value_it = frame_values_.find(inst.uid);
    const int value = value_it == frame_values_.end() ? 0 : value_it->second;
    const sf::Color accent = gate_accent(inst.type);
    sf::Color fill = theme::surface_alt;
    sf::Color border = accent;
    if (inst.type == "source") {
        fill = sf::Color(25, 38, 54);
        border = theme::gold;
    } else if (inst.type == "target") {
        fill = sf::Color(20, 38, 38);
        border = theme::teal;
    }
    if (value == 1) {
        fill = ui::mix(fill, accent, 0.12f + 0.10f * pulse_);
    }
    if (inst.uid == error_target_uid_) {
        border = theme::danger;
        fill = ui::mix(fill, theme::danger, 0.10f);
    }

    const sf::FloatRect body = grow(cell_rect, 6.f);
    ui::draw_rounded_rect(target, body, theme::radius_sm, fill, border, 1.5f);
    ui::fill_rect(target, {body.left + 1.f, body.top + 1.f, body.width - 2.f, 2.f},
                  accent);

    std::string label;
    if (inst.type == "source") label = "S";
    else if (inst.type == "target") label = "T";
    else if (inst.type == "and") label = "AND";
    else if (inst.type == "or") label = "OR";
    else if (inst.type == "not") label = "NOT";
    if (!label.empty()) {
        const unsigned size = static_cast<unsigned>(label.size() <= 1 ? theme::text_title : theme::text_label);
        ui::draw_text_centered(target, font, label, size, body,
                              inst.uid == error_target_uid_ ? theme::danger : theme::text);
    }

    if (inst.type == "target") {
        if (inst.uid == error_target_uid_) {
            const sf::Vector2f p{body.left + body.width - 11.f, body.top + 11.f};
            ui::draw_line(target, {p.x - 4.f, p.y - 4.f}, {p.x + 4.f, p.y + 4.f},
                          theme::danger, 2.f);
            ui::draw_line(target, {p.x + 4.f, p.y - 4.f}, {p.x - 4.f, p.y + 4.f},
                          theme::danger, 2.f);
        } else {
            ui::fill_circle(target, {body.left + body.width - 10.f, body.top + 10.f}, 2.5f,
                            theme::teal);
        }
    }

    draw_ports(target, inst, cell_rect);
}

void BoardView::draw_ports(sf::RenderTarget& target, const Instance& inst,
                           const sf::FloatRect& rect) const {
    const float cx = rect.left + rect.width * 0.5f;
    const float cy = rect.top + rect.height * 0.5f;
    const float s = 5.f;
    const sf::Color in = theme::teal;
    const sf::Color out = theme::gold;

    auto port = [&target](const sf::FloatRect& r, sf::Color color) {
        ui::fill_rect(target, r, color);
        ui::outline_rect(target, r, theme::bg, 1.f);
    };

    if (inst.type == "source") {
        port({rect.left + rect.width - 4.f, cy - s * 0.5f, 4.f, s}, out);
    } else if (inst.type == "target") {
        port({rect.left, cy - s * 0.5f, 4.f, s}, in);
    } else if (inst.type == "not") {
        port({rect.left, cy - s * 0.5f, 4.f, s}, in);
        port({rect.left + rect.width - 4.f, cy - s * 0.5f, 4.f, s}, out);
    } else if (inst.type == "and" || inst.type == "or") {
        port({rect.left, cy - s * 0.5f, 4.f, s}, in);
        port({cx - s * 0.5f, rect.top + rect.height - 4.f, s, 4.f}, in);
        port({rect.left + rect.width - 4.f, cy - s * 0.5f, 4.f, s}, out);
    }
}

}  // namespace river
