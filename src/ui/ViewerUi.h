#pragma once

#include "../core/MusicTypes.h"
#include "../visualizer/WaveformRenderer.h"

#include <SDL.h>
#include <SDL_ttf.h>

#include <memory>
#include <string>
#include <vector>

class ViewerUi {
public:
    bool initialize(std::string& error);
    void shutdown();

    struct Action {
        enum Type { None, Open, Play, Pause, Stop, Prev, Next, Loop, Cinematic, Track, Seek, Mute } type = None;
        int index = -1;
        long seekMs = -1;
    };

    Action draw(SDL_Renderer* renderer,
                const MusicInfo* music,
                int track,
                long positionMs,
                bool playing,
                bool loop,
                int deviceRate,
                int underruns,
                std::vector<bool>& muted,
                const std::vector<std::shared_ptr<ChannelBuffer>>& buffers,
                WaveformRenderer& waveform);

    void scrollTracks(int amount);
    void setCinematic(bool value) { _cinematic = value; }
    bool cinematic() const { return _cinematic; }

private:
    void drawText(SDL_Renderer* renderer, const std::string& value, int x, int y, int size, SDL_Color color);
    void drawButton(SDL_Renderer* renderer, const SDL_Rect& rect, const std::string& label, bool active);
    bool hit(const SDL_Rect& rect, int x, int y) const;
    static std::string timeText(long ms);

    TTF_Font* _font = nullptr;
    TTF_Font* _small = nullptr;
    bool _cinematic = false;
    int _scroll = 0;
    Uint32 _previousMouseButtons = 0;
};
