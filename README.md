# Quake II Classic Client for GNU/Linux with Steam Audio support (binaural sound)

![Quake II classic for Linux](github-screenshot.jpg)


REQUIRED Steam Audio `libphonon.so` library in the .quake2 directory!
---

Download C API(ZIP) from:
[https://valvesoftware.github.io/steam-audio/downloads.html](https://valvesoftware.github.io/steam-audio/downloads.html)

Extract `libphonon.so` library from the zip file `steamaudio/lib/linux-x64/libphonon.so` into the .quake2 directory!

Or extract from the included file `libphonon.so.zip` that you can find in the `linux/libphonon.so.zip` in this project.

* In the console type *s_hrtf 1*

## Acknowledgement

This repository is a streamlined fork of [jdolan/quake2](https://github.com/jdolan/quake2). It includes only the essential source code, excluding the large `.pack` data file for a more lightweight repository. Credit is due to jdolan for their work, which saved Linux base code that made these fixes and improvements possible.

## Overview

This is a 64-bit client for the classic *[Quake II](http://en.wikipedia.org/wiki/Quake_II)* on GNU/Linux, based on the abandoned AprQ2 project by _maniac_.

## Improvements in this Fork

The primary focus of this fork has been a complete overhaul of the legacy sound system. The original AprQ2 client used an SDL 1.2 implementation prone to instability on modern multi-core systems, leading to frequent freezes, crashes, and sound glitches.

This version introduces a robust, thread-safe sound mixer that resolves these critical issues:
*   Fixed numerous race conditions and deadlocks that caused the game to freeze.
*   Implemented an on-the-fly audio conversion system using SDL. This allows the engine to correctly play mixed 8-bit and 16-bit sound files, restoring previously silent weapon sounds and improving overall audio quality.
*   Ensured stable operation on modern Linux distributions using PulseAudio and PipeWire.

## Downloads

If you do not own the full version of Quake II, you can download the shareware game data (v3.14), which includes the first single-player unit and several multiplayer maps. This is enough to get the game running.

*   [Quake II Shareware Data from http://tastyspleen.net/downloads](http://tastyspleen.net/quake/downloads/)

## Compiling from Source

Successfully compiled on Ubuntu 20.04 LTS and similar distributions.

#### 1. Install Build Tools

First, prepare your system with the essential build tools:

```bash
sudo apt-get install build-essential automake git -y
```
Build tools

build-essential – installs the core compilation toolchain (GCC, linker, standard C/C++ libraries, and make) required to build the project.

automake – generates portable Makefile templates used by the build system.

git – version control system used to clone the repository and manage source code changes.

#### 2. Install Dependencies

Next, install the libraries required by the game:

```bash
sudo apt-get install libpng-dev libjpeg-dev libcurl4-openssl-dev libsdl2-dev -y
```
Dependencies overview

libpng-dev – provides support for loading and saving PNG images (used for textures, UI elements, screenshots).

libjpeg-dev – enables JPEG image decoding, commonly used for textures and media assets.

libcurl4-openssl-dev – networking library used for HTTP/HTTPS communication (e.g. update checks, master server queries).

libsdl1.2-dev (legacy, to be removed) – provides window creation, input handling, audio, and OpenGL context management (deprecated).

libsdl2-dev – modern replacement for SDL 1.2; handles windowing, input, audio, and OpenGL/Vulkan context creation in a cross-platform way.

#### 3. Compile the Game

Clone this repository and use `make` to compile the client:

```bash
git clone https://github.com/Fryziu/Q2classic.git
cd Q2classic
make
```

The compiled files will be located in the `bin/` directory.

## Installation and Running

#### 1. Prepare Game Directories

Quake II expects its files to be in a specific location. Create the necessary directories in your home folder:

```bash
mkdir -p ~/.quake2/baseq2
```

#### 2. Copy Game Binaries

Copy the compiled executable and game library to their respective locations:

```bash
cp bin/q2classic ~/.quake2/
cp bin/q2game.so ~/.quake2/baseq2/
```

#### 3. Add Game Data

To play, you need the game data files (`.pak` files).

*   **For the full game:** Copy `pak0.pak` and any other `.pak` files from your retail _Quake II_ installation into your `~/.quake2/baseq2/` directory.
*   **For the demo:** If you downloaded the shareware data, extract it and copy its `.pak` files into `~/.quake2/baseq2/`.

#### 4. Run the Game

Navigate to your `.quake2` directory and execute the game:

```bash
cd ~/.quake2
./q2classic
```
