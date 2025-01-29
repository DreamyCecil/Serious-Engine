/* Copyright (c) 2023-2024 Dreamy Cecil
This program is free software; you can redistribute it and/or modify
it under the terms of version 2 of the GNU General Public License as published by
the Free Software Foundation


This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License along
with this program; if not, write to the Free Software Foundation, Inc.,
51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA. */

// [Cecil] This header determines used platform and operating system and defines useful macro switches
#ifndef SE_INCL_PLATFORM_CONFIG_H
#define SE_INCL_PLATFORM_CONFIG_H

// *****************************************************************
// SYSTEM AND ARCHITECTURE TYPES ----- CAN BE EXPANDED WITH NEW ONES
// *****************************************************************

// Processor architectures (SE1_PLATFORM)
#define PLATFORM_X86 0
#define PLATFORM_X64 1

// Operating systems (SE1_SYSTEM)
#define OS_WINDOWS 0
#define OS_UNIX    1

// ********************************************************************
// AUTOMATIC DETECTION OF THE CURRENT BUILD SYSTEM AND OPERATING SYSTEM
// ********************************************************************

#if defined(__x86_64__) || defined(_M_X64)
  #define SE1_PLATFORM PLATFORM_X64
#elif defined(i386) || defined(__i386__) || defined(__i386) || defined(_M_IX86)
  #define SE1_PLATFORM PLATFORM_X86
#else
  #error Architecture unsupported!
#endif

#if defined(_MSC_VER) || defined(_WIN32) || defined(_WIN64)
  #define SE1_SYSTEM OS_WINDOWS
#elif defined(__GNUC__) || defined(__linux__)
  #define SE1_SYSTEM OS_UNIX
#else
  #error Operating system unsupported!
#endif

// Automatic switches for convenience

#define SE1_X86 (SE1_PLATFORM == PLATFORM_X86) // Is building under x86 platform?
#define SE1_X64 (SE1_PLATFORM == PLATFORM_X64) // Is building under x64 platform?

#define SE1_32BIT (SE1_X86) // Is building under any 32-bit platform?
#define SE1_64BIT (SE1_X64) // Is building under any 64-bit platform?

#define SE1_WIN  (SE1_SYSTEM == OS_WINDOWS) // Is building under Windows OS?
#define SE1_UNIX (SE1_SYSTEM == OS_UNIX)    // Is building under Unix OS?

// Automatic compiler switches

#if defined(_MSC_VER) && _MSC_VER < 1800
  #define SE1_INCOMPLETE_CPP11 1 // Using a compiler with incomplete/no support of C++11
#else
  #define SE1_INCOMPLETE_CPP11 0 // Using a compiler with complete support of C++11
#endif

#if defined(_MSC_VER) && _MSC_VER < 1600
  #define SE1_OLD_COMPILER 1 // Using an old compiler from Visual C++ 6.0
#else
  #define SE1_OLD_COMPILER 0 // Using some new compiler
#endif

// **************************************************************************
// FEATURE SWITCHES ----- CAN BE TOGGLED OR REDEFINED FOR BUILD CUSTOMIZATION
// **************************************************************************

#ifndef SE1_ASMOPT
#define SE1_ASMOPT        1 // Allow use of inline assembly code (0 - No; 1 - Yes)
#endif
#ifndef SE1_DITHERBITMAP
#define SE1_DITHERBITMAP  1 // Enable texture dithering (0 - Disabled; 1 - Enabled)
#endif
#ifndef SE1_MMXINTOPT
#define SE1_MMXINTOPT     1 // Use MMX intrinsics for light calculations (0 - No; 1 - Yes)
#endif
#ifndef SE1_SINGLE_THREAD
#define SE1_SINGLE_THREAD 0 // Run all timer logic manually in the same thread as the application logic (0 - No; 1 - Yes)
#endif
#ifndef SE1_DOUBLE_TIMER
#define SE1_DOUBLE_TIMER  0 // Use double-precision floating-point type for timer values (0 - No; 1 - Yes)
#endif
#ifndef SE1_DIRECT3D
#define SE1_DIRECT3D      0 // Implement rendering using Direct3D 8 (0 - Disabled; 1 - Enabled)
#endif
#ifndef SE1_TRUFORM
#define SE1_TRUFORM       0 // Truform support (0 - Disabled; 1 - Enabled)
#endif
#ifndef SE1_3DFX
#define SE1_3DFX          0 // 3Dfx graphics driver support for OpenGL (0 - Disabled; 1 - Enabled)
#endif
#ifndef SE1_GLEW
#define SE1_GLEW          0 // GLEW support (0 - None; 1 - Available; 2 - Replace manual hooking of OpenGL in the engine)
#endif
#ifndef SE1_USE_SDL
#define SE1_USE_SDL       0 // Prefer SDL over Windows API (0 - No; 1 - Yes)
#endif

// Sound API switches specifically for Windows platforms

#ifndef SE1_SND_WAVEOUT
#define SE1_SND_WAVEOUT   1 // Implement sound using WaveOut (0 - Disabled; 1 - Enabled)
#endif
#ifndef SE1_SND_DSOUND
#define SE1_SND_DSOUND    1 // Implement sound using DirectSound (0 - Disabled; 1 - Enabled)
#endif
#ifndef SE1_SND_EAX
#define SE1_SND_EAX       0 // Implement EAX support via DirectSound, if it's enabled (0 - Disabled; 1 - Enabled)
#endif
#ifndef SE1_SND_SDLAUDIO
#define SE1_SND_SDLAUDIO  1 // Implement sound using SDL Audio regardless of SE1_USE_SDL (0 - Disabled; 1 - Enabled)
#endif

// Automatic switches for convenience

#define SE1_PREFER_SDL (!SE1_WIN || SE1_USE_SDL)                // Prefer SDL over Windows API?
#define SE1_USE_ASM    (SE1_WIN && SE1_X86 && SE1_ASMOPT)       // Prioritize inline assembly under a traditional platform?
#define SE1_USE_MMXINT (!SE1_WIN || (SE1_X86 && SE1_MMXINTOPT)) // Prioritize MMX intrinsic functions?

// **************************************************************************
// EXPERIMENTAL FEATURES ----- USE WITH CAUTION SOLELY FOR DEBUGGING PURPOSES
// **************************************************************************

// For making sure that all printing functions with variadic arguments only take numbers and pointers (e.g. allow 'char *' but not 'CTString')
#define SE1_EXF_VERIFY_VA_IN_PRINTF 0

#endif // include-once check
