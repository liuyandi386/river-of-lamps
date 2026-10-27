#pragma once

#include "core/level_data.h"

#include <filesystem>
#include <nlohmann/json.hpp>
#include <string>
#include <vector>

namespace river {

// 关卡仓库：优先读外部 data 文件，找不到时回退到编译进 exe 的 JSON。
class LevelRepository {
public:
    explicit LevelRepository(const std::filesystem::path& base_dir = {});

    const std::vector<LevelEntry>& entries() const { return entries_; }
    LevelData load_level(const std::string& id) const;
    nlohmann::json elements() const { return elements_; }
    bool external_levels() const { return external_levels_; }
    const std::filesystem::path& levels_dir() const { return levels_dir_; }

private:
    void init(const std::filesystem::path& base_dir);
    static std::filesystem::path find_levels_dir(const std::filesystem::path& base_dir);
    static std::filesystem::path find_elements_file(const std::filesystem::path& base_dir);
    static std::string read_text(const std::filesystem::path& path);
    static nlohmann::json parse_json(const std::string& text);
    static nlohmann::json embedded_level(const std::string& id);

    std::vector<LevelEntry> entries_;
    nlohmann::json elements_;
    std::filesystem::path levels_dir_;
    bool external_levels_ = false;
};

}  // namespace river
