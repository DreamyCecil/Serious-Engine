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

#include "MenuPrinting.h"
#include "MenuStuff.h"
#include "MVideoOptions.h"

// [Cecil] Screen resolution lists and window modes
#include "ScreenResolutions.h"

static INDEX _ctAdapters = 0;
static CTString * _astrAdapterTexts = NULL;
static INDEX _ctResolutions = 0;
static CTString * _astrResolutionTexts = NULL;
static CDisplayMode *_admResolutionModes = NULL;

// make description for a given resolution
static CTString GetResolutionDescription(CDisplayMode &dm) {
  CTString str;

  // if dual head
  if (dm.IsDualHead()) {
    str.PrintF(TRANS("%dx%d double"), dm.dm_pixSizeI / 2, dm.dm_pixSizeJ);

  // if widescreen
  } else if (dm.IsWideScreen()) {
    str.PrintF(TRANS("%dx%d wide"), dm.dm_pixSizeI, dm.dm_pixSizeJ);

  // otherwise it is normal
  } else {
    str.PrintF("%dx%d", dm.dm_pixSizeI, dm.dm_pixSizeJ);
  }

  // [Cecil] Resolution matches the screen
  if (dm.dm_pixSizeI == _vpixScreenRes(1) && dm.dm_pixSizeJ == _vpixScreenRes(2)) {
    str += TRANS(" (Native)");
  }

  return str;
};

// make description for a given resolution
static void SetResolutionInList(INDEX iRes, PIX pixSizeI, PIX pixSizeJ) {
  ASSERT(iRes >= 0 && iRes<_ctResolutions);

  CTString &str = _astrResolutionTexts[iRes];
  CDisplayMode &dm = _admResolutionModes[iRes];
  dm.dm_pixSizeI = pixSizeI;
  dm.dm_pixSizeJ = pixSizeJ;
  str = GetResolutionDescription(dm);
};

static void ResolutionToSize(INDEX iRes, PIX &pixSizeI, PIX &pixSizeJ) {
  ASSERT(iRes >= 0 && iRes<_ctResolutions);
  CDisplayMode &dm = _admResolutionModes[iRes];
  pixSizeI = dm.dm_pixSizeI;
  pixSizeJ = dm.dm_pixSizeJ;
};

static void SizeToResolution(PIX pixSizeI, PIX pixSizeJ, INDEX &iRes) {
  for (iRes = 0; iRes < _ctResolutions; iRes++) {
    CDisplayMode &dm = _admResolutionModes[iRes];
    if (dm.dm_pixSizeI == pixSizeI && dm.dm_pixSizeJ == pixSizeJ) {
      return;
    }
  }

  // if none was found, search for default 
  for (iRes = 0; iRes < _ctResolutions; iRes++) {
    CDisplayMode &dm = _admResolutionModes[iRes];

    if (dm.dm_pixSizeI == 640 && dm.dm_pixSizeJ == 480) {
      return;
    }
  }

  // if still none found
  // [Cecil] No need to report an error, just select the first resolution
  //ASSERT(FALSE);  // this should never happen
  // return first one
  iRes = 0;
};

// [Cecil] Set all resolutions of some aspect ratio in list
static void SetAspectRatioResolutions(const CAspectRatio &arAspectRatio, INDEX &ctResCounter) {
  const INDEX ctResolutions = arAspectRatio.Count();

  for (INDEX iRes = 0; iRes < ctResolutions; iRes++) {
    const PIX2D &vpix = arAspectRatio[iRes];

    if (vpix(1) > _vpixScreenRes(1) || vpix(2) > _vpixScreenRes(2)) {
      continue; // Skip resolutions bigger than the screen
    }

    SetResolutionInList(ctResCounter++, vpix(1), vpix(2));
  }
};

static INDEX sam_old_iWindowMode; // [Cecil] Different window modes
static INDEX sam_old_iScreenSizeI;
static INDEX sam_old_iScreenSizeJ;
static INDEX sam_old_iDisplayDepth;
static INDEX sam_old_iDisplayAdapter;
static INDEX sam_old_iGfxAPI;
static INDEX sam_old_iVideoSetup; // 0==speed, 1==normal, 2==quality, 3==custom

