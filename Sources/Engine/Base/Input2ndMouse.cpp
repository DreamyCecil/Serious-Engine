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

#include <Engine/Base/Input.h>

#if !SE1_WIN

// [Cecil] No second mouse support for non-Windows OS
void CInput::Mouse2_Clear(void) {};
void CInput::Mouse2_Startup(void) {};
void CInput::Mouse2_Shutdown(void) {};
void CInput::Mouse2_Update(void) {};

#else

extern INDEX inp_i2ndMousePort;
extern FLOAT inp_f2ndMouseSensitivity;
extern INDEX inp_b2ndMousePrecision;
extern FLOAT inp_f2ndMousePrecisionThreshold;
extern FLOAT inp_f2ndMousePrecisionTimeout;
extern FLOAT inp_f2ndMousePrecisionFactor;
extern INDEX inp_bFilter2ndMouse;
extern INDEX inp_bInvert2ndMouse;

#define MOUSECOMBUFFERSIZE 256L

static HANDLE _h2ndMouse = NONE;
static BOOL _bIgnoreMouse2 = TRUE;
static INDEX _i2ndMouseX, _i2ndMouseY, _i2ndMouseButtons;
static INDEX _iByteNum = 0;
static UBYTE _aubComBytes[4] = { 0, 0, 0, 0 };
static INDEX _iLastPort = -1;

static void Poll2ndMouse(void) {
  // Reset (mouse reading is relative)
  _i2ndMouseX = 0;
  _i2ndMouseY = 0;

  if (_h2ndMouse == NONE) return;

  // Check
  COMSTAT csComStat;
  DWORD dwErrorFlags;
  ClearCommError(_h2ndMouse, &dwErrorFlags, &csComStat);

  DWORD dwLength = Min(MOUSECOMBUFFERSIZE, (INDEX)csComStat.cbInQue);
  if (dwLength <= 0) return;

  // Readout
  UBYTE aubMouseBuffer[MOUSECOMBUFFERSIZE];
  INDEX iRetries = 999;

  while (!ReadFile(_h2ndMouse, aubMouseBuffer, dwLength, &dwLength, NULL)) {
    iRetries--;

    // Didn't make it
    if (iRetries <= 0) return;
  }

  // Parse the mouse packets
  for (INDEX i = 0; i < dwLength; i++) {
    // Prepare
    if (aubMouseBuffer[i] & 64) _iByteNum = 0;

    if (_iByteNum < 4) {
      _aubComBytes[_iByteNum] = aubMouseBuffer[i];
    }

    _iByteNum++;

    switch (_iByteNum) {
      // Buttons
      case 1: {
        _i2ndMouseButtons &= ~3;
        _i2ndMouseButtons |= (_aubComBytes[0] & (32 + 16)) >> 4;
      } break;

      // Axes
      case 3: {
        char iDX = ((_aubComBytes[0] &  3) << 6) + _aubComBytes[1];
        char iDY = ((_aubComBytes[0] & 12) << 4) + _aubComBytes[2];
        _i2ndMouseX += iDX;
        _i2ndMouseY += iDY;
      } break;

      // Third button
      case 4: {
        _i2ndMouseButtons &= ~4;
        if (aubMouseBuffer[i] & 32) _i2ndMouseButtons |= 4;
      } break;
    }
  }

  if (!_bIgnoreMouse2) return;

  // Ignore pooling
  if (_i2ndMouseX != 0 || _i2ndMouseY != 0) {
    _bIgnoreMouse2 = FALSE;
  }

  _i2ndMouseX = 0;
  _i2ndMouseY = 0;
  _i2ndMouseButtons = 0;
};

void CInput::Mouse2_Clear(void) {
  if (_h2ndMouse != NONE) {
    CloseHandle(_h2ndMouse);
  }

  _h2ndMouse = NONE;
};

