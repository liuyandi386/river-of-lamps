#pragma once

#include "core/circuit_graph.h"

#include <nlohmann/json.hpp>

namespace river {

// 把关卡 JSON 变成 CircuitGraph（与 GDScript 的 LevelBuilder 对齐）。
class LevelBuilder {
public:
    static CircuitGraph build(const nlohmann::json& level, bool include_reference = false);
    static void apply_path(CircuitGraph& graph, const nlohmann::json& path);

private:
    static Vec2i vec(const nlohmann::json& value);
};

}  // namespace river