static void FillResolutionsList(void) {
  CVideoOptionsMenu &gmCurrent = _pGUIM->gmVideoOptionsMenu;

  // free resolutions
  if (_astrResolutionTexts != NULL) {
    delete[] _astrResolutionTexts;
  }

  if (_admResolutionModes != NULL) {
    delete[] _admResolutionModes;
  }

  _ctResolutions = 0;

  // [Cecil] Select current aspect ratio
  const INDEX iAspectRatio = gmCurrent.gm_mgAspectRatiosTrigger.mg_iSelected;
  const CAspectRatio &ar = *_aAspectRatios[iAspectRatio];

  // [Cecil] If 4:3 in borderless or fullscreen
  if (iAspectRatio == 0 && gmCurrent.gm_mgWindowModeTrigger.mg_iSelected != E_WM_WINDOWED) {
    // Get resolutions from the engine
    INDEX ctEngineRes = 0;

    CDisplayMode *pdm = _pGfx->EnumDisplayModes(ctEngineRes,
      SwitchToAPI(gmCurrent.gm_mgDisplayAPITrigger.mg_iSelected), gmCurrent.gm_mgDisplayAdaptersTrigger.mg_iSelected);

    // Remember current amount here to prevent assertions from SetResolutionInList()
    _ctResolutions = ctEngineRes;

    _astrResolutionTexts = new CTString[ctEngineRes];
    _admResolutionModes = new CDisplayMode[ctEngineRes];

    // Add all engine resolutions to the list
    for (INDEX iRes = 0; iRes < ctEngineRes; iRes++) {
      SetResolutionInList(iRes, pdm[iRes].dm_pixSizeI, pdm[iRes].dm_pixSizeJ);
    }

  // [Cecil] If any other aspect ratio or windowed mode
  } else {
    // Amount of resolutions under this aspect ratio
    _ctResolutions = ar.Count();

    _astrResolutionTexts = new CTString[_ctResolutions];
    _admResolutionModes = new CDisplayMode[_ctResolutions];

    // Add all resolutions from the selected aspect ratio
    INDEX ctRes = 0;
    SetAspectRatioResolutions(ar, ctRes);

    _ctResolutions = ctRes;
  }

  gmCurrent.gm_mgResolutionsTrigger.mg_astrTexts = _astrResolutionTexts;
  gmCurrent.gm_mgResolutionsTrigger.mg_ctTexts = _ctResolutions;
};

static void FillAdaptersList(void) {
  CVideoOptionsMenu &gmCurrent = _pGUIM->gmVideoOptionsMenu;

  if (_astrAdapterTexts != NULL) {
    delete[] _astrAdapterTexts;
  }

  _ctAdapters = 0;

  INDEX iApi = SwitchToAPI(gmCurrent.gm_mgDisplayAPITrigger.mg_iSelected);
  _ctAdapters = _pGfx->gl_gaAPI[iApi].ga_ctAdapters;
  _astrAdapterTexts = new CTString[_ctAdapters];

  for (INDEX iAdapter = 0; iAdapter < _ctAdapters; iAdapter++) {
    _astrAdapterTexts[iAdapter] = _pGfx->gl_gaAPI[iApi].ga_adaAdapter[iAdapter].da_strRenderer;
  }

  gmCurrent.gm_mgDisplayAdaptersTrigger.mg_astrTexts = _astrAdapterTexts;
  gmCurrent.gm_mgDisplayAdaptersTrigger.mg_ctTexts = _ctAdapters;
};

