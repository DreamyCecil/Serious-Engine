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

#include <sys/timeb.h>
#include <time.h>
#include "MainWindow.h"
#include <Engine/Templates/Stock_CSoundData.h>
#include <Engine/Templates/Stock_CTextureData.h>
#include "MenuPrinting.h"
#include "LevelInfo.h"
#include "VarList.h"
#include "FileInfo.h"

#include "MenuStuff.h"

// macros for translating radio button text arrays
#define TRANSLATERADIOARRAY(array) TranslateRadioTexts(array, ARRAYCOUNT(array))

INDEX _iLocalPlayer = -1;
GameMode _gmMenuGameMode = GM_NONE;
GameMode _gmRunningGameMode = GM_NONE;

// [Cecil] Defined here
CMenuManager *_pGUIM = NULL;

// last tick done
static TICK _tckMenuLastTickDone = -1;

#if SE1_PREFER_SDL

// [Cecil] Custom game cursor
static CTextureData *_ptdCursorData = NULL;
static SDL_Surface *_pCursorTexture = NULL;
static SDL_Cursor *_pCursor = NULL;

// [Cecil] SDL: Clear custom cursor
static void DestroyGameCursor(void) {
  if (_pCursor != NULL) {
    SDL_DestroyCursor(_pCursor);
    _pCursor = NULL;
  }

  if (_pCursorTexture != NULL) {
    SDL_DestroySurface(_pCursorTexture);
    _pCursorTexture = NULL;
  }

  if (_ptdCursorData != NULL) {
    _pTextureStock->Release(_ptdCursorData);
    _ptdCursorData = NULL;
  }
};

// [Cecil] SDL: Set custom cursor
static void SetGameCursor(void) {
  try {
    // Load cursor texture
    _ptdCursorData = _pTextureStock->Obtain_t(CTFILENAME("TexturesMP\\General\\Pointer.tex"));

    // Needed to actually load pixel data into td_pulFrames and never free/reallocate it
    _ptdCursorData->Force(TEX_STATIC | TEX_CONSTANT);

    if (_ptdCursorData->td_pulFrames == NULL) ThrowF_t(TRANS("Texture has no frames!"));

    // Create SDL texture from its data
    _pCursorTexture = SDL_CreateSurfaceFrom(_ptdCursorData->GetPixWidth(), _ptdCursorData->GetPixHeight(),
      SDL_PIXELFORMAT_ABGR8888, _ptdCursorData->td_pulFrames, _ptdCursorData->GetPixWidth() * 4);

    if (_pCursorTexture == NULL) ThrowF_t(TRANS("Cannot create surface from texture! SDL Error: %s"), SDL_GetError());

    // Create SDL cursor from the SDL texture
    _pCursor = SDL_CreateColorCursor(_pCursorTexture, 1, 1);
    if (_pCursor == NULL) ThrowF_t(TRANS("Cannot create color cursor! SDL Error: %s"), SDL_GetError());

    // Finally, set the cursor
    if (!SDL_SetCursor(_pCursor)) ThrowF_t(TRANS("Cannot set SDL cursor! SDL Error: %s"), SDL_GetError());

  } catch (char *strError) {
    CPrintF(TRANS("Couldn't set SDL cursor:\n  %s\n"), strError);
    DestroyGameCursor();
  }
};

#endif // SE1_PREFER_SDL

BOOL _bVarChanged = FALSE;

// Play a specific menu sound, replacing the previous sound if needed
void CMenuManager::PlayMenuSound(EMenuSound eSound, BOOL bOverOtherSounds) {
  CSoundData *psd = NULL;
  const char *strEffect = NULL;

  // [Cecil] Select sound based on type
  switch (eSound) {
    case E_MSND_SELECT:
      psd = m_psdSelect;
      strEffect = "Menu_select";
      break;

    case E_MSND_PRESS:
      psd = m_psdPress;
      strEffect = "Menu_press";
      break;

    case E_MSND_RETURN:
      psd = m_psdReturn;
      strEffect = "Menu_press";
      break;

    case E_MSND_DISABLED:
      psd = m_psdDisabled;
      break;

    default:
      ASSERTALWAYS("Unknown menu sound type in PlayMenuSound()!");
      return;
  }

  if (bOverOtherSounds || !m_soMenuSound.IsPlaying()) {
    m_soMenuSound.Play(psd, SOF_NONGAME);
  }

  // [Cecil] Play IFeel effects here
  if (strEffect != NULL) {
    IFeel_PlayEffect(strEffect);
  }
};

