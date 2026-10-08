#pragma once

#include <cstdint>
#include <string>
#include <vector>

struct TrackInfo {
    std::string title;
    std::string game;
    std::string system;
    std::string author;
    std::string copyright;
    std::string comment;
    long lengthMs = -1;
};

struct ChannelInfo {
    std::string name;
};

struct MusicInfo {
    std::string path;
    std::string format;
    std::string system;
    std::vector<TrackInfo> tracks;
    std::vector<ChannelInfo> channels;
};
