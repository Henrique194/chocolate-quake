## v1.0.0

### General Updates

- **64-bit System Support**: Chocolate Quake now compiles and runs correctly on
  64-bit systems.
- **Linux Build Support**: Added support for compiling Chocolate Quake on Linux.
- **macOS Performance Improvements**: macOS builds now use the GPU as a screen
  blitter via Metal, significantly improving performance.
- **Graceful Quit Handling**: The engine now properly handles quit events
  triggered by the window close button or termination signals (`SIGINT` and
  `SIGTERM`).
- **Restored Video Menu**: Re-implemented the video settings menu with support
  for 15 display modes, including both fullscreen and windowed resolutions.

### Bug Fixes

- **Music Looping**: Background music now loops correctly after reaching the end
  of a track.
- **Music in Shareware Version**: Resolved an issue where music would not play
  when using the Quake shareware files.
- **Crash on Missing Audio Device**: Fixed a segmentation fault when launching
  the game without an active audio output device.
