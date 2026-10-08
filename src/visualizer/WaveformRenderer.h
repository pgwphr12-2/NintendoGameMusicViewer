#pragma once

#include "ChannelBuffer.h"
#include "../core/MusicTypes.h"

#include <SDL.h>

#include <memory>
#include <string>
#include <vector>

struct ChannelView {
    ChannelInfo info;
    std::shared_ptr<ChannelBuffer> buffer;
    bool muted = false;
};

class WaveformRenderer {
public:
    void draw(SDL_Renderer* renderer, const SDL_Rect& area, const std::vector<ChannelView>& channels);

private:
    void drawChannel(SDL_Renderer* renderer, const SDL_Rect& row, const ChannelView& channel, int index);
    static SDL_Color colorFor(int index, bool muted);
};
