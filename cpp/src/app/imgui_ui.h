#pragma once

#include "app/theme.h"

#include "imgui.h"

#include <SFML/Graphics.hpp>

#include <string>
#include <vector>

namespace river::imgui_ui {

ImU32 color(sf::Color value);
ImVec2 vec(sf::Vector2f value);
ImVec4 vec4(sf::Color value);

void set_fonts(ImFont* body, ImFont* heading, ImFont* display);
ImFont* font_for(float size);

float text_width(const std::string& text, float size);
float text_height(const std::string& text, float size);

void text(ImDrawList* draw, const std::string& value, float size, ImVec2 position,
          sf::Color color);
void text_centered(ImDrawList* draw, const std::string& value, float size,
                   const ImVec2& center, sf::Color color);
void text_right(ImDrawList* draw, const std::string& value, float size, float right,
                float y, sf::Color color);

void panel(ImDrawList* draw, const ImVec2& min, const ImVec2& max, sf::Color fill,
           sf::Color border, float radius = theme::radius_md,
           float thickness = theme::border_thin);
void line(ImDrawList* draw, ImVec2 a, ImVec2 b, sf::Color color, float thickness = 1.f);
void hline(ImDrawList* draw, float x0, float x1, float y, sf::Color color,
           float thickness = 1.f);
void vline(ImDrawList* draw, float x, float y0, float y1, sf::Color color,
           float thickness = 1.f);
void dot(ImDrawList* draw, ImVec2 center, float radius, sf::Color color);
void chip(ImDrawList* draw, const ImVec2& min, const ImVec2& max, sf::Color fill,
          sf::Color border, const std::string& label, float size, sf::Color text_color);
void progress(ImDrawList* draw, const ImVec2& min, const ImVec2& max, float t,
              sf::Color track, sf::Color fill);
void star(ImDrawList* draw, const ImVec2& center, float radius, sf::Color fill,
          sf::Color outline, float thickness = 1.f);
void stars(ImDrawList* draw, float center_x, float center_y, int filled, int total,
           float radius, float spacing, sf::Color fill, sf::Color outline);

std::vector<std::string> wrap(const std::string& value, float size, float max_width);
void wrapped_text(ImDrawList* draw, const std::string& value, float size,
                  const ImVec2& min, float max_width, float max_height, sf::Color color,
                  float line_gap = 1.45f);

}  // namespace river::imgui_ui
