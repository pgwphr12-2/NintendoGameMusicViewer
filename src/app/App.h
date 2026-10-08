#pragma once

#include "../audio/PlaybackEngine.h"
#include "../backend/GmeBackend.h"
#include "../ui/ViewerUi.h"
#include "../visualizer/ChannelBuffer.h"
#include "../visualizer/WaveformRenderer.h"

#include <SDL.h>

#include <memory>
#include <string>
#include <vector>

class App {
public:
    bool initialize(std::string& error);
    int run();
    void shutdown();

private:
    bool openFile(const std::string& path);
    bool openDialog();
    void handleEvent(const SDL_Event& event, bool& quit);
    void apply(const ViewerUi::Action& action);

    SDL_Window* _window = nullptr;
    SDL_Renderer* _renderer = nullptr;
    ViewerUi _ui;
    WaveformRenderer _waveform;
    PlaybackEngine _playback;

    MusicInfo _music;
    bool _hasMusic = false;
    bool _loop = false;
    bool _fullscreen = false;
    std::vector<bool> _muted;
    std::vector<std::shared_ptr<ChannelBuffer>> _buffers;
};
