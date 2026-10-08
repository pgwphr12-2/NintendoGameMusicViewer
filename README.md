# Nintendo Game Music Viewer V1

A Windows-focused game music player/viewer written from scratch for classic Nintendo game-music formats.

## What this is

This is a **music player and per-channel oscilloscope viewer**, not an emulator front-end.

Mesen/MesenCE source code is **not included**.

V1 uses the public Game_Music_Emu (libgme) playback backend for:

- NSF
- NSFE
- GBS
- SPC

The UI is designed around a 16:9 recording canvas (logical 1920×1080, default window 1280×720).

## Channel visualization

Each backend voice is kept as a separate visual channel. The renderer uses a short, fixed history window and a per-pixel min/max envelope instead of connecting every raw sample directly. That avoids high-frequency alias flicker and produces a stable oscilloscope trace.

The audible playback path is intentionally fixed at **48,000 Hz internally**. The actual audio device may run at another supported rate; SDL's audio stream performs the conversion. V1 does not expose a fragile user-selectable sample-rate list.

## Current controls

- Open: Windows file dialog
- Drag and drop music files
- Space: play/pause
- Left / Right: seek 5 seconds
- Home: seek to start
- F9: cinematic oscilloscope mode
- F11: fullscreen
- Mouse wheel over track list: scroll
- Per-channel M: visual mute state (audio-channel mute backend hook is reserved)

## Future formats

- GSF: later, using a dedicated GBA music backend
- USF: later
- 2SF: later
- 3SF: later
- FDS-specific NSF playback/channel handling: dedicated backend work later; current libgme 0.6.6 lists NSF/NSFE support with VRC6, Namco 106 and FME-7, while FDS work is separate upstream development.

## Build

CMake downloads pinned third-party dependencies. Windows CI is provided in `.github/workflows/build-windows.yml` so a local .NET installation is not required.
