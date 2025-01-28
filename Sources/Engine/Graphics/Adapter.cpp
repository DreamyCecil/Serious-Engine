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

#include <Engine/Graphics/Adapter.h>
#include <Engine/Graphics/GfxLibrary.h>
#include <Engine/Base/Translation.h>
#include <Engine/Base/Console.h>


#if SE1_DIRECT3D
extern const D3DDEVTYPE d3dDevType;
#endif // SE1_DIRECT3D

// list of all modes avaliable through CDS
static CListHead _lhCDSModes;

#if !SE1_PREFER_SDL

class CResolution {
public:
  PIX   re_pixSizeI;
  PIX   re_pixSizeJ;
};

static CResolution _areResolutions[] =
{
  {  320,  240 },
  {  400,  300 },
  {  480,  360 },
  {  512,  384 },
  {  640,  480 },
  {  720,  540 },
  {  720,  576 },
  {  800,  600 },
  {  960,  720 },
  { 1024,  768 },
  { 1152,  864 },
  { 1280,  960 },
  { 1280, 1024 },
  { 1600, 1200 },
  { 1792, 1344 },
  { 1856, 1392 },
  { 1920, 1440 },
  { 2048, 1536 },

  // matrox dualhead modes
  { 1280,  480 },
  { 1600,  600 },
  { 2048,  768 },

  // NTSC HDTV widescreen
  {  848,  480 },
  {  856,  480 },
};

// [Cecil] Don't go over the adapter limit
static const INDEX MAX_RESOLUTIONS = ClampUp(ARRAYCOUNT(_areResolutions), MAX_DA_DISPLAYMODES);

#if SE1_3DFX

// [Cecil] Setup 3Dfx driver in a separate method
static inline void Setup3dfx(CGfxAPI &api) {
  // Detect presence of 3Dfx standalone OpenGL driver (for Voodoo1/2)
  if (!FileSystem::Exists("3DFXVGL.DLL")) return;

  // Set adapter and force some enumeration of Voodoo1/2 display modes
  api.ga_ctAdapters++;

  CDisplayAdapter *pda = &api.ga_adaAdapter[1];
  pda->da_ulFlags = DAF_ONEWINDOW | DAF_FULLSCREENONLY | DAF_16BITONLY;
  pda->da_strVendor   = "3Dfx";
  pda->da_strRenderer = "3Dfx Voodoo2";
  pda->da_strVersion  = "1.1+";

  // Voodoos only have 4 display modes
  pda->da_ctDisplayModes = 4;

  CDisplayMode *adm = &pda->da_admDisplayModes[0];
  adm[0].Configure( 512, 384, DD_16BIT);
  adm[1].Configure( 640, 480, DD_16BIT);
  adm[2].Configure( 800, 600, DD_16BIT);
  adm[3].Configure(1024, 768, DD_16BIT);
};

#endif // SE1_3DFX

#if SE1_DIRECT3D

