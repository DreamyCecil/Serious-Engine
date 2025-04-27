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

#include "MenuActions.h"
#include "MenuStuff.h"
#include "MenuStarters.h"

// macros for translating radio button text arrays
#define TRANSLATERADIOARRAY(array) TranslateRadioTexts(array, ARRAYCOUNT(array))

extern BOOL bMenuActive;
extern BOOL bMenuRendering;
extern CTextureObject *_ptoLogoCT;
extern CTextureObject *_ptoLogoODI;
extern CTextureObject *_ptoLogoEAX;

INDEX _iLocalPlayer = -1;
BOOL  _bPlayerMenuFromSinglePlayer = FALSE;

GameMode _gmMenuGameMode = GM_NONE;
GameMode _gmRunningGameMode = GM_NONE;
CListHead _lhServers;

void OnPlayerSelect(void);

// last tick done
static TICK _tckMenuLastTickDone = -1;
// all possible menu entities
CListHead lhMenuEntities;

CTString _strLastPlayerAppearance = "";
extern CTString sam_strNetworkSettings;

// function to activate when level is chosen
void (*_pAfterLevelChosen)(void);

// functions for init actions

void FixupBackButton(CGameMenu *pgm);

// mouse cursor position
FLOAT _fCursorPosI = 0;
FLOAT _fCursorPosJ = 0;
FLOAT _fCursorExternPosI = 0;
FLOAT _fCursorExternPosJ = 0;
BOOL _bMouseUsedLast = FALSE;
CMenuGadget *_pmgUnderCursor =  NULL;
extern BOOL _bDefiningKey;
extern BOOL _bEditingString;

// thumbnail for showing in menu
CTextureObject _toThumbnail;
BOOL _bThumbnailOn = FALSE;

CFontData _fdBig;
CFontData _fdMedium;
CFontData _fdSmall;
CFontData _fdTitle;

CSoundData *_psdSelect = NULL;
CSoundData *_psdPress = NULL;
CSoundObject *_psoMenuSound = NULL;

static CTextureObject _toLogoMenuA;
static CTextureObject _toLogoMenuB;

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

// -------------- All possible menu entities
#define BIG_BUTTONS_CT 6

#define CHANGETRIGGERARRAY(ltbmg, astr) \
  ltbmg.mg_astrTexts = astr;\
  ltbmg.mg_ctTexts = sizeof( astr)/sizeof( astr[0]);\
  ltbmg.mg_iSelected = 0;\
  ltbmg.SetText(astr[ltbmg.mg_iSelected];

#define PLACEMENT(x,y,z) CPlacement3D( FLOAT3D( x, y, z), \
  ANGLE3D( AngleDeg(0.0f), AngleDeg(0.0f), AngleDeg(0.0f)))

// ptr to current menu
CGameMenu *pgmCurrentMenu = NULL;

// global back button
CMGButton mgBack;

// -------- console variable adjustment menu
BOOL _bVarChanged = FALSE;

extern void PlayMenuSound(CSoundData *psd)
{
  if (_psoMenuSound!=NULL && !_psoMenuSound->IsPlaying()) {
    _psoMenuSound->Play(psd, SOF_NONGAME);
  }
}

// translate all texts in array for one radio button
void TranslateRadioTexts(CTString astr[], INDEX ct)
{
  for (INDEX i=0; i<ct; i++) {
    astr[i] = TranslateConst(astr[i], 4);
  }
}

// set new thumbnail
void SetThumbnail(CTFileName fn)
{
  _bThumbnailOn = TRUE;
  try {
    _toThumbnail.SetData_t(fn.NoExt()+"Tbn.tex");
  } catch(char *strError) {
    (void)strError;
    try {
      _toThumbnail.SetData_t(fn.NoExt()+".tbn");
    } catch(char *strError) {
      (void)strError;
      _toThumbnail.SetData(NULL);
    }
  }
}

// remove thumbnail
void ClearThumbnail(void)
{
  _bThumbnailOn = FALSE;
  _toThumbnail.SetData(NULL);
  _pShell->Execute( "FreeUnusedStock();");
}

