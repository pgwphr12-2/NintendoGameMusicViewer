#pragma once

#include "../core/MusicTypes.h"

#include <cstdint>
#include <string>
#include <vector>

class IMusicBackend {
public:
    virtual ~IMusicBackend() = default;

    virtual bool load(const std::vector<std::uint8_t>& data, MusicInfo& info, std::string& error) = 0;
    virtual bool startTrack(int index, std::string& error) = 0;
    virtual bool seekMs(long ms, std::string& error) = 0;
    virtual bool renderStereo(int frames, std::vector<std::int16_t>& out, std::string& error) = 0;
    virtual bool renderChannels(int frames, std::vector<std::int16_t>& outInterleaved, int& voices, std::string& error) = 0;
    virtual long positionMs() const = 0;
    virtual bool ended() const = 0;
    virtual int voiceCount() const = 0;
};
