/* Copyright (c) 2025 Dreamy Cecil
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

#ifndef SE_INCL_API_CURRENTVERSION_H
#define SE_INCL_API_CURRENTVERSION_H

#ifdef PRAGMA_ONCE
  #pragma once
#endif

// Auto-generated header for Visual Studio projects
#ifdef SE1_INCLUDE_CURRENTCOMMITHASH
  #include <Engine/CurrentCommitHash.h>

#else
  // Dummy definitions
  #ifndef SE1_CURRENT_BRANCH_NAME
  #define SE1_CURRENT_BRANCH_NAME "unknown"
  #endif
  #ifndef SE1_CURRENT_COMMIT_HASH
  #define SE1_CURRENT_COMMIT_HASH "0000000000000000000000000000000000000000"
  #endif
  #ifndef SE1_BUILD_DATETIME
  #define SE1_BUILD_DATETIME "1970-01-01T00:00:00"
  #endif
#endif

#define _SE_BUILD_MAJOR 10000  // Use new number for each released version
#define _SE_BUILD_MINOR    10  // Minor versions that are data-compatibile, but are not netgame-compatibile
#define _SE_BUILD_EXTRA    ""  // Extra version with minor code changes
#define _SE_VER_STRING  "1.10" // Usually shown in server browser, etc

// Get current engine build version as a readable string
ENGINE_API CTString SE_GetBuildVersion(void);

#endif // include-once check
