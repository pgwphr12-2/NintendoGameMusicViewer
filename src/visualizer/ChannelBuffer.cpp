#include "ChannelBuffer.h"

#include <algorithm>

ChannelBuffer::ChannelBuffer(std::size_t capacitySamples)
    : _ring(std::max<std::size_t>(capacitySamples, 1024u), 0.0f)
{
}

void ChannelBuffer::clear()
{
    std::lock_guard<std::mutex> lock(_mutex);
    std::fill(_ring.begin(), _ring.end(), 0.0f);
    _write = 0;
    _full = false;
}

void ChannelBuffer::pushStereo(const std::int16_t* stereo, std::size_t frames)
{
    if (!stereo || frames == 0) return;
    std::lock_guard<std::mutex> lock(_mutex);
    for (std::size_t i = 0; i < frames; ++i) {
        const float left = static_cast<float>(stereo[i * 2u]) / 32768.0f;
        const float right = static_cast<float>(stereo[i * 2u + 1u]) / 32768.0f;
        _ring[_write] = std::clamp((left + right) * 0.5f, -1.0f, 1.0f);
        _write = (_write + 1u) % _ring.size();
        if (_write == 0) _full = true;
    }
}

void ChannelBuffer::snapshot(std::vector<float>& out) const
{
    std::lock_guard<std::mutex> lock(_mutex);
    out.clear();
    if (!_full && _write == 0) return;
    const std::size_t count = _full ? _ring.size() : _write;
    out.resize(count);
    if (_full) {
        const std::size_t first = _ring.size() - _write;
        std::copy_n(_ring.data() + _write, first, out.data());
        if (_write > 0) std::copy_n(_ring.data(), _write, out.data() + first);
    } else {
        std::copy_n(_ring.data(), _write, out.data());
    }
}