// translate all texts in array for one radio button
void TranslateRadioTexts(CTString astr[], INDEX ct)
{
  for (INDEX i=0; i<ct; i++) {
    astr[i] = TranslateConst(astr[i], 4);
  }
}

// Set new thumbnail
void CMenuManager::SetThumbnail(const CTString &fnm) {
  m_bThumbnailOn = TRUE;
  const CTString fnmNoExt = fnm.NoExt();

  try {
    m_toThumbnail.SetData_t(fnmNoExt + "Tbn.tex");

  } catch (char *strError) {
    (void)strError;

    try {
      m_toThumbnail.SetData_t(fnmNoExt + ".tbn");

    } catch (char *strError) {
      (void)strError;
      m_toThumbnail.SetData(NULL);
    }
  }
};

// Remove thumbnail
void CMenuManager::ClearThumbnail(void) {
  m_bThumbnailOn = FALSE;
  m_toThumbnail.SetData(NULL);
  _pShell->Execute("FreeUnusedStock();");
};

void CMenuManager::StartMenus(EQuickMenu eMenu) {
  _tckMenuLastTickDone = _pTimer->GetRealTime();

  // disable printing of last lines
  CON_DiscardLastLineTimes();

  // stop all IFeel effects
  IFeel_StopEffect(NULL);

  // [Cecil] When opening a special menu, discard all the active ones
  BOOL bSpecialMenu = FALSE;

  if (eMenu != E_QCKM_NONE) {
    ClearVisitedMenus();
    bSpecialMenu = TRUE;

    switch (eMenu) {
      case E_QCKM_LOAD: {
        extern void StartCurrentLoadMenu(void);
        StartCurrentLoadMenu();
      } break;

      case E_QCKM_SAVE: {
        extern void StartCurrentSaveMenu(void);
        StartCurrentSaveMenu();
      } break;

      case E_QCKM_CONTROLS: {
        CControlsMenu::ChangeTo();
      } break;

      case E_QCKM_JOIN: {
        extern void StartSelectPlayersMenuFromOpen(void);
        StartSelectPlayersMenuFromOpen();
      } break;

      case E_QCKM_HIGHSCORE: {
        CHighScoreMenu::ChangeTo();
      } break;

      default: {
        ASSERTALWAYS("No special menu to open for this type!");
        bSpecialMenu = FALSE;
      }
    }
  }

  // Otherwise open a root menu or the last active one
  if (!bSpecialMenu) {
    CGameMenu *pgmCurrent = GetCurrentMenu();

    // If there's no last active menu
    if (pgmCurrent == NULL) {
      // Go to the root menu depending on the game state
      if (_gmRunningGameMode == GM_NONE) {
        pgmCurrent = &gmMainMenu;
      } else {
        pgmCurrent = &gmInGameMenu;
      }

      // Start the menu
      ASSERT(pgmCurrent != NULL);
      ChangeToMenu(pgmCurrent);

    // [Cecil] Otherwise reactivate the last menu
    } else {
      pgmCurrent->StartMenu();
    }
  }

  m_bMenuActive = TRUE;
  m_bMenuRendering = TRUE;
};

void CMenuManager::StopMenus(BOOL bGoToRoot) {
  ClearThumbnail();

  CGameMenu *pgmCurrent = GetCurrentMenu();

  if (pgmCurrent != NULL && m_bMenuActive) {
    pgmCurrent->EndMenu();
  }

  m_bMenuActive = FALSE;

  if (bGoToRoot) {
    // [Cecil] Clear all visited menus to open the root one next time
    ClearVisitedMenus();
  }
};

// [Cecil] Check if it's a root menu
BOOL CMenuManager::IsMenuRoot(class CGameMenu *pgm) {
  return pgm == NULL || pgm == &gmMainMenu || pgm == &gmInGameMenu;
};

