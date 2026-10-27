#pragma once

#include <SFML/Graphics.hpp>

#include <string>
#include <vector>

namespace river::ui {

sf::String utf8(const std::string& text);
sf::Text make_text(const sf::Font& font, const std::string& text, unsigned size, sf::Color color);

void fill_rect(sf::RenderTarget& target, const sf::FloatRect& rect, sf::Color color);
void outline_rect(sf::RenderTarget& target, const sf::FloatRect& rect, sf::Color color, float thickness = 1.f);
void fill_circle(sf::RenderTarget& target, const sf::Vector2f& center, float radius, sf::Color color);
void draw_line(sf::RenderTarget& target, const sf::Vector2f& a, const sf::Vector2f& b,
               sf::Color color, float thickness = 1.f);

void draw_rounded_rect(sf::RenderTarget& target, const sf::FloatRect& rect, float radius,
                       sf::Color fill, sf::Color outline, float thickness = 1.f);

void draw_text(sf::RenderTarget& target, const sf::Font& font, const std::string& text,
               unsigned size, const sf::Vector2f& position, sf::Color color);
void draw_text_centered(sf::RenderTarget& target, const sf::Font& font, const std::string& text,
                        unsigned size, const sf::FloatRect& rect, sf::Color color);
void draw_star(sf::RenderTarget& target, const sf::Vector2f& center, float radius,
               sf::Color fill, sf::Color outline, float thickness = 1.f);
void draw_star_row(sf::RenderTarget& target, float center_x, float center_y, int filled,
                   int total, float radius, float spacing, sf::Color fill, sf::Color outline);
std::vector<std::string> wrap_text(const sf::Font& font, const std::string& text,
                                   unsigned size, float max_width);
void draw_wrapped_text(sf::RenderTarget& target, const sf::Font& font, const std::string& text,
                       unsigned size, const sf::FloatRect& rect, sf::Color color,
                       float line_spacing = 1.45f);

sf::Color with_alpha(sf::Color color, std::uint8_t alpha);
sf::Color mix(sf::Color a, sf::Color b, float t);

std::vector<std::string> split_utf8(const std::string& text);

}  // namespace river::ui
