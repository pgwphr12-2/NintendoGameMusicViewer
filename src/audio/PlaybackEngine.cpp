#include "PlaybackEngine.h"

#include <algorithm>
#include <chrono>
#include <thread>

namespace {
constexpr int kFramesPerBlock = 1024;
}

PlaybackEngine::~PlaybackEngine()
{
    shutdown();
}

bool PlaybackEngine::initialize(std::string& error)
{
    if (!_output.initialize(error)) return false;
    _running = true;
    _worker = std::thread(&PlaybackEngine::workerLoop, this);
    return true;
}

void PlaybackEngine::shutdown()
{
    if (_running.exchange(false)) {
        {
            std::lock_guard<std::mutex> lock(_commandMutex);
            _commands.push_back({CommandType::Shutdown});
        }
        _commandCv.notify_one();
    }
    if (_worker.joinable()) _worker.join();
    _output.shutdown();
    _backend.reset();
    _channels.clear();
}

bool PlaybackEngine::load(std::shared_ptr<IMusicBackend> backend, int track,
                          std::vector<std::shared_ptr<ChannelBuffer>> channels,
                          bool autoplay)
{
    if (!backend) return false;
    Command command;
    command.type = CommandType::Load;
    command.value = track;
    command.autoplay = autoplay;
    command.backend = std::move(backend);
    command.channels = std::move(channels);
    {
        std::lock_guard<std::mutex> lock(_commandMutex);
        _commands.clear();
        _commands.push_back(std::move(command));
    }
    _commandCv.notify_one();
    return true;
}

void PlaybackEngine::play()
{
    std::lock_guard<std::mutex> lock(_commandMutex);
    _commands.push_back({CommandType::Play});
    _commandCv.notify_one();
}

void PlaybackEngine::pause()
{
    std::lock_guard<std::mutex> lock(_commandMutex);
    _commands.push_back({CommandType::Pause});
    _commandCv.notify_one();
}

void PlaybackEngine::togglePlay()
{
    if (playing()) pause(); else play();
}

void PlaybackEngine::stop()
{
    std::lock_guard<std::mutex> lock(_commandMutex);
    _commands.push_back({CommandType::Stop});
    _commandCv.notify_one();
}

void PlaybackEngine::seek(long ms)
{
    Command command;
    command.type = CommandType::Seek;
    command.value = std::max<long>(0, ms);
    {
        std::lock_guard<std::mutex> lock(_commandMutex);
        _commands.push_back(std::move(command));
    }
    _commandCv.notify_one();
}

void PlaybackEngine::selectTrack(int track, bool autoplay)
{
    Command command;
    command.type = CommandType::Track;
    command.value = track;
    command.autoplay = autoplay;
    {
        std::lock_guard<std::mutex> lock(_commandMutex);
        _commands.push_back(std::move(command));
    }
    _commandCv.notify_one();
}

void PlaybackEngine::setLoop(bool loop)
{
    Command command;
    command.type = CommandType::SetLoop;
    command.value = loop ? 1 : 0;
    {
        std::lock_guard<std::mutex> lock(_commandMutex);
        _commands.push_back(std::move(command));
    }
    _commandCv.notify_one();
}

void PlaybackEngine::resetBuffers()
{
    for (const auto& channel : _channels) {
        if (channel) channel->clear();
    }
}

bool PlaybackEngine::popCommand(Command& command)
{
    std::lock_guard<std::mutex> lock(_commandMutex);
    if (_commands.empty()) return false;
    command = std::move(_commands.front());
    _commands.pop_front();
    return true;
}

