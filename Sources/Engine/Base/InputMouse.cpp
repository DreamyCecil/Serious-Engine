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
FLOAT _fGlobalMouseScroll = 0.0f;

// Get input from a mouse
void CInput::GetMouseInput(BOOL bPreScan) {
  float fMouseX, fMouseY, fDX, fDY, fDZ;

#if SE1_PREFER_SDL
  SDL_GetRelativeMouseState(&fMouseX, &fMouseY);
  fDX = fMouseX;
  fDY = fMouseY;

#else
  OS::GetMouseState(&fMouseX, &fMouseY, FALSE);
  fDX = FLOAT(fMouseX - inp_slScreenCenterX);
  fDY = FLOAT(fMouseY - inp_slScreenCenterY);
#endif

  fDZ = _fGlobalMouseScroll;
  _fGlobalMouseScroll = 0;

  FLOAT fSensitivity = inp_fMouseSensitivity;
  if (inp_bAllowMouseAcceleration) fSensitivity *= 0.25f;

  if (inp_bMousePrecision) {
    static FLOAT _tmTime = 0.0f;
    FLOAT fD = Sqrt(fDX * fDX + fDY * fDY);

    if (fD < inp_fMousePrecisionThreshold) {
      _tmTime += 0.05f;
    } else {
      _tmTime = 0.0f;
    }

    if (_tmTime > inp_fMousePrecisionTimeout) fSensitivity /= inp_fMousePrecisionFactor;
  }

  static FLOAT fDXOld;
  static FLOAT fDYOld;
  static TIME tmOldDelta;
  static CTimerValue tvBefore;
  CTimerValue tvNow = _pTimer->GetHighPrecisionTimer();
  TIME tmNowDelta = ClampDn((tvNow - tvBefore).GetSeconds(), 0.001);

  tvBefore = tvNow;

  FLOAT fDXSmooth = (fDXOld * tmOldDelta + fDX * tmNowDelta) / (tmOldDelta + tmNowDelta);
  FLOAT fDYSmooth = (fDYOld * tmOldDelta + fDY * tmNowDelta) / (tmOldDelta + tmNowDelta);
  fDXOld = fDX;
  fDYOld = fDY;
  tmOldDelta = tmNowDelta;

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
