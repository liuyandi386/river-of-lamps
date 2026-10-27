#pragma once

#include <filesystem>
#include <map>
#include <string>
#include <vector>

namespace river {

struct Progress {
    int unlocked = 1;
    std::vector<std::string> completed;
    std::map<std::string, int> best_stars;

    bool is_completed(const std::string& id) const;
    int stars_for(const std::string& id) const;
    void record_stars(const std::string& id, int stars);
};

class ProgressStore {
public:
    static std::filesystem::path default_path();
    static Progress load();
    static void save(const Progress& progress);
    static void reset();
};

}  // namespace river