void StartMenus(const CTString &str)
{
  _tckMenuLastTickDone = _pTimer->GetRealTime();
  // disable printing of last lines
  CON_DiscardLastLineTimes();

  // stop all IFeel effects
  IFeel_StopEffect(NULL);
  if (pgmCurrentMenu == &_pGUIM->gmMainMenu || pgmCurrentMenu == &_pGUIM->gmInGameMenu) {
    if (_gmRunningGameMode==GM_NONE) {
    pgmCurrentMenu = &_pGUIM->gmMainMenu;
    } else {
    pgmCurrentMenu = &_pGUIM->gmInGameMenu;
    }
  }

  // start main menu, or last active one
  if (pgmCurrentMenu!=NULL) {
    ChangeToMenu(pgmCurrentMenu);
  } else {
    if (_gmRunningGameMode==GM_NONE) {
      CMainMenu::ChangeTo();
    } else {
      CInGameMenu::ChangeTo();
    }
  }

  if (str == "load") {
    StartCurrentLoadMenu();
    _pGUIM->gmLoadSaveMenu.gm_pgmParentMenu = NULL;
  }

  if (str == "save") {
    StartCurrentSaveMenu();
    _pGUIM->gmLoadSaveMenu.gm_pgmParentMenu = NULL;
    FixupBackButton(&_pGUIM->gmLoadSaveMenu);
  }

  if (str == "controls") {
    void StartControlsMenuFromOptions(void);
    StartControlsMenuFromOptions();
    _pGUIM->gmControls.gm_pgmParentMenu = NULL;
    FixupBackButton(&_pGUIM->gmControls);
  }

  if (str == "join") {
    void StartSelectPlayersMenuFromOpen(void);
    StartSelectPlayersMenuFromOpen();
    _pGUIM->gmSelectPlayersMenu.gm_pgmParentMenu = &_pGUIM->gmMainMenu;
    FixupBackButton(&_pGUIM->gmSelectPlayersMenu);
  }

  if (str == "hiscore") {
    CHighScoreMenu::ChangeTo();
    _pGUIM->gmHighScoreMenu.gm_pgmParentMenu = &_pGUIM->gmMainMenu;
    FixupBackButton(&_pGUIM->gmHighScoreMenu);
  }

  bMenuActive = TRUE;
  bMenuRendering = TRUE;
}


void StopMenus( BOOL bGoToRoot /*=TRUE*/)
{
  ClearThumbnail();
  if (pgmCurrentMenu!=NULL && bMenuActive) {
    pgmCurrentMenu->EndMenu();
  }
  bMenuActive = FALSE;
  if (bGoToRoot) {
    if (_gmRunningGameMode==GM_NONE) {
    pgmCurrentMenu = &_pGUIM->gmMainMenu;
    } else {
    pgmCurrentMenu = &_pGUIM->gmInGameMenu;
    }
  }
}

BOOL IsMenusInRoot(void)
{
  return pgmCurrentMenu == NULL || pgmCurrentMenu == &_pGUIM->gmMainMenu || pgmCurrentMenu == &_pGUIM->gmInGameMenu;
}

