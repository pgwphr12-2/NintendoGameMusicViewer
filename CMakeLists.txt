cmake_minimum_required(VERSION 3.24)
project(NintendoGameMusicViewer VERSION 1.0.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_POLICY_DEFAULT_CMP0077 NEW)

option(NGMV_BUILD_TESTS "Build smoke tests" ON)

include(FetchContent)

# ---- SDL2 ----
set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)
set(SDL_SHARED OFF CACHE BOOL "" FORCE)
set(SDL_STATIC ON CACHE BOOL "" FORCE)
set(SDL_TESTS OFF CACHE BOOL "" FORCE)
set(SDL_TEST_LIBRARY OFF CACHE BOOL "" FORCE)
set(SDL_EXAMPLES OFF CACHE BOOL "" FORCE)
set(SDL2_DISABLE_SDL2MAIN ON CACHE BOOL "" FORCE)

FetchContent_Declare(
    SDL2
    GIT_REPOSITORY https://github.com/libsdl-org/SDL.git
    GIT_TAG release-2.30.9
    GIT_SHALLOW TRUE
)
FetchContent_MakeAvailable(SDL2)

# ---- SDL2_ttf ----
set(SDL2TTF_VENDORED ON CACHE BOOL "" FORCE)
set(SDL2TTF_SAMPLES OFF CACHE BOOL "" FORCE)
set(SDL2TTF_INSTALL OFF CACHE BOOL "" FORCE)
set(SDL2TTF_BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)
set(SDL2TTF_HARFBUZZ OFF CACHE BOOL "" FORCE)
set(SDL2TTF_PLUTOSVG OFF CACHE BOOL "" FORCE)

FetchContent_Declare(
    SDL2_ttf
    GIT_REPOSITORY https://github.com/libsdl-org/SDL_ttf.git
    GIT_TAG release-2.24.0
    GIT_SHALLOW TRUE
)
FetchContent_MakeAvailable(SDL2_ttf)

# ---- Game_Music_Emu ----
# External playback backend only. No Mesen/MesenCE source is included.
set(USE_GME_GBS ON CACHE BOOL "" FORCE)
set(USE_GME_NSF ON CACHE BOOL "" FORCE)
set(USE_GME_NSFE ON CACHE BOOL "" FORCE)
set(USE_GME_SPC ON CACHE BOOL "" FORCE)
set(USE_GME_GYM OFF CACHE BOOL "" FORCE)
set(USE_GME_HES OFF CACHE BOOL "" FORCE)
set(USE_GME_KSS OFF CACHE BOOL "" FORCE)
set(USE_GME_AY OFF CACHE BOOL "" FORCE)
set(USE_GME_SAP OFF CACHE BOOL "" FORCE)
set(USE_GME_VGM OFF CACHE BOOL "" FORCE)
set(GME_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(GME_BUILD_TESTING OFF CACHE BOOL "" FORCE)
set(GME_BUILD_STATIC ON CACHE BOOL "" FORCE)
set(GME_BUILD_SHARED OFF CACHE BOOL "" FORCE)

FetchContent_Declare(
    game_music_emu
    GIT_REPOSITORY https://github.com/libgme/game-music-emu.git
    GIT_TAG 0.6.6
    GIT_SHALLOW TRUE
)
FetchContent_MakeAvailable(game_music_emu)

set(NGMV_SOURCES
    src/main.cpp
    src/app/App.cpp
    src/app/App.h
    src/audio/AudioOutput.cpp
    src/audio/AudioOutput.h
    src/audio/PlaybackEngine.cpp
    src/audio/PlaybackEngine.h
    src/backend/IMusicBackend.h
    src/backend/GmeBackend.cpp
    src/backend/GmeBackend.h
    src/core/MusicTypes.h
    src/ui/ViewerUi.cpp
    src/ui/ViewerUi.h
    src/visualizer/ChannelBuffer.cpp
    src/visualizer/ChannelBuffer.h
    src/visualizer/WaveformRenderer.cpp
    src/visualizer/WaveformRenderer.h
)

add_executable(NintendoGameMusicViewer WIN32 ${NGMV_SOURCES})

target_include_directories(NintendoGameMusicViewer PRIVATE src)
target_compile_definitions(NintendoGameMusicViewer PRIVATE SDL_MAIN_HANDLED)

if(TARGET SDL2::SDL2-static)
    set(NGMV_SDL_TARGET SDL2::SDL2-static)
elseif(TARGET SDL2::SDL2)
    set(NGMV_SDL_TARGET SDL2::SDL2)
else()
    message(FATAL_ERROR "SDL2 target was not created")
endif()

if(TARGET SDL2_ttf::SDL2_ttf-static)
    set(NGMV_TTF_TARGET SDL2_ttf::SDL2_ttf-static)
elseif(TARGET SDL2_ttf::SDL2_ttf)
    set(NGMV_TTF_TARGET SDL2_ttf::SDL2_ttf)
else()
    message(FATAL_ERROR "SDL2_ttf target was not created")
endif()

if(TARGET gme::gme)
    set(NGMV_GME_TARGET gme::gme)
elseif(TARGET gme)
    set(NGMV_GME_TARGET gme)
elseif(TARGET gme-static)
    set(NGMV_GME_TARGET gme-static)
else()
    message(FATAL_ERROR "Game_Music_Emu target was not created")
endif()

target_link_libraries(NintendoGameMusicViewer PRIVATE
    ${NGMV_SDL_TARGET}
    ${NGMV_TTF_TARGET}
    ${NGMV_GME_TARGET}
)

if(WIN32)
    target_link_libraries(NintendoGameMusicViewer PRIVATE
        comdlg32 ole32 imm32 winmm version setupapi shell32
    )
endif()

if(MSVC)
    set_property(TARGET NintendoGameMusicViewer PROPERTY CXX_STANDARD 17)
    set_property(TARGET NintendoGameMusicViewer PROPERTY MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
    target_compile_options(NintendoGameMusicViewer PRIVATE /W4 /permissive- /utf-8)
else()
    target_compile_options(NintendoGameMusicViewer PRIVATE -Wall -Wextra -Wpedantic)
endif()

set_target_properties(NintendoGameMusicViewer PROPERTIES OUTPUT_NAME "NintendoGameMusicViewer")

if(NGMV_BUILD_TESTS)
    enable_testing()
    add_executable(NgmvBackendSmokeTest
        tests/backend_smoke.cpp
        src/backend/GmeBackend.cpp
        src/core/MusicTypes.h
        src/backend/IMusicBackend.h
    )
    target_include_directories(NgmvBackendSmokeTest PRIVATE src)
    target_link_libraries(NgmvBackendSmokeTest PRIVATE ${NGMV_GME_TARGET})
    if(MSVC)
        set_property(TARGET NgmvBackendSmokeTest PROPERTY MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
    endif()
    add_test(NAME NgmvBackendSmokeTest COMMAND NgmvBackendSmokeTest)
endif()
