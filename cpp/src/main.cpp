#include "app/app.h"

#include <filesystem>
#include <string>

#ifdef _WIN32
#include <windows.h>
#endif

int main(int argc, char** argv) {
#ifdef _WIN32
    SetProcessDPIAware();
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
    for (int i = 1; i + 2 < argc; ++i) {
        if (std::string(argv[i]) == "--capture") {
            capture_path = argv[i + 1];
            capture_mode = argv[i + 2];
            i += 2;
        }
    }

    river::App app(exe_dir, capture_path, capture_mode);
    return app.run();
}
