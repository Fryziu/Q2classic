<!-- Name: Quake2 classic -->

# Quake II classic for GNU Linux

![Quake II classic for Linux](github-screenshot.jpg)

## Overview

Here you'll find packages of classic _[Quake II](http://en.wikipedia.org/wiki/Quake_II)_ for 64 bit GNU/Linux.
These builds are based on the abandoned *AprQ2* project by _maniac_.
Some of changes include:

 * R1Q2 Protocol 35 support
 * Anisotropic filtering for SDL video
 * Multisample (FSAA) for SDL video
 * Stencil shadows for SDL video
 * Numerous stability fixes around SDL audio
 * Numerous 64 bit compatibility fixes

A full list of changes is available in the [CHANGELOG](CHANGELOG).

## Downloads

You may download additional packages with the 3.14 demo and 3.20 point release data (one Single Player mission, official Deathmatch maps, and Capture the Flag). You can play the game immediately after installing.

 * [Quake II (Quetoo.org)](http://quetoo.org/files/quake2-quetoo.org-x86_64.tar.gz) for GNU/Linux 64 bit

## Compiling

Tested with Xubuntu 20.04 LTS.
Before compiling this client of _Quake II_ 
be sure to prepare your system for compilation, you will need:

_build-essential automake git_

that can be installed with this command:

    sudo apt-get install build-essential automake git -y

Dependencies of the game itself are:

_liblz1 libjpeg-dev libcurl4-gnutls-dev mesa-common-dev libsdl1.2-dev_ 

that can be installed with this command:

    sudo apt-get install liblz1 libjpeg-dev libcurl4-gnutls-dev mesa-common-dev libsdl1.2-dev -y

## Installation

Using the terminal clone the data with a command:

    git clone https://github.com/Fryziu/Quake2classic.git

Get inside `Quake2classic` and use a command _make_ to compile it.
Compiled files you may find in `Quake2classic/bin`.
copy _quake2_ to your `~.quake2` folder,
copy _game.so_ to your `~.quake2/baseq2` folder.
Using the terminal get into your `.quake2` folder and type _./quake2_ to run

If you want to play the full single-player game, you must provide the retail game data that came with your _Quake II_ .
Locate the `pak0.pak` file in your _Quake II_ and copy it to your user-specific `.quake2/baseq2` folder:
  
    
