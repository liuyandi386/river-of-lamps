#pragma once

#include "core/circuit_graph.h"

#include <map>
#include <optional>
#include <string>
#include <vector>

namespace river {

// 离散按拍模拟器（与 GDScript 的 Simulator 对齐）。
class Simulator {
public:
    static constexpr int kMaxIters = 16;
    static constexpr int kDefaultBudget = 64;

    void setup(const CircuitGraph& graph, int tick_budget, int max_components = 999,
               int three_star_tick = 0, int two_star_tick = 0, int one_star_tick = 0);
    void reset();

    const TickFrame& step();
    RunResult run_until_done(int max_ticks = 200);

    std::vector<std::string> validation_errors() const;

    std::vector<int> target_actual(int uid) const;
    int first_target_uid() const;

    int tick() const { return tick_; }
    int tick_budget() const { return tick_budget_; }
    bool finished() const { return finished_; }
    bool passed() const { return passed_; }
    int stars() const { return stars_; }
    int completion_tick() const { return completion_tick_; }
    int three_star_tick() const { return three_star_tick_; }
    int two_star_tick() const { return two_star_tick_; }
    int one_star_tick() const { return one_star_tick_; }
    const std::string& fail_reason() const { return fail_reason_; }
    const std::optional<ErrorInfo>& first_error() const { return first_error_; }
    const TickFrame& last_frame() const { return last_frame_; }
    const std::vector<GlitchMark>& glitch_history() const { return glitch_history_; }

private:
    void build_runtime();
    void build_links();
    void link_output_to_cell(int from_uid, const Vec2i& from_cell, const Vec2i& to_cell);
    void add_link(int from_uid, int to_uid, int input_index);
    void build_gate_order();
    void visit_gate(int uid, const std::map<int, bool>& gate_set, std::map<int, int>& state);

    void set_value(int uid, int value);
    int source_value(const Instance& inst, int tick) const;
    int gate_output(const Instance& inst) const;
    int input_or(int uid, int input_index) const;

    CircuitGraph graph_;
    int tick_budget_ = kDefaultBudget;
    int max_components_ = 999;
    int tick_ = -1;
    int three_star_tick_ = 0;
    int two_star_tick_ = 0;
    int one_star_tick_ = 0;

    std::map<int, int> values_;
    std::map<int, std::map<int, int>> pending_;
    std::map<int, std::vector<std::vector<int>>> inputs_by_index_;

    std::vector<int> source_uids_;
    std::vector<int> wire_uids_;
    std::vector<int> gate_uids_;
    std::vector<int> target_uids_;

    std::map<int, std::vector<int>> target_actual_;
    std::map<int, bool> target_matched_;
    std::map<int, int> target_match_tick_;
    std::vector<int> gate_order_;
    bool gate_cycle_ = false;

    bool finished_ = false;
    bool passed_ = false;
    int completion_tick_ = -1;
    int stars_ = 0;
    std::string fail_reason_;
    std::optional<ErrorInfo> first_error_;
    TickFrame last_frame_;
    bool has_frame_ = false;

    std::map<int, int> flips_this_tick_;
    std::vector<int> glitched_this_tick_;
    std::vector<GlitchMark> glitch_history_;
};

}  // namespace river