// [Cecil] Setup Direct3D in a separate method
static inline void SetupD3D(void) {
  LPDIRECT3D8 &pD3D = _pGfx->gl_pD3D;
  CGfxAPI &apiD3D = _pGfx->gl_gaAPI[GAT_D3D];

  // Determine DX8 adapters and display modes
  const INDEX ctMaxAdapters = pD3D->GetAdapterCount();

  INDEX &ctAdapters = apiD3D.ga_ctAdapters;
  ctAdapters = 0;

  for (INDEX iAdapter = 0; iAdapter < ctMaxAdapters; iAdapter++)
  {
    CDisplayAdapter *pda = &apiD3D.ga_adaAdapter[ctAdapters];
    pda->da_ulFlags = 0;
    pda->da_ctDisplayModes = 0;

    // Check whether 32-bit rendering modes are supported
    HRESULT hr = pD3D->CheckDeviceType(iAdapter, d3dDevType, D3DFMT_A8R8G8B8, D3DFMT_A8R8G8B8, FALSE);

    if (hr != D3D_OK) {
      hr = pD3D->CheckDeviceType(iAdapter, d3dDevType, D3DFMT_X8R8G8B8, D3DFMT_X8R8G8B8, FALSE);

      if(hr != D3D_OK) {
        pda->da_ulFlags |= DAF_16BITONLY;
      }
    }

    // Check whether windowed rendering modes are supported
    D3DCAPS8 d3dCaps;
    pD3D->GetDeviceCaps(iAdapter, d3dDevType, &d3dCaps);

    if (!(d3dCaps.Caps2 & D3DCAPS2_CANRENDERWINDOWED)) {
      pda->da_ulFlags |= DAF_FULLSCREENONLY;
    }

    // Enumerate modes through resolution list
    const INDEX ctModes = pD3D->GetAdapterModeCount(iAdapter);

    for (INDEX iRes = 0; iRes < MAX_RESOLUTIONS; iRes++) {
      CResolution &re = _areResolutions[iRes];

      for (INDEX iMode = 0; iMode < ctModes; iMode++)
      {
        D3DDISPLAYMODE d3dMode;
        pD3D->EnumAdapterModes(iAdapter, iMode, &d3dMode);

        // Check if resolution matches and display depth is 16 or 32 bits
        const BOOL bMatches = (d3dMode.Width == re.re_pixSizeI && d3dMode.Height == re.re_pixSizeJ
         && (d3dMode.Format == D3DFMT_A8R8G8B8 || d3dMode.Format == D3DFMT_X8R8G8B8
         ||  d3dMode.Format == D3DFMT_A1R5G5B5 || d3dMode.Format == D3DFMT_X1R5G5B5 || d3dMode.Format == D3DFMT_R5G6B5));

        if (!bMatches) continue;

        hr = pD3D->CheckDeviceType(iAdapter, d3dDevType, d3dMode.Format, d3dMode.Format, FALSE);
        if (hr != D3D_OK) continue;

        // Add new display mode
        CDisplayMode &dm = pda->da_admDisplayModes[pda->da_ctDisplayModes++];
        dm.Configure(re.re_pixSizeI, re.re_pixSizeJ, DD_DEFAULT);
        break;
      }
    }

    ctAdapters++;

    // Get adapter identifier
    D3DADAPTER_IDENTIFIER8 d3dID;
    pD3D->GetAdapterIdentifier(iAdapter, D3DENUM_NO_WHQL_LEVEL, &d3dID);

    pda->da_strVendor = "MS DirectX 8";
    pda->da_strRenderer = d3dID.Description;

    LARGE_INTEGER &iVer = d3dID.DriverVersion;
    pda->da_strVersion.PrintF("%d.%d.%d.%d", iVer.HighPart >> 16, iVer.HighPart & 0xFFFF, iVer.LowPart >> 16, iVer.LowPart & 0xFFFF);
  }

  // Shut down DX8 (will be restarted if needed)
  D3DRELEASE(pD3D, TRUE);

  if(_pGfx->gl_hiDriver != NONE) {
    OS::FreeLib(_pGfx->gl_hiDriver);
    _pGfx->gl_hiDriver = NONE;
  }
};

#endif // SE1_DIRECT3D

#endif // !SE1_PREFER_SDL

