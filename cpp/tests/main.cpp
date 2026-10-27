// River of Lamps：C++ 逻辑测试。
// 迁移自旧 tests/run_tests.gd 的 40 条断言，并额外校验星级窗口、边界和进度存档。
// 构建后直接运行：build\river_tests.exe

#include "app/progress.h"
#include "core/circuit_graph.h"
#include "core/level_builder.h"
#include "core/level_repository.h"
#include "core/simulator.h"

#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <string>
#include <vector>

#ifdef _WIN32
#include <windows.h>
#endif

using namespace river;

namespace {

int g_failures = 0;

void ok(bool condition, const std::string& message) {
    if (condition) {
        std::cout << "  ok   - " << message << "\n";
    } else {
        ++g_failures;
        std::cerr << "  FAIL - " << message << "\n";
    }
}

CircuitGraph make_graph(int w = 8, int h = 8) {
    CircuitGraph graph;
    graph.setup(w, h);
    return graph;
}

int add_source(CircuitGraph& graph, const Vec2i& cell, const std::string& mode = "high", int pulse_at = 0) {
    const int uid = graph.add_instance("source", cell, true);
    graph.instances[uid].mode = mode;
    graph.instances[uid].pulse_at_tick = pulse_at;
    return uid;
}

int add_wire(CircuitGraph& graph, const Vec2i& cell, const Vec2i& dir) {
    return graph.add_wire(cell, dir);
}

int add_gate(CircuitGraph& graph, const Vec2i& cell, const std::string& type) {
    return graph.add_instance(type, cell, true);
}

int add_target(CircuitGraph& graph, const Vec2i& cell, const std::vector<int>& expect, int start) {
    const int uid = graph.add_instance("target", cell, true);
    graph.instances[uid].expect = expect;
    graph.instances[uid].start_tick = start;
    return uid;
}

Simulator make_sim(const CircuitGraph& graph, int budget = 20, int three = 0, int two = 0,
                   int one = 0) {
    Simulator sim;
    sim.setup(graph, budget, 999, three, two, one);
    return sim;
}

void test_wire_delay() {
    CircuitGraph graph = make_graph(8, 1);
    add_source(graph, {0, 0}, "high");
    for (int x = 1; x <= 5; ++x) {
        add_wire(graph, {x, 0}, kRight);
    }
    add_target(graph, {6, 0}, {1}, 5);

    Simulator sim = make_sim(graph);
    ok(sim.validation_errors().empty(), "连线校验通过");
    const RunResult result = sim.run_until_done(100);
    ok(result.passed, "5 段灯槽后目标在正确拍收到高电平");
    ok(result.ticks == 5, "通过拍数应为 5，实际 " + std::to_string(result.ticks));
}

void test_not_pulse() {
    CircuitGraph graph = make_graph(8, 1);
    add_source(graph, {0, 0}, "pulse");
    for (int x = 1; x <= 3; ++x) {
        add_wire(graph, {x, 0}, kRight);
    }
    add_gate(graph, {4, 0}, "not");
    add_target(graph, {5, 0}, {0, 1}, 3);

    Simulator sim = make_sim(graph);
    ok(sim.validation_errors().empty(), "非门输入已连接");
    const RunResult result = sim.run_until_done(100);
    ok(result.passed, "脉冲经过非门后，目标先灭后亮");
    ok(result.ticks == 4, "通过拍数应为 4，实际 " + std::to_string(result.ticks));
}

void test_and_alignment() {
    CircuitGraph graph = make_graph(8, 8);
    add_source(graph, {0, 4}, "pulse");
    for (int x = 1; x <= 4; ++x) {
        add_wire(graph, {x, 4}, kRight);
    }
    add_gate(graph, {5, 4}, "and");

    add_source(graph, {5, 7}, "pulse");
    add_wire(graph, {6, 7}, kUp);
    add_wire(graph, {6, 6}, kUp);
    add_wire(graph, {6, 5}, kLeft);
    add_wire(graph, {5, 5}, kUp);

    add_target(graph, {6, 4}, {1}, 4);

    Simulator sim = make_sim(graph);
    ok(sim.validation_errors().empty(), "与门两个输入都已连接");
    const RunResult result = sim.run_until_done(100);
    ok(result.passed, "两条 4 拍路径同时到达，与门输出高电平");
    ok(result.ticks == 4, "通过拍数应为 4，实际 " + std::to_string(result.ticks));
}

void test_or_sequence() {
    CircuitGraph graph = make_graph(8, 8);
    add_source(graph, {0, 4}, "pulse");
    for (int x = 1; x <= 4; ++x) {
        add_wire(graph, {x, 4}, kRight);
    }
    add_gate(graph, {5, 4}, "or");

    add_source(graph, {5, 7}, "pulse");
    add_wire(graph, {6, 7}, kRight);
    add_wire(graph, {7, 7}, kUp);
    add_wire(graph, {7, 6}, kUp);
    add_wire(graph, {7, 5}, kLeft);
    add_wire(graph, {6, 5}, kLeft);
    add_wire(graph, {5, 5}, kUp);

    add_target(graph, {6, 4}, {1, 0, 1}, 4);

    Simulator sim = make_sim(graph);
    ok(sim.validation_errors().empty(), "或门两个输入都已连接");
    const RunResult result = sim.run_until_done(100);
    ok(result.passed, "两路相差 2 拍，目标看到 亮-灭-亮");
    ok(result.ticks == 6, "通过拍数应为 6，实际 " + std::to_string(result.ticks));
    const std::vector<int> actual = sim.target_actual(sim.first_target_uid());
    ok(actual.size() >= 7, "实际序列至少记录到第 6 拍");
    if (actual.size() >= 7) {
        ok(actual[4] == 1 && actual[5] == 0 && actual[6] == 1,
           "第 4/5/6 拍实际为 1/0/1");
    }
}

void test_first_error() {
    CircuitGraph graph = make_graph(8, 1);
    add_source(graph, {0, 0}, "high");
    add_target(graph, {1, 0}, {0}, 0);

    Simulator sim = make_sim(graph);
    const RunResult result = sim.run_until_done(100);
    ok(!result.passed, "期望低但收到高，应失败");
    ok(result.reason == "mismatch", "失败原因应为 mismatch，实际 " + result.reason);
    ok(result.ticks == 0, "首错拍应为 0，实际 " + std::to_string(result.ticks));
    if (result.first_error.has_value()) {
        ok(result.first_error->expected == 0, "错误期望值应为 0");
        ok(result.first_error->got == 1, "错误实际值应为 1");
    } else {
        ok(false, "错误期望值应为 0");
        ok(false, "错误实际值应为 1");
    }
}

void test_gate_chain() {
    CircuitGraph graph = make_graph(8, 1);
    add_source(graph, {0, 0}, "low");
    add_gate(graph, {1, 0}, "not");
    add_gate(graph, {2, 0}, "not");
    add_target(graph, {3, 0}, {0}, 0);

    Simulator sim = make_sim(graph);
    ok(sim.validation_errors().empty(), "门链输入已连接");
    const RunResult result = sim.run_until_done(100);
    ok(result.passed, "低 -> 非 -> 非 = 低");
    ok(result.ticks == 0, "门是零延迟，应在第 0 拍通过，实际 " + std::to_string(result.ticks));
    ok(sim.last_frame().glitched.empty(), "普通门链不应被误判为毛刺");
}

void test_validation() {
    CircuitGraph graph = make_graph(8, 1);
    add_source(graph, {0, 0}, "high");
    add_target(graph, {2, 0}, {1}, 0);
    Simulator sim = make_sim(graph);
    const std::vector<std::string> errors = sim.validation_errors();
    ok(!errors.empty(), "目标没有连着灯槽时应报错");
    ok(!errors.empty() && errors[0].find("目标") != std::string::npos,
       "错误信息应提到目标端口");
}

void test_levels() {
    LevelRepository repo;
    const std::vector<std::string> ids = {
        "level_01", "level_02", "level_03", "level_04", "level_05",
    };
    const std::vector<int> expected_ticks = {6, 3, 4, 4, 6};

    for (std::size_t i = 0; i < ids.size(); ++i) {
        const std::string& id = ids[i];
        const LevelData level = repo.load_level(id);
        ok(level.valid, id + " 能加载");
        if (!level.valid) {
            continue;
        }
        CircuitGraph graph = LevelBuilder::build(level.raw, true);
        Simulator sim;
        sim.setup(graph, level.tick_budget, level.max_components,
                  level.stars.three, level.stars.two, level.stars.one);
        const std::vector<std::string> errors = sim.validation_errors();
        std::string why = "OK";
        if (!errors.empty()) {
            why.clear();
            for (const auto& error : errors) {
                if (!why.empty()) {
                    why += "; ";
                }
                why += error;
            }
        }
        ok(errors.empty(), id + " 参考解连线合法（" + why + "）");
        if (!errors.empty()) {
            continue;
        }
        const RunResult result = sim.run_until_done(100);
        ok(result.passed, id + " 参考解能通关（reason=" + result.reason +
                                   " tick=" + std::to_string(result.ticks) + "）");
        ok(result.ticks == expected_ticks[i],
           id + " 通关拍数应为 " + std::to_string(expected_ticks[i]) +
               "，实际 " + std::to_string(result.ticks));
        ok(result.stars == 3, id + " 参考解应拿 3 星，实际 " +
                                   std::to_string(result.stars) + " 星");
        ok(result.completion_tick == level.reference_finish,
           id + " 完成拍应等于 JSON reference_finish=" +
               std::to_string(level.reference_finish) + "，实际 " +
               std::to_string(result.completion_tick));
    }
}

CircuitGraph make_star_path(int wire_count, const std::vector<int>& expect, int start) {
    CircuitGraph graph = make_graph(12, 1);
    add_source(graph, {0, 0}, "high");
    for (int x = 1; x <= wire_count; ++x) {
        add_wire(graph, {x, 0}, kRight);
    }
    add_target(graph, {wire_count + 1, 0}, expect, start);
    return graph;
}

void test_star_boundaries() {
    // 5 格灯槽：第 5 拍结束，刚好 3 星。
    {
        CircuitGraph graph = make_star_path(5, {1}, 5);
        Simulator sim = make_sim(graph, 20, 5, 7, 10);
        const RunResult result = sim.run_until_done(100);
        ok(result.passed && result.stars == 3 && result.completion_tick == 5,
           "刚好 3 星：完成拍=5，星级=3");
    }
    // 6 格灯槽：第 6 拍结束；three=5，不能 3 星；two=6，刚好 2 星。
    {
        CircuitGraph graph = make_star_path(6, {1}, 5);
        Simulator sim = make_sim(graph, 20, 5, 6, 9);
        const RunResult result = sim.run_until_done(100);
        ok(result.passed && result.stars == 2 && result.completion_tick == 6,
           "刚好 2 星：完成拍=6，星级=2");
    }
    // 7 格灯槽：第 7 拍结束；超过 two=6，落在 one=9 内，拿 1 星。
    {
        CircuitGraph graph = make_star_path(7, {1}, 5);
        Simulator sim = make_sim(graph, 20, 5, 6, 9);
        const RunResult result = sim.run_until_done(100);
        ok(result.passed && result.stars == 1 && result.completion_tick == 7,
           "超过 2 星线仍在窗口内：完成拍=7，星级=1");
    }
    // 目标永远不亮：窗口结束应失败，并保留首错拍。
    {
        CircuitGraph graph = make_graph(12, 1);
        add_source(graph, {0, 0}, "low");
        for (int x = 1; x <= 5; ++x) {
            add_wire(graph, {x, 0}, kRight);
        }
        add_target(graph, {6, 0}, {1}, 5);
        Simulator sim = make_sim(graph, 20, 5, 7, 8);
        const RunResult result = sim.run_until_done(100);
        ok(!result.passed, "窗口内没有匹配应失败");
        ok(result.stars == 0, "失败时星级应为 0");
        ok(result.first_error.has_value() && result.first_error->tick == 5,
           "失败应保留第 5 拍的首错信息");
    }
}

void test_progress_stars() {
#ifdef _WIN32
    const char* original = std::getenv("APPDATA");
    const std::string original_value = original == nullptr ? std::string{} : std::string{original};
    const std::filesystem::path temp =
        std::filesystem::temp_directory_path() / "river_progress_test";
    std::filesystem::remove_all(temp);
    _putenv_s("APPDATA", temp.string().c_str());

    Progress progress;
    progress.record_stars("level_01", 2);
    progress.record_stars("level_01", 3);
    progress.record_stars("level_02", 1);
    ok(progress.stars_for("level_01") == 3, "进度应保留最高星级（3 星）");
    ok(progress.stars_for("level_02") == 1, "进度应记录不同关卡的星级");

    ProgressStore::save(progress);
    const Progress loaded = ProgressStore::load();
    ok(loaded.stars_for("level_01") == 3, "存档/读档后 3 星应保留");
    ok(loaded.is_completed("level_02"), "有星级的关卡应视为已通过");

    ProgressStore::reset();
    ok(!std::filesystem::exists(ProgressStore::default_path()), "重置进度应删除存档文件");

    std::filesystem::remove_all(temp);
    _putenv_s("APPDATA", original_value.c_str());
#else
    ok(true, "进度存档测试只在 Windows 上运行");
#endif
}

}  // namespace

int main() {
#ifdef _WIN32
    SetConsoleOutputCP(CP_UTF8);
#endif
    std::cout << "== 1. 灯槽延迟 ==\n";
    test_wire_delay();
    std::cout << "== 2. 非门 + 脉冲 ==\n";
    test_not_pulse();
    std::cout << "== 3. 与门对齐 ==\n";
    test_and_alignment();
    std::cout << "== 4. 或门序列 ==\n";
    test_or_sequence();
    std::cout << "== 5. 首错拍 ==\n";
    test_first_error();
    std::cout << "== 6. 零延迟门链 ==\n";
    test_gate_chain();
    std::cout << "== 7. 连线校验 ==\n";
    test_validation();
    std::cout << "== 8. 5 关参考解 ==\n";
    test_levels();
    std::cout << "== 9. 星级边界 ==\n";
    test_star_boundaries();
    std::cout << "== 10. 星级进度存档 ==\n";
    test_progress_stars();

    if (g_failures == 0) {
        std::cout << "\nALL TESTS PASSED（40 条原断言 + 星级窗口/边界/进度断言）\n";
        return 0;
    }
    std::cout << "\nTESTS FAILED: " << g_failures << "\n";
    return 1;
}
