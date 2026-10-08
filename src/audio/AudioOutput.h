#pragma once

#include <SDL.h>

#include <cstdint>
#include <atomic>
#include <string>

class AudioOutput {
public:
    static constexpr int kSourceRate = 48000;

    AudioOutput() = default;
    ~AudioOutput();

    bool initialize(std::string& error);
    void shutdown();
    void clear();
    bool push(const std::int16_t* stereo, int frames, std::string& error);
    int availableBytes() const;
    int targetBytes() const;
    int deviceRate() const { return _deviceSpec.freq; }
    int underruns() const { return _underruns.load(); }

private:
    static void callback(void* userdata, Uint8* stream, int len);

    SDL_AudioDeviceID _device = 0;
    SDL_AudioSpec _deviceSpec{};
    SDL_AudioStream* _stream = nullptr;
    std::atomic<int> _underruns{0};
};
