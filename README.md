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

# Music

Chocolate Quake supports external music playback in MP3, OGG, FLAC and WAV
formats. To enable it:

* Create a directory named `music` inside your `id1` game folder.
* Place your music tracks in this directory.

Tracks should follow the naming convention track02.ogg through track11.ogg,
matching the original CD audio.

## Supported Platforms

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
