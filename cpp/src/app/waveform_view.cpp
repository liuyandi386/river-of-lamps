#include "app/waveform_view.h"

#include "app/theme.h"
#include "app/ui_draw.h"

#include <algorithm>
#include <string>

namespace river {

void WaveformView::set_state(const std::vector<int>& expect, int start_tick, int budget,
                             const std::vector<int>& actual, const std::optional<ErrorInfo>& error,
                             const std::vector<GlitchMark>& glitches) {
    expect_ = expect;
    start_tick_ = start_tick;
    budget_ = std::max(1, budget);
    actual_ = actual;
    error_ = error;
    glitches_ = glitches;
}

sf::Color WaveformView::color_for(int value) {
    if (value == 1) return theme::gold;
    if (value == 0) return theme::wire_low;
    return theme::bg_elevated;
}

void WaveformView::draw(sf::RenderTarget& target, const sf::Font& font,
                        const sf::FloatRect& rect) const {
    const float label_w = 46.f;
    const float top = rect.top + 18.f;
    const float row_h = 18.f;
    const float gap = 10.f;
    const float cell_w = std::clamp((rect.width - label_w - 8.f) / static_cast<float>(budget_),
                                    5.f, 14.f);

    ui::draw_text(target, font, "目标", static_cast<unsigned>(theme::text_body), {rect.left, top - 14.f}, theme::text_muted);
    ui::draw_text(target, font, "实际", static_cast<unsigned>(theme::text_body), {rect.left, top + row_h + gap + 1.f},
                  theme::text_muted);

    const float y_exp = top + 6.f;
    const float y_act = y_exp + row_h + gap;

    for (int i = 0; i < budget_; ++i) {
        const float x = rect.left + label_w + i * cell_w;
        int v_exp = -1;
        const int idx = i - start_tick_;
        if (idx >= 0 && idx < static_cast<int>(expect_.size())) {
            v_exp = expect_[idx];
        }
        int v_act = -1;
        if (i < static_cast<int>(actual_.size())) {
            v_act = actual_[i];
        }
        ui::fill_rect(target, {x, y_exp, cell_w - 1.f, row_h}, color_for(v_exp));
        ui::fill_rect(target, {x, y_act, cell_w - 1.f, row_h}, color_for(v_act));
    }

    const float sx = rect.left + label_w + start_tick_ * cell_w;
    ui::draw_line(target, {sx, y_exp - 2.f}, {sx, y_act + row_h + 2.f},
                  sf::Color(theme::gold.r, theme::gold.g, theme::gold.b, 178), 1.f);

    if (error_.has_value() && error_->tick >= 0) {
        const float ex = rect.left + label_w + error_->tick * cell_w;
        ui::outline_rect(target, {ex - 1.f, y_exp - 3.f, cell_w + 1.f,
                                  y_act + row_h - y_exp + 6.f},
                         theme::danger, 2.f);
    }

    for (const GlitchMark& glitch : glitches_) {
        if (glitch.tick < 0) continue;
        const float gx = rect.left + label_w + glitch.tick * cell_w + cell_w * 0.5f;
        ui::fill_circle(target, {gx, y_act + row_h + 9.f}, 3.f, theme::danger);
    }

    const int step = budget_ <= 32 ? 2 : 4;
    for (int i = 0; i < budget_; i += step) {
        const float x = rect.left + label_w + i * cell_w;
        ui::draw_text(target, font, std::to_string(i), static_cast<unsigned>(theme::text_caption), {x, rect.top + rect.height - 15.f},
                      theme::text_dim);
    }
}

}  // namespace river
