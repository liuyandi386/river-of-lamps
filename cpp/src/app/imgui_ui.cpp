#include "app/imgui_ui.h"

#include <algorithm>
#include <cmath>
#include <vector>

namespace river::imgui_ui {

namespace {

ImFont* g_body_font = nullptr;
ImFont* g_heading_font = nullptr;
ImFont* g_display_font = nullptr;

std::size_t utf8_length(unsigned char lead) {
    if ((lead & 0x80U) == 0U) return 1;
    if ((lead & 0xE0U) == 0xC0U) return 2;
    if ((lead & 0xF0U) == 0xE0U) return 3;
    if ((lead & 0xF8U) == 0xF0U) return 4;
    return 1;
}

}  // namespace

void set_fonts(ImFont* body, ImFont* heading, ImFont* display) {
    g_body_font = body;
    g_heading_font = heading;
    g_display_font = display;
}

ImFont* font_for(float size) {
    if (size >= 60.f && g_display_font != nullptr) return g_display_font;
    if (size >= 22.f && g_heading_font != nullptr) return g_heading_font;
    if (g_body_font != nullptr) return g_body_font;
    return ImGui::GetFont();
}

ImU32 color(sf::Color value) {
    return IM_COL32(value.r, value.g, value.b, value.a);
}

ImVec2 vec(sf::Vector2f value) {
    return ImVec2(value.x, value.y);
}

ImVec4 vec4(sf::Color value) {
    return ImVec4(value.r / 255.f, value.g / 255.f, value.b / 255.f, value.a / 255.f);
}

float text_width(const std::string& text, float size) {
    if (text.empty()) return 0.f;
    const ImFont* font = font_for(size);
    if (font == nullptr) return 0.f;
    return font->CalcTextSizeA(size, FLT_MAX, 0.f, text.c_str()).x;
}

float text_height(const std::string& text, float size) {
    if (text.empty()) return size;
    const ImFont* font = font_for(size);
    if (font == nullptr) return size;
    return font->CalcTextSizeA(size, FLT_MAX, 0.f, text.c_str()).y;
}

void text(ImDrawList* draw, const std::string& value, float size, ImVec2 position,
          sf::Color color_value) {
    if (draw == nullptr || value.empty()) return;
    draw->AddText(font_for(size), size, position, color(color_value), value.c_str());
}

void text_centered(ImDrawList* draw, const std::string& value, float size,
                   const ImVec2& center, sf::Color color_value) {
    if (value.empty()) return;
    text(draw, value, size,
         {center.x - text_width(value, size) * 0.5f,
          center.y - text_height(value, size) * 0.5f},
         color_value);
}

void text_right(ImDrawList* draw, const std::string& value, float size, float right,
                float y, sf::Color color_value) {
    text(draw, value, size, {right - text_width(value, size), y}, color_value);
}

void panel(ImDrawList* draw, const ImVec2& min, const ImVec2& max, sf::Color fill,
           sf::Color border_value, float radius, float thickness) {
    if (draw == nullptr) return;
    draw->AddRectFilled(min, max, color(fill), radius, ImDrawFlags_RoundCornersAll);
    if (border_value.a > 0) {
        draw->AddRect(min, max, color(border_value), radius, ImDrawFlags_RoundCornersAll,
                      thickness);
    }
}

void line(ImDrawList* draw, ImVec2 a, ImVec2 b, sf::Color color_value, float thickness) {
    if (draw == nullptr) return;
    draw->AddLine(a, b, color(color_value), thickness);
}

void hline(ImDrawList* draw, float x0, float x1, float y, sf::Color color_value,
           float thickness) {
    line(draw, {x0, y}, {x1, y}, color_value, thickness);
}

void vline(ImDrawList* draw, float x, float y0, float y1, sf::Color color_value,
           float thickness) {
    line(draw, {x, y0}, {x, y1}, color_value, thickness);
}

void dot(ImDrawList* draw, ImVec2 center, float radius, sf::Color color_value) {
    if (draw == nullptr) return;
    draw->AddCircleFilled(center, radius, color(color_value), 24);
}

void chip(ImDrawList* draw, const ImVec2& min, const ImVec2& max, sf::Color fill,
          sf::Color border_value, const std::string& label, float size,
          sf::Color text_color) {
    panel(draw, min, max, fill, border_value, theme::radius_sm, 1.f);
    const ImVec2 center{(min.x + max.x) * 0.5f, (min.y + max.y) * 0.5f};
    text_centered(draw, label, size, center, text_color);
}

void progress(ImDrawList* draw, const ImVec2& min, const ImVec2& max, float t,
              sf::Color track, sf::Color fill) {
    if (draw == nullptr) return;
    t = std::clamp(t, 0.f, 1.f);
    draw->AddRectFilled(min, max, color(track), 2.f, ImDrawFlags_RoundCornersAll);
    if (t > 0.f) {
        const ImVec2 end{min.x + (max.x - min.x) * t, max.y};
        draw->AddRectFilled(min, end, color(fill), 2.f, ImDrawFlags_RoundCornersAll);
    }
}

void star(ImDrawList* draw, const ImVec2& center, float radius, sf::Color fill,
          sf::Color outline, float thickness) {
    if (draw == nullptr || radius <= 0.f) return;
    ImVec2 points[10];
    for (int i = 0; i < 10; ++i) {
        const float angle = -1.57079633f + static_cast<float>(i) * 0.62831853f;
        const float r = (i % 2 == 0) ? radius : radius * 0.45f;
        points[i] = {center.x + std::cos(angle) * r, center.y + std::sin(angle) * r};
    }
    draw->AddConvexPolyFilled(points, 10, color(fill));
    draw->AddPolyline(points, 10, color(outline), ImDrawFlags_Closed, thickness);
}

void stars(ImDrawList* draw, float center_x, float center_y, int filled, int total,
           float radius, float spacing, sf::Color fill, sf::Color outline) {
    if (total <= 0) return;
    const float start = center_x - (total - 1) * spacing * 0.5f;
    for (int i = 0; i < total; ++i) {
        star(draw, {start + i * spacing, center_y}, radius,
             i < filled ? fill : sf::Color(24, 33, 47),
             i < filled ? fill : outline, 1.f);
    }
}

std::vector<std::string> wrap(const std::string& value, float size, float max_width) {
    std::vector<std::string> lines;
    if (value.empty()) return lines;
    const ImFont* font = font_for(size);
    std::string line;
    for (std::size_t i = 0; i < value.size();) {
        const unsigned char lead = static_cast<unsigned char>(value[i]);
        const std::size_t length = std::min<std::size_t>(utf8_length(lead), value.size() - i);
        const std::string ch = value.substr(i, length);
        i += length;
        if (ch == "\n") {
            lines.push_back(line);
            line.clear();
            continue;
        }
        const std::string candidate = line + ch;
        const float width = font == nullptr
                                ? 0.f
                                : font->CalcTextSizeA(size, FLT_MAX, 0.f, candidate.c_str()).x;
        if (!line.empty() && width > max_width) {
            lines.push_back(line);
            line = ch;
        } else {
            line = candidate;
        }
    }
    if (!line.empty()) lines.push_back(line);
    return lines;
}

void wrapped_text(ImDrawList* draw, const std::string& value, float size,
                  const ImVec2& min, float max_width, float max_height, sf::Color color_value,
                  float line_gap) {
    if (draw == nullptr || value.empty()) return;
    const std::vector<std::string> lines = wrap(value, size, max_width);
    const float line_height = size * line_gap;
    float y = min.y;
    for (const std::string& line : lines) {
        if (y + line_height > min.y + max_height + 0.5f) break;
        text(draw, line, size, {min.x, y}, color_value);
        y += line_height;
    }
}

}  // namespace river::imgui_ui
