#include "App.h"

#include <SDL_ttf.h>

#include <algorithm>
#include <fstream>
#include <memory>
#include <vector>

#ifdef _WIN32
#include <commdlg.h>
#include <windows.h>
#endif

namespace {
#ifdef _WIN32
std::string wideToUtf8(const std::wstring& value)
{
    if (value.empty()) return {};
    int size = WideCharToMultiByte(CP_UTF8, 0, value.c_str(), static_cast<int>(value.size()), nullptr, 0, nullptr, nullptr);
    std::string result(static_cast<std::size_t>(size), '\0');
    WideCharToMultiByte(CP_UTF8, 0, value.c_str(), static_cast<int>(value.size()), result.data(), size, nullptr, nullptr);
    return result;
}
#endif
}

bool App::initialize(std::string& error)
{
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_EVENTS) != 0) {
        error = SDL_GetError();
        return false;
    }
    if (TTF_Init() != 0) {
        error = TTF_GetError();
        SDL_Quit();
        return false;
    }

    _window = SDL_CreateWindow(
        "Nintendo Game Music Viewer",
        SDL_WINDOWPOS_CENTERED,
        SDL_WINDOWPOS_CENTERED,
        1280,
        720,
        SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI);
    if (!_window) {
        error = SDL_GetError();
        shutdown();
        return false;
    }

    _renderer = SDL_CreateRenderer(_window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (!_renderer) _renderer = SDL_CreateRenderer(_window, -1, SDL_RENDERER_SOFTWARE);
    if (!_renderer) {
        error = SDL_GetError();
        shutdown();
        return false;
    }

    SDL_RenderSetLogicalSize(_renderer, 1920, 1080);
    SDL_SetRenderDrawBlendMode(_renderer, SDL_BLENDMODE_BLEND);

    if (!_ui.initialize(error)) {
        shutdown();
        return false;
    }
    if (!_playback.initialize(error)) {
        shutdown();
        return false;
    }

    return true;
}

void App::shutdown()
{
    _playback.shutdown();
    _ui.shutdown();
    if (_renderer) { SDL_DestroyRenderer(_renderer); _renderer = nullptr; }
    if (_window) { SDL_DestroyWindow(_window); _window = nullptr; }
    TTF_Quit();
    SDL_Quit();
}

bool App::openFile(const std::string& path)
{
    std::ifstream input(path, std::ios::binary);
    if (!input) return false;
    input.seekg(0, std::ios::end);
    const auto end = input.tellg();
    if (end <= 0) return false;
    input.seekg(0, std::ios::beg);
    std::vector<std::uint8_t> data(static_cast<std::size_t>(end));
    input.read(reinterpret_cast<char*>(data.data()), static_cast<std::streamsize>(data.size()));
    if (!input) return false;

    auto backend = std::make_shared<GmeBackend>();
    MusicInfo info;
    std::string error;
    if (!backend->load(data, info, error)) {
        SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Cannot open music", error.c_str(), _window);
        return false;
    }

    _buffers.clear();
    _buffers.reserve(info.channels.size());
    for (std::size_t i = 0; i < info.channels.size(); ++i) {
        _buffers.push_back(std::make_shared<ChannelBuffer>());
    }
    _muted.assign(info.channels.size(), false);
    _music = std::move(info);
    _music.path = path;
    _hasMusic = true;

    _playback.load(backend, 0, _buffers, true);
    return true;
}

bool App::openDialog()
{
#ifdef _WIN32
    wchar_t fileName[MAX_PATH] = L"";
    OPENFILENAMEW ofn{};
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = _window ? GetActiveWindow() : nullptr;
    ofn.lpstrFile = fileName;
    ofn.nMaxFile = MAX_PATH;
    ofn.lpstrFilter = L"Nintendo Game Music\0*.nsf;*.nsfe;*.spc;*.gbs\0All Files\0*.*\0";
    ofn.nFilterIndex = 1;
    ofn.Flags = OFN_FILEMUSTEXIST | OFN_HIDEREADONLY;
    if (GetOpenFileNameW(&ofn)) return openFile(wideToUtf8(fileName));
    return false;
#else
    return false;
#endif
}

