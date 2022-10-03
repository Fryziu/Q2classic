<!-- Name: Quake2 classic -->

# Quake II classic client for GNU Linux

![Quake II classic for Linux](github-screenshot.jpg)

## Overview

John Carmack and id Software team in year 1997 have released a genre defining game - Quake II.
They have crossed many boundaries before, but in this release for the first time everything was elegant and axiomatic.
Fast-paced first person shooter (or backshooter) with a single player story that helps to get along with mechanics of the game, and build in server engine - so every player can easily start, oversee and invite others to play a multiplayer game in three fundamental ways 
- Cooperative fight against ingame monsters
- Deathmatch, each on his own against all other players
- Capture the flag, teamplay with a flag that can be stolen and touchdown

Engine of the game allows for creating modifications, and there is plenty of those. If you can think about any way / mod / behavior of playing, it was done in Quake 2 (probably long before it was "discovered" in other games).
Programming language here is C, so everything that can be lightning-fast, may be, if you (as a programmer) can optimize your code properly. 
As for now this is one of the best for learning, developing, testing and creative enviroments ever, and it's open (Thank You John Carmack).
Immediately after release of the game, modding community started to tinkering with it, and one of the most intresting "clients" (binary runtime file/files that allow to run the game with added functionalities)  was AprQ2 created by Maniac (pseudonym - it's all i got here)
His work was saved, cleaned out and prepared for modern GNU/Linux systems by jdolan.
Some of changes include:

 * R1Q2 Protocol 35 support
 * Anisotropic filtering for SDL video
 * Multisample (FSAA) for SDL video
 * Stencil shadows for SDL video
 * Numerous stability fixes around SDL audio
 * Numerous 64 bit compatibility fixes

A full list of changes is available in the [CHANGELOG](CHANGELOG).

Last available original code (does not compile properly nowadays) of AprQ2 you can find in one of my repositories, a fork of working jdolan code is in my repositories as well (as a starting point for comparisons).
Here you can see a stripped version, where i intend to do all possible mistakes in my journey into the C and the first coherent Turing - complete open (GNU) creative enviroment given to us by John Carmack. You can feel invited to join it too :-)

## Downloads

To play the game you need at least a demo version of it, which should be available in your Linux distribution (at least if it's Ubuntu based)

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

    git clone https://github.com/Fryziu/Q2classic.git

Get inside `Q2classic` and use a command _make_ to compile it.
Compiled files you may find in `Q2classic/bin`.
copy _quake2_ to your `~.quake2` folder,
copy _game.so_ to your `~.quake2/baseq2` folder.
Using the terminal get into your `.quake2` folder and type _./quake2_ to run

If you want to play the full single-player game, you must provide the retail game data that came with your _Quake II_ .
Locate the `pak0.pak` file in your _Quake II_ and copy it to your user-specific `.quake2/baseq2` folder:
  
    
