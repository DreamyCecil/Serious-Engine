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

#include "StdH.h"

#include <Engine/Base/Input.h>

extern FLOAT inp_fMouseSensitivity;
extern INDEX inp_bAllowMouseAcceleration;
extern INDEX inp_bMousePrecision;
extern FLOAT inp_fMousePrecisionFactor;
extern FLOAT inp_fMousePrecisionThreshold;
extern FLOAT inp_fMousePrecisionTimeout;
extern FLOAT inp_bInvertMouse;
extern INDEX inp_bFilterMouse;

// Accumulated wheel scroll in a certain direction (Z)
FLOAT _fGlobalMouseScroll = 0;

// Specific mouse data that's remembered between the input reads
struct MouseInputData_t {
  FLOAT tmTime;
  FLOAT fDXOld;
  FLOAT fDYOld;
  TIME tmOldDelta;
  CTimerValue tvBefore;

  MouseInputData_t() : tmTime(0.0f), fDXOld(0.0f), fDYOld(0.0f), tmOldDelta(0.0) {
    tvBefore.Clear();
  };
};

static MouseInputData_t _midGlobal;

#if SE1_PREFER_SDL

CInput::MouseDevice_t::MouseDevice_t() : iID(0), iInfoSlot(-1), pmid(new MouseInputData_t)
{
  ResetMotion();
};

CInput::MouseDevice_t::~MouseDevice_t() {
  Disconnect();
  delete pmid;
};

// Open a mouse under some slot
void CInput::MouseDevice_t::Connect(SDL_MouseID iDevice, INDEX iArraySlot) {
  ASSERT(!IsConnected() && iInfoSlot == -1);

  iID = iDevice;
  iInfoSlot = iArraySlot + 1;

  ResetMotion();
  *pmid = MouseInputData_t();

  // [Cecil] TEMP: Remember device name
  const char *strDevice = SDL_GetMouseNameForID(iID);
  if (strDevice == NULL) strDevice = SDL_GetError();

  strName = strDevice;
};

// Close an open mouse
void CInput::MouseDevice_t::Disconnect(void) {
  if (IsConnected()) {
    // [Cecil] FIXME: Doesn't work during SDL_EVENT_MOUSE_REMOVED
    /*const char *strName = SDL_GetMouseNameForID(iID);

    if (strName == NULL) {
      strName = SDL_GetError();
    }*/

    CPrintF(TRANS("Disconnected '%s' from controller slot %d\n"), strName.ConstData(), iInfoSlot);
  }

  iID = 0;
  iInfoSlot = -1;
};

// Check if the mouse is connected
BOOL CInput::MouseDevice_t::IsConnected(void) const {
  return (iID != 0);
};

// Display info about current mice
void CInput::PrintMiceInfo(void) {
  if (_pInput == NULL) return;

  const INDEX ct = _pInput->inp_aMice.Count();
  CPrintF(TRANS("%d mouse slots:\n"), ct);

  for (INDEX i = 0; i < ct; i++) {
    MouseDevice_t &mouse = _pInput->inp_aMice[i];
    CPrintF(" %d. ", i + 1);

    if (!mouse.IsConnected()) {
      CPutString(TRANS("not connected\n"));
      continue;
    }

    CPrintF("'%s'\n", mouse.strName.ConstData());
  }

  CPutString("-\n");
};

// Open a mouse under some device index
void CInput::OpenMouse(SDL_MouseID iDevice) {
  // Check if this mouse is already connected
  if (GetMouseSlotForDevice(iDevice) != -1) return;

  // Find an empty slot for opening a new mouse
  MouseDevice_t *pToOpen = NULL;
  INDEX iArraySlot = -1;
  const INDEX ct = inp_aMice.Count();

  for (INDEX i = 0; i < ct; i++) {
    if (!inp_aMice[i].IsConnected()) {
      // Remember mouse and its slot
      pToOpen = &inp_aMice[i];
      iArraySlot = i;
      break;
    }
  }

  // No slots left
  if (pToOpen == NULL || iArraySlot == -1) {
    CPrintF(TRANS("Cannot open another mouse due to all %d slots being occupied!\n"), ct);
    return;
  }

  pToOpen->Connect(iDevice, iArraySlot);

  if (!pToOpen->IsConnected()) {
    CPrintF(TRANS("Cannot open another mouse! SDL Error: %s\n"), SDL_GetError());
    return;
  }

  // Report mouse info
  CPrintF(TRANS("Connected '%s' to mouse slot %d\n"), pToOpen->strName.ConstData(), pToOpen->iInfoSlot);
};

// Close a mouse under some device index
void CInput::CloseMouse(SDL_MouseID iDevice) {
  INDEX iSlot = GetMouseSlotForDevice(iDevice);

  if (iSlot != -1) {
    inp_aMice[iSlot].Disconnect();
  }
};

// [Cecil] Retrieve a mouse device by its device index
CInput::MouseDevice_t *CInput::GetMouseByID(SDL_JoystickID iDevice, INDEX *piSlot) {
  INDEX i = GetMouseSlotForDevice(iDevice);
  if (piSlot != NULL) *piSlot = i;

  return GetMouseFromSlot(i);
};

// Retrieve a mouse device by its array index
CInput::MouseDevice_t *CInput::GetMouseFromSlot(INDEX iSlot) {
  if (iSlot < 0 || iSlot >= inp_aMice.Count()) return NULL;
  return &inp_aMice[iSlot];
};

