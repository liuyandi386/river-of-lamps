#pragma once

#include <cstddef>

namespace river::embedded {

struct LevelBlob {
    const char* id;
    const char* json;
};

extern const char* const kIndexJson;
extern const char* const kElementsJson;
extern const LevelBlob kLevels[];
extern const std::size_t kLevelCount;

// 由 tools/make_embedded_assets.py 生成的 Noto Sans SC 子集。
extern const unsigned char kFontData[];
extern const std::size_t kFontSize;

}  // namespace river::embedded
