#pragma once

#include "core/types.h"

#include <map>
#include <string>

namespace river {

// 网格 + 元件图（与 GDScript 的 CircuitGraph 对齐）。
class CircuitGraph {
public:
    int width = 8;
    int height = 8;
    int next_uid = 1;
    std::map<int, Instance> instances;
    std::map<Vec2i, int> cells;

    void setup(int w, int h);
    bool in_bounds(const Vec2i& cell) const;
    bool is_empty(const Vec2i& cell) const;
    bool has_instance(const Vec2i& cell) const;

    Instance* get_instance_at(const Vec2i& cell);
    const Instance* get_instance_at(const Vec2i& cell) const;

    int add_instance(const std::string& type, const Vec2i& cell, bool is_preset = false);
    int add_wire(const Vec2i& cell, const Vec2i& flow_dir = Vec2i{0, 0});

    Vec2i get_wire_flow(const Vec2i& cell) const;
    void set_wire_flow(const Vec2i& cell, const Vec2i& dir);

    void remove_instance_at(const Vec2i& cell);
    void remove_path_at(const Vec2i& cell);

    // 只数玩家放置的功能元件；灯槽不算。
    int count_player_functional() const;
};

}  // namespace river
