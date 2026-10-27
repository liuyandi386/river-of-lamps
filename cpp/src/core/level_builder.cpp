#include "core/level_builder.h"

#include <string>

namespace river {

Vec2i LevelBuilder::vec(const nlohmann::json& value) {
    if (!value.is_array() || value.size() < 2) {
        return Vec2i{0, 0};
    }
    return Vec2i{value[0].get<int>(), value[1].get<int>()};
}

CircuitGraph LevelBuilder::build(const nlohmann::json& level, bool include_reference) {
    CircuitGraph graph;
    const nlohmann::json grid = level.value("grid", nlohmann::json::object());
    graph.setup(grid.value("w", 8), grid.value("h", 8));

    if (level.contains("preset") && level["preset"].is_array()) {
        for (const auto& p : level["preset"]) {
            if (!p.is_object()) {
                continue;
            }
            const Vec2i cell = vec(p.value("pos", nlohmann::json::array({0, 0})));
            const std::string type = p.value("type", std::string{});
            const int uid = graph.add_instance(type, cell, true);
            if (uid < 0) {
                continue;
            }
            const nlohmann::json params = p.value("params", nlohmann::json::object());
            if (type == "source") {
                graph.instances[uid].mode = params.value("mode", std::string{"high"});
                graph.instances[uid].pulse_at_tick = params.value("pulse_at_tick", 0);
            }
        }
    }

    if (level.contains("targets") && level["targets"].is_array()) {
        for (const auto& t : level["targets"]) {
            if (!t.is_object()) {
                continue;
            }
            const Vec2i cell = vec(t.value("pos", nlohmann::json::array({0, 0})));
            const int uid = graph.add_instance("target", cell, true);
            if (uid < 0) {
                continue;
            }
            Instance& inst = graph.instances[uid];
            inst.id = t.value("id", std::string{"target"});
            inst.start_tick = t.value("start_tick", 0);
            if (t.contains("expect") && t["expect"].is_array()) {
                for (const auto& v : t["expect"]) {
                    inst.expect.push_back(v.get<int>());
                }
            }
        }
    }

    if (include_reference) {
        const nlohmann::json ref = level.value("reference_solution", nlohmann::json::object());
        if (ref.is_object() && ref.contains("wires") && ref["wires"].is_array()) {
            for (const auto& path : ref["wires"]) {
                apply_path(graph, path);
            }
        }
    }

    return graph;
}

void LevelBuilder::apply_path(CircuitGraph& graph, const nlohmann::json& path) {
    if (!path.is_array()) {
        return;
    }
    Vec2i prev_cell{-1, -1};
    bool prev_is_wire = false;

    for (const auto& item : path) {
        const Vec2i cell = vec(item);
        const Instance* existing = graph.get_instance_at(cell);
        if (existing != nullptr && existing->type != "wire") {
            if (prev_is_wire && prev_cell != Vec2i{-1, -1}) {
                graph.set_wire_flow(prev_cell, cell - prev_cell);
            }
            return;
        }
        if (existing != nullptr && existing->type == "wire") {
            prev_cell = cell;
            prev_is_wire = true;
            continue;
        }
        if (!graph.is_empty(cell)) {
            return;
        }
        graph.add_wire(cell);
        if (prev_is_wire && prev_cell != Vec2i{-1, -1}) {
            graph.set_wire_flow(prev_cell, cell - prev_cell);
        }
        prev_cell = cell;
        prev_is_wire = true;
    }
}

}  // namespace river
