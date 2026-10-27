#pragma once

#include <nlohmann/json.hpp>

#include <string>

namespace river {

struct LevelEntry {
    std::string id;
    std::string name;
};

struct StarThresholds {
    int three = 0;  // finish tick <= three -> 3 stars
    int two = 0;    // finish tick <= two   -> 2 stars
    int one = 0;    // finish tick <= one   -> pass with 1 star

    bool enabled() const { return one > 0; }
};

struct LevelData {
    std::string id;
    std::string name;
    std::string chapter;
    int tick_budget = 24;
    int max_components = 999;
    StarThresholds stars;
    int reference_finish = -1;
    std::string teach;
    nlohmann::json raw;
    bool valid = false;
};

}  // namespace river