// ------------------------ Global menu function implementation
void InitializeMenus(void)
{
  _pGUIM = new CMenuManager();

  try {
    // initialize and load corresponding fonts
    _fdSmall.Load_t(  CTFILENAME( "Fonts\\Display3-narrow.fnt"));
    _fdMedium.Load_t( CTFILENAME( "Fonts\\Display3-normal.fnt"));
    _fdBig.Load_t(    CTFILENAME( "Fonts\\Display3-caps.fnt"));
    _fdTitle.Load_t(  CTFILENAME( "Fonts\\Title2.fnt"));
    _fdSmall.SetCharSpacing(-1);
    _fdSmall.SetLineSpacing( 0);
    _fdSmall.SetSpaceWidth(0.4f);
    _fdMedium.SetCharSpacing(+1);
    _fdMedium.SetLineSpacing( 0);
    _fdMedium.SetSpaceWidth(0.4f);
    _fdBig.SetCharSpacing(+1);
    _fdBig.SetLineSpacing( 0);
    _fdTitle.SetCharSpacing(+1);
    _fdTitle.SetLineSpacing( 0);

    // load menu sounds
    _psdSelect = _pSoundStock->Obtain_t( CTFILENAME("Sounds\\Menu\\Select.wav"));
    _psdPress  = _pSoundStock->Obtain_t( CTFILENAME("Sounds\\Menu\\Press.wav"));
    _psoMenuSound = new CSoundObject;

    // initialize and load menu textures
    _toLogoMenuA.SetData_t(  CTFILENAME( "Textures\\Logo\\sam_menulogo256a.tex"));
    _toLogoMenuB.SetData_t(  CTFILENAME( "Textures\\Logo\\sam_menulogo256b.tex"));
  }
  catch( char *strError) {
    FatalError( strError);
  }
  // force logo textures to be of maximal size
  ((CTextureData*)_toLogoMenuA.GetData())->Force(TEX_CONSTANT);
  ((CTextureData*)_toLogoMenuB.GetData())->Force(TEX_CONSTANT);

  // menu's relative placement
  CPlacement3D plRelative = CPlacement3D( FLOAT3D( 0.0f, 0.0f, -9.0f), 
                            ANGLE3D( AngleDeg(0.0f), AngleDeg(0.0f), AngleDeg(0.0f)));
  try
  {
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

    // ------------------- Initialize menus
  _pGUIM->gmConfirmMenu.Initialize_t();
  _pGUIM->gmConfirmMenu.gm_pgmParentMenu = NULL;
  InitActionsForConfirmMenu();

  _pGUIM->gmMainMenu.Initialize_t();
  _pGUIM->gmMainMenu.gm_pgmParentMenu = NULL;
  InitActionsForMainMenu();

  _pGUIM->gmInGameMenu.Initialize_t();
  _pGUIM->gmInGameMenu.gm_pgmParentMenu = NULL;
  InitActionsForInGameMenu();

  _pGUIM->gmSinglePlayerMenu.Initialize_t();
  _pGUIM->gmSinglePlayerMenu.gm_pgmParentMenu = &_pGUIM->gmMainMenu;
  InitActionsForSinglePlayerMenu();

  _pGUIM->gmSinglePlayerNewMenu.Initialize_t();
  _pGUIM->gmSinglePlayerNewMenu.gm_pgmParentMenu = &_pGUIM->gmSinglePlayerMenu;
  InitActionsForSinglePlayerNewMenu();

  _pGUIM->gmPlayerProfile.Initialize_t();
  InitActionsForPlayerProfileMenu();

  _pGUIM->gmControls.Initialize_t();
  InitActionsForControlsMenu();

    // warning! parent menu has to be set inside button activate function from where
    // Load/Save menu is called
  _pGUIM->gmLoadSaveMenu.Initialize_t();

  _pGUIM->gmHighScoreMenu.Initialize_t();

  _pGUIM->gmCustomizeKeyboardMenu.Initialize_t();
  _pGUIM->gmCustomizeKeyboardMenu.gm_pgmParentMenu = &_pGUIM->gmControls;

  _pGUIM->gmCustomizeAxisMenu.Initialize_t();
  _pGUIM->gmCustomizeAxisMenu.gm_pgmParentMenu = &_pGUIM->gmControls;
  InitActionsForCustomizeAxisMenu();

  _pGUIM->gmOptionsMenu.Initialize_t();
  _pGUIM->gmOptionsMenu.gm_pgmParentMenu = &_pGUIM->gmMainMenu;
  InitActionsForOptionsMenu();

  _pGUIM->gmVideoOptionsMenu.Initialize_t();
  _pGUIM->gmVideoOptionsMenu.gm_pgmParentMenu = &_pGUIM->gmOptionsMenu;
  InitActionsForVideoOptionsMenu();

  _pGUIM->gmAudioOptionsMenu.Initialize_t();
  _pGUIM->gmAudioOptionsMenu.gm_pgmParentMenu = &_pGUIM->gmOptionsMenu;
  InitActionsForAudioOptionsMenu();

  _pGUIM->gmLevelsMenu.Initialize_t();
  _pGUIM->gmLevelsMenu.gm_pgmParentMenu = &_pGUIM->gmSinglePlayerMenu;

  _pGUIM->gmVarMenu.Initialize_t();
  _pGUIM->gmVarMenu.gm_pgmParentMenu = &_pGUIM->gmNetworkStartMenu;
  InitActionsForVarMenu();

  _pGUIM->gmServersMenu.Initialize_t();
  _pGUIM->gmServersMenu.gm_pgmParentMenu = &_pGUIM->gmNetworkOpenMenu;
  InitActionsForServersMenu();

  _pGUIM->gmNetworkMenu.Initialize_t();
  _pGUIM->gmNetworkMenu.gm_pgmParentMenu = &_pGUIM->gmMainMenu;
  InitActionsForNetworkMenu();

  _pGUIM->gmNetworkStartMenu.Initialize_t();
  _pGUIM->gmNetworkStartMenu.gm_pgmParentMenu = &_pGUIM->gmNetworkMenu;
  InitActionsForNetworkStartMenu();

  _pGUIM->gmNetworkJoinMenu.Initialize_t();
  _pGUIM->gmNetworkJoinMenu.gm_pgmParentMenu = &_pGUIM->gmNetworkMenu;
  InitActionsForNetworkJoinMenu();

  _pGUIM->gmSelectPlayersMenu.gm_bAllowDedicated = FALSE;
  _pGUIM->gmSelectPlayersMenu.gm_bAllowObserving = FALSE;
  _pGUIM->gmSelectPlayersMenu.Initialize_t();
  InitActionsForSelectPlayersMenu();

  _pGUIM->gmNetworkOpenMenu.Initialize_t();
  _pGUIM->gmNetworkOpenMenu.gm_pgmParentMenu = &_pGUIM->gmNetworkJoinMenu;
  InitActionsForNetworkOpenMenu();

  _pGUIM->gmSplitScreenMenu.Initialize_t();
  _pGUIM->gmSplitScreenMenu.gm_pgmParentMenu = &_pGUIM->gmMainMenu;
  InitActionsForSplitScreenMenu();

  _pGUIM->gmSplitStartMenu.Initialize_t();
  _pGUIM->gmSplitStartMenu.gm_pgmParentMenu = &_pGUIM->gmSplitScreenMenu;
  InitActionsForSplitStartMenu();
  }
  catch( char *strError)
  {
    FatalError( strError);
  }

#if SE1_PREFER_SDL
  // [Cecil] SDL: Set custom cursor
  SetGameCursor();
#endif
}

