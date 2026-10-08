#pragma once

#include "IMusicBackend.h"

#include <gme/gme.h>

class GmeBackend final : public IMusicBackend {
public:
    static constexpr long kInternalSampleRate = 48000;

    GmeBackend() = default;
    ~GmeBackend() override;

    GmeBackend(const GmeBackend&) = delete;
    GmeBackend& operator=(const GmeBackend&) = delete;

    bool load(const std::vector<std::uint8_t>& data, MusicInfo& info, std::string& error) override;
    bool startTrack(int index, std::string& error) override;
    bool seekMs(long ms, std::string& error) override;
    bool renderStereo(int frames, std::vector<std::int16_t>& out, std::string& error) override;
    bool renderChannels(int frames, std::vector<std::int16_t>& outInterleaved, int& voices, std::string& error) override;
    long positionMs() const override;
    bool ended() const override;
    int voiceCount() const override { return _voiceCount; }

private:
    static std::string text(const char* value);
    void close();

    Music_Emu* _stereo = nullptr;
    Music_Emu* _channels = nullptr;
    gme_type_t _type = nullptr;
    int _voiceCount = 0;
};