static void UpdateVideoOptionsButtons(INDEX iSelected) {
  CVideoOptionsMenu &gmCurrent = _pGUIM->gmVideoOptionsMenu;

  const BOOL _bVideoOptionsChanged = (iSelected != -1);

  const BOOL bOGLEnabled = _pGfx->HasAPI(GAT_OGL);
  const BOOL bD3DEnabled = _pGfx->HasAPI(GAT_D3D);
  ASSERT(bOGLEnabled || bD3DEnabled);

  CDisplayAdapter &da = _pGfx->gl_gaAPI[SwitchToAPI(gmCurrent.gm_mgDisplayAPITrigger.mg_iSelected)]
    .ga_adaAdapter[gmCurrent.gm_mgDisplayAdaptersTrigger.mg_iSelected];

  // number of available preferences is higher if video setup is custom
  gmCurrent.gm_mgDisplayPrefsTrigger.mg_ctTexts = 3;

  if (sam_iVideoSetup == 3) gmCurrent.gm_mgDisplayPrefsTrigger.mg_ctTexts++;

  // enumerate adapters
  FillAdaptersList();

  // show or hide buttons
  gmCurrent.gm_mgDisplayAPITrigger.mg_bEnabled = bOGLEnabled && bD3DEnabled; // [Cecil] Check for D3D
  gmCurrent.gm_mgDisplayAdaptersTrigger.mg_bEnabled = _ctAdapters > 1;
  gmCurrent.gm_mgApply.mg_bEnabled = _bVideoOptionsChanged;

  // determine which should be visible
  gmCurrent.gm_mgWindowModeTrigger.mg_bEnabled = TRUE;

  if (da.da_ulFlags & DAF_FULLSCREENONLY) {
    gmCurrent.gm_mgWindowModeTrigger.mg_bEnabled = FALSE;
    gmCurrent.gm_mgWindowModeTrigger.mg_iSelected = E_WM_FULLSCREEN;
    gmCurrent.gm_mgWindowModeTrigger.ApplyCurrentSelection();
  }

  gmCurrent.gm_mgBitsPerPixelTrigger.mg_bEnabled = TRUE;

  // [Cecil] If not fullscreen
  if (gmCurrent.gm_mgWindowModeTrigger.mg_iSelected != E_WM_FULLSCREEN) {
    gmCurrent.gm_mgBitsPerPixelTrigger.mg_bEnabled = FALSE;
    gmCurrent.gm_mgBitsPerPixelTrigger.mg_iSelected = DepthToSwitch(DD_DEFAULT);
    gmCurrent.gm_mgBitsPerPixelTrigger.ApplyCurrentSelection();

  } else if (da.da_ulFlags & DAF_16BITONLY) {
    gmCurrent.gm_mgBitsPerPixelTrigger.mg_bEnabled = FALSE;
    gmCurrent.gm_mgBitsPerPixelTrigger.mg_iSelected = DepthToSwitch(DD_16BIT);
    gmCurrent.gm_mgBitsPerPixelTrigger.ApplyCurrentSelection();
  }

  // remember current selected resolution
  PIX pixSizeI, pixSizeJ;
  ResolutionToSize(gmCurrent.gm_mgResolutionsTrigger.mg_iSelected, pixSizeI, pixSizeJ);

  // select same resolution again if possible
  FillResolutionsList();
  SizeToResolution(pixSizeI, pixSizeJ, gmCurrent.gm_mgResolutionsTrigger.mg_iSelected);

  // apply adapter and resolutions
  gmCurrent.gm_mgDisplayAdaptersTrigger.ApplyCurrentSelection();
  gmCurrent.gm_mgResolutionsTrigger.ApplyCurrentSelection();
  gmCurrent.gm_mgAspectRatiosTrigger.ApplyCurrentSelection(); // [Cecil]
};

