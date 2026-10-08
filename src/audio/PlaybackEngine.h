#pragma once

#include "AudioOutput.h"
#include "../backend/IMusicBackend.h"
#include "../visualizer/ChannelBuffer.h"

#include <atomic>
#include <condition_variable>
#include <cstdint>
#include <deque>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

class PlaybackEngine {
public:
    PlaybackEngine() = default;
    ~PlaybackEngine();

    bool initialize(std::string& error);
    void shutdown();

    bool load(std::shared_ptr<IMusicBackend> backend, int track,
              std::vector<std::shared_ptr<ChannelBuffer>> channels,
              bool autoplay);

    void play();
    void pause();
    void stop();
    void togglePlay();
    void seek(long ms);
    void selectTrack(int track, bool autoplay);
    void setLoop(bool loop);

    bool playing() const { return _playing.load(); }
    long positionMs() const { return _positionMs.load(); }
    int currentTrack() const { return _currentTrack.load(); }
    int deviceRate() const { return _output.deviceRate(); }
    int underruns() const { return _output.underruns(); }

private:
    enum class CommandType { Load, Play, Pause, Stop, Seek, Track, SetLoop, Shutdown };
    struct Command {
        CommandType type = CommandType::Pause;
        long value = 0;
        bool autoplay = false;
        std::shared_ptr<IMusicBackend> backend;
        std::vector<std::shared_ptr<ChannelBuffer>> channels;
    };

    void workerLoop();
    bool popCommand(Command& command);
    void handleCommand(Command& command, bool& workerQuit);
    void resetBuffers();

    AudioOutput _output;
    std::thread _worker;
    mutable std::mutex _commandMutex;
    std::condition_variable _commandCv;
    std::deque<Command> _commands;

    std::shared_ptr<IMusicBackend> _backend;
    std::vector<std::shared_ptr<ChannelBuffer>> _channels;

    std::atomic<bool> _running{false};
    std::atomic<bool> _playing{false};
    std::atomic<long> _positionMs{0};
    std::atomic<int> _currentTrack{0};
    std::atomic<bool> _loop{false};
};
