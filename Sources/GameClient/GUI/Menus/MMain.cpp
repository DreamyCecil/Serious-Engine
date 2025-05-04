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
#include "MMain.h"

CTFileName _fnmModSelected;
extern CTFileName _fnmModToLoad;

BOOL LSLoadDemo(CGameMenu *pgm, const CTString &fnm) {
  _pGame->gm_StartSplitScreenCfg = CGame::SSC_OBSERVER;
  _pGame->gm_strNetworkProvider = "Local";

  // play the demo
  if (_pGame->StartDemoPlay(fnm)) {
    // exit menu and pull up the console
    _pGUIM->StopMenus();

    if (_pGame->gm_csConsoleState != CS_OFF) {
      _pGame->gm_csConsoleState = CS_TURNINGOFF;
    }
    _gmRunningGameMode = GM_DEMO;

  } else {
    _gmRunningGameMode = GM_NONE;
  }

  return TRUE;
};

static void StartDemoLoadMenu(void) {
  _gmMenuGameMode = GM_DEMO;

  CLoadSaveMenu &gmCurrent = _pGUIM->gmLoadSaveMenu;
  gmCurrent.gm_mgTitle.SetText(TRANS("PLAY DEMO"));
  gmCurrent.gm_bAllowThumbnails = TRUE;
  gmCurrent.gm_iSortType = LSSORT_FILEDN;
  gmCurrent.gm_bSave = FALSE;
  gmCurrent.gm_bManage = TRUE;
  gmCurrent.gm_fnmDirectory = CTString("Demos\\");
  gmCurrent.gm_fnmSelected = CTString("");
  gmCurrent.gm_fnmExt = CTString(".dem");
  gmCurrent.gm_pAfterFileChosen = &LSLoadDemo;
  gmCurrent.gm_mgNotes.SetText("");
  CLoadSaveMenu::ChangeTo();
};

static void ModLoadYes(void) {
  _fnmModToLoad = _fnmModSelected;
};

static BOOL LSLoadMod(CGameMenu *pgm, const CTString &fnm) {
  _fnmModSelected = fnm;
  CConfirmMenu::ChangeTo(TRANS("LOAD THIS MOD?"), &ModLoadYes, NULL, TRUE);
  return TRUE;
};

static void StartModsLoadMenu(void) {
  CLoadSaveMenu &gmCurrent = _pGUIM->gmLoadSaveMenu;
  gmCurrent.gm_mgTitle.SetText(TRANS("CHOOSE MOD"));
  gmCurrent.gm_bAllowThumbnails = TRUE;
  gmCurrent.gm_iSortType = LSSORT_NAMEUP;
  gmCurrent.gm_bSave = FALSE;
  gmCurrent.gm_bManage = FALSE;
  gmCurrent.gm_fnmDirectory = SE1_MODS_SUBDIR;
  gmCurrent.gm_fnmSelected = CTString("");
  gmCurrent.gm_fnmExt = CTString(".des");
  gmCurrent.gm_pAfterFileChosen = &LSLoadMod;
  CLoadSaveMenu::ChangeTo();
};

static void ExitGame(void) {
  _bRunning = FALSE;
  _bQuitScreen = TRUE;
};

void ExitConfirm(void) {
  CConfirmMenu::ChangeTo(TRANS("ARE YOU SERIOUS?"), &ExitGame, NULL, TRUE);
};

