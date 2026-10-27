#pragma once

#include <map>
#include <optional>
#include <string>
#include <vector>

namespace river {

// 网格坐标：左上角为 (0,0)，x 向右、y 向下。
struct Vec2i {
    int x = 0;
    int y = 0;

    constexpr Vec2i() = default;
    constexpr Vec2i(int px, int py) : x(px), y(py) {}

    constexpr Vec2i operator+(const Vec2i& other) const { return {x + other.x, y + other.y}; }
    constexpr Vec2i operator-(const Vec2i& other) const { return {x - other.x, y - other.y}; }
    constexpr Vec2i operator-() const { return {-x, -y}; }
    constexpr bool operator==(const Vec2i& other) const { return x == other.x && y == other.y; }
    constexpr bool operator!=(const Vec2i& other) const { return !(*this == other); }

    // std::map 需要严格弱序。
    constexpr bool operator<(const Vec2i& other) const {
        if (x != other.x) {
            return x < other.x;
        }
        return y < other.y;
    }
};

inline constexpr Vec2i kZero{0, 0};
inline constexpr Vec2i kLeft{-1, 0};
inline constexpr Vec2i kRight{1, 0};
inline constexpr Vec2i kUp{0, -1};
inline constexpr Vec2i kDown{0, 1};

inline int manhattan(const Vec2i& a, const Vec2i& b) {
    const int dx = a.x > b.x ? a.x - b.x : b.x - a.x;
    const int dy = a.y > b.y ? a.y - b.y : b.y - a.y;
    return dx + dy;
}

struct Instance {
    int uid = -1;
    std::string type;  // source / wire / and / or / not / target
    Vec2i origin;
    bool is_preset = false;

    // wire
    Vec2i flow_dir{0, 0};

    // source
    std::string mode = "high";
    int pulse_at_tick = 0;

    // target
    std::vector<int> expect;
    int start_tick = 0;
    std::string id;

    bool is_wire() const { return type == "wire"; }
};

struct ErrorInfo {
    int target_uid = -1;
    int tick = -1;
    int expected = -1;
    int got = -1;
};

struct TickFrame {
    int tick = -1;
    std::map<int, int> values;
    std::vector<int> flipped;
    std::vector<int> glitched;
    std::optional<ErrorInfo> error;
};

struct RunResult {
    bool passed = false;
    std::string reason;
    int ticks = -1;
    int stars = 0;
    int completion_tick = -1;
    std::optional<ErrorInfo> first_error;
};

struct GlitchMark {
    int tick = -1;
    int uid = -1;
};

}  // namespace river