// initialize CDS support (enumerate modes at startup)
void CGfxLibrary::InitAPIs(void)
{
  // No need for graphics on a dedicated server
  if (_bDedicatedServer) return;

  // [Cecil] Reset adapters
  for (INDEX iApi = 0; iApi < GAT_MAX; iApi++) {
    gl_gaAPI[iApi].ga_ctAdapters = 0;
    gl_gaAPI[iApi].ga_iCurrentAdapter = 0;
  }

  // Fill OpenGL adapter info
  CGfxAPI &apiOGL = gl_gaAPI[GAT_OGL];
  apiOGL.ga_ctAdapters = 1;

  CDisplayAdapter *pda = &apiOGL.ga_adaAdapter[0];
  pda->da_ulFlags = 0; // [Cecil] NOTE: DAF_USEGDIFUNCTIONS is removed due to being unused
  pda->da_strVendor   = TRANS("unknown");
  pda->da_strRenderer = TRANS("Default ICD");
  pda->da_strVersion  = "1.1+";

  pda->da_ctDisplayModes = 0;
  pda->da_iCurrentDisplayMode = -1;

#if !SE1_PREFER_SDL
  // Detect current mode and report it
  DEVMODE devmode;
  memset(&devmode, 0, sizeof(devmode));
  devmode.dmSize = sizeof(devmode);

  EnumDisplaySettingsA(NULL, ENUM_CURRENT_SETTINGS, &devmode);
  CPrintF(TRANS("Current display: '%s' version %d - %dx%dx%d\n\n"),
          devmode.dmDeviceName, devmode.dmDriverVersion,
          devmode.dmPelsWidth, devmode.dmPelsHeight, devmode.dmBitsPerPel);

  // Detect modes for OpenGL ICD
  for (INDEX iRes = 0; iRes < MAX_RESOLUTIONS; iRes++) {
    CResolution &re = _areResolutions[iRes];

    // Check if it's possible to set the mode
    memset(&devmode, 0, sizeof(devmode));
    devmode.dmSize = sizeof(devmode);
    devmode.dmPelsWidth  = re.re_pixSizeI;
    devmode.dmPelsHeight = re.re_pixSizeJ;
    devmode.dmDisplayFlags = CDS_FULLSCREEN;
    devmode.dmFields = DM_PELSWIDTH | DM_PELSHEIGHT | DM_DISPLAYFLAGS;

    // Skip if unsuccessful
    if (ChangeDisplaySettingsA(&devmode, CDS_TEST | CDS_FULLSCREEN) != DISP_CHANGE_SUCCESSFUL) continue;

    // Add new display mode
    CDisplayMode &dm = pda->da_admDisplayModes[pda->da_ctDisplayModes++];
    dm.Configure(re.re_pixSizeI, re.re_pixSizeJ, DD_DEFAULT);
  }

  // [Cecil] Separate methods
  #if SE1_3DFX
    Setup3dfx(apiOGL);
  #endif

  #if SE1_DIRECT3D
    if (InitDriver_D3D()) SetupD3D();
  #endif

#else
  // [Cecil] SDL: Detect modes for OpenGL ICD
  int ctDisplayModes;
  SDL_DisplayMode **aModes = SDL_GetFullscreenDisplayModes(SDL_GetPrimaryDisplay(), &ctDisplayModes);

  if (aModes == NULL) {
    FatalError(TRANS("Cannot retrieve fullscreen display modes! SDL Error: %s"), SDL_GetError());
  }

  INDEX &ctModes = pda->da_ctDisplayModes;

  // Go in the reverse order (from lowest resolution to highest)
  for (INDEX iMode = ctDisplayModes - 1; iMode >= 0; iMode--)
  {
    if (ctModes >= MAX_DA_DISPLAYMODES) break;

    const SDL_DisplayMode &mode = *aModes[iMode];

    INDEX iBPP = SDL_BITSPERPIXEL(mode.format);
    if (iBPP < 16) continue;

    // Make sure resolutions aren't repeating
    if (ctModes > 0) {
      CDisplayMode &dmLast = pda->da_admDisplayModes[ctModes - 1];

      if (dmLast.dm_pixSizeI == mode.w && dmLast.dm_pixSizeJ == mode.h) continue;
    }

    // Add new display mode
    CDisplayMode &dm = pda->da_admDisplayModes[ctModes++];
    dm.Configure(mode.w, mode.h, DD_DEFAULT);
  }

  SDL_free(aModes);
#endif // !SE1_PREFER_SDL
}

