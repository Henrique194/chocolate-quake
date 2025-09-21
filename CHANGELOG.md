## v1.2.0

### General Updates

- **Restored Quake End Screen**: The VGA end screen from the DOS version of
  Quake now appears when quitting the game.
- **Music Volume Control**: Music volume slider now works properly.
- **New Logo**: Updated Chocolate Quake icon created by Florian Piesche.
- **Distribution Improvements**: Added distribution files, including install
  targets for the icon, metainfo, and other assets on Linux.

---

## v1.1.1

### Bug Fixes

- **Windows XP Compatibility**: Restored support for running Chocolate Quake on
  Windows XP.
- **Mouse Wheel Console Scrolling**: Fixed an issue where the mouse wheel would
  not scroll the console.

---

## v1.1.0

### General Updates

- **Gamepad Support**: Added support for modern game controllers.
- **5-button Mouse Support**: Added support for extended mouse buttons.
- **Continuous Integration**: Set up GitHub Actions for automated builds.
- **Improved Dependency Management**: Integrated vcpkg to simplify dependency
  handling and improve cross-platform builds.

### Bug Fixes

- **Fixed Crash in Abyss of Pandemonium**: Fixed a crash that occurred when
  playing on Easy mode by restoring original limit on sound effects.
- **Texture Cleanup on Shutdown**: Fixed a double-free issue with textures when
  closing the window.

---

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