void App::apply(const ViewerUi::Action& action)
{
    using Type = ViewerUi::Action::Type;
    const int count = static_cast<int>(_music.tracks.size());
    switch (action.type) {
        case Type::Open:
            openDialog();
            break;
        case Type::Play:
            _playback.play();
            break;
        case Type::Pause:
            _playback.pause();
            break;
        case Type::Stop:
            _playback.stop();
            break;
        case Type::Prev:
            if (_hasMusic && count > 0) {
                const int current = _playback.currentTrack();
                _playback.selectTrack((current - 1 + count) % count, true);
            }
            break;
        case Type::Next:
            if (_hasMusic && count > 0) {
                const int current = _playback.currentTrack();
                _playback.selectTrack((current + 1) % count, true);
            }
            break;
        case Type::Loop:
            _loop = !_loop;
            _playback.setLoop(_loop);
            break;
        case Type::Cinematic:
            _ui.setCinematic(!_ui.cinematic());
            break;
        case Type::Track:
            if (_hasMusic && action.index >= 0 && action.index < count) {
                _playback.selectTrack(action.index, true);
            }
            break;
        case Type::Seek:
            if (action.seekMs >= 0) _playback.seek(action.seekMs);
            break;
        case Type::Mute:
            if (action.index >= 0 && action.index < static_cast<int>(_muted.size())) {
                _muted[static_cast<std::size_t>(action.index)] = !_muted[static_cast<std::size_t>(action.index)];
                // V1 visually mutes the waveform; audio-channel mute integration is reserved for the backend control API.
            }
            break;
        default:
            break;
    }
}

void App::handleEvent(const SDL_Event& event, bool& quit)
{
    if (event.type == SDL_QUIT) {
        quit = true;
        return;
    }
    if (event.type == SDL_DROPFILE && event.drop.file) {
        openFile(event.drop.file);
        SDL_free(event.drop.file);
        return;
    }
    if (event.type == SDL_MOUSEWHEEL && !_ui.cinematic()) {
        int mx = 0, my = 0;
        SDL_GetMouseState(&mx, &my);
        if (mx < 340) _ui.scrollTracks(-event.wheel.y);
        return;
    }
    if (event.type == SDL_KEYDOWN && !event.key.repeat) {
        switch (event.key.keysym.sym) {
            case SDLK_SPACE: _playback.togglePlay(); break;
            case SDLK_LEFT: _playback.seek(std::max<long>(0, _playback.positionMs() - 5000)); break;
            case SDLK_RIGHT: _playback.seek(_playback.positionMs() + 5000); break;
            case SDLK_HOME: _playback.seek(0); break;
            case SDLK_F9: _ui.setCinematic(!_ui.cinematic()); break;
            case SDLK_F11:
                _fullscreen = !_fullscreen;
                SDL_SetWindowFullscreen(_window, _fullscreen ? SDL_WINDOW_FULLSCREEN_DESKTOP : 0);
                break;
            case SDLK_ESCAPE:
                if (_ui.cinematic()) _ui.setCinematic(false);
                break;
            default: break;
        }
    }
}

int App::run()
{
    bool quit = false;
    while (!quit) {
        SDL_Event event;
        while (SDL_PollEvent(&event)) handleEvent(event, quit);

        const int current = _playback.currentTrack();
        ViewerUi::Action action = _ui.draw(
            _renderer,
            _hasMusic ? &_music : nullptr,
            current,
            _playback.positionMs(),
            _playback.playing(),
            _loop,
            _playback.deviceRate(),
            _playback.underruns(),
            _muted,
            _buffers,
            _waveform);
        apply(action);
        SDL_RenderPresent(_renderer);
    }
    return 0;
}
