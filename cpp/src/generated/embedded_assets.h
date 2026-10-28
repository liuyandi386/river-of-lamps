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

// 由 tools/make_font.py 生成的 Sarasa UI SC 子集。
extern const unsigned char kFontRegularData[];
extern const std::size_t kFontRegularSize;
extern const unsigned char kFontHeadingData[];
extern const std::size_t kFontHeadingSize;
extern const unsigned char kFontDisplayData[];
extern const std::size_t kFontDisplaySize;

}  // namespace river::embedded
