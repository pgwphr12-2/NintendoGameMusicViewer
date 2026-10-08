#include "WaveformRenderer.h"

#include <algorithm>
#include <cmath>

namespace {
constexpr std::size_t kVisibleSamples = 8640; // 180 ms @ 48 kHz

SDL_Color palette(int index, bool muted)
{
    static constexpr SDL_Color colors[] = {
        { 91, 207, 255, 255 },
        { 255, 121, 157, 255 },
        { 110, 235, 166, 255 },
        { 245, 202, 91, 255 },
        { 176, 142, 255, 255 },
        { 255, 164, 104, 255 },
        { 84, 224, 218, 255 },
        { 237, 125, 224, 255 }
    };
    SDL_Color c = colors[index % (sizeof(colors) / sizeof(colors[0]))];
    if (muted) {
        c.r = static_cast<Uint8>(c.r * 0.28f);
        c.g = static_cast<Uint8>(c.g * 0.28f);
        c.b = static_cast<Uint8>(c.b * 0.28f);
    }
    return c;
}
}

SDL_Color WaveformRenderer::colorFor(int index, bool muted)
{
    return palette(index, muted);
}

void WaveformRenderer::draw(SDL_Renderer* renderer, const SDL_Rect& area, const std::vector<ChannelView>& channels)
{
    SDL_SetRenderDrawColor(renderer, 9, 12, 17, 255);
    SDL_RenderFillRect(renderer, &area);

    const int count = static_cast<int>(channels.size());
    if (count == 0) return;

    const int gap = count <= 8 ? 8 : 5;
    const int rowH = std::max(44, (area.h - gap * (count - 1)) / count);
    for (int i = 0; i < count; ++i) {
        SDL_Rect row{area.x, area.y + i * (rowH + gap), area.w, rowH};
        drawChannel(renderer, row, channels[static_cast<std::size_t>(i)], i);
    }
}

void WaveformRenderer::drawChannel(SDL_Renderer* renderer, const SDL_Rect& row,
                                    const ChannelView& channel, int index)
{
    SDL_SetRenderDrawColor(renderer, 15, 18, 25, 255);
    SDL_RenderFillRect(renderer, &row);
    SDL_SetRenderDrawColor(renderer, 37, 44, 55, 255);
    SDL_RenderDrawRect(renderer, &row);

    const int left = row.x + 130;
    const int right = row.x + row.w - 18;
    const int top = row.y + 8;
    const int bottom = row.y + row.h - 8;
    if (right <= left || bottom <= top) return;

    const int mid = (top + bottom) / 2;
    SDL_SetRenderDrawColor(renderer, 35, 42, 53, 255);
    SDL_RenderDrawLine(renderer, left, mid, right, mid);

    const int gridStep = std::max(80, (right - left) / 12);
    for (int x = left + gridStep; x < right; x += gridStep) {
        SDL_SetRenderDrawColor(renderer, 22, 27, 36, 255);
        SDL_RenderDrawLine(renderer, x, top, x, bottom);
    }

    std::vector<float> samples;
    if (channel.buffer) channel.buffer->snapshot(samples);
    if (samples.empty()) return;

    const std::size_t window = std::min(kVisibleSamples, samples.size());
    const std::size_t begin = samples.size() - window;
    const int width = right - left;
    const float amplitude = static_cast<float>(bottom - top) * 0.45f;
    const SDL_Color c = colorFor(index, channel.muted);
    SDL_SetRenderDrawColor(renderer, c.r, c.g, c.b, c.a);

    int previousMid = mid;
    for (int px = 0; px < width; ++px) {
        std::size_t s0 = (static_cast<std::size_t>(px) * window) / static_cast<std::size_t>(width);
        std::size_t s1 = (static_cast<std::size_t>(px + 1) * window) / static_cast<std::size_t>(width);
        if (s1 <= s0) s1 = s0 + 1;
        s1 = std::min(s1, window);
        if (s0 >= s1) continue;

        float minValue = 1.0f;
        float maxValue = -1.0f;
        for (std::size_t s = s0; s < s1; ++s) {
            const float v = std::clamp(samples[begin + s], -1.0f, 1.0f);
            minValue = std::min(minValue, v);
            maxValue = std::max(maxValue, v);
        }

        const int x = left + px;
        const int yHigh = mid - static_cast<int>(maxValue * amplitude);
        const int yLow = mid - static_cast<int>(minValue * amplitude);
        SDL_RenderDrawLine(renderer, x, yHigh, x, yLow);

        const int currentMid = (yHigh + yLow) / 2;
        if (px > 0) SDL_RenderDrawLine(renderer, x - 1, previousMid, x, currentMid);
        previousMid = currentMid;
    }
}