void DestroyMenus( void)
{
#if SE1_PREFER_SDL
  // [Cecil] SDL: Clear custom cursor
  DestroyGameCursor();
#endif

  _pGUIM->gmMainMenu.Destroy();
  pgmCurrentMenu = NULL;
  _pSoundStock->Release(_psdSelect);
  _pSoundStock->Release(_psdPress);
  delete _psoMenuSound;
  _psdSelect = NULL;
  _psdPress = NULL;
  _psoMenuSound = NULL;
}

// go to parent menu if possible
void MenuGoToParent(void)
{
  // if there is no parent menu
  if( pgmCurrentMenu->gm_pgmParentMenu == NULL) {
    // if in game
    if (_gmRunningGameMode!=GM_NONE) {
      // exit menus
      StopMenus();
    // if no game is running
    } else {
      // go to main menu
      CMainMenu::ChangeTo();
    }
  // if there is some parent menu
  } else {
    // go to parent menu
    ChangeToMenu( pgmCurrentMenu->gm_pgmParentMenu);
  }
}

void MenuOnKeyDown(PressedMenuButton pmb)
{
  // [Cecil] Check if mouse buttons are used separately
  _bMouseUsedLast = (pmb.iMouse != -1);

  // ignore mouse when editing
  if (_bEditingString && _bMouseUsedLast) {
    _bMouseUsedLast = FALSE;
    return;
  }

  // initially the message is not handled
  BOOL bHandled = FALSE;

  // if not a mouse button, or mouse is over some gadget
  if (!_bMouseUsedLast || _pmgUnderCursor!=NULL) {
    // ask current menu to handle the key
    bHandled = pgmCurrentMenu->OnKeyDown(pmb);
  }

  // if not handled
  if(!bHandled) {
    if (pmb.Back(TRUE)) {
      if (pgmCurrentMenu==&_pGUIM->gmLoadSaveMenu && _pGUIM->gmLoadSaveMenu.gm_bNoEscape) {
        return;
      }
      // go to parent menu if possible
      MenuGoToParent();
    }
  }
}

void MenuOnChar(const OS::SE1Event &event)
{
  // check if mouse buttons used
  _bMouseUsedLast = FALSE;

  // ask current menu to handle the key
  pgmCurrentMenu->OnChar(event);
}

void MenuOnMouseMove(PIX pixI, PIX pixJ)
{
  static PIX pixLastI = 0;
  static PIX pixLastJ = 0;
  if (pixLastI==pixI && pixLastJ==pixJ) {
    return;
  }
  pixLastI = pixI;
  pixLastJ = pixJ;
  _bMouseUsedLast = !_bEditingString && !_bDefiningKey && !_pInput->IsInputEnabled();
}

