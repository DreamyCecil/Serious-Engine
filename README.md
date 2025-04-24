# Serious Engine

This is a fork of Croteam's Serious Engine 1.10 with personal adjustments aimed at making the code easier to maintain and allowing the engine to be built under any sort of configuration. It also includes a lot of quality-of-life improvements over the original code without adding too much.

**This project is constantly work-in-progress with currently no final goal or any intention of ever becoming commercially-ready. Fork at your own risk.**

# Table of Contents

- [Project structure](#Project-structure)
- [Building](#Building)
  - [Windows](#Windows)
  - [Linux](#Linux)
- [Optional features](#Optional-features)
  - [Global features](#Global-features)
  - [Windows-only features](#Windows-only-features)
- [Notes](#Notes)
  - [Running](#Running)
  - [Common problems](#Common-problems)
- [License](#License)

# Project structure

### Engine components
- `DedicatedServer` - Dedicated server application for hosting multiplayer games
- `Engine` - Serious Engine 1.10
- `EngineGUI` - Common GUI components for game tools
- `EntitiesMP` - All the entity logic
- `GameGUIMP` - Specific GUI components for game tools
- `GameMP` - Module for handling basic game logic
- `SeriousSam` - The main game executable
- `Shaders` - Compiled shaders for SKA models

### Engine tools
- `DecodeReport` - Tool for decoding crash report files (`*.rpt`)
- `Depend` - Tool for generating a list of dependency files based on a list of root files
- `MakeFONT` - Tool for generating font files for the game (`*.fnt`)
- `Modeler` - Serious Modeler application for creating and configuring models with vertex animations
- `RCon` - Remote console application for connecting to servers using an admin password
- `SeriousSkaStudio` - Serious SKA Studio application for creating and configuring models with skeletal animations
- `WorldEditor` - Serious Editor application for creating in-game levels

### Other projects
- `ECC` - Separate fork of Entity Class Compiler from Serious Engine 1 that is engine-agnostic
- `GameAgent` - Custom master server emulator written in Python
- `libogg`, `libvorbis` - Third party libraries for playing OGG-encoded in-game music
- `LWSkaExporter` - Exporter of SKA models in ASCII format for use in LightWave
- `SDL3` - Third party library that helps with porting the engine to other platforms
- `zlib` - Third party static library for working with ZIP archives

These have been modified to run correctly under the recent versions of Windows. (Tested: Win7 x64, Win8 x64, Win8.1 x64, Win10 x64)

# Building

## Windows

### Prerequisite
Before building, you need to update all submodules in the repository:
```
git submodule update --init --recursive
```

### Instructions
1. Install **Visual Studio 2010** or later.
2. Open `Sources/All.sln` solution.
3. Press F7 or **Build** -> **Build solution** to build the entire project.

Once the project is built, the libraries and executables will be copied under the `Bin/` directory.

> [!TIP]
> - To temporarily prevent all projects from rebuilding each time `Engine` project is built, set the first value in the `Sources/Engine/GenerateCommitHash.ps1` script to `$false`.

## Linux

### Prerequisite
Before building, you need to install certain modules if they aren't already there.

1. Install **Git** and **CMake** tools with `sudo apt install git cmake`
2. Install **GCC** and other build tools with `sudo apt install build-essential` (if you don't already have GCC)
3. Install **Bison** and **Flex** tools with `sudo apt install bison flex` (if you've installed GCC separately without them)
4. Install **OpenAL** library with `sudo apt-get install libopenal-dev`
5. Clone the repository in any directory with `git clone https://github.com/DreamyCecil/Serious-Engine.git`
6. Update all submodules in the repository with `git submodule update --init --recursive`

### Instructions
1. Navigate to the `Sources/` directory and open the terminal from there.
2. Run `build-linux32.sh` or `build-linux64.sh` script to build the project under the `x86` or `x64` platform respectively.

You can modify the build scripts to suit your needs, e.g. select a different build configuration or toggle specific behavior flags.

Once the project is built, you can install it into the `Bin/` directory by running `Sources/deploy-linux.sh` script.

> [!TIP]
> - To rebuild the project from scratch, you can add `rebuild` argument to the script like this: `./build-linux64.sh rebuild`.
> - To temporarily prevent all projects from rebuilding each time, add `-DSE1_DUMMY_BUILD_INFO=1` to the CMake command.

# Optional features

These features are disabled by default but can be enabled if you wish to extend the capabilities of your Serious Engine build.

Some features that can be customized using macros can be toggled by modifying the `Sources/Engine/SE_Config.h` header file instead of property sheets or project files.

## Global features

These features are available on all platforms, or at least can be built out-of-the-box without problems.

### Truform
Its functionality is disabled by default. To enable it, you need to switch the `SE1_TRUFORM` macro to `1` (or define it as such for all projects).

### Simple DirectMedia Layer (SDL)
It's included with the source code and is initialized by the engine but its functionality is mostly unused by default for the Windows platform. To prioritize it over Win32 API, you need to switch the `SE1_USE_SDL` macro to `1` (or define it as such for all projects).
- It is always prioritized on non-Windows platforms because cross-platform code cannot function without it.
- SDL is incompatible with Microsoft's MFC library, which is used for tool applications (such as Serious Editor), making it impossible to build them with the game on Windows.

### OpenAL
The engine can optionally implement cross-platform sound playback using OpenAL. Under Windows platforms, it relies on prebuilt binaries of the OpenAL Soft library.

## Windows-only features

These features can only be built and used on Windows systems. Some of the features might become global in the future, if cross-platform code is written to support them.

### DirectX
Download DirectX8 SDK (headers & libraries) ( https://www.microsoft.com/en-us/download/details.aspx?id=6812 ) and then switch the `SE1_DIRECT3D` macro to `1` (or define it as such for all projects). You will also need to make sure the DirectX8 headers and libraries are located in the following folders (make the folder structure if it doesn't exist):
- `Tools.Win32/Libraries/DX8SDK/Include/`
- `Tools.Win32/Libraries/DX8SDK/Lib/`

### 3Dfx
Support for this outdated driver for outdated graphics cards is still present but is disabled in favor of modernity. To enable it, switch the `SE1_3DFX` macro to `1` (or define it as such for all projects).

### MP3 playback
Copy `amp11lib.dll` library that used to be distributed with older versions of **Serious Sam Classic: The First Encounter** near the executable files under the `Bin/` directory.

### 3D Exploration
Support is disabled due to copyright issues. If you need to create new models, either use editing tools from any of the original games or write your own code for 3D object import/export.

### IFeel
Support is disabled due to copyright issues. If you need IFeel support, copy `IFC22.dll` and `ImmWrapper.dll` from the original games near the executable files under the `Bin/` directory.

### The OpenGL Extension Wrangler Library (GLEW)
It's included with the source code but its functionality is disabled by default. To enable it, you need to switch the `SE1_GLEW` macro to `1` (or define it as such for all projects).
- Defining the macro with `2` makes it replace internal OpenGL hooking in the engine with GLEW methods, in case you intend to write new code using GLEW that's compatible with the engine.

# Notes

Extra information about the project and its source code.

## Running

This version of the engine comes with a set of resources (`SE1_10.gro`) that allow you to freely use the engine without any additional resources required.  
However, if you wish to open or modify levels from **Serious Sam Classic: The First Encounter** or **The Second Encounter** (including most user-made levels), you will have to copy the game's resources (`.gro` files) into the game folder.

When running a selected project, make sure that its project settings under **Debugging** are setup correctly:
- Command: `$(PostBuildCopyDir)$(TargetFileName)`
- Working Directory: `$(SolutionDir)..\`

## Common problems

- Currently, only levels from **Serious Sam Classic: The Second Encounter** are compatible with the game.
  - Levels from other games require extra adjustments in the Serious Editor to work as intended.
- `SeriousSkaStudio` has some issues with MFC windows that can prevent the main window from being displayed properly.
- Even though **Visual Studio 2010** can be used for building, the code for its support is filled with many awkward workarounds and it generally hasn't been tested very well. Such an old IDE should only be used in very specific cases where a newer Visual Studio won't cut it.
  - Projects use `$(DefaultPlatformToolset)` property for automatically selecting the toolset from the studio that you're using, which doesn't exist in **Visual Studio 2010**. You will have to manually change it to `v100`.

# License

Serious Engine is licensed under the GNU GPL v2 license (see `LICENSE` file).

### Serious Engine forks

Some ideas and code in this fork have been borrowed from other Serious Engine projects:

- [Original Linux port](https://github.com/icculus/Serious-Engine) by icculus
- [Serious Engine: Ray Traced](https://github.com/sultim-t/Serious-Engine-RT) by Sultim
- [Serious Sam Evolution](https://gitlab.com/TwilightWingsStudio/SSE/SuperProject) by Twilight Wings Studio

### Third-party

Some of the code included with the engine sources under `Sources/ThirdParty/` is not licensed under GNU GPL v2:

- **GLEW** (`glew/`) from https://glew.sourceforge.net/
- **libogg** & **libvorbis** (`libogg/`, `libvorbis/`) by Xiph.Org Foundation ( https://xiph.org/vorbis/ )
- **LightWave SDK** (`LWSkaExporter/SDK/`) by NewTek Inc.
- **OpenAL Soft** (`OpenAL-Soft/`) by kcat ( https://github.com/kcat/openal-soft )
- **SDL** (`SDL/`) from https://libsdl.org/
- **zlib** (`zlib/`) by Jean-loup Gailly and Mark Adler ( https://zlib.net/ )