static void LoadAndForceTexture(CTextureObject &to, const CTFileName &fnm) {
  try {
    to.SetData_t(fnm);

    CTextureData *ptd = (CTextureData *)to.GetData();
    ptd->Force(TEX_CONSTANT);

    ptd = ptd->td_ptdBaseTexture;

    if (ptd != NULL) {
      ptd->Force(TEX_CONSTANT);
    }

  } catch (char *strError) {
    (void *)strError;
    to.SetData(NULL);
  }
};

CMenuManager::CMenuManager() {
  m_bMenuActive = FALSE;
  m_bMenuRendering = FALSE;

  m_bDefiningKey = FALSE;
  m_bEditingString = FALSE;

  m_aCursorPos[0] = m_aCursorPos[1] = 0;
  m_aCursorExternPos[0] = m_aCursorExternPos[1] = 0;

  m_bMouseUsedLast = FALSE;
  m_pmgUnderCursor = NULL;

  m_ctVisitedMenus = 0;

  m_bThumbnailOn = FALSE;
  m_psdSelect = NULL;
  m_psdPress = NULL;
  m_psdReturn = NULL;
  m_psdDisabled = NULL;

  // Load optional logo textures
  LoadAndForceTexture(m_toLogoCT,  CTFILENAME("Textures\\Logo\\LogoCT.tex"));
  LoadAndForceTexture(m_toLogoODI, CTFILENAME("Textures\\Logo\\GodGamesLogo.tex"));
  LoadAndForceTexture(m_toLogoEAX, CTFILENAME("Textures\\Logo\\LogoEAX.tex"));

  try {
    // initialize and load corresponding fonts
    m_fdMedium.Load_t(CTFILENAME("Fonts\\Display3-normal.fnt"));
    m_fdMedium.SetCharSpacing(+1);
    m_fdMedium.SetLineSpacing(0);
    m_fdMedium.SetSpaceWidth(0.4f);

    m_fdBig.Load_t(CTFILENAME("Fonts\\Display3-caps.fnt"));
    m_fdBig.SetCharSpacing(+1);
    m_fdBig.SetLineSpacing(0);

    m_fdTitle.Load_t(CTFILENAME("Fonts\\Title2.fnt"));
    m_fdTitle.SetCharSpacing(+1);
    m_fdTitle.SetLineSpacing(0);

    // load menu sounds
    m_psdSelect   = _pSoundStock->Obtain_t(CTFILENAME("Sounds\\Menu\\Select.wav"));
    m_psdPress    = _pSoundStock->Obtain_t(CTFILENAME("Sounds\\Menu\\Press.wav"));
    m_psdReturn   = _pSoundStock->Obtain_t(CTFILENAME("Sounds\\Menu\\Press.wav"));
    m_psdDisabled = _pSoundStock->Obtain_t(CTFILENAME("Sounds\\Menu\\Press.wav"));

    // initialize and load menu textures
    m_toLogoMenuA.SetData_t(CTFILENAME("Textures\\Logo\\sam_menulogo256a.tex"));
    m_toLogoMenuB.SetData_t(CTFILENAME("Textures\\Logo\\sam_menulogo256b.tex"));

    // force logo textures to be of maximal size
    ((CTextureData *)m_toLogoMenuA.GetData())->Force(TEX_CONSTANT);
    ((CTextureData *)m_toLogoMenuB.GetData())->Force(TEX_CONSTANT);

    // Translate strings in arrays
    TRANSLATERADIOARRAY(astrNoYes);
    TRANSLATERADIOARRAY(astrComputerInvoke);
    //TRANSLATERADIOARRAY(astrDisplayAPIRadioTexts);
    TRANSLATERADIOARRAY(astrDisplayPrefsRadioTexts);
    TRANSLATERADIOARRAY(astrBitsPerPixelRadioTexts);
    TRANSLATERADIOARRAY(astrFrequencyRadioTexts);
    //TRANSLATERADIOARRAY(astrSoundAPIRadioTexts);
    TRANSLATERADIOARRAY(astrDifficultyRadioTexts);
    TRANSLATERADIOARRAY(astrMaxPlayersRadioTexts);
    TRANSLATERADIOARRAY(astrWeapon);
    //TRANSLATERADIOARRAY(astrSplitScreenRadioTexts);

    // initialize game type strings table
    InitGameTypes();

    // Initialize menus
    gmConfirmMenu.Initialize_t();
    gmMainMenu.Initialize_t();
    gmInGameMenu.Initialize_t();
    gmSinglePlayerMenu.Initialize_t();
    gmSinglePlayerNewMenu.Initialize_t();
    gmPlayerProfile.Initialize_t();
    gmControls.Initialize_t();
    gmLoadSaveMenu.Initialize_t();
    gmHighScoreMenu.Initialize_t();
    gmCustomizeKeyboardMenu.Initialize_t();
    gmCustomizeAxisMenu.Initialize_t();
    gmOptionsMenu.Initialize_t();
    gmVideoOptionsMenu.Initialize_t();
    gmAudioOptionsMenu.Initialize_t();
    gmLevelsMenu.Initialize_t();
    gmVarMenu.Initialize_t();
    gmServersMenu.Initialize_t();
    gmNetworkMenu.Initialize_t();
    gmNetworkStartMenu.Initialize_t();
    gmNetworkJoinMenu.Initialize_t();
    gmSelectPlayersMenu.Initialize_t();
    gmNetworkOpenMenu.Initialize_t();
    gmSplitScreenMenu.Initialize_t();
    gmSplitStartMenu.Initialize_t();

  } catch (char *strError) {
    FatalError(strError);
  }

#if SE1_PREFER_SDL
  // [Cecil] SDL: Set custom cursor
  SetGameCursor();
#endif
};