// Find mouse slot from its device index
INDEX CInput::GetMouseSlotForDevice(SDL_MouseID iDevice) const {
  INDEX i = inp_aMice.Count();

  while (--i >= 0) {
    const MouseDevice_t &mouse = inp_aMice[i];

    // No open mouse
    if (!mouse.IsConnected()) continue;

    // Check device ID of the mouse
    if (mouse.iID == iDevice) {
      return i;
    }
  }

  // Couldn't find
  return -1;
};

#endif // SE1_PREFER_SDL

// Mouse setup on initialization
void CInput::StartupMice(void) {
#if SE1_PREFER_SDL
  // Create an empty array of mice
  ASSERT(inp_aMice.Count() == 0);
  inp_aMice.New(_ctMaxInputDevices);

  // Report on available mouse amounts
  int ctMice;
  SDL_MouseID *aMice = SDL_GetMice(&ctMice);

  if (aMice != NULL) {
    CPrintF(TRANS("  mice found: %d\n"), ctMice);
    CPrintF(TRANS("  mice allowed: %d\n"), inp_aMice.Count());

    // [Cecil] FIXME: This will "register" all connected mice on game start, though ideally SDL should be sending
    // an SDL_EVENT_MOUSE_ADDED event for each mouse automatically like it already does for game controllers.
    for (int iMouse = 0; iMouse < ctMice; iMouse++) {
      _pInput->OpenMouse(aMice[iMouse]);
    }

    SDL_free(aMice);
    return;
  }

  CPrintF(TRANS("  Cannot retrieve an array of mice! SDL Error: %s\n"), SDL_GetError());
  CPrintF(TRANS("  mice allowed: %d\n"), inp_aMice.Count());
#endif // SE1_PREFER_SDL
};

// Mouse cleanup on destruction
void CInput::ShutdownMice(void) {
#if SE1_PREFER_SDL
  inp_aMice.Clear();
#endif
};

// Get input from a specific mouse (-1 for any)
void CInput::GetMouseInput(BOOL bPreScan, INDEX iMouse) {
  float fMouseX, fMouseY, fDX, fDY, fDZ;

#if SE1_PREFER_SDL
  MouseInputData_t *pmid = &_midGlobal;

  if (iMouse >= 0 && iMouse < inp_aMice.Count()) {
    MouseDevice_t &mouse = inp_aMice[iMouse];

    // Get accumulated mouse motion and reset it
    fDX = mouse.vMotion[0];
    fDY = mouse.vMotion[1];
    fDZ = mouse.fScroll;
    mouse.ResetMotion();
    pmid = inp_aMice[iMouse].pmid;

  } else {
    SDL_GetRelativeMouseState(&fMouseX, &fMouseY);
    fDX = fMouseX;
    fDY = fMouseY;
    fDZ = _fGlobalMouseScroll;
    _fGlobalMouseScroll = 0;
  }

  MouseInputData_t &mid = *pmid;

#else
  (void)iMouse;

  OS::GetMouseState(&fMouseX, &fMouseY, FALSE);
  fDX = FLOAT(fMouseX - inp_slScreenCenterX);
  fDY = FLOAT(fMouseY - inp_slScreenCenterY);
  fDZ = _fGlobalMouseScroll;
  _fGlobalMouseScroll = 0;

  MouseInputData_t &mid = _midGlobal;
#endif

  FLOAT fSensitivity = inp_fMouseSensitivity;
  if (inp_bAllowMouseAcceleration) fSensitivity *= 0.25f;

  if (inp_bMousePrecision) {
    FLOAT fD = Sqrt(fDX * fDX + fDY * fDY);

    if (fD < inp_fMousePrecisionThreshold) {
      mid.tmTime += 0.05f;
    } else {
      mid.tmTime = 0.0f;
    }

    if (mid.tmTime > inp_fMousePrecisionTimeout) fSensitivity /= inp_fMousePrecisionFactor;
  }

  CTimerValue tvNow = _pTimer->GetHighPrecisionTimer();
  TIME tmNowDelta = ClampDn((tvNow - mid.tvBefore).GetSeconds(), 0.001);

  mid.tvBefore = tvNow;

  FLOAT fDXSmooth = (mid.fDXOld * mid.tmOldDelta + fDX * tmNowDelta) / (mid.tmOldDelta + tmNowDelta);
  FLOAT fDYSmooth = (mid.fDYOld * mid.tmOldDelta + fDY * tmNowDelta) / (mid.tmOldDelta + tmNowDelta);
  mid.fDXOld = fDX;
  mid.fDYOld = fDY;
  mid.tmOldDelta = tmNowDelta;

  if (inp_bFilterMouse) {
    fDX = fDXSmooth;
    fDY = fDYSmooth;
  }

  // Get final mouse values
  FLOAT fMouseRelX = +fDX * fSensitivity;
  FLOAT fMouseRelY = -fDY * fSensitivity;

  if (inp_bInvertMouse) {
    fMouseRelY = -fMouseRelY;
  }

  // Just interpret values as normal
  inp_aInputActions[FIRST_AXIS_ACTION + EIA_MOUSE_X].ida_fReading = fMouseRelX;
  inp_aInputActions[FIRST_AXIS_ACTION + EIA_MOUSE_Y].ida_fReading = fMouseRelY;
  inp_aInputActions[FIRST_AXIS_ACTION + EIA_MOUSE_Z].ida_fReading = fDZ * MOUSEWHEEL_SCROLL_INTERVAL;

#if !SE1_PREFER_SDL
  // Set cursor position to the screen center
  if (FloatToInt(fMouseX) != inp_slScreenCenterX || FloatToInt(fMouseY) != inp_slScreenCenterY) {
    SetCursorPos(inp_slScreenCenterX, inp_slScreenCenterY);
  }
#endif
};
