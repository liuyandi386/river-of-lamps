#include "app/ui_draw.h"

#include <algorithm>
#include <cmath>
#include <cstdint>

namespace river::ui {

sf::String utf8(const std::string& text) {
    return sf::String::fromUtf8(text.begin(), text.end());
}

sf::Text make_text(const sf::Font& font, const std::string& text, unsigned size, sf::Color color) {
    sf::Text result;
    result.setFont(font);
    result.setString(utf8(text));
    result.setCharacterSize(size);
    result.setFillColor(color);
    return result;
}

void fill_rect(sf::RenderTarget& target, const sf::FloatRect& rect, sf::Color color) {
    if (rect.width <= 0.f || rect.height <= 0.f) {
        return;
    }
    sf::RectangleShape shape({rect.width, rect.height});
    shape.setPosition({rect.left, rect.top});
    shape.setFillColor(color);
    target.draw(shape);
}

void outline_rect(sf::RenderTarget& target, const sf::FloatRect& rect, sf::Color color, float thickness) {
    if (rect.width <= 0.f || rect.height <= 0.f) {
        return;
    }
    sf::RectangleShape shape({rect.width, rect.height});
    shape.setPosition({rect.left, rect.top});
    shape.setFillColor(sf::Color::Transparent);
    shape.setOutlineColor(color);
    shape.setOutlineThickness(thickness);
    target.draw(shape);
}

void fill_circle(sf::RenderTarget& target, const sf::Vector2f& center, float radius, sf::Color color) {
    if (radius <= 0.f) {
        return;
    }
    sf::CircleShape shape(radius);
    shape.setOrigin({radius, radius});
    shape.setPosition(center);
    shape.setFillColor(color);
    target.draw(shape);
}

void draw_line(sf::RenderTarget& target, const sf::Vector2f& a, const sf::Vector2f& b,
               sf::Color color, float thickness) {
    const sf::Vector2f delta = b - a;
    const float length = std::sqrt(delta.x * delta.x + delta.y * delta.y);
    if (length <= 0.0001f) {
        return;
    }
    const sf::Vector2f normal{-delta.y / length * thickness * 0.5f,
                              delta.x / length * thickness * 0.5f};
    sf::VertexArray quad(sf::Quads, 4);
    quad[0] = sf::Vertex(a + normal, color);
    quad[1] = sf::Vertex(b + normal, color);
    quad[2] = sf::Vertex(b - normal, color);
    quad[3] = sf::Vertex(a - normal, color);
    target.draw(quad);
}

void draw_rounded_rect(sf::RenderTarget& target, const sf::FloatRect& rect, float radius,
                       sf::Color fill, sf::Color outline, float thickness) {
    if (rect.width <= 0.f || rect.height <= 0.f) {
        return;
    }
    const float r = std::min({radius, rect.width * 0.5f, rect.height * 0.5f});
    if (r <= 0.5f) {
        fill_rect(target, rect, fill);
        if (outline.a > 0) {
            outline_rect(target, rect, outline, thickness);
        }
        return;
    }

    constexpr int segments = 6;
    std::vector<sf::Vector2f> points;
    points.reserve(segments * 4 + 4);
    const sf::Vector2f centers[4] = {
        {rect.left + r, rect.top + r},
        {rect.left + rect.width - r, rect.top + r},
        {rect.left + rect.width - r, rect.top + rect.height - r},
        {rect.left + r, rect.top + rect.height - r},
    };
    const float start_angles[4] = {3.14159265f, 4.71238898f, 0.f, 1.57079633f};
    for (int corner = 0; corner < 4; ++corner) {
        for (int i = 0; i <= segments; ++i) {
            const float angle = start_angles[corner] + (1.57079633f * static_cast<float>(i) / segments);
            points.push_back({centers[corner].x + std::cos(angle) * r,
                              centers[corner].y + std::sin(angle) * r});
        }
    }

    sf::ConvexShape shape;
    shape.setPointCount(points.size());
    for (std::size_t i = 0; i < points.size(); ++i) {
        shape.setPoint(i, points[i]);
    }
    shape.setFillColor(fill);
    shape.setOutlineColor(outline);
    shape.setOutlineThickness(thickness);
    target.draw(shape);
}

void draw_text(sf::RenderTarget& target, const sf::Font& font, const std::string& text,
               unsigned size, const sf::Vector2f& position, sf::Color color) {
    if (text.empty()) {
        return;
    }
    sf::Text value = make_text(font, text, size, color);
    value.setPosition(position);
    target.draw(value);
}

void draw_text_centered(sf::RenderTarget& target, const sf::Font& font, const std::string& text,
                        unsigned size, const sf::FloatRect& rect, sf::Color color) {
    if (text.empty()) {
        return;
    }
    sf::Text value = make_text(font, text, size, color);
    const sf::FloatRect bounds = value.getLocalBounds();
    value.setPosition({rect.left + (rect.width - bounds.width) * 0.5f - bounds.left,
                       rect.top + (rect.height - bounds.height) * 0.5f - bounds.top});
    target.draw(value);
}

void draw_star(sf::RenderTarget& target, const sf::Vector2f& center, float radius,
               sf::Color fill, sf::Color outline, float thickness) {
    sf::ConvexShape star;
    star.setPointCount(10);
    for (int i = 0; i < 10; ++i) {
        const float angle = -1.57079633f + static_cast<float>(i) * 0.62831853f;
        const float r = (i % 2 == 0) ? radius : radius * 0.45f;
        star.setPoint(static_cast<std::size_t>(i),
                      {center.x + std::cos(angle) * r, center.y + std::sin(angle) * r});
    }
    star.setFillColor(fill);
    star.setOutlineColor(outline);
    star.setOutlineThickness(thickness);
    target.draw(star);
}

void draw_star_row(sf::RenderTarget& target, float center_x, float center_y, int filled,
                   int total, float radius, float spacing, sf::Color fill, sf::Color outline) {
    if (total <= 0) {
        return;
    }
    const float start = center_x - (total - 1) * spacing * 0.5f;
    for (int i = 0; i < total; ++i) {
        draw_star(target, {start + i * spacing, center_y}, radius,
                  i < filled ? fill : sf::Color(30, 38, 56),
                  i < filled ? fill : outline, 1.f);
    }
}

std::vector<std::string> split_utf8(const std::string& text) {
    std::vector<std::string> result;
    for (std::size_t i = 0; i < text.size();) {
        const unsigned char c = static_cast<unsigned char>(text[i]);
        std::size_t length = 1;
        if ((c & 0x80U) == 0U) {
            length = 1;
        } else if ((c & 0xE0U) == 0xC0U) {
            length = 2;
        } else if ((c & 0xF0U) == 0xE0U) {
            length = 3;
        } else if ((c & 0xF8U) == 0xF0U) {
            length = 4;
        }
        length = std::min(length, text.size() - i);
        result.push_back(text.substr(i, length));
        i += length;
    }
    return result;
}

std::vector<std::string> wrap_text(const sf::Font& font, const std::string& text,
                                   unsigned size, float max_width) {
    std::vector<std::string> lines;
    if (text.empty()) {
        return lines;
    }
    std::string line;
    for (const auto& ch : split_utf8(text)) {
        if (ch == "\n") {
            lines.push_back(line);
            line.clear();
            continue;
        }
        std::string candidate = line + ch;
        sf::Text measure = make_text(font, candidate, size, sf::Color::White);
        if (!line.empty() && measure.getLocalBounds().width > max_width) {
            lines.push_back(line);
            line = ch;
        } else {
            line = candidate;
        }
    }
    if (!line.empty()) {
        lines.push_back(line);
    }
    return lines;
}

void draw_wrapped_text(sf::RenderTarget& target, const sf::Font& font, const std::string& text,
                       unsigned size, const sf::FloatRect& rect, sf::Color color,
                       float line_spacing) {
    const auto lines = wrap_text(font, text, size, rect.width);
    const float line_height = static_cast<float>(size) * line_spacing;
    float y = rect.top;
    for (const auto& line : lines) {
        if (y + line_height > rect.top + rect.height + 0.5f) {
            break;
        }
        draw_text(target, font, line, size, {rect.left, y}, color);
        y += line_height;
    }
}

sf::Color with_alpha(sf::Color color, std::uint8_t alpha) {
    color.a = alpha;
    return color;
}

sf::Color mix(sf::Color a, sf::Color b, float t) {
    t = std::clamp(t, 0.f, 1.f);
    return sf::Color(
        static_cast<std::uint8_t>(a.r + (b.r - a.r) * t),
        static_cast<std::uint8_t>(a.g + (b.g - a.g) * t),
        static_cast<std::uint8_t>(a.b + (b.b - a.b) * t),
        static_cast<std::uint8_t>(a.a + (b.a - a.a) * t));
}

}  // namespace river::ui