void CMainMenu::Initialize_t(void)
{
  extern CTString sam_strVersion;
  gm_mgVersionLabel.SetText(sam_strVersion);
  gm_mgVersionLabel.mg_boxOnScreen = BoxVersion();
  gm_mgVersionLabel.mg_bfsFontSize = BFS_MEDIUM;
  gm_mgVersionLabel.mg_iCenterI = +1;
  gm_mgVersionLabel.mg_bEnabled = FALSE;
  gm_mgVersionLabel.mg_bLabel = TRUE;
  AddChild(&gm_mgVersionLabel);

  extern CTString sam_strModName;
  gm_mgModLabel.SetText(sam_strModName);
  gm_mgModLabel.mg_boxOnScreen = BoxMediumRow(-2.0f);
  gm_mgModLabel.mg_bfsFontSize = BFS_MEDIUM;
  gm_mgModLabel.mg_iCenterI = 0;
  gm_mgModLabel.mg_bEnabled = FALSE;
  gm_mgModLabel.mg_bLabel = TRUE;
  AddChild(&gm_mgModLabel);

  gm_mgSingle.SetText(TRANS("SINGLE PLAYER"));
  gm_mgSingle.mg_bfsFontSize = BFS_LARGE;
  gm_mgSingle.mg_boxOnScreen = BoxBigRow(0.0f);
  gm_mgSingle.mg_strTip = TRANS("single player game menus");
  AddChild(&gm_mgSingle);
  gm_mgSingle.mg_pmgUp = &gm_mgQuit;
  gm_mgSingle.mg_pmgDown = &gm_mgNetwork;
  gm_mgSingle.mg_pCallbackFunction = &CSinglePlayerMenu::ChangeTo;

  gm_mgNetwork.SetText(TRANS("NETWORK"));
  gm_mgNetwork.mg_bfsFontSize = BFS_LARGE;
  gm_mgNetwork.mg_boxOnScreen = BoxBigRow(1.0f);
  gm_mgNetwork.mg_strTip = TRANS("LAN/iNet multiplayer menus");
  AddChild(&gm_mgNetwork);
  gm_mgNetwork.mg_pmgUp = &gm_mgSingle;
  gm_mgNetwork.mg_pmgDown = &gm_mgSplitScreen;
  gm_mgNetwork.mg_pCallbackFunction = &CNetworkMenu::ChangeTo;

  gm_mgSplitScreen.SetText(TRANS("SPLIT SCREEN"));
  gm_mgSplitScreen.mg_bfsFontSize = BFS_LARGE;
  gm_mgSplitScreen.mg_boxOnScreen = BoxBigRow(2.0f);
  gm_mgSplitScreen.mg_strTip = TRANS("play with multiple players on one computer");
  AddChild(&gm_mgSplitScreen);
  gm_mgSplitScreen.mg_pmgUp = &gm_mgNetwork;
  gm_mgSplitScreen.mg_pmgDown = &gm_mgDemo;
  gm_mgSplitScreen.mg_pCallbackFunction = &CSplitScreenMenu::ChangeTo;

  gm_mgDemo.SetText(TRANS("DEMO"));
  gm_mgDemo.mg_bfsFontSize = BFS_LARGE;
  gm_mgDemo.mg_boxOnScreen = BoxBigRow(3.0f);
  gm_mgDemo.mg_strTip = TRANS("play a game demo");
  AddChild(&gm_mgDemo);
  gm_mgDemo.mg_pmgUp = &gm_mgSplitScreen;
  gm_mgDemo.mg_pmgDown = &gm_mgMods;
  gm_mgDemo.mg_pCallbackFunction = &StartDemoLoadMenu;

  gm_mgMods.SetText(TRANS("MODS"));
  gm_mgMods.mg_bfsFontSize = BFS_LARGE;
  gm_mgMods.mg_boxOnScreen = BoxBigRow(4.0f);
  gm_mgMods.mg_strTip = TRANS("run one of installed game modifications");
  AddChild(&gm_mgMods);
  gm_mgMods.mg_pmgUp = &gm_mgDemo;
  gm_mgMods.mg_pmgDown = &gm_mgHighScore;
  gm_mgMods.mg_pCallbackFunction = &StartModsLoadMenu;

  gm_mgHighScore.SetText(TRANS("HIGH SCORES"));
  gm_mgHighScore.mg_bfsFontSize = BFS_LARGE;
  gm_mgHighScore.mg_boxOnScreen = BoxBigRow(5.0f);
  gm_mgHighScore.mg_strTip = TRANS("view list of top ten best scores");
  AddChild(&gm_mgHighScore);
  gm_mgHighScore.mg_pmgUp = &gm_mgMods;
  gm_mgHighScore.mg_pmgDown = &gm_mgOptions;
  gm_mgHighScore.mg_pCallbackFunction = &CHighScoreMenu::ChangeTo;

  gm_mgOptions.SetText(TRANS("OPTIONS"));
  gm_mgOptions.mg_bfsFontSize = BFS_LARGE;
  gm_mgOptions.mg_boxOnScreen = BoxBigRow(6.0f);
  gm_mgOptions.mg_strTip = TRANS("adjust video, audio and input options");
  AddChild(&gm_mgOptions);
  gm_mgOptions.mg_pmgUp = &gm_mgHighScore;
  gm_mgOptions.mg_pmgDown = &gm_mgQuit;
  gm_mgOptions.mg_pCallbackFunction = &COptionsMenu::ChangeTo;

  gm_mgQuit.SetText(TRANS("QUIT"));
  gm_mgQuit.mg_bfsFontSize = BFS_LARGE;
  gm_mgQuit.mg_boxOnScreen = BoxBigRow(7.0f);
  gm_mgQuit.mg_strTip = TRANS("exit game immediately");
  AddChild(&gm_mgQuit);
  gm_mgQuit.mg_pmgUp = &gm_mgOptions;
  gm_mgQuit.mg_pmgDown = &gm_mgSingle;
  gm_mgQuit.mg_pCallbackFunction = &ExitConfirm;
}