CMenuManager::~CMenuManager() {
#if SE1_PREFER_SDL
  // [Cecil] SDL: Clear custom cursor
  DestroyGameCursor();
#endif

  ClearVisitedMenus();

  _pSoundStock->Release(m_psdSelect);
  _pSoundStock->Release(m_psdPress);
  _pSoundStock->Release(m_psdReturn);
  _pSoundStock->Release(m_psdDisabled);
  m_psdSelect = NULL;
  m_psdPress = NULL;
  m_psdReturn = NULL;
  m_psdDisabled = NULL;
};

// go to parent menu if possible
void CMenuManager::MenuGoToParent(void) {
  // [Cecil] Return to the previous menu with a sound
  _pGUIM->ChangeToMenu(NULL);
  _pGUIM->PlayMenuSound(E_MSND_RETURN);
};

void CMenuManager::MenuOnKeyDown(PressedMenuButton pmb) {
  // [Cecil] Check if mouse buttons are used separately
  m_bMouseUsedLast = (pmb.iMouse != -1);

  // Ignore the mouse when editing
  if (m_bEditingString && m_bMouseUsedLast) {
    m_bMouseUsedLast = FALSE;
    return;
  }

  // [Cecil] Let the menu handle the button regardless of anything
  CGameMenu *pgm = GetCurrentMenu()->GetLastMenu();
  const BOOL bHandled = (pgm != NULL ? pgm->OnKeyDown(pmb) : FALSE);

  // Return to the previous menu if the back button wasn't handled
  if (!bHandled && pmb.Back(TRUE)) {
    MenuGoToParent();
  }
};

void CMenuManager::MenuOnChar(const OS::SE1Event &event) {
  // check if mouse buttons used
  m_bMouseUsedLast = FALSE;

  // ask current menu to handle the key
  CGameMenu *pgm = GetCurrentMenu()->GetLastMenu();
  if (pgm != NULL) pgm->OnChar(event);
};

void CMenuManager::MenuOnMouseMove(PIX pixI, PIX pixJ) {
  static PIX pixLastI = 0;
  static PIX pixLastJ = 0;

  // No movement since last time
  if (pixLastI == pixI && pixLastJ == pixJ) return;

  pixLastI = pixI;
  pixLastJ = pixJ;

  m_bMouseUsedLast = !m_bEditingString && !m_bDefiningKey && !_pInput->IsInputEnabled();
};

