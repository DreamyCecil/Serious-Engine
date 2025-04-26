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
#include "MainWindow.h"
#include "Resource.h"

extern HINSTANCE _hInstance;

BOOL _bWindowChanging = FALSE;    // ignores window messages while this is set
OS::Window _hwndMain = NULL;
char _achWindowTitle[256]; // current window title

#if !SE1_PREFER_SDL

static HBITMAP _hbmSplash = NULL;
static BITMAP  _bmSplash;

// for window reposition function
PIX _pixLastSizeI, _pixLastSizeJ;


// window procedure active while window changes are occuring
LRESULT WindowProc_WindowChanging( HWND hWnd, UINT message, 
			    WPARAM wParam, LPARAM lParam )
{
    switch( message ) {
    case WM_PAINT: {
      PAINTSTRUCT ps;
      BeginPaint(hWnd, &ps); 
      EndPaint(hWnd, &ps); 
 
      return 0;
                   } break;
    case WM_ERASEBKGND: {

      PAINTSTRUCT ps;
      BeginPaint(hWnd, &ps); 
      RECT rect;
      GetClientRect(hWnd, &rect); 
      FillRect(ps.hdc, &rect, (HBRUSH)GetStockObject(BLACK_BRUSH));
      HDC hdcMem = CreateCompatibleDC(ps.hdc); 
      SelectObject(hdcMem, _hbmSplash); 
      BitBlt(ps.hdc, 0, 0, _bmSplash.bmWidth, _bmSplash.bmHeight, hdcMem, 0, 0, SRCCOPY); 
//      StretchBlt(ps.hdc, 0, 0, rect.right, rect.bottom,
//        hdcMem, 0,0, _bmSplash.bmWidth, _bmSplash.bmHeight, SRCCOPY); 

      EndPaint(hWnd, &ps); 

      return 1; 
                   } break;
    case WM_CLOSE:
      return 0;
      break;
    }
    return DefWindowProcA(hWnd, message, wParam, lParam);
}

// window procedure active normally
LRESULT WindowProc_Normal( HWND hWnd, UINT message, 
			    WPARAM wParam, LPARAM lParam )
{
  switch( message ) {

  // system commands
  case WM_SYSCOMMAND: {
    switch( wParam & ~0x0F) {
    // window resizing messages
    case SC_MINIMIZE:
    case SC_RESTORE:
    case SC_MAXIMIZE:
      // relay to application
  	  PostMessage(NULL, message, wParam & ~0x0F, lParam);
      // do not allow automatic resizing
      return 0;
      break;
    // prevent screen saver and monitor power down
    case SC_SCREENSAVE:
    case SC_MONITORPOWER:
      return 0;
    }
                      } break;
  // when close box is clicked
  case WM_CLOSE:
    // relay to application
  	PostMessage(NULL, message, wParam, lParam);
    // do not pass to default wndproc
    return 0;

  // some standard focus loose/gain messages
  case WM_LBUTTONUP:
  case WM_LBUTTONDOWN:
  case WM_RBUTTONDOWN:
  case WM_CANCELMODE:
  case WM_KILLFOCUS:
  case WM_ACTIVATEAPP:
    // relay to application
  	PostMessage(NULL, message, wParam, lParam);
    // pass to default wndproc
    break;
  }

  // if we get to here, we pass the message to default procedure
  return DefWindowProcA(hWnd, message, wParam, lParam);
}

// main window procedure
LRESULT CALLBACK WindowProc( HWND hWnd, UINT message, 
			    WPARAM wParam, LPARAM lParam )
{
  // dispatch to proper window procedure
  if(_bWindowChanging) {
    return WindowProc_WindowChanging(hWnd, message, wParam, lParam);
  } else {
    return WindowProc_Normal(hWnd, message, wParam, lParam);
  }
}


