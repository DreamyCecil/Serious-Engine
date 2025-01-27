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

// [Cecil] This header defines a cross-platform interface for interacting with operating systems
#ifndef SE_INCL_OS_H
#define SE_INCL_OS_H

// Compatibility with multiple platforms
#include <Engine/OS/Keycodes.h>
#include <Engine/OS/PlatformSpecific.h>

#if SE1_WIN

// Custom controller events
#define WM_CTRLAXISMOTION 0x100000
#define WM_CTRLBUTTONDOWN 0x100001
#define WM_CTRLBUTTONUP   0x100002

#else

// Unique types for some Windows messages
extern SDL_EventType WM_SYSCOMMAND;
extern SDL_EventType WM_SYSKEYDOWN;
extern SDL_EventType WM_SYSKEYUP;
extern SDL_EventType WM_LBUTTONDOWN;
extern SDL_EventType WM_LBUTTONUP;
extern SDL_EventType WM_RBUTTONDOWN;
extern SDL_EventType WM_RBUTTONUP;
extern SDL_EventType WM_MBUTTONDOWN;
extern SDL_EventType WM_MBUTTONUP;
extern SDL_EventType WM_XBUTTONDOWN;
extern SDL_EventType WM_XBUTTONUP;

// Redefined Windows messages
#define WM_NULL       SDL_EVENT_FIRST
#define WM_CHAR       SDL_EVENT_TEXT_INPUT
#define WM_KEYDOWN    SDL_EVENT_KEY_DOWN
#define WM_KEYUP      SDL_EVENT_KEY_UP
#define WM_MOUSEMOVE  SDL_EVENT_MOUSE_MOTION
#define WM_MOUSEWHEEL SDL_EVENT_MOUSE_WHEEL
#define WM_QUIT       SDL_EVENT_QUIT
#define WM_CLOSE      SDL_EVENT_QUIT

// Custom controller events
#define WM_CTRLAXISMOTION SDL_EVENT_GAMEPAD_AXIS_MOTION
#define WM_CTRLBUTTONDOWN SDL_EVENT_GAMEPAD_BUTTON_DOWN
#define WM_CTRLBUTTONUP   SDL_EVENT_GAMEPAD_BUTTON_UP

#endif

// For mimicking Win32 wheel scrolling
#define MOUSEWHEEL_SCROLL_INTERVAL 120

class ENGINE_API OS {
  public:
    // Handle pointers
  #if SE1_PREFER_SDL
    typedef SDL_Window *WndHandle; // Window handle
    typedef WndHandle DvcContext;  // Alias for compatibility
  #else
    typedef HWND WndHandle; // Window handle
    typedef HDC DvcContext; // Device context handle
  #endif

    // Window handler
    struct ENGINE_API Window {
      WndHandle pWindow;

      Window(int i = 0) : pWindow((WndHandle)(size_t)i) {};
      Window(size_t i) : pWindow((WndHandle)i) {};
    #if !SE1_WIN
      Window(long int i) : pWindow((WndHandle)i) {};
    #endif

      Window(const Window &other) : pWindow(other.pWindow) {};
      Window(const WndHandle pSetWindow) : pWindow(pSetWindow) {};

      operator WndHandle() { return pWindow; };
      operator const WndHandle() const { return pWindow; };

      inline bool operator==(const Window &other) const { return pWindow == other.pWindow; };
      inline bool operator!=(const Window &other) const { return pWindow != other.pWindow; };
      inline bool operator==(size_t i) const { return pWindow == (WndHandle)i; };
      inline bool operator!=(size_t i) const { return pWindow != (WndHandle)i; };

      // Destroy current window
      void Destroy(void);

    #if SE1_WIN
      // Retrieve native window handle
      HWND GetNativeHandle(void);
    #endif
    };

    // Depending on build configuration this structure can either be:
    // - A handle to a dynamic module
    // - A dummy for a static module
    struct EngineModule {
      HMODULE hHandle;

