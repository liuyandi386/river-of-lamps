#include "core/component_rules.h"

namespace river {

bool ComponentRules::is_gate_type(const std::string& type) {
    return type == "and" || type == "or" || type == "not";
}

bool ComponentRules::is_gate(const Instance& inst) {
    return is_gate_type(inst.type);
}

int ComponentRules::input_index_facing(const Instance& comp, const Vec2i& from_cell) {
    if (comp.type == "and" || comp.type == "or") {
        if (from_cell == comp.origin + kLeft) {
            return 0;
        }
        if (from_cell == comp.origin + kDown) {
            return 1;
        }
        return -1;
    }
    if (comp.type == "not") {
        return from_cell == comp.origin + kLeft ? 0 : -1;
    }
    if (comp.type == "target") {
        return from_cell == comp.origin + kLeft ? 0 : -1;
    }
    return -1;
}

std::string ComponentRules::display_name(const std::string& type) {
    if (type == "wire") return "灯槽";
    if (type == "and") return "与门";
    if (type == "or") return "或门";
    if (type == "not") return "非门";
    if (type == "source") return "信号源";
    if (type == "target") return "目标端口";
    return type;
}

}  // namespace river
