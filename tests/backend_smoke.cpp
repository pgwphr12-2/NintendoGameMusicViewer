#include "backend/GmeBackend.h"

#include <algorithm>
#include <cstdint>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <iterator>
#include <vector>

int main()
{
    std::ifstream file("tests/fixtures/TestTone.nsf", std::ios::binary);
    if (!file) {
        std::cerr << "fixture missing\n";
        return 2;
    }
    std::vector<std::uint8_t> data((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
    GmeBackend backend;
    MusicInfo info;
    std::string error;
    if (!backend.load(data, info, error)) {
        std::cerr << error << '\n';
        return 3;
    }
    if (info.format != "NSF" || info.channels.size() < 5 || info.tracks.empty()) return 4;
    if (!backend.startTrack(0, error)) return 5;
    std::vector<std::int16_t> stereo;
    if (!backend.renderStereo(1024, stereo, error)) return 6;
    bool audible = false;
    for (auto sample : stereo) {
        if (std::abs(static_cast<int>(sample)) > 32) { audible = true; break; }
    }
    if (!audible) return 7;
    std::vector<std::int16_t> channels;
    int voices = 0;
    if (!backend.renderChannels(256, channels, voices, error)) return 8;
    if (voices != static_cast<int>(info.channels.size())) return 9;
    if (channels.size() != static_cast<std::size_t>(256 * voices * 2)) return 10;
    std::cout << "NSF smoke test OK: voices=" << voices << " source_rate=48000\n";
    return 0;
}