// get list of all modes avaliable through CDS -- do not modify/free the returned list
CListHead &CDS_GetModes(void)
{
  return _lhCDSModes;
}


// set given display mode
BOOL CDS_SetMode( PIX pixSizeI, PIX pixSizeJ, enum DisplayDepth dd)
{
  // no need for gfx when dedicated server is on
  if( _bDedicatedServer) return FALSE;

#if SE1_WIN
  // prepare general mode parameters
  DEVMODE devmode;
  memset(&devmode, 0, sizeof(devmode));
  devmode.dmSize = sizeof(devmode);
  devmode.dmPelsWidth  = pixSizeI;
  devmode.dmPelsHeight = pixSizeJ;
  devmode.dmDisplayFlags = CDS_FULLSCREEN;
  devmode.dmFields = DM_PELSWIDTH|DM_PELSHEIGHT|DM_DISPLAYFLAGS;
  extern INDEX gap_iRefreshRate;
  if( gap_iRefreshRate>0) {
    devmode.dmFields |= DM_DISPLAYFREQUENCY;
    devmode.dmDisplayFrequency = gap_iRefreshRate;
  }
  // determine bits per pixel to try to set
  SLONG slBPP2 = 0;
  switch(dd) {
  case DD_16BIT:
    devmode.dmBitsPerPel = 16;
    slBPP2 = 15;
    devmode.dmFields |= DM_BITSPERPEL;
    break;
  case DD_32BIT:
    devmode.dmBitsPerPel = 32;
    slBPP2 = 24;
    devmode.dmFields |= DM_BITSPERPEL;
    break;
  case DD_DEFAULT:
    NOTHING;
    break;
  default:
    ASSERT(FALSE);
    NOTHING;
  }

  // try to set primary depth
  LONG lRes = ChangeDisplaySettings(&devmode, CDS_FULLSCREEN);

  // if failed
  if( lRes!=DISP_CHANGE_SUCCESSFUL) {
    // try to set secondary depth
    devmode.dmBitsPerPel = slBPP2;
    LONG lRes2 = ChangeDisplaySettings(&devmode, CDS_FULLSCREEN);
    // if failed
    if( lRes2!=DISP_CHANGE_SUCCESSFUL) {
      CTString strError;
      switch(lRes) {
      case DISP_CHANGE_SUCCESSFUL:  strError = "DISP_CHANGE_SUCCESSFUL"; break;
      case DISP_CHANGE_RESTART:     strError = "DISP_CHANGE_RESTART"; break;
      case DISP_CHANGE_BADFLAGS:    strError = "DISP_CHANGE_BADFLAGS"; break;
      case DISP_CHANGE_BADPARAM:    strError = "DISP_CHANGE_BADPARAM"; break;
      case DISP_CHANGE_FAILED:      strError = "DISP_CHANGE_FAILED"; break;
      case DISP_CHANGE_BADMODE:     strError = "DISP_CHANGE_BADMODE"; break;
      case DISP_CHANGE_NOTUPDATED:  strError = "DISP_CHANGE_NOTUPDATED"; break;
      default: strError.PrintF("%d", lRes); break;
      }
      CPrintF(TRANS("CDS error: %s\n"), strError.ConstData());
      return FALSE;
    }
  }
  // report
  CPrintF(TRANS("  CDS: mode set to %dx%dx%d\n"), pixSizeI, pixSizeJ, devmode.dmBitsPerPel);
#endif // SE1_WIN

  return TRUE;
}


// reset windows to mode chosen by user within windows diplay properties
void CDS_ResetMode(void)
{
  // no need for gfx when dedicated server is on
  if( _bDedicatedServer) return;

#if SE1_WIN
  LONG lRes = ChangeDisplaySettings( NULL, 0);
  ASSERT(lRes==DISP_CHANGE_SUCCESSFUL);
  CPrintF(TRANS("  CDS: mode reset to original desktop settings\n"));
#endif // SE1_WIN
}