void PlaybackEngine::handleCommand(Command& command, bool& workerQuit)
{
    std::string error;
    switch (command.type) {
        case CommandType::Load:
            _backend = std::move(command.backend);
            _channels = std::move(command.channels);
            _currentTrack = static_cast<int>(command.value);
            _output.clear();
            resetBuffers();
            _positionMs = 0;
            _playing = false;
            if (_backend && !_backend->startTrack(_currentTrack, error)) {
                _backend.reset();
                _channels.clear();
            } else if (command.autoplay) {
                _playing = true;
            }
            break;
        case CommandType::Play:
            if (_backend) _playing = true;
            break;
        case CommandType::Pause:
            _playing = false;
            _output.clear();
            break;
        case CommandType::Stop:
            if (_backend) {
                _playing = false;
                _output.clear();
                resetBuffers();
                _backend->startTrack(_currentTrack.load(), error);
                _positionMs = 0;
            }
            break;
        case CommandType::Seek:
            if (_backend) {
                _output.clear();
                resetBuffers();
                _backend->seekMs(command.value, error);
                _positionMs = command.value;
            }
            break;
        case CommandType::Track:
            if (_backend) {
                const bool wasPlaying = _playing.load();
                _playing = false;
                _output.clear();
                resetBuffers();
                if (_backend->startTrack(static_cast<int>(command.value), error)) {
                    _currentTrack = static_cast<int>(command.value);
                    _positionMs = 0;
                    _playing = command.autoplay || wasPlaying;
                }
            }
            break;
        case CommandType::SetLoop:
            _loop = command.value != 0;
            break;
        case CommandType::Shutdown:
            workerQuit = true;
            break;
    }
}

void PlaybackEngine::workerLoop()
{
    std::vector<std::int16_t> stereo;
    std::vector<std::int16_t> channelSamples;

    while (_running.load()) {
        bool workerQuit = false;
        Command command;
        while (popCommand(command)) {
            handleCommand(command, workerQuit);
            if (workerQuit) break;
        }
        if (workerQuit) break;

        if (!_playing.load() || !_backend) {
            std::unique_lock<std::mutex> lock(_commandMutex);
            _commandCv.wait_for(lock, std::chrono::milliseconds(4), [this] {
                return !_commands.empty() || !_running.load();
            });
            continue;
        }

        if (_output.availableBytes() >= _output.targetBytes()) {
            std::this_thread::sleep_for(std::chrono::milliseconds(2));
            continue;
        }

        std::string error;
        if (!_backend->renderStereo(kFramesPerBlock, stereo, error)) {
            _playing = false;
            continue;
        }
        if (!_output.push(stereo.data(), kFramesPerBlock, error)) {
            _playing = false;
            continue;
        }

        int voices = 0;
        if (_backend->renderChannels(kFramesPerBlock, channelSamples, voices, error)) {
            const std::size_t stride = static_cast<std::size_t>(voices) * 2u;
            std::vector<std::int16_t> oneVoice(static_cast<std::size_t>(kFramesPerBlock) * 2u);
            for (int voice = 0; voice < voices && voice < static_cast<int>(_channels.size()); ++voice) {
                const auto& buffer = _channels[static_cast<std::size_t>(voice)];
                if (!buffer) continue;
                const std::size_t offset = static_cast<std::size_t>(voice) * 2u;
                // GME multi-channel output is frame-major: [frame][voice][L/R].
                for (int frame = 0; frame < kFramesPerBlock; ++frame) {
                    const std::size_t source = static_cast<std::size_t>(frame) * stride + offset;
                    oneVoice[static_cast<std::size_t>(frame) * 2u] = channelSamples[source];
                    oneVoice[static_cast<std::size_t>(frame) * 2u + 1u] = channelSamples[source + 1u];
                }
                buffer->pushStereo(oneVoice.data(), static_cast<std::size_t>(kFramesPerBlock));
            }
        }

        _positionMs = _backend->positionMs();
        if (_backend->ended()) {
            if (_loop.load()) {
                if (_backend->startTrack(_currentTrack.load(), error)) {
                    _output.clear();
                    resetBuffers();
                    _positionMs = 0;
                }
            } else {
                _playing = false;
            }
        }
    }
}
