#include "core/simulator.h"

#include "core/component_rules.h"

#include <algorithm>
#include <set>
#include <utility>

namespace river {

void Simulator::setup(const CircuitGraph& graph, int tick_budget, int max_components,
                      int three_star_tick, int two_star_tick, int one_star_tick) {
    graph_ = graph;
    tick_budget_ = tick_budget > 0 ? tick_budget : kDefaultBudget;
    max_components_ = max_components;
    three_star_tick_ = three_star_tick;
    two_star_tick_ = two_star_tick;
    one_star_tick_ = one_star_tick;

    if (one_star_tick_ > 0) {
        one_star_tick_ = std::min(one_star_tick_, tick_budget_ - 1);
        if (two_star_tick_ <= 0 || two_star_tick_ > one_star_tick_) {
            two_star_tick_ = one_star_tick_;
        }
        if (three_star_tick_ <= 0 || three_star_tick_ > two_star_tick_) {
            three_star_tick_ = two_star_tick_;
        }
    } else {
        three_star_tick_ = 0;
        two_star_tick_ = 0;
        one_star_tick_ = 0;
    }
    reset();
}

void Simulator::reset() {
    tick_ = -1;
    values_.clear();
    pending_.clear();
    inputs_by_index_.clear();
    source_uids_.clear();
    wire_uids_.clear();
    gate_uids_.clear();
    target_uids_.clear();
    target_actual_.clear();
    target_matched_.clear();
    target_match_tick_.clear();
    gate_order_.clear();
    gate_cycle_ = false;
    finished_ = false;
    passed_ = false;
    completion_tick_ = -1;
    stars_ = 0;
    fail_reason_.clear();
    first_error_.reset();
    last_frame_ = TickFrame{};
    has_frame_ = false;
    flips_this_tick_.clear();
    glitched_this_tick_.clear();
    glitch_history_.clear();
    build_runtime();
}

void Simulator::build_runtime() {
    for (const auto& [uid, inst] : graph_.instances) {
        values_[uid] = 0;
        pending_[uid] = {};
        const std::string& type = inst.type;
        if (type == "source") {
            source_uids_.push_back(uid);
            inputs_by_index_[uid] = {};
        } else if (type == "wire") {
            wire_uids_.push_back(uid);
            inputs_by_index_[uid] = std::vector<std::vector<int>>(1);
        } else if (type == "target") {
            target_uids_.push_back(uid);
            inputs_by_index_[uid] = std::vector<std::vector<int>>(1);
            target_actual_[uid] = {};
            target_matched_[uid] = false;
            target_match_tick_[uid] = -1;
        } else if (type == "and" || type == "or") {
            gate_uids_.push_back(uid);
            inputs_by_index_[uid] = std::vector<std::vector<int>>(2);
        } else if (type == "not") {
            gate_uids_.push_back(uid);
            inputs_by_index_[uid] = std::vector<std::vector<int>>(2);
        } else {
            inputs_by_index_[uid] = {};
        }
    }

    build_links();
    build_gate_order();
}

void Simulator::build_links() {
    for (const auto& [uid, inst] : graph_.instances) {
        const std::string& type = inst.type;
        if (type == "wire") {
            if (inst.flow_dir == Vec2i{0, 0}) {
                continue;
            }
            link_output_to_cell(uid, inst.origin, inst.origin + inst.flow_dir);
        } else if (type == "source" || type == "and" || type == "or" || type == "not") {
            link_output_to_cell(uid, inst.origin, inst.origin + kRight);
        }
    }
}

void Simulator::link_output_to_cell(int from_uid, const Vec2i& from_cell, const Vec2i& to_cell) {
    if (!graph_.in_bounds(to_cell)) {
        return;
    }
    const Instance* target = graph_.get_instance_at(to_cell);
    if (target == nullptr) {
        return;
    }
    if (target->type == "wire") {
        add_link(from_uid, target->uid, 0);
        return;
    }
    const int idx = ComponentRules::input_index_facing(*target, from_cell);
    if (idx >= 0) {
        add_link(from_uid, target->uid, idx);
    }
}

void Simulator::add_link(int from_uid, int to_uid, int input_index) {
    auto it = inputs_by_index_.find(to_uid);
    if (it == inputs_by_index_.end()) {
        return;
    }
    auto& buckets = it->second;
    if (input_index < 0 || input_index >= static_cast<int>(buckets.size())) {
        return;
    }
    auto& bucket = buckets[input_index];
    if (std::find(bucket.begin(), bucket.end(), from_uid) == bucket.end()) {
        bucket.push_back(from_uid);
    }
}

void Simulator::build_gate_order() {
    std::map<int, bool> gate_set;
    for (int uid : gate_uids_) {
        gate_set[uid] = true;
    }
    std::map<int, int> state;
    for (int uid : gate_uids_) {
        visit_gate(uid, gate_set, state);
    }
}

void Simulator::visit_gate(int uid, const std::map<int, bool>& gate_set, std::map<int, int>& state) {
    const int st = state[uid];
    if (st == 1) {
        gate_cycle_ = true;
        return;
    }
    if (st == 2) {
        return;
    }
    state[uid] = 1;
    auto it = inputs_by_index_.find(uid);
    if (it != inputs_by_index_.end()) {
        for (const auto& bucket : it->second) {
            for (int up : bucket) {
                if (gate_set.find(up) != gate_set.end()) {
                    visit_gate(up, gate_set, state);
                }
            }
        }
    }
    state[uid] = 2;
    gate_order_.push_back(uid);
}

const TickFrame& Simulator::step() {
    if (finished_) {
        return last_frame_;
    }

    ++tick_;
    flips_this_tick_.clear();
    glitched_this_tick_.clear();

    // 1) 灯槽：应用本拍已调度的输出
    for (int uid : wire_uids_) {
        int v = 0;
        auto it = pending_[uid].find(tick_);
        if (it != pending_[uid].end()) {
            v = it->second;
        }
        set_value(uid, v);
    }

    // 2) 信号源
    for (int uid : source_uids_) {
        set_value(uid, source_value(graph_.instances.at(uid), tick_));
    }

    // 3) 组合门
    if (gate_cycle_) {
        bool changed = true;
        int iters = 0;
        while (changed && iters < kMaxIters) {
            changed = false;
            for (int uid : gate_uids_) {
                const int out = gate_output(graph_.instances.at(uid));
                if (values_[uid] != out) {
                    values_[uid] = out;
                    ++flips_this_tick_[uid];
                    changed = true;
                }
            }
            ++iters;
        }
        if (changed) {
            for (int uid : gate_uids_) {
                glitched_this_tick_.push_back(uid);
            }
        } else {
            for (int uid : gate_uids_) {
                if (flips_this_tick_[uid] > 1) {
                    glitched_this_tick_.push_back(uid);
                }
            }
        }
    } else {
        for (int uid : gate_order_) {
            const int out = gate_output(graph_.instances.at(uid));
            if (values_[uid] != out) {
                values_[uid] = out;
                ++flips_this_tick_[uid];
            }
        }
    }

    // 4) 灯槽采样本拍输入，调度下一拍
    for (int uid : wire_uids_) {
        pending_[uid][tick_ + 1] = input_or(uid, 0);
    }

    // 5) 目标比对
    // 有星级窗口时：目标序列可以“整体后移”，只要在 one_star_tick 前结束，
    // 最早结束的那次匹配决定星级。没有窗口时保持旧的严格按 start_tick 比对。
    std::optional<ErrorInfo> frame_error;
    if (one_star_tick_ > 0) {
        for (int uid : target_uids_) {
            const Instance& inst = graph_.instances.at(uid);
            const int actual = input_or(uid, 0);
            target_actual_[uid].push_back(actual);
            if (target_matched_[uid]) {
                continue;
            }

            const int expect_size = static_cast<int>(inst.expect.size());
            if (expect_size <= 0) {
                continue;
            }
            const int start = inst.start_tick;
            const int first_finish = start + expect_size - 1;
            if (tick_ < first_finish || tick_ > one_star_tick_) {
                continue;
            }
            const int candidate_start = tick_ - expect_size + 1;
            if (candidate_start < start) {
                continue;
            }

            const auto& history = target_actual_[uid];
            bool matched = true;
            for (int i = 0; i < expect_size; ++i) {
                if (history[candidate_start + i] != inst.expect[i]) {
                    matched = false;
                    break;
                }
            }
            if (matched) {
                target_matched_[uid] = true;
                target_match_tick_[uid] = tick_;
                completion_tick_ = std::max(completion_tick_, tick_);
            } else if (tick_ == first_finish && !first_error_.has_value()) {
                for (int i = 0; i < expect_size; ++i) {
                    if (history[start + i] != inst.expect[i]) {
                        first_error_ = ErrorInfo{uid, start + i, inst.expect[i], history[start + i]};
                        break;
                    }
                }
            }
        }

        if (!finished_ && !target_uids_.empty()) {
            bool all_ok = true;
            for (int uid : target_uids_) {
                if (!target_matched_[uid]) {
                    all_ok = false;
                    break;
                }
            }
            if (all_ok) {
                finished_ = true;
                passed_ = true;
                if (completion_tick_ <= three_star_tick_) {
                    stars_ = 3;
                } else if (completion_tick_ <= two_star_tick_) {
                    stars_ = 2;
                } else {
                    stars_ = 1;
                }
            }
        }

        if (!finished_ && tick_ >= one_star_tick_) {
            finished_ = true;
            passed_ = false;
            fail_reason_ = first_error_.has_value() ? "mismatch" : "timeout";
            if (first_error_.has_value()) {
                frame_error = first_error_;
            }
        }
    } else {
        for (int uid : target_uids_) {
            const Instance& inst = graph_.instances.at(uid);
            const int actual = input_or(uid, 0);
            target_actual_[uid].push_back(actual);
            const int start = inst.start_tick;
            const int idx = tick_ - start;
            const int expect_size = static_cast<int>(inst.expect.size());
            if (idx >= 0 && idx < expect_size) {
                const int exp = inst.expect[idx];
                if (actual != exp) {
                    if (!first_error_.has_value()) {
                        first_error_ = ErrorInfo{uid, tick_, exp, actual};
                    }
                    finished_ = true;
                    passed_ = false;
                    fail_reason_ = "mismatch";
                    frame_error = first_error_;
                    break;
                }
                if (idx == expect_size - 1) {
                    target_matched_[uid] = true;
                    target_match_tick_[uid] = tick_;
                    completion_tick_ = std::max(completion_tick_, tick_);
                }
            }
        }

        if (!finished_ && !target_uids_.empty()) {
            bool all_ok = true;
            for (int uid : target_uids_) {
                if (!target_matched_[uid]) {
                    all_ok = false;
                    break;
                }
            }
            if (all_ok) {
                finished_ = true;
                passed_ = true;
            }
        }

        if (!finished_ && tick_ >= tick_budget_ - 1) {
            finished_ = true;
            passed_ = false;
            fail_reason_ = "timeout";
        }
    }

    for (int uid : glitched_this_tick_) {
        glitch_history_.push_back(GlitchMark{tick_, uid});
    }

    TickFrame frame;
    frame.tick = tick_;
    frame.values = values_;
    for (const auto& [uid, count] : flips_this_tick_) {
        if (count > 0) {
            frame.flipped.push_back(uid);
        }
    }
    frame.glitched = glitched_this_tick_;
    frame.error = frame_error;
    last_frame_ = std::move(frame);
    has_frame_ = true;
    return last_frame_;
}

void Simulator::set_value(int uid, int value) {
    const int old = values_[uid];
    values_[uid] = value;
    if (old != value) {
        ++flips_this_tick_[uid];
    }
}

int Simulator::source_value(const Instance& inst, int tick) const {
    if (inst.mode == "high") {
        return 1;
    }
    if (inst.mode == "low") {
        return 0;
    }
    if (inst.mode == "pulse") {
        return tick == inst.pulse_at_tick ? 1 : 0;
    }
    return 0;
}

int Simulator::gate_output(const Instance& inst) const {
    const int a = input_or(inst.uid, 0);
    if (inst.type == "not") {
        return 1 - a;
    }
    if (inst.type == "and") {
        return a & input_or(inst.uid, 1);
    }
    if (inst.type == "or") {
        return a | input_or(inst.uid, 1);
    }
    return 0;
}

int Simulator::input_or(int uid, int input_index) const {
    const auto it = inputs_by_index_.find(uid);
    if (it == inputs_by_index_.end()) {
        return 0;
    }
    const auto& buckets = it->second;
    if (input_index < 0 || input_index >= static_cast<int>(buckets.size())) {
        return 0;
    }
    int v = 0;
    for (int up : buckets[input_index]) {
        if (values_.at(up) == 1) {
            v = 1;
        }
    }
    return v;
}

RunResult Simulator::run_until_done(int max_ticks) {
    if (tick_ < 0) {
        reset();
    }
    int guard = 0;
    while (!finished_ && guard < max_ticks) {
        step();
        ++guard;
    }
    if (!finished_) {
        finished_ = true;
        passed_ = false;
        fail_reason_ = "max_ticks";
    }

    RunResult result;
    result.passed = passed_;
    result.reason = fail_reason_;
    result.ticks = tick_;
    result.stars = stars_;
    result.completion_tick = completion_tick_;
    result.first_error = first_error_;
    return result;
}

std::vector<std::string> Simulator::validation_errors() const {
    std::vector<std::string> errors;

    if (target_uids_.empty()) {
        errors.emplace_back("关卡没有目标端口。");
    }

    for (int uid : target_uids_) {
        const auto it = inputs_by_index_.find(uid);
        if (it == inputs_by_index_.end() || it->second.empty() || it->second[0].empty()) {
            errors.emplace_back("目标端口没有接收到信号：把灯槽接到目标的左侧。");
        }
        const Instance& inst = graph_.instances.at(uid);
        if (inst.expect.empty()) {
            errors.emplace_back("目标序列为空。");
        } else if (inst.start_tick + static_cast<int>(inst.expect.size()) > tick_budget_) {
            errors.emplace_back("目标序列超出了本关的拍数预算。");
        } else if (one_star_tick_ > 0 &&
                   inst.start_tick + static_cast<int>(inst.expect.size()) - 1 > one_star_tick_) {
            errors.emplace_back("目标序列超出了本关的 1 星窗口。");
        }
    }

    for (int uid : gate_uids_) {
        const Instance& inst = graph_.instances.at(uid);
        const int required = inst.type == "not" ? 1 : 2;
        const auto it = inputs_by_index_.find(uid);
        for (int i = 0; i < required; ++i) {
            const bool empty = it == inputs_by_index_.end() ||
                               i >= static_cast<int>(it->second.size()) ||
                               it->second[i].empty();
            if (empty) {
                errors.push_back(ComponentRules::display_name(inst.type) + " 的输入没有接上。");
            }
        }
    }

    for (int uid : wire_uids_) {
        const auto it = inputs_by_index_.find(uid);
        if (it == inputs_by_index_.end() || it->second.empty() || it->second[0].empty()) {
            errors.emplace_back("有一段灯槽没有接上信号源。");
        }
    }

    const int used = graph_.count_player_functional();
    if (used > max_components_) {
        errors.push_back("功能元件超过上限：" + std::to_string(used) + " / " +
                         std::to_string(max_components_) + "。");
    }

    return errors;
}

std::vector<int> Simulator::target_actual(int uid) const {
    const auto it = target_actual_.find(uid);
    return it == target_actual_.end() ? std::vector<int>{} : it->second;
}

int Simulator::first_target_uid() const {
    return target_uids_.empty() ? -1 : target_uids_.front();
}

}  // namespace river
