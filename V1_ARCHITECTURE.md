# V1 architecture

This project is intentionally independent from Mesen/MesenCE source.

## Playback path

`GmeBackend` is the first format backend. It loads the file, exposes track metadata, produces the audible stereo stream and exposes the same track through the public libgme multi-channel output.

`PlaybackEngine` is the only component allowed to advance the backend. It runs on a worker thread and communicates with the UI through a small command queue. The SDL audio callback never touches the music backend; it only consumes already-rendered audio from `SDL_AudioStream`.

## Sample-rate stability

The backend is locked to 48,000 Hz for V1. The physical audio device is allowed to choose its native supported frequency and format. SDL's audio stream converts the fixed 48 kHz PCM to the obtained device format/rate. There is deliberately no user-selectable source-rate list in V1.

## Waveform stability

Every backend voice has its own `ChannelBuffer`. The visualizer snapshots those buffers on the UI thread. A 180 ms history window is rendered as a per-pixel min/max envelope plus a center trace. This removes the phase/aliasing flicker that occurs when a high-frequency waveform is naively mapped sample-for-pixel.

## Future backends

The `IMusicBackend` interface is the expansion point for GSF, FDS-capable NSF, USF, 2SF and 3SF backends later.