void MenuUpdateMouseFocus(void)
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
  _fCursorPosI += fMouseX - _fCursorExternPosI;
  _fCursorPosJ  = _fCursorExternPosJ;
  _fCursorExternPosI = fMouseX;
  _fCursorExternPosJ = fMouseY;

  // if mouse not used last
  if (!_bMouseUsedLast||_bDefiningKey||_bEditingString) {
    // do nothing
    return;
  }

  CMenuGadget *pmgActive = NULL;
  // for all gadgets in menu
  FOREACHINLIST( CMenuGadget, mg_lnNode, pgmCurrentMenu->gm_lhGadgets, itmg) {
    CMenuGadget &mg = *itmg;
    // if focused
    if( itmg->mg_bFocused) {
      // remember it
      pmgActive = &itmg.Current();
    }
  }

  // if there is some under cursor
  if (_pmgUnderCursor!=NULL) {
    _pmgUnderCursor->OnMouseOver(_fCursorPosI, _fCursorPosJ);
    // if the one under cursor has no neighbours
    if (_pmgUnderCursor->mg_pmgLeft ==NULL 
      &&_pmgUnderCursor->mg_pmgRight==NULL 
      &&_pmgUnderCursor->mg_pmgUp   ==NULL 
      &&_pmgUnderCursor->mg_pmgDown ==NULL) {
      // it cannot be focused
      _pmgUnderCursor = NULL;
      return;
    }

    // if the one under cursor is not active and not disappearing
    if (pmgActive!=_pmgUnderCursor && _pmgUnderCursor->mg_bVisible) {
      // change focus
      if (pmgActive!=NULL) {
        pmgActive->OnKillFocus();
      }
      _pmgUnderCursor->OnSetFocus();
    }
  }
}

static CTimerValue _tvInitialization;
static TICK _tckInitializationTick = -1;

