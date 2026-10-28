#pragma once

#include <SFML/Graphics.hpp>

namespace river::theme {

// 极简扁平色板：低饱和、有限色相，靠明度和描边拉开层级。
inline const sf::Color bg{10, 14, 20};
inline const sf::Color bg_elevated{14, 20, 29};
inline const sf::Color surface{18, 26, 38};
inline const sf::Color surface_alt{22, 31, 45};
inline const sf::Color surface_hover{27, 38, 54};
inline const sf::Color border{36, 50, 70};
inline const sf::Color border_strong{55, 76, 104};

inline const sf::Color text{232, 238, 247};
inline const sf::Color text_muted{147, 163, 184};
inline const sf::Color text_dim{95, 113, 136};

inline const sf::Color gold{230, 184, 92};
inline const sf::Color gold_bright{244, 206, 132};
inline const sf::Color teal{99, 195, 176};
inline const sf::Color blue{108, 155, 201};
inline const sf::Color danger{217, 112, 112};
inline const sf::Color success{121, 201, 154};

inline const sf::Color wire_low{36, 52, 73};
inline const sf::Color wire_high{216, 169, 79};

inline const sf::Color board_bg{12, 18, 27};
inline const sf::Color board_cell{15, 22, 32};
inline const sf::Color board_grid{27, 39, 54};

inline const sf::Color overlay{5, 8, 13, 190};

// 4px 基准栅格。
inline constexpr float space_xs = 4.f;
inline constexpr float space_sm = 8.f;
inline constexpr float space_md = 12.f;
inline constexpr float space_lg = 16.f;
inline constexpr float space_xl = 24.f;
inline constexpr float space_xxl = 32.f;

inline constexpr float radius_sm = 4.f;
inline constexpr float radius_md = 8.f;
inline constexpr float radius_lg = 14.f;

inline constexpr float border_thin = 1.f;
inline constexpr float border_active = 2.f;

// 固定 1280×720 设计坐标下的字号层级；渲染时按窗口缩放。
inline constexpr float text_nano = 8.f;
inline constexpr float text_micro = 9.f;
inline constexpr float text_caption = 10.f;
inline constexpr float text_label = 11.f;
inline constexpr float text_small = 12.f;
inline constexpr float text_body = 13.f;
inline constexpr float text_body_lg = 14.f;
inline constexpr float text_subhead = 15.f;
inline constexpr float text_title = 16.f;
inline constexpr float text_title_lg = 18.f;
inline constexpr float text_h3 = 20.f;
inline constexpr float text_h3_lg = 21.f;
inline constexpr float text_number = 22.f;
inline constexpr float text_h2 = 24.f;
inline constexpr float text_h1 = 32.f;
inline constexpr float text_display = 76.f;

inline constexpr float duration_fast = 0.12f;
inline constexpr float duration_normal = 0.18f;
inline constexpr float duration_slow = 0.28f;

inline float ease_out(float t) {
    if (t <= 0.f) return 0.f;
    if (t >= 1.f) return 1.f;
    const float inv = 1.f - t;
    return 1.f - inv * inv * inv;
}

}  // namespace river::theme