// init/end main window management
void MainWindow_Init(void)
{
  // register the window class
  WNDCLASSEXA wc;
  wc.cbSize = sizeof(wc);
  wc.style = CS_HREDRAW | CS_VREDRAW;
  wc.lpfnWndProc = WindowProc;
  wc.cbClsExtra = 0;
  wc.cbWndExtra = 0;
  wc.hInstance = _hInstance;
  wc.hIcon = LoadIconA( _hInstance, (LPCSTR)IDR_MAINFRAME );
  wc.hCursor = NULL;
  wc.hbrBackground = NULL;
  wc.lpszMenuName = APPLICATION_NAME;
  wc.lpszClassName = APPLICATION_NAME;
  wc.hIconSm = NULL;
  if (0 == RegisterClassExA(&wc)) {
    // [Cecil] Simplified and more informative error message
    FatalError(TRANS("Cannot open main window:\n%s"), GetWindowsError(GetLastError()).ConstData());
  }

  // load bitmaps
  _hbmSplash = LoadBitmapA(_hInstance, (char*)IDB_SPLASH);
  ASSERT(_hbmSplash!=NULL);
  GetObject(_hbmSplash, sizeof(BITMAP), (LPSTR) &_bmSplash); 
  // here was loading and setting of no-windows-mouse-cursor
}


void MainWindow_End(void)
{
  DeleteObject(_hbmSplash);
}

void ResetMainWindowNormal(void)
{
  ShowWindow( _hwndMain, SW_HIDE);
  // add edges and title bar to window size so client area would have size that we requested
  RECT rWindow, rClient;
  GetClientRect( _hwndMain, &rClient);
  GetWindowRect( _hwndMain, &rWindow);
  const PIX pixWidth  = _pixLastSizeI + (rWindow.right-rWindow.left) - (rClient.right-rClient.left);
  const PIX pixHeight = _pixLastSizeJ + (rWindow.bottom-rWindow.top) - (rClient.bottom-rClient.top);
  const PIX pixPosX = (_vpixScreenRes(1) - pixWidth) / 2;
  const PIX pixPosY = (_vpixScreenRes(2) - pixHeight) / 2;
  // set new window size and show it
  SetWindowPos( _hwndMain, NULL, pixPosX,pixPosY, pixWidth,pixHeight, SWP_NOZORDER);
  ShowWindow(   _hwndMain, SW_SHOW);
}

#else

// [Cecil] SDL: No extra setup required, and splash display on window change isn't very necessary
void MainWindow_Init(void) {};
void MainWindow_End(void) {};

#endif // !SE1_PREFER_SDL

// [Cecil] Destroy window universally
void CloseMainWindow(void) {
  _hwndMain.Destroy();
};

// [Cecil] Make sure the window is always created
static inline void AssertWindowCreation(void) {
  if (_hwndMain != NULL) return;

#if SE1_PREFER_SDL
  CTString strErrorMessage = SDL_GetError();
#else
  CTString strErrorMessage = GetWindowsError(GetLastError());
#endif

  FatalError(TRANS("Cannot open main window:\n%s"), strErrorMessage.ConstData());
};

// open the main application window for windowed mode
void OpenMainWindowNormal( PIX pixSizeI, PIX pixSizeJ)
{
  ASSERT(_hwndMain==NULL);

#if !SE1_PREFER_SDL
  // create a window, invisible initially
  _hwndMain = CreateWindowExA(
	  WS_EX_APPWINDOW,
	  APPLICATION_NAME,
	  "",   // title
    WS_OVERLAPPED|WS_CAPTION|WS_MINIMIZEBOX|WS_MAXIMIZEBOX|WS_SYSMENU,
	  10,10,
	  100,100,  // window size
	  NULL,
	  NULL,
	  _hInstance,
	  NULL);

  AssertWindowCreation(); // [Cecil]

  // set window title
  sprintf(_achWindowTitle, TRANS("Serious Sam (Window %dx%d)"), pixSizeI, pixSizeJ);
  SetWindowTextA(_hwndMain, _achWindowTitle);
  _pixLastSizeI = pixSizeI;
  _pixLastSizeJ = pixSizeJ;
  ResetMainWindowNormal();

#else
  // [Cecil] SDL: Create normal window
  _snprintf(_achWindowTitle, sizeof(_achWindowTitle), TRANS("Serious Sam (Window %dx%d)"), pixSizeI, pixSizeJ);
  _hwndMain = SDL_CreateWindow(_achWindowTitle, pixSizeI, pixSizeJ, SDL_WINDOW_OPENGL);

  AssertWindowCreation();

  // Enable text input events
  SDL_StartTextInput(_hwndMain);
#endif // !SE1_PREFER_SDL

  SE_UpdateWindowHandle(_hwndMain);
}


