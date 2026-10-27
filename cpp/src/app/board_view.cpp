#include "app/board_view.h"

#include "app/ui_draw.h"
#include "core/component_rules.h"

#include <algorithm>
#include <cmath>

namespace river {

namespace {

const sf::Color kBg(9, 13, 22);
const sf::Color kGrid(28, 38, 59);
const sf::Color kLow(36, 59, 107);
const sf::Color kHigh(255, 184, 56);
const sf::Color kGlitch(255, 46, 46);
const sf::Color kGate(41, 51, 77);
const sf::Color kText(217, 230, 255);
const sf::Color kInPort(140, 191, 255);
const sf::Color kOutPort(255, 199, 77);

sf::FloatRect grow(const sf::FloatRect& rect, float amount) {
    return {rect.left + amount, rect.top + amount, rect.width - amount * 2.f,
            rect.height - amount * 2.f};
}

}  // namespace

void BoardView::set_graph(CircuitGraph* graph) {
    graph_ = graph;
    drawing_ = false;
    last_is_component_ = false;
    hover_ = {-1, -1};
}

void BoardView::set_frame(const TickFrame* frame) {
    frame_values_.clear();
    frame_glitched_.clear();
    if (frame != nullptr) {
        frame_values_ = frame->values;
        frame_glitched_ = frame->glitched;
    }
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
    ui::fill_rect(target, board, kBg);

    for (int y = 0; y < graph_->height; ++y) {
        for (int x = 0; x < graph_->width; ++x) {
            const sf::FloatRect cell_rect(origin_.x + x * cell_size, origin_.y + y * cell_size,
                                          static_cast<float>(cell_size), static_cast<float>(cell_size));
            ui::outline_rect(target, cell_rect, kGrid, 1.f);
        }
    }
    ui::outline_rect(target, board, sf::Color(51, 77, 128), 2.f);

    if (hover_.x >= 0 && graph_->in_bounds(hover_) && editable_) {
        const sf::FloatRect hover_rect(origin_.x + hover_.x * cell_size,
                                       origin_.y + hover_.y * cell_size,
                                       static_cast<float>(cell_size), static_cast<float>(cell_size));
        if (graph_->is_empty(hover_)) {
            ui::fill_rect(target, grow(hover_rect, 2.f), sf::Color(153, 204, 255, 30));
        } else {
            ui::fill_rect(target, grow(hover_rect, 2.f), sf::Color(255, 77, 77, 26));
        }
    }

    for (const auto& [uid, inst] : graph_->instances) {
        (void)uid;
        if (!inst.is_wire()) {
            draw_component(target, font, inst);
        }
    }
    for (const auto& [uid, inst] : graph_->instances) {
        (void)uid;
        if (inst.is_wire()) {
            draw_wire(target, font, inst);
        }
    }
}

void BoardView::draw_wire(sf::RenderTarget& target, const sf::Font& font, const Instance& inst) const {
    (void)font;
    const sf::FloatRect cell_rect(origin_.x + inst.origin.x * cell_size,
                                  origin_.y + inst.origin.y * cell_size,
                                  static_cast<float>(cell_size), static_cast<float>(cell_size));
    const auto value_it = frame_values_.find(inst.uid);
    const int value = value_it == frame_values_.end() ? 0 : value_it->second;
    const bool glitch = std::find(frame_glitched_.begin(), frame_glitched_.end(), inst.uid) !=
                        frame_glitched_.end();

    sf::Color color = value == 1 ? kHigh : kLow;
    if (glitch) {
        color = kGlitch;
    }
    if (value == 1 && !glitch) {
        ui::fill_circle(target, {cell_rect.left + cell_rect.width * 0.5f,
                                 cell_rect.top + cell_rect.height * 0.5f},
                        15.f, sf::Color(255, 184, 51, 31));
    }
    const sf::FloatRect body = grow(cell_rect, 9.f);
    ui::fill_rect(target, body, color);
    ui::fill_circle(target, {cell_rect.left + cell_rect.width * 0.5f,
                             cell_rect.top + cell_rect.height * 0.5f},
                    6.f, ui::mix(color, sf::Color::White, 0.22f));

    if (inst.flow_dir != Vec2i{0, 0}) {
        const sf::Vector2f center{cell_rect.left + cell_rect.width * 0.5f,
                                  cell_rect.top + cell_rect.height * 0.5f};
        const sf::Vector2f end{center.x + inst.flow_dir.x * cell_size * 0.26f,
                               center.y + inst.flow_dir.y * cell_size * 0.26f};
        ui::draw_line(target, center, end, sf::Color(5, 8, 13, 140), 3.f);
    }
}

void BoardView::draw_component(sf::RenderTarget& target, const sf::Font& font,
                               const Instance& inst) const {
    const sf::FloatRect cell_rect(origin_.x + inst.origin.x * cell_size,
                                  origin_.y + inst.origin.y * cell_size,
                                  static_cast<float>(cell_size), static_cast<float>(cell_size));
    sf::Color fill = kGate;
    if (inst.type == "source") {
        fill = sf::Color(36, 66, 107);
    } else if (inst.type == "target") {
        fill = sf::Color(36, 56, 87);
    }
    const auto value_it = frame_values_.find(inst.uid);
    if (value_it != frame_values_.end() && value_it->second == 1) {
        fill = ui::mix(fill, sf::Color::White, 0.18f);
    }

    const sf::FloatRect body = grow(cell_rect, 6.f);
    ui::fill_rect(target, body, fill);

    sf::Color border = inst.is_preset ? sf::Color(140, 184, 255) : sf::Color(107, 148, 230);
    if (inst.uid == error_target_uid_) {
        border = sf::Color(255, 51, 51);
    }
    ui::outline_rect(target, body, border, 2.f);

    std::string label;
    if (inst.type == "source") label = "S";
    else if (inst.type == "target") label = "T";
    else if (inst.type == "and") label = "AND";
    else if (inst.type == "or") label = "OR";
    else if (inst.type == "not") label = "NOT";
    if (!label.empty()) {
        const unsigned size = label.size() <= 1 ? 15 : 12;
        ui::draw_text_centered(target, font, label, size, body, kText);
    }

    if (inst.type == "target") {
        const sf::Color dot = inst.uid == error_target_uid_ ? sf::Color(255, 64, 64)
                                                            : sf::Color(255, 204, 77);
        ui::fill_circle(target, {body.left + body.width - 12.f, body.top + 12.f}, 6.f, dot);
    }

    draw_ports(target, inst, cell_rect);
}

void BoardView::draw_ports(sf::RenderTarget& target, const Instance& inst,
                           const sf::FloatRect& rect) const {
    const float cx = rect.left + rect.width * 0.5f;
    const float cy = rect.top + rect.height * 0.5f;
    const float s = 6.f;

    if (inst.type == "source") {
        ui::fill_rect(target, {rect.left + rect.width - 5.f, cy - s * 0.5f, 5.f, s}, kOutPort);
    } else if (inst.type == "target") {
        ui::fill_rect(target, {rect.left, cy - s * 0.5f, 5.f, s}, kInPort);
    } else if (inst.type == "not") {
        ui::fill_rect(target, {rect.left, cy - s * 0.5f, 5.f, s}, kInPort);
        ui::fill_rect(target, {rect.left + rect.width - 5.f, cy - s * 0.5f, 5.f, s}, kOutPort);
    } else if (inst.type == "and" || inst.type == "or") {
        ui::fill_rect(target, {rect.left, cy - s * 0.5f, 5.f, s}, kInPort);
        ui::fill_rect(target, {cx - s * 0.5f, rect.top + rect.height - 5.f, s, 5.f}, kInPort);
        ui::fill_rect(target, {rect.left + rect.width - 5.f, cy - s * 0.5f, 5.f, s}, kOutPort);
    }
}

}  // namespace river