void CMainMenu::StartMenu(void)
{
  gm_mgSingle.mg_bEnabled = IsMenuEnabled("Single Player");
  gm_mgNetwork.mg_bEnabled = IsMenuEnabled("Network");
  gm_mgSplitScreen.mg_bEnabled = IsMenuEnabled("Split Screen");
  gm_mgHighScore.mg_bEnabled = IsMenuEnabled("High Score");
  CGameMenu::StartMenu();
}

// [Cecil] Render menu background
void CMainMenu::RenderBackground(CDrawPort *pdp, bool bSubmenu) {
  // Skip background if the menu isn't active
  if (!_pGUIM->IsActive()) return;

  CGameMenu::RenderBackground(pdp, bSubmenu);

  const PIX pixW = pdp->GetWidth();
  const PIX pixH = pdp->GetHeight();
  const FLOAT fScaleW = (FLOAT)pixW / 640.0f;
  const FLOAT fScaleH = (FLOAT)pixH / 480.0f;

  PIX2D vStart, vSize;

  // Render extra logo
  if (_pGUIM->m_toLogoODI.GetData() != NULL) {
    CTextureData &td = (CTextureData &)*_pGUIM->m_toLogoODI.GetData();

    const FLOAT fLogoSize = 50;
    const PIX pixLogoW = fLogoSize * pdp->dp_fWideAdjustment;
    const PIX pixLogoH = fLogoSize * td.GetHeight() / td.GetWidth();

    vStart = PIX2D((640 - pixLogoW - 16) * fScaleW, (480 - pixLogoH - 16) * fScaleH);
    vSize = PIX2D(pixLogoW * fScaleW, pixLogoH * fScaleH);

    pdp->PutTexture(&_pGUIM->m_toLogoODI, PIXaabbox2D(vStart, vStart + vSize));
  }

  // Render developer logo
  if (_pGUIM->m_toLogoCT.GetData() != NULL) {
    CTextureData &td = (CTextureData &)*_pGUIM->m_toLogoCT.GetData();

    const FLOAT fLogoSize = 50;
    const PIX pixLogoW = fLogoSize * pdp->dp_fWideAdjustment;
    const PIX pixLogoH = fLogoSize * td.GetHeight() / td.GetWidth();

    vStart = PIX2D(12 * fScaleW, (480 - pixLogoH - 16) * fScaleH);
    vSize = PIX2D(pixLogoW * fScaleW, pixLogoH * fScaleH);

    pdp->PutTexture(&_pGUIM->m_toLogoCT, PIXaabbox2D(vStart, vStart + vSize));
  }

  // Render game logo
  {
    const FLOAT fResize = Min(fScaleW, fScaleH);

    vStart = PIX2D(pixW / 2, 10 * fResize);
    vSize = PIX2D(256 * fResize, 64 * fResize);

    pdp->PutTexture(&_pGUIM->m_toLogoMenuB, PIXaabbox2D(vStart, vStart + vSize));
    vStart(1) -= vSize(1);
    pdp->PutTexture(&_pGUIM->m_toLogoMenuA, PIXaabbox2D(vStart, vStart + vSize));
  }

  // [Cecil] Display build version
  {
    pdp->SetFont(_pfdConsoleFont);
    pdp->SetTextScaling(1.0f);
    pdp->SetTextCharSpacing(-1);

    const PIX pixBuildX = pixW - 16;
    const PIX pixBuildY = pixH - _pfdConsoleFont->fd_pixCharHeight;
    pdp->PutTextR(SE_GetBuildVersion(), pixBuildX, pixBuildY, 0xFFFFFFFF);
  }
};

// [Cecil] Change to the menu
void CMainMenu::ChangeTo(void) {
  _pGUIM->ChangeToMenu(&_pGUIM->gmMainMenu);
};