void CMenuManager::MenuUpdateMouseFocus(CGameMenu *pgm)
{
  // get real cursor position
  float fMouseX, fMouseY;
  OS::GetMouseState(&fMouseX, &fMouseY);
  extern INDEX sam_bWideScreen;
  extern CDrawPort *pdp;
  if( sam_bWideScreen) {
    const PIX pixHeight = pdp->GetHeight();
    fMouseY -= (pixHeight / 0.75f - pixHeight) / 2;
  }
  m_aCursorPos[0] += fMouseX - m_aCursorExternPos[0];
  m_aCursorPos[1]  = m_aCursorExternPos[1];
  m_aCursorExternPos[0] = fMouseX;
  m_aCursorExternPos[1] = fMouseY;

  // if mouse not used last
  if (!m_bMouseUsedLast || m_bDefiningKey || m_bEditingString) {
    // do nothing
    return;
  }

  // if there is some under cursor
  if (m_pmgUnderCursor != NULL) {
    m_pmgUnderCursor->OnMouseOver(m_aCursorPos[0], m_aCursorPos[1]);
    // if the one under cursor has no neighbours
    if (m_pmgUnderCursor->mg_pmgLeft  == NULL
     && m_pmgUnderCursor->mg_pmgRight == NULL
     && m_pmgUnderCursor->mg_pmgUp    == NULL
     && m_pmgUnderCursor->mg_pmgDown  == NULL) {
      // it cannot be focused
      m_pmgUnderCursor = NULL;
      return;
    }

    // [Cecil] Change focus to the gadget under the cursor in the current menu
    if (m_pmgUnderCursor->mg_bVisible) {
      pgm->FocusGadget(m_pmgUnderCursor);
    }
  }
}

void SetMenuLerping(void) {
  static CTimerValue _tvInitialization;
  static TICK _tckInitializationTick = -1;

  CTimerValue tvNow = _pTimer->GetHighPrecisionTimer();
  
  // if lerping was never set before
  if (_tckInitializationTick < 0) {
    // initialize it
    _tvInitialization = tvNow;
    _tckInitializationTick = _tckMenuLastTickDone;
  }

  // get passed time from session state starting in precise time and in ticks
  const SECOND tmRealDelta = (tvNow - _tvInitialization).GetSeconds();
  const SECOND tmTickDelta = SECOND(_tckMenuLastTickDone - _tckInitializationTick) / (SECOND)_pTimer->TickRate;
  // calculate factor
  SECOND fFactor = 1.0 - (tmTickDelta - tmRealDelta) * (SECOND)_pTimer->TickRate;

  // if the factor starts getting below zero
  if (fFactor<0) {
    // clamp it
    fFactor = 0.0;
    // readjust timers so that it gets better
    _tvInitialization = tvNow;
    _tckInitializationTick = _tckMenuLastTickDone - 1;
  }
  if (fFactor>1) {
    // clamp it
    fFactor = 1.0;
    // readjust timers so that it gets better
    _tvInitialization = tvNow;
    _tckInitializationTick = _tckMenuLastTickDone;
  }
  // set lerping factor and timer
  _pTimer->SetGameTick(_tckMenuLastTickDone);
  _pTimer->SetLerp(fFactor);
}

// Render mouse cursor if needed
void CMenuManager::RenderMouseCursor(CDrawPort *pdp)
{
  // if mouse not used last
  if (!m_bMouseUsedLast || m_bDefiningKey || m_bEditingString) {
    // don't render cursor
    return;
  }
  _pGame->LCDSetDrawport(pdp);

  // [Cecil] Render the in-game cursor the old-fashioned way under Win32
#if !SE1_PREFER_SDL
  _pGame->LCDDrawPointer(m_aCursorPos[0], m_aCursorPos[1]);
#endif
};

