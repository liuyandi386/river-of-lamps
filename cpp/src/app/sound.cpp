#include "app/sound.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstring>

#ifdef _WIN32
#include <windows.h>
#include <mmsystem.h>
#endif

namespace river {

namespace {

constexpr int kSampleRate = 22050;
constexpr float kPi = 3.14159265358979323846f;

void write_u16(std::vector<char>& buffer, std::size_t offset, std::uint16_t value) {
    buffer[offset + 0] = static_cast<char>(value & 0xFF);
    buffer[offset + 1] = static_cast<char>((value >> 8) & 0xFF);
}

void write_u32(std::vector<char>& buffer, std::size_t offset, std::uint32_t value) {
    buffer[offset + 0] = static_cast<char>(value & 0xFF);
    buffer[offset + 1] = static_cast<char>((value >> 8) & 0xFF);
    buffer[offset + 2] = static_cast<char>((value >> 16) & 0xFF);
    buffer[offset + 3] = static_cast<char>((value >> 24) & 0xFF);
}

}  // namespace

std::vector<char> SoundPlayer::make_wav(const std::vector<std::pair<float, int>>& notes, float volume) {
    std::vector<std::int16_t> samples;
    for (const auto& [frequency, duration_ms] : notes) {
        const int count = std::max(1, kSampleRate * duration_ms / 1000);
        const int fade = std::max(1, count / 8);
        for (int i = 0; i < count; ++i) {
            const float t = static_cast<float>(i) / static_cast<float>(kSampleRate);
            const float envelope = std::min({1.0f, static_cast<float>(i) / fade,
                                             static_cast<float>(count - i) / fade});
            const float sample = std::sin(2.0f * kPi * frequency * t) * volume * envelope;
            samples.push_back(static_cast<std::int16_t>(sample * 32767.0f));
        }
    }

    const std::uint32_t data_size = static_cast<std::uint32_t>(samples.size() * sizeof(std::int16_t));
    std::vector<char> wav(44 + data_size, 0);
    std::memcpy(wav.data() + 0, "RIFF", 4);
    write_u32(wav, 4, 36 + data_size);
    std::memcpy(wav.data() + 8, "WAVE", 4);
    std::memcpy(wav.data() + 12, "fmt ", 4);
    write_u32(wav, 16, 16);
    write_u16(wav, 20, 1);  // PCM
    write_u16(wav, 22, 1);  // mono
    write_u32(wav, 24, kSampleRate);
    write_u32(wav, 28, kSampleRate * 2);
    write_u16(wav, 32, 2);
    write_u16(wav, 34, 16);
    std::memcpy(wav.data() + 36, "data", 4);
    write_u32(wav, 40, data_size);
    std::memcpy(wav.data() + 44, samples.data(), data_size);
    return wav;
}

SoundPlayer::SoundPlayer()
    : click_(make_wav({{880.0f, 22}}, 0.22f)),
      run_(make_wav({{523.0f, 40}, {784.0f, 55}}, 0.22f)),
      pass_(make_wav({{880.0f, 70}, {1174.0f, 70}, {1568.0f, 110}}, 0.24f)),
      fail_(make_wav({{220.0f, 120}, {164.0f, 170}}, 0.26f)) {}

SoundPlayer::~SoundPlayer() {
#ifdef _WIN32
    PlaySoundA(nullptr, nullptr, 0);
#endif
}

void SoundPlayer::toggle_mute() {
    set_muted(!muted_);
}

void SoundPlayer::set_muted(bool muted) {
    muted_ = muted;
#ifdef _WIN32
    if (muted_) {
        PlaySoundA(nullptr, nullptr, 0);
    }
#endif
}

void SoundPlayer::play(const std::vector<char>& data) {
#ifdef _WIN32
    if (muted_ || data.empty()) {
        return;
    }
    PlaySoundA(reinterpret_cast<LPCSTR>(data.data()), nullptr,
               SND_MEMORY | SND_ASYNC | SND_NODEFAULT);
#else
    (void)data;
#endif
}

void SoundPlayer::play_click() {
    play(click_);
}

void SoundPlayer::play_run() {
    play(run_);
}

void SoundPlayer::play_pass() {
    play(pass_);
}

void SoundPlayer::play_fail() {
    play(fail_);
}

}  // namespace river