void SetMenuLerping(void)
{
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


// render mouse cursor if needed
void RenderMouseCursor(CDrawPort *pdp)
{
  // if mouse not used last
  if (!_bMouseUsedLast|| _bDefiningKey || _bEditingString) {
    // don't render cursor
    return;
  }
  _pGame->LCDSetDrawport(pdp);

  // [Cecil] Render the in-game cursor the old-fashioned way under Win32
#if !SE1_PREFER_SDL
  _pGame->LCDDrawPointer(_fCursorPosI, _fCursorPosJ);
#endif
}


BOOL DoMenu( CDrawPort *pdp)
{
  pdp->Unlock();
  CDrawPort dpMenu(pdp, TRUE);
  dpMenu.Lock();

  MenuUpdateMouseFocus();

  // if in fullscreen
  CDisplayMode dmCurrent;
  _pGfx->GetCurrentDisplayMode(dmCurrent);
  if (dmCurrent.IsFullScreen()) {
    // clamp mouse pointer
    _fCursorPosI = Clamp(_fCursorPosI, 0L, dpMenu.GetWidth());
    _fCursorPosJ = Clamp(_fCursorPosJ, 0L, dpMenu.GetHeight());
  // if in window
  } else {
    // use same mouse pointer as windows
    _fCursorPosI = _fCursorExternPosI;
    _fCursorPosJ = _fCursorExternPosJ;
  }

  pgmCurrentMenu->Think();

  const TICK tckTickNow = _pTimer->GetRealTime();

  while (_tckMenuLastTickDone < tckTickNow)
  {
    _pTimer->SetGameTick(_tckMenuLastTickDone);
    // call think for all gadgets in menu
    FOREACHINLIST( CMenuGadget, mg_lnNode, pgmCurrentMenu->gm_lhGadgets, itmg) {
      itmg->Think();
    }
    _tckMenuLastTickDone++;
  }

  SetMenuLerping();

  PIX pixW = dpMenu.GetWidth();
  PIX pixH = dpMenu.GetHeight();

  // blend background if menu is on
  if( bMenuActive)
  {
    // get current time
    TIME  tmNow = _pTimer->GetLerpedCurrentTick();
    UBYTE ubH1  = (INDEX)(tmNow*08.7f) & 255;
    UBYTE ubH2  = (INDEX)(tmNow*27.6f) & 255;
    UBYTE ubH3  = (INDEX)(tmNow*16.5f) & 255;
    UBYTE ubH4  = (INDEX)(tmNow*35.4f) & 255;

    // clear screen with background texture
    _pGame->LCDPrepare(1.0f);
    _pGame->LCDSetDrawport(&dpMenu);
    // do not allow game to show through
    dpMenu.Fill(C_BLACK|255);
    _pGame->LCDRenderClouds1();
    _pGame->LCDRenderGrid();
    _pGame->LCDRenderClouds2();

    FLOAT fScaleW = (FLOAT)pixW / 640.0f;
    FLOAT fScaleH = (FLOAT)pixH / 480.0f;
    PIX   pixI0, pixJ0, pixI1, pixJ1;
    // put logo(s) to main menu (if logos exist)
  if (pgmCurrentMenu == &_pGUIM->gmMainMenu)
    {
      if( _ptoLogoODI!=NULL) {
        CTextureData &td = (CTextureData&)*_ptoLogoODI->GetData();
        #define LOGOSIZE 50
        const PIX pixLogoWidth  = LOGOSIZE * dpMenu.dp_fWideAdjustment;
        const PIX pixLogoHeight = LOGOSIZE* td.GetHeight() / td.GetWidth();
        pixI0 = (640-pixLogoWidth -16)*fScaleW;
        pixJ0 = (480-pixLogoHeight-16)*fScaleH;
        pixI1 = pixI0+ pixLogoWidth *fScaleW;
        pixJ1 = pixJ0+ pixLogoHeight*fScaleH;
        dpMenu.PutTexture( _ptoLogoODI, PIXaabbox2D( PIX2D( pixI0, pixJ0),PIX2D( pixI1, pixJ1)));
        #undef LOGOSIZE
      }  
      if( _ptoLogoCT!=NULL) {
        CTextureData &td = (CTextureData&)*_ptoLogoCT->GetData();
        #define LOGOSIZE 50
        const PIX pixLogoWidth  = LOGOSIZE * dpMenu.dp_fWideAdjustment;
        const PIX pixLogoHeight = LOGOSIZE* td.GetHeight() / td.GetWidth();
        pixI0 = 12*fScaleW;
        pixJ0 = (480-pixLogoHeight-16)*fScaleH;
        pixI1 = pixI0+ pixLogoWidth *fScaleW;
        pixJ1 = pixJ0+ pixLogoHeight*fScaleH;
        dpMenu.PutTexture( _ptoLogoCT, PIXaabbox2D( PIX2D( pixI0, pixJ0),PIX2D( pixI1, pixJ1)));
        #undef LOGOSIZE
      } 
      
      {
        FLOAT fResize = Min(dpMenu.GetWidth()/640.0f, dpMenu.GetHeight()/480.0f);
        PIX pixSizeI = 256*fResize;
        PIX pixSizeJ = 64*fResize;
        PIX pixCenterI = dpMenu.GetWidth()/2;
        PIX pixHeightJ = 10*fResize;
        dpMenu.PutTexture(&_toLogoMenuA, PIXaabbox2D( 
          PIX2D( pixCenterI-pixSizeI, pixHeightJ),PIX2D( pixCenterI, pixHeightJ+pixSizeJ)));
        dpMenu.PutTexture(&_toLogoMenuB, PIXaabbox2D( 
          PIX2D( pixCenterI, pixHeightJ),PIX2D( pixCenterI+pixSizeI, pixHeightJ+pixSizeJ)));
      }

      // [Cecil] Display build version
      {
        dpMenu.SetFont(_pfdConsoleFont);
        dpMenu.SetTextScaling(1.0f);
        dpMenu.SetTextCharSpacing(-1);

        const PIX pixBuildX = dpMenu.GetWidth() - 16;
        const PIX pixBuildY = dpMenu.GetHeight() - _pfdConsoleFont->fd_pixCharHeight;
        dpMenu.PutTextR(SE_GetBuildVersion(), pixBuildX, pixBuildY, 0xFFFFFFFF);
      }

  } else if (pgmCurrentMenu == &_pGUIM->gmAudioOptionsMenu) {
      if( _ptoLogoEAX!=NULL) {
        CTextureData &td = (CTextureData&)*_ptoLogoEAX->GetData();
        const INDEX iSize = 95;
        const PIX pixLogoWidth  = iSize * dpMenu.dp_fWideAdjustment;
        const PIX pixLogoHeight = iSize * td.GetHeight() / td.GetWidth();
        pixI0 =  (640-pixLogoWidth - 35)*fScaleW;
        pixJ0 = (480-pixLogoHeight - 7)*fScaleH;
        pixI1 = pixI0+ pixLogoWidth *fScaleW;
        pixJ1 = pixJ0+ pixLogoHeight*fScaleH;
        dpMenu.PutTexture( _ptoLogoEAX, PIXaabbox2D( PIX2D( pixI0, pixJ0),PIX2D( pixI1, pixJ1)));
      }
    }

#define THUMBW 96
#define THUMBH 96
    // if there is a thumbnail
    if( _bThumbnailOn) {
      const FLOAT fThumbScaleW = fScaleW * dpMenu.dp_fWideAdjustment;
      PIX pixOfs = 8*fScaleW;
      pixI0 = 8*fScaleW;
      pixJ0 = (240-THUMBW/2)*fScaleH;
      pixI1 = pixI0+ THUMBW*fThumbScaleW;
      pixJ1 = pixJ0+ THUMBH*fScaleH;
      if( _toThumbnail.GetData()!=NULL)
      { // show thumbnail with shadow and border
        dpMenu.Fill( pixI0+pixOfs, pixJ0+pixOfs, THUMBW*fThumbScaleW, THUMBH*fScaleH, C_BLACK|128);
        dpMenu.PutTexture( &_toThumbnail, PIXaabbox2D( PIX2D( pixI0, pixJ0), PIX2D( pixI1, pixJ1)), C_WHITE|255);
        dpMenu.DrawBorder( pixI0,pixJ0, THUMBW*fThumbScaleW,THUMBH*fScaleH, _pGame->LCDGetColor(C_mdGREEN|255, "thumbnail border"));
      } else {
        dpMenu.SetFont( _pfdDisplayFont);
        dpMenu.SetTextScaling( fScaleW);
        dpMenu.SetTextAspect( 1.0f);
        dpMenu.PutTextCXY( TRANS("no thumbnail"), (pixI0+pixI1)/2, (pixJ0+pixJ1)/2, _pGame->LCDGetColor(C_GREEN|255, "no thumbnail"));
      }
    }

    // assure we can listen to non-3d sounds
    _pSound->UpdateSounds();
  }

  // if this is popup menu
  if (pgmCurrentMenu->gm_bPopup) {

    // render parent menu first
    if (pgmCurrentMenu->gm_pgmParentMenu!=NULL) {
      _pGame->MenuPreRenderMenu(pgmCurrentMenu->gm_pgmParentMenu->GetName());
      FOREACHINLIST( CMenuGadget, mg_lnNode, pgmCurrentMenu->gm_pgmParentMenu->gm_lhGadgets, itmg) {
        if( itmg->mg_bVisible) {
          itmg->Render( &dpMenu);
        }
      }
      _pGame->MenuPostRenderMenu(pgmCurrentMenu->gm_pgmParentMenu->GetName());
    }

    // gray it out
    dpMenu.Fill(C_BLACK|128);

    // clear popup box
    dpMenu.Unlock();
    PIXaabbox2D box = FloatBoxToPixBox(&dpMenu, BoxPopup());
    CDrawPort dpPopup(pdp, box);
    dpPopup.Lock();
    _pGame->LCDSetDrawport(&dpPopup);
    dpPopup.Fill(C_BLACK|255);
    _pGame->LCDRenderClouds1();
    _pGame->LCDRenderGrid();
  //_pGame->LCDRenderClouds2();
    _pGame->LCDScreenBox(_pGame->LCDGetColor(C_GREEN|255, "popup box"));
    dpPopup.Unlock();
    dpMenu.Lock();
  }

  // no entity is under cursor initially
  _pmgUnderCursor = NULL;

  BOOL bStilInMenus = FALSE;
  _pGame->MenuPreRenderMenu(pgmCurrentMenu->GetName());
  // for each menu gadget
  FOREACHINLIST( CMenuGadget, mg_lnNode, pgmCurrentMenu->gm_lhGadgets, itmg) {
    // if gadget is visible
    if( itmg->mg_bVisible) {
      bStilInMenus = TRUE;
      itmg->Render( &dpMenu);
      if (FloatBoxToPixBox(&dpMenu, itmg->mg_boxOnScreen) >= PIX2D(_fCursorPosI, _fCursorPosJ)) {
        _pmgUnderCursor = itmg;
      }
    }
  }
  _pGame->MenuPostRenderMenu(pgmCurrentMenu->GetName());

  // no currently active gadget initially
  CMenuGadget *pmgActive = NULL;
  // if mouse was not active last
  if (!_bMouseUsedLast) {
    // find focused gadget
    FOREACHINLIST( CMenuGadget, mg_lnNode, pgmCurrentMenu->gm_lhGadgets, itmg) {
      CMenuGadget &mg = *itmg;
      // if focused
      if( itmg->mg_bFocused) {
        // it is active
        pmgActive = &itmg.Current();
        break;
      }
    }
  // if mouse was active last
  } else {
    // gadget under cursor is active
    pmgActive = _pmgUnderCursor;
  }

  // if editing
  if (_bEditingString && pmgActive!=NULL) {
    // dim the menu  bit
    dpMenu.Fill(C_BLACK|0x40);
    // render the edit gadget again
    pmgActive->Render(&dpMenu);
  }
  
  // if there is some active gadget and it has tips
  if (pmgActive!=NULL && (pmgActive->mg_strTip!="" || _bEditingString)) {
    CTString strTip = pmgActive->mg_strTip;
    if (_bEditingString) {
      strTip = TRANS("Enter - OK, Escape - Cancel");
    }
    // print the tip
    SetFontMedium(&dpMenu);
    dpMenu.PutTextC(strTip, 
      pixW*0.5f, pixH*0.92f, _pGame->LCDGetColor(C_WHITE|255, "tool tip"));
  }

  _pGame->ConsolePrintLastLines(&dpMenu);

  RenderMouseCursor(&dpMenu);

  dpMenu.Unlock();
  pdp->Lock();

  return bStilInMenus;
}

void MenuBack(void)
{
  MenuGoToParent();
}

extern void FixupBackButton(CGameMenu *pgm)
{
  BOOL bResume = FALSE;

  if (mgBack.mg_lnNode.IsLinked()) {
    mgBack.mg_lnNode.Remove();
  }

  BOOL bHasBack = TRUE;

  if (pgm->gm_bPopup) {
    bHasBack = FALSE;
  }

  if (pgm->gm_pgmParentMenu==NULL) {
    if (_gmRunningGameMode==GM_NONE) {
      bHasBack = FALSE;
    } else {
      bResume = TRUE;
    }
  }
  if (!bHasBack) {
    mgBack.Disappear();
    return;
  }

  if (bResume) {
    mgBack.SetText(TRANS("RESUME"));
    mgBack.mg_strTip = TRANS("return to game");
  } else {
    if (_bVarChanged) {
      mgBack.SetText(TRANS("CANCEL"));
      mgBack.mg_strTip = TRANS("cancel changes");
    } else {
      mgBack.SetText(TRANS("BACK"));
      mgBack.mg_strTip = TRANS("return to previous menu");
    }
  }

  mgBack.mg_iCenterI = -1;
  mgBack.mg_bfsFontSize = BFS_LARGE;
  mgBack.mg_boxOnScreen = BoxBack();
  mgBack.mg_boxOnScreen = BoxLeftColumn(16.5f);
  pgm->gm_lhGadgets.AddTail( mgBack.mg_lnNode);

  mgBack.mg_pmgLeft = 
  mgBack.mg_pmgRight = 
  mgBack.mg_pmgUp = 
  mgBack.mg_pmgDown = pgm->GetDefaultGadget();

  mgBack.mg_pActivatedFunction = &MenuBack;

  mgBack.Appear();
}

void ChangeToMenu( CGameMenu *pgmNewMenu)
{
  // auto-clear old thumbnail when going out of menu
  ClearThumbnail();

  if( pgmCurrentMenu != NULL) {
    if (!pgmNewMenu->gm_bPopup) {
      pgmCurrentMenu->EndMenu();
    } else {
      FOREACHINLIST(CMenuGadget, mg_lnNode, pgmCurrentMenu->gm_lhGadgets, itmg) {
        itmg->OnKillFocus();
      }
    }
  }
  pgmNewMenu->StartMenu();

  CMenuGadget *pmgDefault = pgmNewMenu->GetDefaultGadget();

  if (pmgDefault) {
    if (mgBack.mg_bFocused) {
      mgBack.OnKillFocus();
    }
    pmgDefault->OnSetFocus();
  }
  FixupBackButton(pgmNewMenu);
  pgmCurrentMenu = pgmNewMenu;
}