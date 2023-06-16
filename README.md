kweb
====

Minimal Kiosk Browser is a small and fast web browser based on the webkit engine (same as Midori or Chromium) with support for playing audio and video files (including m3u paylists) and web video (youtube and other websites supported by youtube-dl and HTML5 video) with omxplayer. It also supports opening PDF files directly from the browser (using either mupdf or xpdf) and downloading files of any kind (using wget).

Credit goes to gkreidl (http://www.raspberrypi.org/forums/memberlist.php?mode=viewprofile&u=9343&sid=911aab9d2d1d860bdaf1bc8c30e6e712)

Updating to use gtk4 done by hestiahacker

Reference :
- http://www.raspberrypi.org/forums/viewtopic.php?t=40860

# Note
This branch is an experimental one, which cut out a bunch of functionality that
was not being used by the HestiaPi project. It is not a suitable replacement for
the default branch of kweb in all situations. If you would like the additional
features to be re-implemented, you'll have to be willing to fund the development
effort.

# Compilation

## Build Dependencies
The webkitgtk-6.0 package is only available on bookworm and later.
[source](https://packages.debian.org/bookworm/libwebkitgtk-6.0-dev)

```sh
sudo apt install webkitgtk-6.0 libgtk-4-dev
```

## Compilation
make

## Installation
sudo make install
