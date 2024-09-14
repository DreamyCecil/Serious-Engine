/* Copyright (c) 2023 Dreamy Cecil
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

#include "OS.h"

#if SE1_WIN
  #include <SDL2/include/SDL_syswm.h>

#else
  // Unique types for some Windows messages
  SDL_EventType WM_SYSKEYDOWN;
  SDL_EventType WM_SYSKEYUP;
  SDL_EventType WM_LBUTTONDOWN;
  SDL_EventType WM_LBUTTONUP;
  SDL_EventType WM_RBUTTONDOWN;
  SDL_EventType WM_RBUTTONUP;
  SDL_EventType WM_MBUTTONDOWN;
  SDL_EventType WM_MBUTTONUP;
  SDL_EventType WM_XBUTTONDOWN;
  SDL_EventType WM_XBUTTONUP;
#endif

// Destroy current window
void OS::Window::Destroy(void) {
  if (pWindow == NULL) return;

#if SE1_PREFER_SDL
  SDL_DestroyWindow(pWindow);
#else
  DestroyWindow(pWindow);
#endif

  pWindow = NULL;
};

#if SE1_WIN

// Retrieve native window handle
HWND OS::Window::GetNativeHandle(void) {
  if (pWindow == NULL) return NULL;

#if SE1_PREFER_SDL
  SDL_SysWMinfo info;

  if (!SDL_GetWindowWMInfo(pWindow, &info)) {
    FatalError(TRANS("Couldn't retrieve driver-specific information about an SDL window:\n%s"), SDL_GetError());
  }

  return info.info.win.window;

#else
  // Already native
  return pWindow;
#endif
};

#endif

// Setup controller events from button & axis actions
static BOOL SetupControllerEvent(INDEX iCtrl, OS::SE1Event &event)
{
  GameController_t &ctrl = _pInput->inp_aControllers[iCtrl];
  if (!ctrl.IsConnected()) return FALSE;

  static BOOL _abButtonStates[MAX_JOYSTICKS * SDL_CONTROLLER_BUTTON_MAX] = { 0 };
  static BOOL _abAxisStates[MAX_JOYSTICKS * SDL_CONTROLLER_AXIS_MAX] = { 0 };

  for (ULONG eButton = 0; eButton < SDL_CONTROLLER_BUTTON_MAX; eButton++) {
    const BOOL bHolding = !!SDL_GameControllerGetButton(ctrl.handle, (SDL_GameControllerButton)eButton);

    const BOOL bJustPressed  = (bHolding && !_abButtonStates[iCtrl + eButton]);
    const BOOL bJustReleased = (!bHolding && _abButtonStates[iCtrl + eButton]);

    _abButtonStates[iCtrl + eButton] = bHolding;

    if (bJustPressed) {
      SDL_zero(event);
      event.type = WM_CTRLBUTTONDOWN;
      event.ctrl.action = eButton;
      event.ctrl.dir = TRUE;
      return TRUE;
    }

    if (bJustReleased) {
      SDL_zero(event);
      event.type = WM_CTRLBUTTONUP;
      event.ctrl.action = eButton;
      event.ctrl.dir = TRUE;
      return TRUE;
    }
  }

  // [Cecil] NOTE: This code only checks whether some axis has been moved past 50% in either direction
  // in order to determine when it has been significantly moved and reset
  for (ULONG eAxis = 0; eAxis < SDL_CONTROLLER_AXIS_MAX; eAxis++) {
    const SLONG slMotion = SDL_GameControllerGetAxis(ctrl.handle, (SDL_GameControllerAxis)eAxis);

    // Holding the axis past the half of the max value in either direction
    const BOOL bHolding = Abs(slMotion) > SDL_JOYSTICK_AXIS_MAX / 2;
    const BOOL bJustPressed = (bHolding && !_abAxisStates[iCtrl + eAxis]);

    _abAxisStates[iCtrl + eAxis] = bHolding;

    if (bJustPressed) {
      SDL_zero(event);
      event.type = WM_CTRLAXISMOTION;
      event.ctrl.action = eAxis;
      event.ctrl.dir = Sgn(slMotion);
      return TRUE;
    }
  }

  return FALSE;
};

BOOL OS::PollEvent(OS::SE1Event &event) {
#if SE1_PREFER_SDL
  // Read comment above the function definition
  extern int SE_PollEventForInput(SDL_Event *pEvent);

  // Go in the loop until it finds an event it can process and return TRUE on it
  // Otherwise break from switch-case and try checking the next event
  // If none found, exits the loop and returns FALSE because there are no more events
  SDL_Event sdlevent;

  while (SE_PollEventForInput(&sdlevent))
  {
    // Reset the event
    SDL_zero(event);
    event.type = WM_NULL;

    switch (sdlevent.type) {
      // Key events
      case SDL_TEXTINPUT: {
        event.type = WM_CHAR;
        event.key.code = sdlevent.text.text[0]; // [Cecil] FIXME: Use all characters from the array
      } return TRUE;

      case SDL_KEYDOWN: {
        event.type = (sdlevent.key.keysym.mod & KMOD_ALT) ? WM_SYSKEYDOWN : WM_KEYDOWN;
        event.key.code = sdlevent.key.keysym.sym;
      } return TRUE;

      case SDL_KEYUP: {
        event.type = (sdlevent.key.keysym.mod & KMOD_ALT) ? WM_SYSKEYUP : WM_KEYUP;
        event.key.code = sdlevent.key.keysym.sym;
      } return TRUE;

      // Mouse events
      case SDL_MOUSEMOTION: {
        event.type = WM_MOUSEMOVE;
        event.mouse.x = sdlevent.motion.x;
        event.mouse.y = sdlevent.motion.y;
      } return TRUE;

      case SDL_MOUSEWHEEL: {
        event.type = WM_MOUSEWHEEL;
        event.mouse.y = sdlevent.wheel.y * MOUSEWHEEL_SCROLL_INTERVAL;
      } return TRUE;

      case SDL_MOUSEBUTTONDOWN: {
        switch (sdlevent.button.button) {
          case SDL_BUTTON_LEFT:   event.type = WM_LBUTTONDOWN; break;
          case SDL_BUTTON_RIGHT:  event.type = WM_RBUTTONDOWN; break;
          case SDL_BUTTON_MIDDLE: event.type = WM_MBUTTONDOWN; break;
          case SDL_BUTTON_X1:     event.type = WM_XBUTTONDOWN; break;
          case SDL_BUTTON_X2:     event.type = WM_XBUTTONDOWN; break;
          default: event.type = WM_NULL; // Unknown
        }

        event.mouse.button = sdlevent.button.button;
        event.mouse.pressed = sdlevent.button.state;
      } return TRUE;

      case SDL_MOUSEBUTTONUP: {
        switch (sdlevent.button.button) {
          case SDL_BUTTON_LEFT:   event.type = WM_LBUTTONUP; break;
          case SDL_BUTTON_RIGHT:  event.type = WM_RBUTTONUP; break;
          case SDL_BUTTON_MIDDLE: event.type = WM_MBUTTONUP; break;
          case SDL_BUTTON_X1:     event.type = WM_XBUTTONUP; break;
          case SDL_BUTTON_X2:     event.type = WM_XBUTTONUP; break;
          default: event.type = WM_NULL; // Unknown
        }

        event.mouse.button = sdlevent.button.button;
        event.mouse.pressed = sdlevent.button.state;
      } return TRUE;

      // Window events
      case SDL_WINDOWEVENT: {
        event.type = WM_SYSCOMMAND;
        event.window.event = sdlevent.window.event;
      } return TRUE;

      case SDL_QUIT: {
        event.type = WM_QUIT;
      } return TRUE;

      // Controller input
      case SDL_CONTROLLERDEVICEADDED: {
        _pInput->OpenGameController(sdlevent.cdevice.which);
      } return TRUE;

      case SDL_CONTROLLERDEVICEREMOVED: {
        _pInput->CloseGameController(sdlevent.cdevice.which);
      } return TRUE;

      case SDL_CONTROLLERBUTTONDOWN:
      case SDL_CONTROLLERBUTTONUP:
      case SDL_CONTROLLERAXISMOTION: {
        INDEX iSlot = _pInput->GetControllerSlotForDevice(sdlevent.cdevice.which);

        if (iSlot != -1 && SetupControllerEvent(iSlot, event)) {
          return TRUE;
        }
      } break;

      default: break;
    }
  }

  return FALSE;

#else
  // Manual joystick update
  _pInput->UpdateJoysticks();

  // Process event for the first controller that sends it
  const INDEX ctControllers = _pInput->inp_aControllers.Count();

  for (INDEX iCtrl = 0; iCtrl < ctControllers; iCtrl++) {
    if (SetupControllerEvent(iCtrl, event)) {
      return TRUE;
    }
  }

  // Go in the loop until it finds an event it can process and return TRUE on it
  // Otherwise break from switch-case and try checking the next event
  // If none found, exits the loop and returns FALSE because there are no more events
  MSG msg;

  while (::PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
  {
    // If it's not a mouse message
    if (msg.message < WM_MOUSEFIRST || msg.message > WM_MOUSELAST) {
      // And not system key messages
      if (!((msg.message == WM_KEYDOWN && msg.wParam == VK_F10) || msg.message == WM_SYSKEYDOWN)) {
        // Dispatch it
        ::TranslateMessage(&msg);
        ::DispatchMessage(&msg);
      }
    }

    // Reset the event
    SDL_zero(event);
    event.type = msg.message;

    switch (msg.message) {
      // Key events
      case WM_CHAR:
      case WM_SYSKEYDOWN: case WM_SYSKEYUP:
      case WM_KEYDOWN: case WM_KEYUP: {
        event.key.code = msg.wParam;
      } return TRUE;

      // Mouse events
      case WM_MOUSEMOVE: {
        event.mouse.y = HIWORD(msg.lParam);
        event.mouse.x = LOWORD(msg.lParam);
      } return TRUE;

      case WM_MOUSEWHEEL: {
        event.mouse.y = (SWORD)(UWORD)HIWORD(msg.wParam);
      } return TRUE;

      case WM_LBUTTONDOWN: case WM_LBUTTONDBLCLK: {
        event.type = WM_LBUTTONDOWN;
        event.mouse.button = SDL_BUTTON_LEFT;
        event.mouse.pressed = TRUE;
      } return TRUE;

      case WM_LBUTTONUP: {
        event.mouse.button = SDL_BUTTON_LEFT;
        event.mouse.pressed = FALSE;
      } return TRUE;

      case WM_RBUTTONDOWN: case WM_RBUTTONDBLCLK: {
        event.type = WM_RBUTTONDOWN;
        event.mouse.button = SDL_BUTTON_RIGHT;
        event.mouse.pressed = TRUE;
      } return TRUE;

      case WM_RBUTTONUP: {
        event.mouse.button = SDL_BUTTON_RIGHT;
        event.mouse.pressed = FALSE;
      } return TRUE;

      case WM_MBUTTONDOWN: case WM_MBUTTONDBLCLK: {
        event.type = WM_MBUTTONDOWN;
        event.mouse.button = SDL_BUTTON_MIDDLE;
        event.mouse.pressed = TRUE;
      } return TRUE;

      case WM_MBUTTONUP: {
        event.mouse.button = SDL_BUTTON_MIDDLE;
        event.mouse.pressed = FALSE;
      } return TRUE;

      case WM_XBUTTONDOWN: case WM_XBUTTONDBLCLK: {
        event.type = WM_XBUTTONDOWN;

        if (GET_XBUTTON_WPARAM(msg.wParam) & XBUTTON2) {
          event.mouse.button = SDL_BUTTON_X2;
        } else {
          event.mouse.button = SDL_BUTTON_X1;
        }
        event.mouse.pressed = TRUE;
      } return TRUE;

      case WM_XBUTTONUP: {
        if (GET_XBUTTON_WPARAM(msg.wParam) & XBUTTON2) {
          event.mouse.button = SDL_BUTTON_X2;
        } else {
          event.mouse.button = SDL_BUTTON_X1;
        }
        event.mouse.pressed = FALSE;
      } return TRUE;

      // Window events
      case WM_QUIT: case WM_CLOSE:
        return TRUE;

      case WM_COMMAND: {
        event.window.event = msg.wParam;
        event.window.data = msg.lParam;
      } return TRUE;

      case WM_SYSCOMMAND: {
        switch (msg.wParam & ~0x0F) {
          case SC_MINIMIZE: event.window.event = SDL_WINDOWEVENT_MINIMIZED; break;
          case SC_MAXIMIZE: event.window.event = SDL_WINDOWEVENT_MAXIMIZED; break;
          case SC_RESTORE:  event.window.event = SDL_WINDOWEVENT_RESTORED; break;
          default: event.window.event = SDL_WINDOWEVENT_NONE; // Unknown
        }
      } return TRUE;

      case WM_PAINT: {
        event.type = WM_SYSCOMMAND;
        event.window.event = SDL_WINDOWEVENT_EXPOSED;
      } return TRUE;

      case WM_CANCELMODE: {
        event.type = WM_SYSCOMMAND;
        event.window.event = SDL_WINDOWEVENT_FOCUS_LOST;
      } return TRUE;

      case WM_KILLFOCUS: {
        event.type = WM_SYSCOMMAND;
        event.window.event = SDL_WINDOWEVENT_FOCUS_LOST;
      } return TRUE;

      case WM_SETFOCUS: {
        event.type = WM_SYSCOMMAND;
        event.window.event = SDL_WINDOWEVENT_FOCUS_GAINED;
      } return TRUE;

      case WM_ACTIVATE: {
        switch (LOWORD(msg.wParam))
        {
          case WA_ACTIVE: case WA_CLICKACTIVE: {
            event.type = WM_SYSCOMMAND;
            event.window.event = SDL_WINDOWEVENT_ENTER;
          } return TRUE;

          case WA_INACTIVE: {
            event.type = WM_SYSCOMMAND;
            event.window.event = SDL_WINDOWEVENT_LEAVE;
          } return TRUE;

          default:
            // Minimized
            if (HIWORD(msg.wParam)) {
              // Deactivated
              event.type = WM_SYSCOMMAND;
              event.window.event = SDL_WINDOWEVENT_LEAVE;
              return TRUE;
            }
        }
      } break;

      case WM_ACTIVATEAPP: {
        if (msg.wParam) {
          event.type = WM_SYSCOMMAND;
          event.window.event = SDL_WINDOWEVENT_ENTER;
        } else {
          event.type = WM_SYSCOMMAND;
          event.window.event = SDL_WINDOWEVENT_LEAVE;
        }
      } return TRUE;

      default: break;
    }
  }

  return FALSE;
#endif // !SE1_PREFER_SDL
};

BOOL OS::IsIconic(OS::Window hWnd)
{
#if SE1_PREFER_SDL
  return (hWnd != NULL && !!(SDL_GetWindowFlags(hWnd) & SDL_WINDOW_MINIMIZED));
#else
  return ::IsIconic(hWnd);
#endif
};

UWORD OS::GetKeyState(ULONG iKey)
{
#if SE1_PREFER_SDL
  const Uint8 *aState = SDL_GetKeyboardState(NULL);
  SDL_Scancode eScancode = SDL_GetScancodeFromKey(iKey);

  return (aState[eScancode] ? 0x8000 : 0x0);
#else
  return ::GetAsyncKeyState(iKey);
#endif
};

ULONG OS::GetMouseState(int *piX, int *piY, BOOL bRelativeToWindow) {
  ULONG ulMouse = 0;

#if SE1_PREFER_SDL
  if (bRelativeToWindow) {
    ulMouse = SDL_GetMouseState(piX, piY);
  } else {
    ulMouse = SDL_GetGlobalMouseState(piX, piY);
  }

#else
  POINT pt;
  BOOL bResult = ::GetCursorPos(&pt);

  // Can't afford to handle errors
  if (!bResult) {
    pt.x = pt.y = 0;

  } else if (bRelativeToWindow) {
    bResult = ::ScreenToClient(GetActiveWindow(), &pt);
  }

  if (piX != NULL) *piX = pt.x;
  if (piY != NULL) *piY = pt.y;

  // Gather mouse states
  if (::GetKeyState(VK_LBUTTON) & 0x8000) ulMouse |= SDL_BUTTON_LMASK;
  if (::GetKeyState(VK_RBUTTON) & 0x8000) ulMouse |= SDL_BUTTON_RMASK;
  if (::GetKeyState(VK_MBUTTON) & 0x8000) ulMouse |= SDL_BUTTON_MMASK;
  if (::GetKeyState(VK_XBUTTON1) & 0x8000) ulMouse |= SDL_BUTTON_X1MASK;
  if (::GetKeyState(VK_XBUTTON2) & 0x8000) ulMouse |= SDL_BUTTON_X2MASK;
#endif

  return ulMouse;
};

int OS::ShowCursor(BOOL bShow)
{
#if SE1_PREFER_SDL
  static int ct = 0;
  ct += (bShow) ? 1 : -1;

  SDL_ShowCursor((ct >= 0) ? SDL_TRUE : SDL_FALSE);
  return ct;

#else
  return ::ShowCursor(bShow);
#endif
};
