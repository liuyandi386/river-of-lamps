#include "core/circuit_graph.h"

#include <algorithm>
#include <array>

namespace river {

void CircuitGraph::setup(int w, int h) {
    width = w;
    height = h;
    next_uid = 1;
    instances.clear();
    cells.clear();
}

bool CircuitGraph::in_bounds(const Vec2i& cell) const {
    return cell.x >= 0 && cell.y >= 0 && cell.x < width && cell.y < height;
}

bool CircuitGraph::is_empty(const Vec2i& cell) const {
    return in_bounds(cell) && cells.find(cell) == cells.end();
}

bool CircuitGraph::has_instance(const Vec2i& cell) const {
    return cells.find(cell) != cells.end();
}

Instance* CircuitGraph::get_instance_at(const Vec2i& cell) {
    const auto it = cells.find(cell);
    if (it == cells.end()) {
        return nullptr;
    }
    const auto inst_it = instances.find(it->second);
    return inst_it == instances.end() ? nullptr : &inst_it->second;
}

const Instance* CircuitGraph::get_instance_at(const Vec2i& cell) const {
    const auto it = cells.find(cell);
    if (it == cells.end()) {
        return nullptr;
    }
    const auto inst_it = instances.find(it->second);
    return inst_it == instances.end() ? nullptr : &inst_it->second;
}

int CircuitGraph::add_instance(const std::string& type, const Vec2i& cell, bool is_preset) {
    if (!is_empty(cell)) {
        return -1;
    }
    Instance inst;
    inst.uid = next_uid++;
    inst.type = type;
    inst.origin = cell;
    inst.is_preset = is_preset;
    instances[inst.uid] = inst;
    cells[cell] = inst.uid;
    return inst.uid;
}

int CircuitGraph::add_wire(const Vec2i& cell, const Vec2i& flow_dir) {
    if (!is_empty(cell)) {
        return -1;
    }
    const int uid = add_instance("wire", cell, false);
    if (uid >= 0) {
        instances[uid].flow_dir = flow_dir;
    }
    return uid;
}

Vec2i CircuitGraph::get_wire_flow(const Vec2i& cell) const {
    const Instance* inst = get_instance_at(cell);
    if (inst == nullptr || !inst->is_wire()) {
        return Vec2i{0, 0};
    }
    return inst->flow_dir;
}

void CircuitGraph::set_wire_flow(const Vec2i& cell, const Vec2i& dir) {
    Instance* inst = get_instance_at(cell);
    if (inst != nullptr && inst->is_wire()) {
        inst->flow_dir = dir;
    }
}

void CircuitGraph::remove_instance_at(const Vec2i& cell) {
    const auto it = cells.find(cell);
    if (it == cells.end()) {
        return;
    }
    instances.erase(it->second);
    cells.erase(it);
}

void CircuitGraph::remove_path_at(const Vec2i& cell) {
    const Instance* inst = get_instance_at(cell);
    if (inst == nullptr || !inst->is_wire()) {
        return;
    }

    std::vector<Vec2i> path{cell};
    Vec2i cur = cell;
    while (true) {
        const Vec2i d = get_wire_flow(cur);
        if (d == Vec2i{0, 0}) {
            break;
        }
        const Vec2i nxt = cur + d;
        const Instance* n = get_instance_at(nxt);
        if (n == nullptr || !n->is_wire()) {
            break;
        }
        if (std::find(path.begin(), path.end(), nxt) != path.end()) {
            break;
        }
        path.push_back(nxt);
        cur = nxt;
    }

    cur = cell;
    const std::array<Vec2i, 4> dirs{kLeft, kRight, kUp, kDown};
    while (true) {
        Vec2i found{-1, -1};
        for (const Vec2i& d : dirs) {
            const Vec2i prev = cur + d;
            const Instance* p = get_instance_at(prev);
            if (p == nullptr || !p->is_wire()) {
                continue;
            }
            const Vec2i back{-d.x, -d.y};
            if (get_wire_flow(prev) == back) {
                found = prev;
                break;
            }
        }
        if (found == Vec2i{-1, -1}) {
            break;
        }
        if (std::find(path.begin(), path.end(), found) != path.end()) {
            break;
        }
        path.push_back(found);
        cur = found;
    }

    for (const Vec2i& c : path) {
        remove_instance_at(c);
    }
}

int CircuitGraph::count_player_functional() const {
    int n = 0;
    for (const auto& [uid, inst] : instances) {
        (void)uid;
        if (inst.is_preset) {
            continue;
        }
        if (inst.type == "wire") {
            continue;
        }
        ++n;
    }
    return n;
}

}  // namespace river