BOOL CMenuManager::DoMenu(CDrawPort *pdp)
{
  // [Cecil] No current menu
  if (GetCurrentMenu() == NULL) return FALSE;

  pdp->Unlock();
  CDrawPort dpMenu(pdp, TRUE);
  dpMenu.Lock();

  // [Cecil] Only process the last possible submenu
  CGameMenu *pgmThink = GetCurrentMenu()->GetLastMenu();

  if (pgmThink != NULL) {
    MenuUpdateMouseFocus(pgmThink);

    // if in fullscreen
    CDisplayMode dmCurrent;
    _pGfx->GetCurrentDisplayMode(dmCurrent);
    if (dmCurrent.IsFullScreen()) {
      // clamp mouse pointer
      m_aCursorPos[0] = Clamp(m_aCursorPos[0], 0L, dpMenu.GetWidth());
      m_aCursorPos[1] = Clamp(m_aCursorPos[1], 0L, dpMenu.GetHeight());
    // if in window
    } else {
      // use same mouse pointer as windows
      m_aCursorPos[0] = m_aCursorExternPos[0];
      m_aCursorPos[1] = m_aCursorExternPos[1];
    }

    pgmThink->Think();

    const TICK tckTickNow = _pTimer->GetRealTime();

    while (_tckMenuLastTickDone < tckTickNow)
    {
      _pTimer->SetGameTick(_tckMenuLastTickDone);

      FOREACHNODE(pgmThink, CAbstractMenuElement, itme) {
        if (itme->IsMenu()) continue;

        CMenuGadget &mg = (CMenuGadget &)itme.Current();
        mg.Think();
      }

      _tckMenuLastTickDone++;
    }
  }

  SetMenuLerping();

  // [Cecil] Prepare game for rendering the menu background
  _pGame->LCDPrepare(1.0f);

  // no entity is under cursor initially
  m_pmgUnderCursor = NULL;

  // [Cecil] Render the current menu
  const BOOL bStilInMenus = GetCurrentMenu()->Render(&dpMenu, &m_pmgUnderCursor);

  const PIX pixW = dpMenu.GetWidth();
  const PIX pixH = dpMenu.GetHeight();

  // If the menu is still active
  if (m_bMenuActive) {
    // Render a potential thumbnail, if needed
    if (m_bThumbnailOn) {
      const FLOAT fScaleW = (FLOAT)pixW / 640.0f;
      const FLOAT fScaleH = (FLOAT)pixH / 480.0f;

      const PIX2D vThumb(96, 96);
      const FLOAT fThumbScaleW = fScaleW * dpMenu.dp_fWideAdjustment;

      PIX2D vStart(8 * fScaleW, (240 - vThumb(1) / 2) * fScaleH);
      PIX2D vSize(vThumb(1) * fThumbScaleW, vThumb(2) * fScaleH);

      // Show thumbnail with shadow and border
      if (m_toThumbnail.GetData() != NULL) {
        const PIX pixShadowOffset = 8 * fScaleW;

        dpMenu.Fill(vStart(1) + pixShadowOffset, vStart(2) + pixShadowOffset, vSize(1), vSize(2), C_BLACK | 0x7F);
        dpMenu.PutTexture(&m_toThumbnail, PIXaabbox2D(vStart, vStart + vSize), C_WHITE | 0xFF);
        dpMenu.DrawBorder(vStart(1), vStart(2), vSize(1), vSize(2), _pGame->LCDGetColor(C_mdGREEN | 0xFF, "thumbnail border"));

      } else {
        dpMenu.SetFont(_pfdDisplayFont);
        dpMenu.SetTextScaling(fScaleH);
        dpMenu.SetTextAspect(1.0f);

        vStart += vSize / 2;
        dpMenu.PutTextCXY(TRANS("no thumbnail"), vStart(1), vStart(2), _pGame->LCDGetColor(C_GREEN | 0xFF, "no thumbnail"));
      }
    }

    // Assure that we can listen to non-3D sounds
    _pSound->UpdateSounds();
  }

  // no currently active gadget initially
  CMenuGadget *pmgActive = NULL;

  // if mouse was not active last
  if (!m_bMouseUsedLast || m_bDefiningKey || m_bEditingString) {
    pmgActive = GetCurrentMenu()->GetFocused();

  // if mouse was active last
  } else {
    // gadget under cursor is active
    pmgActive = m_pmgUnderCursor;
  }

  // if editing
  if (m_bEditingString && pmgActive!=NULL) {
    // dim the menu  bit
    dpMenu.Fill(C_BLACK|0x40);
    // render the edit gadget again
    pmgActive->Render(&dpMenu);
  }
  
  // if there is some active gadget and it has tips
  if (pmgActive!=NULL && (pmgActive->mg_strTip!="" || m_bEditingString)) {
    CTString strTip = pmgActive->mg_strTip;
    if (m_bEditingString) {
      strTip = TRANS("Enter - OK, Escape - Cancel");
    }
    // print the tip
    SetFontMedium(&dpMenu, 1.0f);
    dpMenu.PutTextC(strTip, 
      pixW*0.5f, pixH*0.92f, _pGame->LCDGetColor(C_WHITE|255, "tool tip"));
  }

  _pGame->ConsolePrintLastLines(&dpMenu);

  RenderMouseCursor(&dpMenu);

  dpMenu.Unlock();
  pdp->Lock();

  return bStilInMenus;
}

