# Chocolate Quake

Chocolate Quake is a minimalist source port of Quake focused on preserving the
original experience of version 1.09 and earlier DOS releases. Inspired by the
philosophy of Chocolate Doom, this project aims for accuracy and authenticity
over modern enhancements.

Chocolate Quake’s aims are:

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

# Music

Chocolate Quake supports external music playback in OGG format. To enable it:

* Create a directory named `music` inside your `id1` game folder.
* Place your OGG music tracks in this directory.

Tracks should follow the naming convention track02.ogg through track11.ogg,
matching the original CD audio.

## Supported Platforms

| Platform | is supported? |
|:--------:|:-------------:|
| Windows  |      yes      |
|  Linux   |      yes      |
|  MacOS   |      yes      |

# Credits

* Portions of the sound subsystem are based on code
  from [QuakeSpasm Spiked](https://github.com/Shpoike/Quakespasm), an
  enhanced Quake source port by Ozkan, Eric, Sander and Stevenauus.