// open the main application window for fullscreen mode
void OpenMainWindowFullScreen( PIX pixSizeI, PIX pixSizeJ)
{
  ASSERT( _hwndMain==NULL);

#if !SE1_PREFER_SDL
  // create a window, invisible initially
  _hwndMain = CreateWindowExA(
    WS_EX_TOPMOST | WS_EX_APPWINDOW,
    APPLICATION_NAME,
    "",   // title
    WS_POPUP,
    0,0,
    pixSizeI, pixSizeJ,  // window size
    NULL,
    NULL,
    _hInstance,
    NULL);

  AssertWindowCreation(); // [Cecil]

  // set window title and show it
  sprintf(_achWindowTitle, TRANS("Serious Sam (FullScreen %dx%d)"), pixSizeI, pixSizeJ);
  SetWindowTextA(_hwndMain, _achWindowTitle);
  ShowWindow(    _hwndMain, SW_SHOWNORMAL);

#else
  // [Cecil] SDL: Create fullscreen window
  _snprintf(_achWindowTitle, sizeof(_achWindowTitle), TRANS("Serious Sam (FullScreen %dx%d)"), pixSizeI, pixSizeJ);
  _hwndMain = SDL_CreateWindow(_achWindowTitle, pixSizeI, pixSizeJ, SDL_WINDOW_OPENGL | SDL_WINDOW_FULLSCREEN);

  AssertWindowCreation();

  // Enable text input events
  SDL_StartTextInput(_hwndMain);
#endif // !SE1_PREFER_SDL

  SE_UpdateWindowHandle(_hwndMain);
}

// [Cecil] Open the main application window in borderless mode
void OpenMainWindowBorderless(PIX pixSizeI, PIX pixSizeJ) {
  ASSERT(_hwndMain==NULL);

#if !SE1_PREFER_SDL
  // Create an invisible window
  _hwndMain = CreateWindowExA(WS_EX_APPWINDOW, APPLICATION_NAME, "",
    WS_POPUP, 0, 0, pixSizeI, pixSizeJ, NULL, NULL, _hInstance, NULL);

  AssertWindowCreation();

  // Set window title and show it
  sprintf(_achWindowTitle, TRANS("Serious Sam (Borderless %dx%d)"), pixSizeI, pixSizeJ);
  SetWindowTextA(_hwndMain, _achWindowTitle);

  _pixLastSizeI = pixSizeI;
  _pixLastSizeJ = pixSizeJ;

  ResetMainWindowNormal();

#else
  // [Cecil] SDL: Create normal window
  _snprintf(_achWindowTitle, sizeof(_achWindowTitle), TRANS("Serious Sam (Borderless %dx%d)"), pixSizeI, pixSizeJ);
  _hwndMain = SDL_CreateWindow(_achWindowTitle, pixSizeI, pixSizeJ, SDL_WINDOW_OPENGL | SDL_WINDOW_BORDERLESS);

  AssertWindowCreation();

  // Enable text input events
  SDL_StartTextInput(_hwndMain);
#endif // !SE1_PREFER_SDL

  SE_UpdateWindowHandle(_hwndMain);
};

// open the main application window invisible
void OpenMainWindowInvisible(void)
{
  ASSERT(_hwndMain==NULL);

#if !SE1_PREFER_SDL
  // create a window, invisible initially
  _hwndMain = CreateWindowExA(
	  WS_EX_APPWINDOW,
	  APPLICATION_NAME,
	  "",   // title
    WS_POPUP,
	  0,0,
	  10, 10,  // window size
	  NULL,
	  NULL,
	  _hInstance,
	  NULL);

  AssertWindowCreation(); // [Cecil]

  // set window title
  sprintf(_achWindowTitle, "Serious Sam");
  SetWindowTextA( _hwndMain, _achWindowTitle);

#else
  // [Cecil] SDL: Create invisible window
  _hwndMain = SDL_CreateWindow("Serious Sam", 10, 10, SDL_WINDOW_OPENGL | SDL_WINDOW_HIDDEN);
  AssertWindowCreation();
#endif // !SE1_PREFER_SDL

  SE_UpdateWindowHandle(_hwndMain);
}
