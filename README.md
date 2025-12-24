# Chocolate Quake

Chocolate Quake is a minimalist source port of Quake focused on preserving the
original experience of version 1.09 and earlier DOS releases. Inspired by the
philosophy of Chocolate Doom, this project aims for accuracy and authenticity
over modern enhancements.

Chocolate Quake's aims are:

* Reproduces the behavior of Quake v1.09 (WinQuake) and earlier DOS versions
  with high accuracy, including original bugs and quirks.
* Input handling, rendering, and timing are designed to closely match the
  original experience.
* No hardware acceleration or modern visual effects.

# Philosophy

This port is for purists: no fancy enhancements, no modern effects, just Quake
as it was. If you're looking for visual upgrades or modern features, this may
not be the port for you. But if you want Quake exactly as it felt in the '90s,
you're in the right place.

# Build Instructions

Chocolate Quake uses CMake (>= 3.21) and is built as a C99 project.
Predefined CMake presets are provided to simplify building across platforms.

The repository includes vcpkg as a Git submodule, which is the recommended way
to build Chocolate Quake on Windows and macOS.

## Requirements

* C compiler with C99 support
* CMake 3.21 or newer
* Ninja (recommended)
* Git
* SDL2 >= 2.26.5
* SDL2_net
* Audio libraries:
    * libvorbis + libvorbisfile
    * libflac
    * libmad (MP3)

## Cloning the repository

If you are not using vcpkg:
> git clone https://github.com/Henrique194/chocolate-quake.git

If you plan to use vcpkg to manage dependencies:
> git clone --recurse-submodules https://github.com/Henrique194/chocolate-quake.git

If you already cloned the repository without submodules and want to fetch
vcpkg later:
> git submodule update --init --recursive

## Windows and macOS (using bundled vcpkg)

The project provides CMake presets to build with vcpkg.
From the repository root:
> cmake --preset release-vcpkg
>
> cmake --build --preset release-vcpkg

For a debug build:
> cmake --preset debug-vcpkg
>
> cmake --build --preset debug-vcpkg

## Linux

On Linux, Chocolate Quake is typically built against system-provided libraries.

Install the required dependencies using your distribution's package manager,
then build using the provided presets:
> cmake --preset release
>
> cmake --build --preset release

For a debug build:
> cmake --preset debug
>
> cmake --build --preset debug

## Build output

Once compilation is complete, the resulting executable can be found at:
- **Release build**: `cmake-build-release/src/Release`
- **Debug build**: `cmake-build-debug/src/Debug`

# Running The Game

To run Chocolate Quake, you need a directory named `id1` containing your
game data (PAK files) next to the executable. You can also customize the
location and name of the game directory using the command-line parameters
(`-basedir` and `-game`, respectively). Most IDEs also allow you to set a
working directory so the game directory can be placed in a more convenient
location, without the need for command-line parameters.

## Music

Chocolate Quake supports external music playback in MP3, OGG, FLAC and WAV
formats. To enable it:

* Create a directory named `music` inside your `id1` game folder.
* Place your music tracks in this directory.

Tracks should follow the naming convention track02.ogg through track11.ogg,
matching the original CD audio.

# Supported Platforms

| Platform | is supported? |
|:--------:|:-------------:|
| Windows  |      yes      |
|  Linux   |      yes      |
|  MacOS   |      yes      |

# Credits

Chocolate Quake builds upon the work of the Quake community and open-source
contributors. Special thanks to:

* [QuakeSpasm Spiked](https://github.com/Shpoike/Quakespasm) - Portions of the
  sound and input subsystems are adapted from QuakeSpasm Spiked. Thanks to the
  authors for their solid groundwork.
* [@arrowgent](https://github.com/arrowgent) - For thorough Linux testing and
  valuable bug reports.
* The Chocolate Quake icon is based on graphics from the [EmojiTwo](https://github.com/EmojiTwo/emojitwo)
  project, licensed under [Creative Commons Attribution International 4.0 (CC-BY-4.0)](https://creativecommons.org/licenses/by/4.0/),
  with modifications made by [@fpiesche](https://github.com/fpiesche).

Additional thanks to the broader Quake modding and source port community for
maintaining an ecosystem that made this project possible.
