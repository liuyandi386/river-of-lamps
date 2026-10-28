#include "app/app.h"

#include <filesystem>
#include <string>

#ifdef _WIN32
#include <windows.h>
#include <imm.h>
#endif

int main(int argc, char** argv) {
#ifdef _WIN32
    SetProcessDPIAware();
    // 本作没有文本输入框；关闭进程级 IME，避免无边框置顶窗口与候选窗/语言栏抢 Z 序。
    ImmDisableIME(0xFFFFFFFFu);
    char buffer[MAX_PATH] = {};
    const DWORD length = GetModuleFileNameA(nullptr, buffer, MAX_PATH);
    std::filesystem::path exe_dir;
    if (length > 0) {
        exe_dir = std::filesystem::path(buffer).parent_path();
    } else {
        exe_dir = std::filesystem::current_path();
    }
#else
    std::filesystem::path exe_dir = std::filesystem::current_path();
#endif

    std::string capture_path;
    std::string capture_mode;
    int forced_width = 0;
    int forced_height = 0;
    for (int i = 1; i < argc; ++i) {
        const std::string arg = argv[i];
        if (arg == "--capture" && i + 2 < argc) {
            capture_path = argv[i + 1];
            capture_mode = argv[i + 2];
            i += 2;
        } else if (arg == "--size" && i + 1 < argc) {
            const std::string size = argv[i + 1];
            const std::size_t x = size.find('x');
            if (x != std::string::npos) {
                try {
                    forced_width = std::stoi(size.substr(0, x));
                    forced_height = std::stoi(size.substr(x + 1));
                } catch (...) {
                    forced_width = 0;
                    forced_height = 0;
                }
            }
            ++i;
        }
    }

    river::App app(exe_dir, capture_path, capture_mode, forced_width, forced_height);
    return app.run();
}
