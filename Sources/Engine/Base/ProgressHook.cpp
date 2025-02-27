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

#include "StdH.h"
#include <Engine/Base/CTString.h>
#include <Engine/Base/ProgressHook.h>
#include <Engine/Network/Network.h>
#include <Engine/Network/CommunicationInterface.h>

static FProgressHook _pLoadingHook_t = NULL;  // hook for loading/connecting
static CProgressHookInfo _phiLoadingInfo; // info passed to the hook
BOOL _bRunNetUpdates = FALSE;
static BOOL _bCallLoadingHook = TRUE; // [Cecil] Hook state toggle

// set hook for loading/connecting
void SetProgressHook(FProgressHook pHook) {
  _pLoadingHook_t = pHook;
  _bCallLoadingHook = TRUE; // [Cecil] Enable new hook by default
};

// [Cecil] Get current hook (e.g. for restoration purposes)
FProgressHook GetProgressHook(void) {
  return _pLoadingHook_t;
};

// [Cecil] Toggle hook state
void SetProgressHookState(BOOL bState) {
  _bCallLoadingHook = bState;
};

// [Cecil] Get hook state
BOOL GetProgressHookState(void) {
  return _bCallLoadingHook;
};

// call loading/connecting hook
void SetProgressDescription(const CTString &strDescription) {
  _phiLoadingInfo.phi_strDescription = strDescription;
};

void CallProgressHook_t(FLOAT fCompleted)
{
  // [Cecil] Only call the hook if it exists and is enabled
  if (_pLoadingHook_t != NULL && _bCallLoadingHook) {
    _phiLoadingInfo.phi_fCompleted = fCompleted;
    _pLoadingHook_t(&_phiLoadingInfo);
  }

  // [Cecil] Only update the network stuff when needed
  if (!_bRunNetUpdates) return;

  // Get initial time of the last update
  static CTimerValue tvLastUpdate = _pTimer->GetHighPrecisionTimer();
  extern FLOAT net_fSendRetryWait;

  CTimerValue tvDiff = (_pTimer->GetHighPrecisionTimer() - tvLastUpdate);
  CTimerValue tvRetryDelay((DOUBLE)net_fSendRetryWait * 1.1);

  if (tvDiff > tvRetryDelay) {
    // Handle server messages
    if (_pNetwork->IsServer()) {
      _cmiComm.Server_Update();

    // Handle client messages
    } else {
      _cmiComm.Client_Update();
    }

    tvLastUpdate = _pTimer->GetHighPrecisionTimer();
  }
};
