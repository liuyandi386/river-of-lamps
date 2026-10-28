#include "app/settings.h"

#include <nlohmann/json.hpp>

#include <algorithm>
#include <cstdlib>
#include <fstream>

namespace river {

namespace {
constexpr int kMinWidth = 640;
constexpr int kMaxWidth = 7680;
constexpr int kMinHeight = 360;
constexpr int kMaxHeight = 4320;
}  // namespace

bool is_windowed_mode(DisplayMode mode) {
    return mode == DisplayMode::Windowed;
}

const char* display_mode_name(DisplayMode mode) {
    switch (mode) {
        case DisplayMode::Windowed: return "窗口模式";
        case DisplayMode::Borderless: return "无边框全屏";
    }
    return "窗口模式";
}

std::filesystem::path SettingsStore::default_path() {
#ifdef _WIN32
    if (const char* appdata = std::getenv("APPDATA")) {
        return std::filesystem::path(appdata) / "RiverOfLamps" / "settings.json";
    }
#endif
    return std::filesystem::current_path() / "settings.json";
}

AppSettings SettingsStore::load() {
    AppSettings settings;
    const auto path = default_path();
    if (!std::filesystem::exists(path)) {
        return settings;
    }
    std::ifstream file(path, std::ios::binary);
    if (!file) {
        return settings;
    }
    const nlohmann::json data = nlohmann::json::parse(file, nullptr, false);
    if (data.is_discarded() || !data.is_object()) {
        return settings;
    }

    const int mode = data.value("display_mode", 0);
    // 兼容旧 settings.json：旧值 1（独占全屏）和 2（无边框）都迁移到无边框全屏。
    if (mode == 1 || mode == 2) {
        settings.display_mode = DisplayMode::Borderless;
    } else if (mode == 0) {
        settings.display_mode = DisplayMode::Windowed;
    }
    settings.window_width = std::clamp(data.value("window_width", 1280),
                                       kMinWidth, kMaxWidth);
    settings.window_height = std::clamp(data.value("window_height", 720),
                                        kMinHeight, kMaxHeight);
    settings.splash_enabled = data.value("splash_enabled", true);
    return settings;
}

void SettingsStore::save(const AppSettings& settings) {
    const auto path = default_path();
    std::error_code ec;
    std::filesystem::create_directories(path.parent_path(), ec);

    nlohmann::json data;
    data["display_mode"] = static_cast<int>(settings.display_mode);
    data["window_width"] = std::clamp(settings.window_width, kMinWidth, kMaxWidth);
    data["window_height"] = std::clamp(settings.window_height, kMinHeight, kMaxHeight);
    data["splash_enabled"] = settings.splash_enabled;

    std::ofstream file(path, std::ios::binary | std::ios::trunc);
    if (file) {
        file << data.dump(2) << "\n";
    }
}

}  // namespace river
