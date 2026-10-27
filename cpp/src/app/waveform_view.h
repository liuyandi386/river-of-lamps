#pragma once

#include "core/types.h"

#include <SFML/Graphics.hpp>

#include <optional>
#include <vector>

namespace river {

// 最小波形面板：上排目标序列、下排实际序列。
class WaveformView {
public:
    void set_state(const std::vector<int>& expect, int start_tick, int budget,
                   const std::vector<int>& actual, const std::optional<ErrorInfo>& error,
                   const std::vector<GlitchMark>& glitches);

    void draw(sf::RenderTarget& target, const sf::Font& font, const sf::FloatRect& rect) const;

private:
    static sf::Color color_for(int value);

    std::vector<int> expect_;
    int start_tick_ = 0;
    int budget_ = 24;
    std::vector<int> actual_;
    std::optional<ErrorInfo> error_;
    std::vector<GlitchMark> glitches_;
};

}  // namespace river
