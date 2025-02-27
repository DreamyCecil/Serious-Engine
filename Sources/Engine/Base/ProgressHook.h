/* Copyright (c) 2002-2012 Croteam Ltd. 
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

#ifndef SE_INCL_PROGRESSHOOK_H
#define SE_INCL_PROGRESSHOOK_H
#ifdef PRAGMA_ONCE
  #pragma once
#endif

// structure describing current state during loading, passed to loading hook
class CProgressHookInfo {
public:
  CTString phi_strDescription;    // world/savegame/session that is loading/connecting, etc.
  FLOAT phi_fCompleted;           // completed ratio [0..1]
};

// [Cecil] Function pointer type for convenience
typedef void (*FProgressHook)(CProgressHookInfo *pgli);

// set hook for loading/connecting
ENGINE_API void SetProgressHook(FProgressHook pHook_t);
// call loading/connecting hook
ENGINE_API void SetProgressDescription(const CTString &strDescription);
ENGINE_API void CallProgressHook_t(FLOAT fCompleted);

// [Cecil] Get current hook (e.g. for restoration purposes)
ENGINE_API FProgressHook GetProgressHook(void);

// [Cecil] Toggle hook state
ENGINE_API void SetProgressHookState(BOOL bState);

// [Cecil] Get hook state
ENGINE_API BOOL GetProgressHookState(void);

#endif  /* include-once check. */