void CInput::Mouse2_Startup(void) {
  Mouse2_Shutdown();

  inp_i2ndMousePort = Clamp(inp_i2ndMousePort, 0L, 4L);
  const INDEX iPort = inp_i2ndMousePort;

  // Skip if disabled
  ASSERT(iPort >= 0 && iPort <= 4);
  if (iPort == 0) return;

  // Determine port string
  CTString str2ndMousePort(0, "COM%d", iPort);

  // Create COM handle if needed
  if (_h2ndMouse == NONE) {
    _h2ndMouse = CreateFileA(str2ndMousePort.ConstData(), GENERIC_READ | GENERIC_WRITE, 0,
      NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);

    // Failed
    if (_h2ndMouse == INVALID_HANDLE_VALUE) {
      INDEX iError = GetLastError();

      /*if (iError == 5) {
        CPrintF("Cannot open %s (access denied).\n"
                "The port is probably already used by another device (mouse, modem...)\n", str2ndMousePort);
      } else {
        CPrintF("Cannot open %s (error %d).\n", str2ndMousePort, iError);
      }*/

      _h2ndMouse = NONE;
      return;
    }
  }

  // Setup and purge buffers
  SetupComm(_h2ndMouse, MOUSECOMBUFFERSIZE, MOUSECOMBUFFERSIZE);
  PurgeComm(_h2ndMouse, PURGE_TXABORT | PURGE_RXABORT | PURGE_TXCLEAR | PURGE_RXCLEAR);

  // Setup port to 1200 7N1
  DCB dcb;
  dcb.DCBlength = sizeof(DCB);
  GetCommState(_h2ndMouse, &dcb);

  dcb.BaudRate = CBR_1200;
  dcb.ByteSize = 7;
  dcb.Parity = NOPARITY;
  dcb.StopBits = ONESTOPBIT;
  dcb.fDtrControl = DTR_CONTROL_ENABLE;
  dcb.fRtsControl = RTS_CONTROL_ENABLE;
  dcb.fBinary = TRUE;
  dcb.fParity = TRUE;
  SetCommState(_h2ndMouse, &dcb);

  // Reset
  _iByteNum = 0;
  _aubComBytes[0] = _aubComBytes[1] = _aubComBytes[2] = _aubComBytes[3] = 0;
  _bIgnoreMouse2 = TRUE; // Ignore mouse polling until 1 after non-0 readout
  _iLastPort = iPort;

  //CPutString("STARTUP M2!\n");
}

void CInput::Mouse2_Shutdown(void) {
  // Already disabled
  if (_h2ndMouse == NONE) return;

  // Disable
  SetCommMask(_h2ndMouse, 0);
  EscapeCommFunction(_h2ndMouse, CLRDTR);
  EscapeCommFunction(_h2ndMouse, CLRRTS);
  PurgeComm(_h2ndMouse, PURGE_TXABORT | PURGE_RXABORT | PURGE_TXCLEAR | PURGE_RXCLEAR);

  // Close port if changed
  if (_iLastPort != inp_i2ndMousePort) {
    Mouse2_Clear();
  }

  _bIgnoreMouse2 = TRUE;
};

void CInput::Mouse2_Update(void) {
  if (_h2ndMouse == NONE) return;

  Poll2ndMouse();
  //CPrintF("m2X: %4d, m2Y: %4d, m2B: 0x%02X\n", _i2ndMouseX, _i2ndMouseY, _i2ndMouseButtons);

  // Handle 2nd mouse buttons
  if (_i2ndMouseButtons & 2) inp_aInputActions[KID_2MOUSE1].ida_fReading = 1;
  if (_i2ndMouseButtons & 1) inp_aInputActions[KID_2MOUSE2].ida_fReading = 1;
  if (_i2ndMouseButtons & 4) inp_aInputActions[KID_2MOUSE3].ida_fReading = 1;

  // Handle 2nd mouse movement
  FLOAT fDX = _i2ndMouseX;
  FLOAT fDY = _i2ndMouseY;
  FLOAT fSensitivity = inp_f2ndMouseSensitivity;

  if (inp_b2ndMousePrecision) {
    static FLOAT _tm2Time = 0.0f;
    const FLOAT fD = Sqrt(fDX * fDX + fDY * fDY);

    if (fD < inp_f2ndMousePrecisionThreshold) {
      _tm2Time += 0.05f;
    } else {
      _tm2Time = 0.0f;
    }

    if (_tm2Time > inp_f2ndMousePrecisionTimeout) {
      fSensitivity /= inp_f2ndMousePrecisionFactor;
    }
  }

  static FLOAT f2DXOld;
  static FLOAT f2DYOld;
  static TIME tm2OldDelta;
  static CTimerValue tv2Before;

  CTimerValue tvNow = _pTimer->GetHighPrecisionTimer();
  TIME tmNowDelta = ClampDn((tvNow - tv2Before).GetSeconds(), 0.001);

  tv2Before = tvNow;

  const FLOAT fDXSmooth = (f2DXOld * tm2OldDelta + fDX * tmNowDelta) / (tm2OldDelta + tmNowDelta);
  const FLOAT fDYSmooth = (f2DYOld * tm2OldDelta + fDY * tmNowDelta) / (tm2OldDelta + tmNowDelta);
  f2DXOld = fDX;
  f2DYOld = fDY;
  tm2OldDelta = tmNowDelta;

  if (inp_bFilter2ndMouse) {
    fDX = fDXSmooth;
    fDY = fDYSmooth;
  }

  // Get final mouse values
  FLOAT fMouseRelX = +fDX * fSensitivity;
  FLOAT fMouseRelY = -fDY * fSensitivity;

  if (inp_bInvert2ndMouse) fMouseRelY = -fMouseRelY;

  // Just interpret values as normal
  inp_aInputActions[FIRST_AXIS_ACTION + EIA_MOUSE2_X].ida_fReading = fMouseRelX;
  inp_aInputActions[FIRST_AXIS_ACTION + EIA_MOUSE2_Y].ida_fReading = fMouseRelY;
};

#endif // SE1_WIN