static void InitVideoOptionsButtons(void) {
  CVideoOptionsMenu &gmCurrent = _pGUIM->gmVideoOptionsMenu;

  // [Cecil] Limit to existing window modes
  INDEX iWindowMode = Clamp(sam_iWindowMode, (INDEX)E_WM_WINDOWED, (INDEX)E_WM_FULLSCREEN);
  gmCurrent.gm_mgWindowModeTrigger.mg_iSelected = iWindowMode;

  gmCurrent.gm_mgDisplayAPITrigger.mg_iSelected = APIToSwitch((GfxAPIType)(INDEX)sam_iGfxAPI);
  gmCurrent.gm_mgDisplayAdaptersTrigger.mg_iSelected = sam_iDisplayAdapter;
  gmCurrent.gm_mgBitsPerPixelTrigger.mg_iSelected = DepthToSwitch((enum DisplayDepth)(INDEX)sam_iDisplayDepth);

  // [Cecil] Find aspect ratio and the resolution within it
  PIX2D vScreen(sam_iScreenSizeI, sam_iScreenSizeJ);
  SizeToAspectRatio(vScreen, gmCurrent.gm_mgAspectRatiosTrigger.mg_iSelected);

  FillResolutionsList();
  SizeToResolution(vScreen(1), vScreen(2), gmCurrent.gm_mgResolutionsTrigger.mg_iSelected);
  gmCurrent.gm_mgDisplayPrefsTrigger.mg_iSelected = Clamp(int(sam_iVideoSetup), 0, 3);

  gmCurrent.gm_mgWindowModeTrigger.ApplyCurrentSelection();
  gmCurrent.gm_mgDisplayPrefsTrigger.ApplyCurrentSelection();
  gmCurrent.gm_mgDisplayAPITrigger.ApplyCurrentSelection();
  gmCurrent.gm_mgDisplayAdaptersTrigger.ApplyCurrentSelection();
  gmCurrent.gm_mgResolutionsTrigger.ApplyCurrentSelection();
  gmCurrent.gm_mgAspectRatiosTrigger.ApplyCurrentSelection(); // [Cecil]
  gmCurrent.gm_mgBitsPerPixelTrigger.ApplyCurrentSelection();
};

static void StartRenderingOptionsMenu(void) {
  CVarMenu::ChangeTo(TRANS("RENDERING OPTIONS"), CTFILENAME("Scripts\\Menu\\RenderingOptions.cfg"));
};

static void RevertVideoSettings(void);

static void VideoConfirm(void) {
  CConfirmMenu &gmCurrent = _pGUIM->gmConfirmMenu;

  // FIXUP: keyboard focus lost when going from full screen to window mode
  // due to WM_MOUSEMOVE being sent
  extern BOOL _bMouseUsedLast;
  extern CMenuGadget *_pmgUnderCursor;
  _bMouseUsedLast = FALSE;
  _pmgUnderCursor = gmCurrent.GetDefaultGadget();

  CConfirmMenu::ChangeTo(TRANS("KEEP THIS SETTING?"), NULL, &RevertVideoSettings, TRUE);
};

static void ApplyVideoOptions(void) {
  CVideoOptionsMenu &gmCurrent = _pGUIM->gmVideoOptionsMenu;

  // Remember old video settings
  sam_old_iWindowMode = sam_iWindowMode;
  sam_old_iScreenSizeI = sam_iScreenSizeI;
  sam_old_iScreenSizeJ = sam_iScreenSizeJ;
  sam_old_iDisplayDepth = sam_iDisplayDepth;
  sam_old_iDisplayAdapter = sam_iDisplayAdapter;
  sam_old_iGfxAPI = sam_iGfxAPI;
  sam_old_iVideoSetup = sam_iVideoSetup;

  // [Cecil] Different window modes
  INDEX iWindowMode = gmCurrent.gm_mgWindowModeTrigger.mg_iSelected;
  PIX pixWindowSizeI, pixWindowSizeJ;
  ResolutionToSize(gmCurrent.gm_mgResolutionsTrigger.mg_iSelected, pixWindowSizeI, pixWindowSizeJ);
  enum GfxAPIType gat = SwitchToAPI(gmCurrent.gm_mgDisplayAPITrigger.mg_iSelected);
  enum DisplayDepth dd = SwitchToDepth(gmCurrent.gm_mgBitsPerPixelTrigger.mg_iSelected);
  const INDEX iAdapter = gmCurrent.gm_mgDisplayAdaptersTrigger.mg_iSelected;

  // setup preferences
  extern INDEX _iLastPreferences;
  if (sam_iVideoSetup == 3) _iLastPreferences = 3;
  sam_iVideoSetup = gmCurrent.gm_mgDisplayPrefsTrigger.mg_iSelected;

  // force fullscreen mode if needed
  CDisplayAdapter &da = _pGfx->gl_gaAPI[gat].ga_adaAdapter[iAdapter];
  if (da.da_ulFlags & DAF_FULLSCREENONLY) iWindowMode = E_WM_FULLSCREEN; // [Cecil]
  if (da.da_ulFlags & DAF_16BITONLY) dd = DD_16BIT;
  // force window to always be in default colors
  if (iWindowMode != E_WM_FULLSCREEN) dd = DD_DEFAULT; // [Cecil]

  // (try to) set mode
  StartNewMode(gat, iAdapter, pixWindowSizeI, pixWindowSizeJ, dd, iWindowMode);

  // refresh buttons
  InitVideoOptionsButtons();
  UpdateVideoOptionsButtons(-1);

  // ask user to keep or restore
  if (iWindowMode == E_WM_FULLSCREEN) VideoConfirm(); // [Cecil]
};

