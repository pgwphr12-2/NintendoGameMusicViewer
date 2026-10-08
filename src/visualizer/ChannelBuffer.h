#pragma once

#include <cstddef>
#include <cstdint>
#include <mutex>
#include <vector>

class ChannelBuffer {
public:
    explicit ChannelBuffer(std::size_t capacitySamples = 12000);

    void clear();
    void pushStereo(const std::int16_t* stereo, std::size_t frames);
    void snapshot(std::vector<float>& out) const;

private:
    mutable std::mutex _mutex;
    std::vector<float> _ring;
    std::size_t _write = 0;
    bool _full = false;
};
