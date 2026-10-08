#include "ViewerUi.h"

#include <algorithm>
#include <cstdlib>
#include <filesystem>
#include <vector>

namespace {
constexpr SDL_Color kText{235, 239, 246, 255};
constexpr SDL_Color kDim{136, 147, 165, 255};
constexpr SDL_Color kPanel{17, 21, 28, 255};
constexpr SDL_Color kPanel2{13, 17, 23, 255};
constexpr SDL_Color kLine{43, 51, 64, 255};
constexpr SDL_Color kAccent{86, 183, 255, 255};
}

bool ViewerUi::initialize(std::string& error)
{
    std::vector<std::string> fonts;
#ifdef _WIN32
    if (const char* windir = std::getenv("WINDIR")) {
        fonts.emplace_back(std::string(windir) + "\\Fonts\\malgun.ttf");
        fonts.emplace_back(std::string(windir) + "\\Fonts\\segoeui.ttf");
    }
#endif
#ifdef __linux__
    fonts.emplace_back("/usr/share/fonts/truetype/noto/NotoSansCJK-Regular.ttc");
    fonts.emplace_back("/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf");
#endif
#ifdef __APPLE__
    fonts.emplace_back("/System/Library/Fonts/AppleSDGothicNeo.ttc");
#endif

    std::string path;
    for (const auto& candidate : fonts) {
        if (std::filesystem::exists(candidate)) { path = candidate; break; }
    }
    if (path.empty()) {
        error = "No system font could be found.";
        return false;
    }
    _font = TTF_OpenFont(path.c_str(), 25);
    _small = TTF_OpenFont(path.c_str(), 17);
    if (!_font || !_small) {
        error = TTF_GetError();
        shutdown();
        return false;
    }
    return true;
}

void ViewerUi::shutdown()
{
    if (_font) { TTF_CloseFont(_font); _font = nullptr; }
    if (_small) { TTF_CloseFont(_small); _small = nullptr; }
}

void ViewerUi::drawText(SDL_Renderer* renderer, const std::string& value, int x, int y, int size, SDL_Color color)
{
    if (value.empty()) return;
    TTF_Font* font = size <= 18 ? _small : _font;
    if (!font) return;
    SDL_Surface* surface = TTF_RenderUTF8_Blended(font, value.c_str(), color);
    if (!surface) return;
    SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, surface);
    SDL_Rect dst{x, y, surface->w, surface->h};
    SDL_FreeSurface(surface);
    if (!texture) return;
    SDL_RenderCopy(renderer, texture, nullptr, &dst);
    SDL_DestroyTexture(texture);
}

void ViewerUi::drawButton(SDL_Renderer* renderer, const SDL_Rect& rect, const std::string& label, bool active)
{
    const SDL_Color bg = active ? SDL_Color{39, 76, 111, 255} : SDL_Color{28, 34, 43, 255};
    SDL_SetRenderDrawColor(renderer, bg.r, bg.g, bg.b, bg.a);
    SDL_RenderFillRect(renderer, &rect);
    SDL_SetRenderDrawColor(renderer, kLine.r, kLine.g, kLine.b, 255);
    SDL_RenderDrawRect(renderer, &rect);
    drawText(renderer, label, rect.x + 12, rect.y + 11, 17, kText);
}

bool ViewerUi::hit(const SDL_Rect& rect, int x, int y) const
{
    return x >= rect.x && y >= rect.y && x < rect.x + rect.w && y < rect.y + rect.h;
}

std::string ViewerUi::timeText(long ms)
{
    const long seconds = std::max<long>(0, ms) / 1000L;
    const long minutes = seconds / 60L;
    const long sec = seconds % 60L;
    return std::to_string(minutes) + ":" + (sec < 10 ? "0" : "") + std::to_string(sec);
}

void ViewerUi::scrollTracks(int amount)
{
    _scroll = std::max(0, _scroll + amount);
}

