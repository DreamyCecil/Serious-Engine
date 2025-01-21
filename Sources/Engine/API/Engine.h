/* Copyright (c) 2002-2012 Croteam Ltd.
   Copyright (c) 2025 Dreamy Cecil
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

// [Cecil] This header file contains declarations that used to be in <Engine/Engine.h>
#ifndef SE_INCL_API_ENGINE_H
#define SE_INCL_API_ENGINE_H

#ifdef PRAGMA_ONCE
  #pragma once
#endif

// [Cecil] Engine setup properties
struct SeriousEngineSetup {
  // Possible application types
  enum EAppType {
    E_OTHER,  // Some other application or tool without gameplay functionality
    E_GAME,   // Game client
    E_SERVER, // Dedicated server
    E_EDITOR, // World editor
  };

  // Generic application name, e.g. game ("Serious Sam"), tool ("Serious Editor") etc.
  CTString strAppName;

  // Current application type
  EAppType eAppType;

  // Custom game root directory (just for the setup, not necessarily the same as the eventual _fnmApplicationPath)
  CTString strSetupRootDir;

  // Constructor with application name
  __forceinline SeriousEngineSetup(const CTString &strSetName = "") :
    strAppName(strSetName), eAppType(E_OTHER), strSetupRootDir("")
  {
  };

  // App type checks
  __forceinline bool IsAppOther (void) const { return eAppType == E_OTHER; };
  __forceinline bool IsAppGame  (void) const { return eAppType == E_GAME; };
  __forceinline bool IsAppServer(void) const { return eAppType == E_SERVER; };
  __forceinline bool IsAppEditor(void) const { return eAppType == E_EDITOR; };
};

// [Cecil] Pass setup properties
ENGINE_API void SE_InitEngine(const SeriousEngineSetup &engineSetup);
ENGINE_API void SE_EndEngine(void);
ENGINE_API void SE_LoadDefaultFonts(void);
ENGINE_API void SE_UpdateWindowHandle(OS::Window hwndWindowed);
ENGINE_API void SE_PretouchIfNeeded(void);

// [Cecil] Separate methods for determining and restoring gamma adjustment
ENGINE_API void SE_DetermineGamma(OS::Window hwnd);
ENGINE_API void SE_RestoreGamma(OS::Window hwnd = NULL);

// [Cecil] Engine properties after full initialization
ENGINE_API extern const SeriousEngineSetup &_SE1Setup;

// [Cecil] TEMP: Aliases for old variables (deprecated; use is discouraged)
#define _bDedicatedServer (_SE1Setup.IsAppServer())
#define _bWorldEditorApp  (_SE1Setup.IsAppEditor())

// Engine build information
ENGINE_API extern CTString _strEngineBuild; // Invalid before calling SE_InitEngine()
ENGINE_API extern ULONG _ulEngineBuildMajor;
ENGINE_API extern ULONG _ulEngineBuildMinor;

// Filename of the log file (equal to executable filename by default)
ENGINE_API extern CTString _strLogFile;

#endif // include-once check