void RevertVideoSettings(void) {
  // restore previous variables
  sam_iWindowMode = sam_old_iWindowMode; // [Cecil]
  sam_iScreenSizeI = sam_old_iScreenSizeI;
  sam_iScreenSizeJ = sam_old_iScreenSizeJ;
  sam_iDisplayDepth = sam_old_iDisplayDepth;
  sam_iDisplayAdapter = sam_old_iDisplayAdapter;
  sam_iGfxAPI = sam_old_iGfxAPI;
  sam_iVideoSetup = sam_old_iVideoSetup;

  // update the video mode
  extern void ApplyVideoMode(void);
  ApplyVideoMode();

  // refresh buttons
  InitVideoOptionsButtons();
  UpdateVideoOptionsButtons(-1);
};

void CVideoOptionsMenu::Initialize_t(void)
{
  // [Cecil] Create array of available graphics APIs
  const INDEX ctAPIs = GAT_MAX;

  if (astrDisplayAPIRadioTexts == NULL) {
    astrDisplayAPIRadioTexts = new CTString[ctAPIs];

    for (INDEX iAPI = 0; iAPI < ctAPIs; iAPI++) {
      const CTString &strAPI = CGfxLibrary::GetApiName((GfxAPIType)iAPI);
      astrDisplayAPIRadioTexts[iAPI] = TRANSV(strAPI);
    }
  }

  // [Cecil] Prepare arrays with window resolutions
  PrepareVideoResolutions();

  // intialize video options menu
  gm_mgTitle.mg_boxOnScreen = BoxTitle();
  gm_mgTitle.SetText(TRANS("VIDEO"));
  gm_lhGadgets.AddTail(gm_mgTitle.mg_lnNode);

  TRIGGER_MG(gm_mgDisplayAPITrigger, 0,
    gm_mgApply, gm_mgDisplayAdaptersTrigger, TRANS("GRAPHICS API"), astrDisplayAPIRadioTexts);
  gm_mgDisplayAPITrigger.mg_strTip = TRANS("choose graphics API to be used");
  gm_mgDisplayAPITrigger.mg_ctTexts = ctAPIs; // [Cecil] Amount of available APIs

  TRIGGER_MG(gm_mgDisplayAdaptersTrigger, 1,
    gm_mgDisplayAPITrigger, gm_mgDisplayPrefsTrigger, TRANS("DISPLAY ADAPTER"), astrNoYes);
  gm_mgDisplayAdaptersTrigger.mg_strTip = TRANS("choose display adapter to be used");

  TRIGGER_MG(gm_mgDisplayPrefsTrigger, 2,
    gm_mgDisplayAdaptersTrigger, gm_mgAspectRatiosTrigger, TRANS("PREFERENCES"), astrDisplayPrefsRadioTexts);
  gm_mgDisplayPrefsTrigger.mg_strTip = TRANS("balance between speed and rendering quality, depending on your system");

  // [Cecil] Aspect ratio list
  TRIGGER_MG(gm_mgAspectRatiosTrigger, 3,
    gm_mgDisplayPrefsTrigger, gm_mgResolutionsTrigger, TRANS("ASPECT RATIO"), _astrAspectRatios);
  gm_mgAspectRatiosTrigger.mg_strTip = TRANS("select video mode aspect ratio");

  TRIGGER_MG(gm_mgResolutionsTrigger, 4,
    gm_mgAspectRatiosTrigger, gm_mgWindowModeTrigger, TRANS("RESOLUTION"), astrNoYes);
  gm_mgResolutionsTrigger.mg_strTip = TRANS("select video mode resolution");

  // [Cecil] Changed fullscreen switch to window modes
  TRIGGER_MG(gm_mgWindowModeTrigger, 5,
    gm_mgResolutionsTrigger, gm_mgBitsPerPixelTrigger, TRANS("WINDOW MODE"), _astrWindowModes);
  gm_mgWindowModeTrigger.mg_strTip = TRANS("make game run in a window or in full screen");

  TRIGGER_MG(gm_mgBitsPerPixelTrigger, 6,
    gm_mgWindowModeTrigger, gm_mgVideoRendering, TRANS("BITS PER PIXEL"), astrBitsPerPixelRadioTexts);
  gm_mgBitsPerPixelTrigger.mg_strTip = TRANS("select number of colors used for display");

  gm_mgDisplayPrefsTrigger.mg_pOnTriggerChange = &UpdateVideoOptionsButtons;
  gm_mgDisplayAPITrigger.mg_pOnTriggerChange = &UpdateVideoOptionsButtons;
  gm_mgDisplayAdaptersTrigger.mg_pOnTriggerChange = &UpdateVideoOptionsButtons;
  gm_mgWindowModeTrigger.mg_pOnTriggerChange = &UpdateVideoOptionsButtons; // [Cecil]
  gm_mgAspectRatiosTrigger.mg_pOnTriggerChange = &UpdateVideoOptionsButtons; // [Cecil]
  gm_mgResolutionsTrigger.mg_pOnTriggerChange = &UpdateVideoOptionsButtons;
  gm_mgBitsPerPixelTrigger.mg_pOnTriggerChange = &UpdateVideoOptionsButtons;

  gm_mgVideoRendering.mg_bfsFontSize = BFS_MEDIUM;
  gm_mgVideoRendering.mg_boxOnScreen = BoxMediumRow(8.0f);
  gm_mgVideoRendering.mg_pmgUp = &gm_mgBitsPerPixelTrigger;
  gm_mgVideoRendering.mg_pmgDown = &gm_mgApply;
  gm_mgVideoRendering.SetText(TRANS("RENDERING OPTIONS"));
  gm_mgVideoRendering.mg_strTip = TRANS("manually adjust rendering settings");
  gm_lhGadgets.AddTail(gm_mgVideoRendering.mg_lnNode);
  gm_mgVideoRendering.mg_pActivatedFunction = &StartRenderingOptionsMenu;

  gm_mgApply.mg_bfsFontSize = BFS_LARGE;
  gm_mgApply.mg_boxOnScreen = BoxBigRow(6.5f);
  gm_mgApply.mg_pmgUp = &gm_mgVideoRendering;
  gm_mgApply.mg_pmgDown = &gm_mgDisplayAPITrigger;
  gm_mgApply.SetText(TRANS("APPLY"));
  gm_mgApply.mg_strTip = TRANS("apply selected options");
  gm_lhGadgets.AddTail(gm_mgApply.mg_lnNode);
  gm_mgApply.mg_pActivatedFunction = &ApplyVideoOptions;
}

void CVideoOptionsMenu::StartMenu(void)
{
  InitVideoOptionsButtons();

  CGameMenu::StartMenu();

  UpdateVideoOptionsButtons(-1);
}

// [Cecil] Change to the menu
void CVideoOptionsMenu::ChangeTo(void) {
  ChangeToMenu(&_pGUIM->gmVideoOptionsMenu);
};
