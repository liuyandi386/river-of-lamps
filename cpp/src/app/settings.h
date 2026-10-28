#pragma once

#include <filesystem>

namespace river {

enum class DisplayMode {
    Windowed = 0,
    // 无边框窗口全屏。SFML 的 Style::Fullscreen 会触发 ChangeDisplaySettings，
    // 在 Windows 10/11/DWM 下会闪黑屏，因此不再暴露独占全屏模式。
    Borderless = 2,
};

struct AppSettings {
    DisplayMode display_mode = DisplayMode::Windowed;
    int window_width = 1280;
    int window_height = 720;
    bool splash_enabled = true;
};

// 轻量用户设置，独立于关卡进度。
class SettingsStore {
public:
    static std::filesystem::path default_path();
    static AppSettings load();
    static void save(const AppSettings& settings);
};

bool is_windowed_mode(DisplayMode mode);
const char* display_mode_name(DisplayMode mode);

}  // namespace river
