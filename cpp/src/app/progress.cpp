#include "app/progress.h"

#include <nlohmann/json.hpp>

#include <algorithm>
#include <cstdlib>
#include <fstream>

namespace river {

bool Progress::is_completed(const std::string& id) const {
    return std::find(completed.begin(), completed.end(), id) != completed.end() ||
           stars_for(id) > 0;
}

int Progress::stars_for(const std::string& id) const {
    const auto it = best_stars.find(id);
    return it == best_stars.end() ? 0 : std::clamp(it->second, 0, 3);
}

void Progress::record_stars(const std::string& id, int stars) {
    if (id.empty()) {
        return;
    }
    const int clamped = std::clamp(stars, 0, 3);
    auto it = best_stars.find(id);
    if (it == best_stars.end()) {
        best_stars[id] = clamped;
    } else {
        it->second = std::max(it->second, clamped);
    }
}

std::filesystem::path ProgressStore::default_path() {
#ifdef _WIN32
    if (const char* appdata = std::getenv("APPDATA")) {
        return std::filesystem::path(appdata) / "RiverOfLamps" / "progress.json";
    }
#endif
    return std::filesystem::current_path() / "progress.json";
}

Progress ProgressStore::load() {
    Progress progress;
    const auto path = default_path();
    if (!std::filesystem::exists(path)) {
        return progress;
    }
    std::ifstream file(path, std::ios::binary);
    if (!file) {
        return progress;
    }
    const nlohmann::json data = nlohmann::json::parse(file, nullptr, false);
    if (data.is_discarded() || !data.is_object()) {
        return progress;
    }
    progress.unlocked = data.value("unlocked", 1);
    if (progress.unlocked < 1) {
        progress.unlocked = 1;
    }
    if (data.contains("completed") && data["completed"].is_array()) {
        for (const auto& item : data["completed"]) {
            if (item.is_string()) {
                progress.completed.push_back(item.get<std::string>());
            }
        }
    }
    if (data.contains("stars") && data["stars"].is_object()) {
        for (auto it = data["stars"].begin(); it != data["stars"].end(); ++it) {
            if (it.value().is_number_integer()) {
                progress.best_stars[it.key()] = std::clamp(it.value().get<int>(), 0, 3);
            }
        }
    }
    return progress;
}

void ProgressStore::save(const Progress& progress) {
    const auto path = default_path();
    std::error_code ec;
    std::filesystem::create_directories(path.parent_path(), ec);

    nlohmann::json data;
    data["unlocked"] = progress.unlocked;
    data["completed"] = progress.completed;
    nlohmann::json stars = nlohmann::json::object();
    for (const auto& [id, value] : progress.best_stars) {
        stars[id] = std::clamp(value, 0, 3);
    }
    data["stars"] = stars;

    std::ofstream file(path, std::ios::binary | std::ios::trunc);
    if (file) {
        file << data.dump(2) << "\n";
    }
}

void ProgressStore::reset() {
    std::error_code ec;
    std::filesystem::remove(default_path(), ec);
}

}  // namespace river