      EngineModule(HMODULE hOther = NULL) : hHandle(hOther) {};

    #if defined(SE1_STATIC_BUILD)
      inline BOOL IsLoaded(void) { return TRUE; };
      inline void Load(const char *strLibrary) {};
      inline void LoadOrThrow_t(const char *strLibrary) {};
      inline BOOL Free(void) { return TRUE; };

    #else
      inline BOOL IsLoaded(void) { return hHandle != NULL; };
      inline void Load(const char *strLibrary) { hHandle = LoadLib(strLibrary); };
      inline void LoadOrThrow_t(const char *strLibrary) { hHandle = LoadLibOrThrow_t(strLibrary); };

      inline BOOL Free(void) {
        if (!IsLoaded()) return FALSE;
        BOOL bReturn = FreeLib(hHandle);
        hHandle = NULL;
        return bReturn;
      };
    #endif // SE1_STATIC_BUILD

      inline EngineModule &operator=(const EngineModule &hOther) {
        hHandle = hOther.hHandle;
        return *this;
      };
    };

  // Dynamic library methods
  public:

    // Load library
    static HMODULE LoadLib(const char *strLibrary);

    // Throw an error if unable to load a library
    static HMODULE LoadLibOrThrow_t(const char *strLibrary);

    // Free loaded library
    static BOOL FreeLib(HMODULE hLib);

    // Hook library symbol
    static void *GetLibSymbol(HMODULE hLib, const char *strSymbol);

  // Cross-platform handling of input and window events akin to SDL_Event
  public:

    struct KeyEvent {
      ULONG type;
      BOOL pressed;
      ULONG code;
    };

    struct MouseEvent {
      ULONG type;
      BOOL pressed;
      ULONG button;
      SLONG x;
      SLONG y;
    };

    struct ControllerEvent {
      ULONG type;
      ULONG action;
      INDEX dir;
    };

    struct WindowEvent {
      ULONG type;
      ULONG event;
      SLONG data;
    };

    typedef union {
      // [Cecil] NOTE: Types always use Win32 message types (WM_*) that are redefined for non-Windows OS
      ULONG type;

      KeyEvent key;
      MouseEvent mouse;
      ControllerEvent ctrl;
      WindowEvent window;
    } SE1Event;

    // [SE1_PREFER_SDL = 1]
    // Uses SDL_PollEvent() method with additional internal input handling and translates SDL_Event to SE1Event
    // [SE1_PREFER_SDL = 0]
    // Uses Win32's PeekMessage() method and translates MSG events to SE1Event
    static BOOL PollEvent(SE1Event &event);

    // Check if the game window isn't minimized
    static BOOL IsIconic(Window hWnd);

    // [SE1_PREFER_SDL = 1]
    // Uses SDL_GetKeyboardState() method and returns 0x8000 if the key is held and 0x0 otherwise
    // [SE1_PREFER_SDL = 0]
    // Works just like Win32's GetAsyncKeyState() method
    static UWORD GetKeyState(ULONG iKey);

    // [SE1_PREFER_SDL = 1]
    // Works just like SDL_GetMouseState() or SDL_GetGlobalMouseState() method depending on bRelativeToWindow state
    // [SE1_PREFER_SDL = 0]
    // Uses GetCursorPos() and ScreenToClient() for the cursor position and GetKeyState() with mouse buttons for
    // returning SDL masks with pressed mouse buttons (SDL_BUTTON_MASK)
    static ULONG GetMouseState(float *pfX, float *pfY, BOOL bRelativeToWindow = TRUE);

    // [SE1_PREFER_SDL = 1]
    // Mimics Win32's ShowCursor() functionality: if bShow is TRUE, the display count is incremented, otherwise it's decremented
    // Uses SDL_ShowCursor() or SDL_HideCursor() depending on whether the display count is positive or not
    // [SE1_PREFER_SDL = 0]
    // Works just like Win32's ShowCursor() method
    static int ShowCursor(BOOL bShow);
};

#endif // include-once check
