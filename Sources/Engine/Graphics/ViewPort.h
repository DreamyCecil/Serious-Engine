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

#ifndef SE_INCL_VIEWPORT_H
#define SE_INCL_VIEWPORT_H
#ifdef PRAGMA_ONCE
  #pragma once
#endif

#include <Engine/Graphics/Raster.h>

#if SE1_DIRECT3D
  // Undefine 'new' operator in debug
  #ifndef NDEBUG
    #undef new
  #endif

  #include <d3d8.h>

  // Redefine 'new' operator in debug
  #ifndef NDEBUG
    #define new DEBUG_NEW_CT
  #endif

#else
  #include <d3d8_disabled.h>
#endif // SE1_DIRECT3D

/*
 *  ViewPort
 */

// [Cecil] Helper class for OpenGL reworked for compatibility with SDL
class CTempDC {
  public:
    OS::DvcContext hdc;
    OS::WndHandle hwnd;

  public:
  #if SE1_PREFER_SDL
    CTempDC(OS::WndHandle hWnd) {
      ASSERT(hWnd != NULL);
      hwnd = hWnd;
      hdc = hwnd;
    };

  #else
    CTempDC(OS::WndHandle hWnd) {
      ASSERT(hWnd != NULL);
      hwnd = hWnd;
      hdc = GetDC(hwnd);
      ASSERT(hdc != NULL);
    };

    ~CTempDC(void) {
      ReleaseDC(hwnd, hdc);
    };
  #endif
};

// base abstract class for viewport
class ENGINE_API CViewPort {
public:
// implementation
  OS::Window vp_hWnd;                 // canvas (child) window
  OS::Window vp_hWndParent;           // window of the viewport
  CRaster vp_Raster;            // the used Raster
	LPDIRECT3DSWAPCHAIN8 vp_pSwapChain;  // swap chain for D3D
	LPDIRECT3DSURFACE8   vp_pSurfDepth;  // z-buffer for D3D
  INDEX vp_ctDisplayChanges;    // number of display driver

  // open/close canvas window
  void OpenCanvas(void);
  void CloseCanvas(BOOL bRelease=FALSE);

// interface
  /* Constructor for given window. */
  CViewPort(PIX pixWidth, PIX pixHeight, OS::Window hWnd);
	/* Destructor. */
  ~CViewPort(void);

	/* Display the back buffer on screen. */
  void SwapBuffers(void);
  // change size of this viewport, it's raster and all it's drawports to fit it window
  void Resize(void);
};


#endif  /* include-once check. */

