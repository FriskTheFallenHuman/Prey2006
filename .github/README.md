<p align="center"> <img src="https://cdn2.steamgriddb.com/logo_thumb/c5ab0bc60ac7929182aadd08703f1ec6.png">

> [!IMPORTANT]
> We are currently looking for contributors that wants to contribute into this project.

> [!CAUTION]
> This project is far from being completed, currently there is ~~2~~ a lot of major blockers that prevent this from being playable.
> 1. Death Walk a key feature of Prey is missing meaning that in some parts you need to do noclip.
> 2. Visuals are a major roadblock, Spirit walks is unsable due to the lack of visuals.
> 3. Multiplayer may work but has not been tested
> 
> The binaries are provide as-is, support may be limited due to my lack of knowledge,
> This project was create due to the insatisfaction of some source ports barely fixing or improving 
> and or sacrificing features for other platforms.
> I hope you guys understand, but contributions are welcome :) 

# Introduction
Prey 2006 SDK integrated with Doom 3 GPL release

## About

This project is a code integration of the Prey 2006 SDK against the Doom 3 GPL codebase. 
This means this includes new engine side features that were in the Prey 2006 engine,
and some adjustments to the Prey 2006 SDK code.

This runs in a semi vanilla Dhewm3 v1.5.4 fork that specifically removes imgui, soft particles and joystick support for the time being.

## Compiling
For Windows:
  - Visual Studio 2019 or 2022
  - CMake
  - Run either cmake_msvc2019.cmd or cmake_msvc2022.cmd

For Linux: At the moment compiling for linux is not supported.

## Credits

This fork wouldn't be possible by the 3 previous attempts of porting Prey before me:

- [PreyDoom](https://github.com/jmarshall23/PreyDoom)
- [PreyVR](https://github.com/neitey/PreyVR)
- [com.n0n3m4.diii4a](https://github.com/glKarin/com.n0n3m4.diii4a)
- [dhewm3](https://github.com/dhewm/dhewm3)
