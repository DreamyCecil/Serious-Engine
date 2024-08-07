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

extern BOOL _bWindowChanging;    // ignores window messages while this is set
extern OS::Window _hwndMain;

// Current window title as extern
extern char _achWindowTitle[256];

// For window reposition as extern
extern PIX _pixLastSizeI, _pixLastSizeJ;

// init/end main window management
void MainWindow_Init(void);
void MainWindow_End(void);
// close the main application window
void CloseMainWindow(void);

#if !SE1_PREFER_SDL
  void ResetMainWindowNormal(void);
#endif

// open the main application window for windowed mode
void OpenMainWindowNormal(PIX pixSizeI, PIX pixSizeJ);

// open the main application window for fullscreen mode
void OpenMainWindowFullScreen(PIX pixSizeI, PIX pixSizeJ);

// [Cecil] Open the main application window in borderless mode
void OpenMainWindowBorderless(PIX pixSizeI, PIX pixSizeJ);

// open the main application window invisible
void OpenMainWindowInvisible(void);
