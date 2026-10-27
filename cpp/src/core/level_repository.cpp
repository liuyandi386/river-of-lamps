#include "core/level_repository.h"

#include "generated/embedded_assets.h"

#include <algorithm>
#include <fstream>
#include <sstream>

namespace river {

namespace {

std::vector<std::filesystem::path> candidate_bases(const std::filesystem::path& base_dir) {
    std::vector<std::filesystem::path> bases;
    auto add = [&bases](const std::filesystem::path& p) {
        if (p.empty()) {
            return;
        }
        for (const auto& existing : bases) {
            if (existing == p) {
                return;
            }
        }
        bases.push_back(p);
    };

    add(base_dir);
    add(base_dir.parent_path());
    add(std::filesystem::current_path());
    add(std::filesystem::current_path().parent_path());
    return bases;
}

}  // namespace

LevelRepository::LevelRepository(const std::filesystem::path& base_dir) {
    init(base_dir);
}

void LevelRepository::init(const std::filesystem::path& base_dir) {
    entries_.clear();
    elements_ = nlohmann::json::object();
    levels_dir_.clear();
    external_levels_ = false;

    const std::filesystem::path base = base_dir.empty() ? std::filesystem::current_path() : base_dir;
    levels_dir_ = find_levels_dir(base);
    external_levels_ = !levels_dir_.empty();

    nlohmann::json index = nlohmann::json::object();
    if (external_levels_) {
        const auto index_path = levels_dir_ / "index.json";
        if (std::filesystem::exists(index_path)) {
            index = parse_json(read_text(index_path));
        }
    }
    if (!index.is_object() || !index.contains("levels") || !index["levels"].is_array()) {
        index = parse_json(embedded::kIndexJson);
    }
    if (index.is_object() && index.contains("levels") && index["levels"].is_array()) {
        for (const auto& item : index["levels"]) {
            if (!item.is_object()) {
                continue;
            }
            LevelEntry entry;
            entry.id = item.value("id", std::string{});
            entry.name = item.value("name", entry.id);
            if (!entry.id.empty()) {
                entries_.push_back(std::move(entry));
            }
        }
    }

    const auto elements_path = find_elements_file(base);
    if (!elements_path.empty()) {
        elements_ = parse_json(read_text(elements_path));
    }
    if (!elements_.is_object() || !elements_.contains("elements")) {
        elements_ = parse_json(embedded::kElementsJson);
    }
}

std::filesystem::path LevelRepository::find_levels_dir(const std::filesystem::path& base_dir) {
    for (const auto& base : candidate_bases(base_dir)) {
        const std::filesystem::path candidates[] = {
            base / "game" / "data" / "levels",
            base / "data" / "levels",
            base / ".." / "game" / "data" / "levels",
            base / ".." / "data" / "levels",
        };
        for (const auto& candidate : candidates) {
            if (std::filesystem::exists(candidate / "index.json")) {
                return std::filesystem::weakly_canonical(candidate);
            }
        }
    }
    return {};
}

std::filesystem::path LevelRepository::find_elements_file(const std::filesystem::path& base_dir) {
    for (const auto& base : candidate_bases(base_dir)) {
        const std::filesystem::path candidates[] = {
            base / "game" / "data" / "elements" / "elements.json",
            base / "data" / "elements" / "elements.json",
            base / "data" / "levels" / "elements.json",
            base / ".." / "game" / "data" / "elements" / "elements.json",
            base / ".." / "data" / "levels" / "elements.json",
        };
        for (const auto& candidate : candidates) {
            if (std::filesystem::exists(candidate)) {
                return std::filesystem::weakly_canonical(candidate);
            }
        }
    }
    return {};
}

std::string LevelRepository::read_text(const std::filesystem::path& path) {
    std::ifstream file(path, std::ios::binary);
    if (!file) {
        return {};
    }
    std::ostringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}

nlohmann::json LevelRepository::parse_json(const std::string& text) {
    if (text.empty()) {
        return nlohmann::json::object();
    }
    nlohmann::json parsed = nlohmann::json::parse(text, nullptr, false);
    if (parsed.is_discarded()) {
        return nlohmann::json::object();
    }
    return parsed;
}

nlohmann::json LevelRepository::embedded_level(const std::string& id) {
    for (std::size_t i = 0; i < embedded::kLevelCount; ++i) {
        if (id == embedded::kLevels[i].id) {
            return parse_json(embedded::kLevels[i].json);
        }
    }
    return nlohmann::json::object();
}

LevelData LevelRepository::load_level(const std::string& id) const {
    LevelData data;
    data.id = id;

    nlohmann::json raw = nlohmann::json::object();
    if (external_levels_) {
        const auto path = levels_dir_ / (id + ".json");
        if (std::filesystem::exists(path)) {
            raw = parse_json(read_text(path));
        }
    }
    if (!raw.is_object() || raw.empty()) {
        raw = embedded_level(id);
    }
    if (!raw.is_object() || raw.empty()) {
        return data;
    }

    data.raw = raw;
    data.id = raw.value("id", id);
    if (raw.contains("name")) {
        const auto& name = raw["name"];
        if (name.is_object()) {
            data.name = name.value("zh", name.value("en", data.id));
        } else if (name.is_string()) {
            data.name = name.get<std::string>();
        }
    }
    if (data.name.empty()) {
        data.name = data.id;
    }
    data.chapter = raw.value("chapter", std::string{});
    data.tick_budget = raw.value("tick_budget", 24);
    data.max_components = raw.value("max_components", 999);
    data.reference_finish = raw.value("reference_finish", raw.value("reference_ticks", -1));

    auto localized_text = [](const nlohmann::json& value) -> std::string {
        if (value.is_object()) {
            return value.value("zh", value.value("en", std::string{}));
        }
        return value.is_string() ? value.get<std::string>() : std::string{};
    };
    if (raw.contains("teach")) {
        data.teach = localized_text(raw["teach"]);
    }

    if (raw.contains("star_thresholds") && raw["star_thresholds"].is_object()) {
        const auto& stars = raw["star_thresholds"];
        data.stars.three = stars.value("three", 0);
        data.stars.two = stars.value("two", 0);
        data.stars.one = stars.value("one", 0);
        if (data.stars.one <= 0) {
            data.stars.one = data.tick_budget - 1;
        }
        if (data.stars.two <= 0) {
            data.stars.two = data.stars.one;
        }
        if (data.stars.three <= 0) {
            data.stars.three = data.stars.two;
        }
        data.stars.three = std::min(data.stars.three, data.stars.two);
        data.stars.two = std::min(data.stars.two, data.stars.one);
    }

    data.valid = true;
    return data;
}

}  // namespace river
