#pragma once

#include <string>
#include <utility>
#include <vector>

namespace river {

// 轻量占位音效：用 WinMM 播放内存里的 WAV，不引入额外音频依赖。
class SoundPlayer {
public:
    SoundPlayer();
    ~SoundPlayer();

    void toggle_mute();
    void set_muted(bool muted);
    bool muted() const { return muted_; }

    void play_click();
    void play_run();
    void play_pass();
    void play_fail();

private:
    static std::vector<char> make_wav(const std::vector<std::pair<float, int>>& notes, float volume);
    void play(const std::vector<char>& data);

    std::vector<char> click_;
    std::vector<char> run_;
    std::vector<char> pass_;
    std::vector<char> fail_;
    bool muted_ = false;
};

}  // namespace river
