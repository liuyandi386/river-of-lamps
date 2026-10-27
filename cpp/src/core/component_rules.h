#pragma once

#include "core/types.h"

#include <string>

namespace river {

// 元件端口/朝向的公共规则（与 GDScript 的 ComponentRules 对齐）。
// 固定朝向：源 E；非门 W->E；与/或 W,S->E；目标 W；灯槽由 flow_dir 决定。
class ComponentRules {
public:
    static bool is_gate_type(const std::string& type);
    static bool is_gate(const Instance& inst);
    static int input_index_facing(const Instance& comp, const Vec2i& from_cell);
    static std::string display_name(const std::string& type);
};

}  // namespace river