void CMenuManager::FixupBackButton(CGameMenu *pgm) {
  BOOL bResume = FALSE;
  BOOL bHasBack = TRUE;

  if (GetMenuCount() <= 1) {
    if (_gmRunningGameMode != GM_NONE) {
      bResume = TRUE;

    // [Cecil] Only remove the back button in root menus
    } else if (IsMenuRoot(pgm)) {
      bHasBack = FALSE;
    }
  }

  if (!bHasBack) {
    m_mgBack.Disappear();
    return;
  }

  if (bResume) {
    m_mgBack.SetText(TRANS("RESUME"));
    m_mgBack.mg_strTip = TRANS("return to game");

  } else if (_bVarChanged) {
    m_mgBack.SetText(TRANS("CANCEL"));
    m_mgBack.mg_strTip = TRANS("cancel changes");

  } else {
    m_mgBack.SetText(TRANS("BACK"));
    m_mgBack.mg_strTip = TRANS("return to previous menu");
  }

  m_mgBack.mg_iCenterI = -1;
  m_mgBack.mg_bfsFontSize = BFS_LARGE;
  m_mgBack.mg_boxOnScreen = BoxBack();
  m_mgBack.mg_boxOnScreen = BoxLeftColumn(16.5f);
  pgm->AddChild(&m_mgBack);

  m_mgBack.mg_pmgLeft = m_mgBack.mg_pmgRight
  = m_mgBack.mg_pmgUp = m_mgBack.mg_pmgDown = pgm->GetDefaultGadget();

  m_mgBack.mg_pCallbackFunction = &MenuGoToParent;

  m_mgBack.Appear();
};

void CMenuManager::ChangeToMenu(CGameMenu *pgmNewMenu) {
  // auto-clear old thumbnail when going out of menu
  ClearThumbnail();

  // [Cecil] Reset gadget under the cursor
  m_pmgUnderCursor = NULL;

  // [Cecil] If no new menu specified
  if (pgmNewMenu == NULL) {
    CGameMenu *pgmCurrent = GetCurrentMenu();

    // Just exit if there are no more menus (happens intentionally sometimes)
    if (pgmCurrent == NULL) return;

    // End the current menu and pop it
    pgmCurrent->EndMenu();
    PopMenu();

    // Make the previous menu the current one
    pgmNewMenu = GetCurrentMenu();

    // If no more menus
    if (pgmNewMenu == NULL) {
      // Return to the game
      if (_gmRunningGameMode != GM_NONE) {
        StopMenus();

      // Or open the main menu
      } else {
        CMainMenu::ChangeTo();
      }

      return;
    }

  // Otherwise if some new menu is specified
  } else {
    INDEX iVisited = GetMenuCount();

    // End the current menu, if there is one
    if (iVisited > 0) {
      GetCurrentMenu()->EndMenu();
    }

    // [Cecil] Check whether this menu has already been visited
    while (--iVisited >= 0) {
      CGameMenu *pgm = GetMenu(iVisited);

      // Found the same menu
      if (strcmp(pgm->GetName(), pgmNewMenu->GetName()) == 0) {
        break;
      }
    }

    // If this menu has already been visited before
    if (iVisited >= 0) {
      // Then rewind to the visited one and pop it too
      PopMenusUntil(iVisited - 1);
    }

    // Push the new menu to the top
    PushMenu(pgmNewMenu);
  }

  // Start the new menu
  pgmNewMenu->StartMenu();

  // Add the back button
  FixupBackButton(pgmNewMenu);
};