ViewerUi::Action ViewerUi::draw(SDL_Renderer* renderer,
                                const MusicInfo* music,
                                int track,
                                long positionMs,
                                bool playing,
                                bool loop,
                                int deviceRate,
                                int underruns,
                                std::vector<bool>& muted,
                                const std::vector<std::shared_ptr<ChannelBuffer>>& buffers,
                                WaveformRenderer& waveform)
{
    Action action;
    int w = 1920, h = 1080;
    SDL_RenderGetLogicalSize(renderer, &w, &h);

    int mx = 0, my = 0;
    const Uint32 buttons = SDL_GetMouseState(&mx, &my);
    const bool pressed = (buttons & SDL_BUTTON(SDL_BUTTON_LEFT)) && !(_previousMouseButtons & SDL_BUTTON(SDL_BUTTON_LEFT));
    _previousMouseButtons = buttons;

    SDL_SetRenderDrawColor(renderer, 8, 11, 16, 255);
    SDL_RenderClear(renderer);

    if (!music) {
        drawText(renderer, "Nintendo Game Music Viewer", 70, 62, 38, kText);
        drawText(renderer, "Music player + per-channel oscilloscope", 70, 119, 22, kDim);
        SDL_Rect open{70, 190, 180, 54};
        drawButton(renderer, open, "Open Music", false);
        if (pressed && hit(open, mx, my)) action.type = Action::Open;
        drawText(renderer, "Initial V1 formats: NSF / NSFE / SPC / GBS", 70, h - 95, 18, kDim);
        drawText(renderer, "Fixed 48 kHz engine · hardware sample-rate conversion · 16:9 UI", 70, h - 61, 18, kDim);
        return action;
    }

    if (_cinematic) {
        const TrackInfo& info = music->tracks[static_cast<std::size_t>(std::clamp(track, 0, static_cast<int>(music->tracks.size()) - 1))];
        drawText(renderer, info.title, 50, 38, 34, kText);
        drawText(renderer, music->format + " · " + music->system, 50, 83, 18, kDim);
        SDL_Rect scope{50, 133, w - 100, h - 241};
        std::vector<ChannelView> views;
        views.reserve(music->channels.size());
        for (std::size_t i = 0; i < music->channels.size(); ++i) {
            views.push_back({music->channels[i], i < buffers.size() ? buffers[i] : nullptr, i < muted.size() && muted[i]});
        }
        waveform.draw(renderer, scope, views);
        drawText(renderer, timeText(positionMs), 50, h - 87, 19, kText);
        drawText(renderer, "SPACE Play/Pause  ·  ←/→ 5 sec  ·  F9 UI  ·  F11 Fullscreen", 245, h - 87, 17, kDim);
        return action;
    }

    SDL_SetRenderDrawColor(renderer, kPanel2.r, kPanel2.g, kPanel2.b, 255);
    SDL_Rect header{0, 0, w, 122};
    SDL_RenderFillRect(renderer, &header);
    SDL_SetRenderDrawColor(renderer, kLine.r, kLine.g, kLine.b, 255);
    SDL_RenderDrawLine(renderer, 0, 121, w, 121);

    drawText(renderer, "NINTENDO GAME MUSIC VIEWER", 34, 28, 29, kText);
    drawText(renderer, music->format + " · " + music->system, 34, 67, 17, kDim);

    const int safeTrack = std::clamp(track, 0, static_cast<int>(music->tracks.size()) - 1);
    const TrackInfo& info = music->tracks[static_cast<std::size_t>(safeTrack)];
    drawText(renderer, info.title, 660, 27, 24, kText);
    drawText(renderer, info.game, 660, 61, 17, kDim);

    SDL_Rect open{w - 300, 23, 105, 42};
    SDL_Rect cinematic{w - 185, 23, 155, 42};
    drawButton(renderer, open, "Open", false);
    drawButton(renderer, cinematic, "Oscilloscope", false);

    SDL_Rect sidebar{0, 122, 340, h - 246};
    SDL_SetRenderDrawColor(renderer, kPanel.r, kPanel.g, kPanel.b, 255);
    SDL_RenderFillRect(renderer, &sidebar);
    SDL_SetRenderDrawColor(renderer, kLine.r, kLine.g, kLine.b, 255);
    SDL_RenderDrawLine(renderer, sidebar.x + sidebar.w, sidebar.y, sidebar.x + sidebar.w, h - 124);

    drawText(renderer, "TRACKS", 24, 146, 17, kDim);
    const int rowH = 40;
    const int visible = std::max(1, (sidebar.h - 72) / rowH);
    const int maxScroll = std::max(0, static_cast<int>(music->tracks.size()) - visible);
    _scroll = std::clamp(_scroll, 0, maxScroll);
    for (int i = 0; i < visible; ++i) {
        const int index = i + _scroll;
        if (index >= static_cast<int>(music->tracks.size())) break;
        SDL_Rect r{16, 180 + i * rowH, 308, rowH - 3};
        if (index == safeTrack) {
            SDL_SetRenderDrawColor(renderer, 38, 70, 101, 255);
            SDL_RenderFillRect(renderer, &r);
        }
        drawText(renderer, std::to_string(index + 1), r.x + 9, r.y + 9, 16, index == safeTrack ? kText : kDim);
        drawText(renderer, music->tracks[static_cast<std::size_t>(index)].title, r.x + 39, r.y + 9, 16, index == safeTrack ? kText : kDim);
    }

    SDL_Rect scope{360, 138, w - 390, h - 275};
    std::vector<ChannelView> views;
    views.reserve(music->channels.size());
    for (std::size_t i = 0; i < music->channels.size(); ++i) {
        views.push_back({music->channels[i], i < buffers.size() ? buffers[i] : nullptr, i < muted.size() && muted[i]});
    }
    waveform.draw(renderer, scope, views);

    // Channel labels and mute buttons are deterministic: no per-frame color/index changes.
    const int count = static_cast<int>(music->channels.size());
    const int gap = count <= 8 ? 8 : 5;
    const int rowHeight = std::max(44, (scope.h - gap * (count - 1)) / std::max(1, count));
    for (int i = 0; i < count; ++i) {
        const int y = scope.y + i * (rowHeight + gap);
        drawText(renderer, music->channels[static_cast<std::size_t>(i)].name, scope.x + 12, y + 13, 17, muted[static_cast<std::size_t>(i)] ? kDim : kText);
        SDL_Rect mute{scope.x + scope.w - 54, y + 9, 42, 28};
        drawButton(renderer, mute, muted[static_cast<std::size_t>(i)] ? "M" : "", muted[static_cast<std::size_t>(i)]);
        if (pressed && hit(mute, mx, my)) {
            action.type = Action::Mute;
            action.index = i;
        }
    }

    SDL_SetRenderDrawColor(renderer, kPanel2.r, kPanel2.g, kPanel2.b, 255);
    SDL_Rect bottom{0, h - 124, w, 124};
    SDL_RenderFillRect(renderer, &bottom);
    SDL_SetRenderDrawColor(renderer, kLine.r, kLine.g, kLine.b, 255);
    SDL_RenderDrawLine(renderer, 0, h - 124, w, h - 124);

    SDL_Rect prev{20, h - 92, 62, 44};
    SDL_Rect play{89, h - 92, 88, 44};
    SDL_Rect next{184, h - 92, 62, 44};
    SDL_Rect stop{253, h - 92, 62, 44};
    SDL_Rect loopRect{322, h - 92, 80, 44};
    drawButton(renderer, prev, "<", false);
    drawButton(renderer, play, playing ? "Pause" : "Play", playing);
    drawButton(renderer, next, ">", false);
    drawButton(renderer, stop, "Stop", false);
    drawButton(renderer, loopRect, loop ? "Loop" : "Once", loop);

    const long duration = info.lengthMs > 0 ? info.lengthMs : 0;
    const float ratio = duration > 0 ? std::clamp(static_cast<float>(positionMs) / static_cast<float>(duration), 0.0f, 1.0f) : 0.0f;
    SDL_Rect timeline{440, h - 84, 930, 9};
    SDL_SetRenderDrawColor(renderer, 35, 42, 53, 255);
    SDL_RenderFillRect(renderer, &timeline);
    SDL_Rect fill{timeline.x, timeline.y, static_cast<int>(timeline.w * ratio), timeline.h};
    SDL_SetRenderDrawColor(renderer, kAccent.r, kAccent.g, kAccent.b, 255);
    SDL_RenderFillRect(renderer, &fill);
    drawText(renderer, timeText(positionMs) + " / " + (duration ? timeText(duration) : "--:--"), 440, h - 60, 17, kText);
    drawText(renderer, "ENGINE 48,000 Hz", 1515, h - 88, 16, kDim);
    drawText(renderer, "DEVICE " + std::to_string(deviceRate) + " Hz", 1515, h - 62, 16, kDim);
    drawText(renderer, "UNDERRUNS " + std::to_string(underruns), 1715, h - 88, 16, underruns == 0 ? kDim : SDL_Color{255, 118, 118, 255});

    if (pressed) {
        if (hit(open, mx, my)) action.type = Action::Open;
        else if (hit(cinematic, mx, my)) action.type = Action::Cinematic;
        else if (hit(prev, mx, my)) action.type = Action::Prev;
        else if (hit(play, mx, my)) action.type = playing ? Action::Pause : Action::Play;
        else if (hit(next, mx, my)) action.type = Action::Next;
        else if (hit(stop, mx, my)) action.type = Action::Stop;
        else if (hit(loopRect, mx, my)) action.type = Action::Loop;
        else if (hit(timeline, mx, my) && duration > 0) {
            const float t = std::clamp(static_cast<float>(mx - timeline.x) / static_cast<float>(timeline.w), 0.0f, 1.0f);
            action.type = Action::Seek;
            action.seekMs = static_cast<long>(static_cast<float>(duration) * t);
        } else if (mx >= 16 && mx <= 324 && my >= 180 && my < 180 + visible * rowH) {
            const int row = (my - 180) / rowH;
            const int index = row + _scroll;
            if (index >= 0 && index < static_cast<int>(music->tracks.size())) {
                action.type = Action::Track;
                action.index = index;
            }
        }
    }
    return action;
}
