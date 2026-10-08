#include "GmeBackend.h"

#include <algorithm>
#include <climits>
#include <cstring>

std::string GmeBackend::text(const char* value)
{
    return value ? std::string(value) : std::string();
}

GmeBackend::~GmeBackend()
{
    close();
}

void GmeBackend::close()
{
    if (_stereo) {
        gme_delete(_stereo);
        _stereo = nullptr;
    }
    if (_channels) {
        gme_delete(_channels);
        _channels = nullptr;
    }
    _type = nullptr;
    _voiceCount = 0;
}

bool GmeBackend::load(const std::vector<std::uint8_t>& data, MusicInfo& info, std::string& error)
{
    close();
    if (data.size() < 4 || data.size() > static_cast<std::size_t>(LONG_MAX)) {
        error = "Invalid or empty music file.";
        return false;
    }

    const char* typeName = gme_identify_header(data.data());
    if (!typeName || !*typeName) {
        error = "Unsupported game-music format.";
        return false;
    }

    _type = gme_identify_extension(typeName);
    if (!_type) {
        error = std::string("No backend is registered for ") + typeName + ".";
        return false;
    }

    _stereo = gme_new_emu(_type, kInternalSampleRate);
    _channels = gme_new_emu_multi_channel(_type, kInternalSampleRate);
    if (!_stereo || !_channels) {
        error = "The audio backend could not be created.";
        close();
        return false;
    }

    if (const char* e = gme_load_data(_stereo, data.data(), static_cast<long>(data.size()))) {
        error = e;
        close();
        return false;
    }
    if (const char* e = gme_load_data(_channels, data.data(), static_cast<long>(data.size()))) {
        error = e;
        close();
        return false;
    }

    if (!gme_multi_channel(_channels)) {
        error = "This backend cannot expose separate channel PCM for this file.";
        close();
        return false;
    }

    _voiceCount = gme_voice_count(_channels);
    if (_voiceCount <= 0) {
        error = "The file has no audio channels.";
        close();
        return false;
    }

    info = {};
    info.format = typeName;
    info.system = text(gme_type_system(_type));

    for (int i = 0; i < _voiceCount; ++i) {
        const char* voiceName = gme_voice_name(_channels, i);
        std::string name = voiceName ? voiceName : std::string();
        if (name.empty()) name = "Channel " + std::to_string(i + 1);
        info.channels.push_back({std::move(name)});
    }

    const int tracks = gme_track_count(_stereo);
    info.tracks.reserve(static_cast<std::size_t>(tracks));
    for (int i = 0; i < tracks; ++i) {
        gme_info_t* raw = nullptr;
        TrackInfo track;
        if (!gme_track_info(_stereo, &raw, i) && raw) {
            track.title = text(raw->song);
            track.game = text(raw->game);
            track.system = text(raw->system);
            track.author = text(raw->author);
            track.copyright = text(raw->copyright);
            track.comment = text(raw->comment);
            track.lengthMs = raw->length;
            gme_free_info(raw);
        }
        if (track.title.empty()) track.title = "Track " + std::to_string(i + 1);
        if (track.system.empty()) track.system = info.system;
        info.tracks.push_back(std::move(track));
    }
    if (info.tracks.empty()) {
        error = "The file contains no tracks.";
        close();
        return false;
    }
    return true;
}

bool GmeBackend::startTrack(int index, std::string& error)
{
    if (!_stereo || !_channels) {
        error = "No music is loaded.";
        return false;
    }
    if (const char* e = gme_start_track(_stereo, index)) {
        error = e;
        return false;
    }
    if (const char* e = gme_start_track(_channels, index)) {
        error = e;
        return false;
    }
    return true;
}

bool GmeBackend::seekMs(long ms, std::string& error)
{
    if (!_stereo || !_channels) {
        error = "No music is loaded.";
        return false;
    }
    ms = std::max<long>(0, ms);
    if (const char* e = gme_seek(_stereo, ms)) {
        error = e;
        return false;
    }
    if (const char* e = gme_seek(_channels, ms)) {
        error = e;
        return false;
    }
    return true;
}

bool GmeBackend::renderStereo(int frames, std::vector<std::int16_t>& out, std::string& error)
{
    if (!_stereo || frames <= 0) {
        error = "Stereo playback is not ready.";
        return false;
    }
    out.resize(static_cast<std::size_t>(frames) * 2u);
    if (const char* e = gme_play(_stereo, frames * 2L, out.data())) {
        error = e;
        return false;
    }
    return true;
}

bool GmeBackend::renderChannels(int frames, std::vector<std::int16_t>& outInterleaved, int& voices, std::string& error)
{
    if (!_channels || frames <= 0 || _voiceCount <= 0) {
        error = "Channel playback is not ready.";
        return false;
    }
    voices = _voiceCount;
    const std::size_t sampleCount = static_cast<std::size_t>(frames) * static_cast<std::size_t>(_voiceCount) * 2u;
    outInterleaved.resize(sampleCount);
    if (const char* e = gme_play(_channels, static_cast<long>(sampleCount), outInterleaved.data())) {
        error = e;
        return false;
    }
    return true;
}

long GmeBackend::positionMs() const
{
    return _stereo ? gme_tell(_stereo) : 0;
}

bool GmeBackend::ended() const
{
    return _stereo && gme_track_ended(_stereo) != 0;
}
