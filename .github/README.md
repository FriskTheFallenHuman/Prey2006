<p align="center"> <img src="https://cdn2.steamgriddb.com/logo_thumb/c5ab0bc60ac7929182aadd08703f1ec6.png">

> [!TIP]
> We have a discord server, feel free to join the discussion, the invite is [here.](https://discord.gg/tJDGrk6w4H)

> [!CAUTION]
> You **MUST** source your own legal copy of Prey (2006) to run this, i wouldn't provide any links for this.
> i've been getting a lot of questions about but, please keep in mind this is just a source port.

> [!IMPORTANT]
> We are currently looking for contributors that wants to contribute into this project.

> [!CAUTION]
> This project is far from being completed, currently there is a lot of visual problems with guis and the game portals.
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

- Clone the repo.
- Install [Visual Studio 2019](https://visualstudio.microsoft.com/vs/older-downloads/) or Visual [Visual Studio 2022](https://visualstudio.microsoft.com/vs/)
- [CMake](https://cmake.org/download/)
- Run either **cmake_msvc2019.cmd** or **cmake_msvc2022.cmd**

For Debian/Ubuntu

- Intall the dependencies
  - ```sudo apt install libgl1-mesa-dev libsdl2-dev libopenal-dev libcurl4-openssl-dev cmake ninja-build```
- Make **cmake_linux.sh** a executable and execute with
  - ```.\cmake_linux.sh gcc release```

For other Linux Distros: It should compile just fine in theory.

## Credits

This fork wouldn't be possible by the 3 previous attempts of porting Prey before me:

- [PreyDoom](https://github.com/jmarshall23/PreyDoom)
- [PreyVR](https://github.com/neitey/PreyVR)
- [com.n0n3m4.diii4a](https://github.com/glKarin/com.n0n3m4.diii4a)
- [dhewm3](https://github.com/dhewm/dhewm3)
