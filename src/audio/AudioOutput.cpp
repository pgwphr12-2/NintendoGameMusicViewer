#include "AudioOutput.h"

#include <algorithm>
#include <cstring>

AudioOutput::~AudioOutput()
{
    shutdown();
}

bool AudioOutput::initialize(std::string& error)
{
    SDL_AudioSpec desired{};
    desired.freq = kSourceRate;
    desired.format = AUDIO_S16LSB;
    desired.channels = 2;
    desired.samples = 1024;
    desired.callback = &AudioOutput::callback;
    desired.userdata = this;

    SDL_AudioSpec obtained{};
    _device = SDL_OpenAudioDevice(nullptr, 0, &desired, &obtained,
                                  SDL_AUDIO_ALLOW_FREQUENCY_CHANGE |
                                  SDL_AUDIO_ALLOW_FORMAT_CHANGE |
                                  SDL_AUDIO_ALLOW_CHANNELS_CHANGE);
    if (!_device) {
        error = SDL_GetError();
        return false;
    }
    _deviceSpec = obtained;

    _stream = SDL_NewAudioStream(
        AUDIO_S16LSB, 2, kSourceRate,
        _deviceSpec.format, _deviceSpec.channels, _deviceSpec.freq);
    if (!_stream) {
        error = SDL_GetError();
        SDL_CloseAudioDevice(_device);
        _device = 0;
        return false;
    }

    _underruns.store(0);
    SDL_PauseAudioDevice(_device, 0);
    return true;
}

void AudioOutput::shutdown()
{
    if (_device) {
        SDL_PauseAudioDevice(_device, 1);
        SDL_CloseAudioDevice(_device);
        _device = 0;
    }
    if (_stream) {
        SDL_FreeAudioStream(_stream);
        _stream = nullptr;
    }
}

void AudioOutput::clear()
{
    if (_stream) SDL_AudioStreamClear(_stream);
}

bool AudioOutput::push(const std::int16_t* stereo, int frames, std::string& error)
{
    if (!_stream || !stereo || frames <= 0) {
        error = "Audio output is not initialized.";
        return false;
    }
    const int bytes = frames * 2 * static_cast<int>(sizeof(std::int16_t));
    if (SDL_AudioStreamPut(_stream, stereo, bytes) != 0) {
        error = SDL_GetError();
        return false;
    }
    return true;
}

int AudioOutput::availableBytes() const
{
    return _stream ? SDL_AudioStreamAvailable(_stream) : 0;
}

int AudioOutput::targetBytes() const
{
    if (!_deviceSpec.freq || !_deviceSpec.channels) return 0;
    const int bits = SDL_AUDIO_BITSIZE(_deviceSpec.format);
    const int bytesPerFrame = std::max(1, (bits / 8) * static_cast<int>(_deviceSpec.channels));
    return std::max(4096, (_deviceSpec.freq * bytesPerFrame * 250) / 1000);
}

void AudioOutput::callback(void* userdata, Uint8* stream, int len)
{
    auto* self = static_cast<AudioOutput*>(userdata);
    if (!self || !self->_stream || !stream || len <= 0) {
        if (stream && len > 0) std::memset(stream, 0, static_cast<std::size_t>(len));
        return;
    }

    const int got = SDL_AudioStreamGet(self->_stream, stream, len);
    if (got < len) {
        if (got > 0) std::memset(stream + got, 0, static_cast<std::size_t>(len - got));
        else std::memset(stream, 0, static_cast<std::size_t>(len));
        ++self->_underruns;
    }
}
