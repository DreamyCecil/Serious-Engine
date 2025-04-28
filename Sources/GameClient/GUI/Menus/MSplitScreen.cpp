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
#include "MSplitScreen.h"

extern CTFileName _fnGameToLoad;

static void StartSplitScreenGameLoad(void) {
  _pGame->gm_StartSplitScreenCfg = _pGame->gm_MenuSplitScreenCfg;

  for (INDEX iLocal = 0; iLocal < NET_MAXLOCALPLAYERS; iLocal++) {
    _pGame->gm_aiStartLocalPlayers[iLocal] = _pGame->gm_aiMenuLocalPlayers[iLocal];
  }

  _pGame->gm_strNetworkProvider = "Local";

  if (_pGame->LoadGame(_fnGameToLoad)) {
    StopMenus();
    _gmRunningGameMode = GM_SPLIT_SCREEN;
  } else {
    _gmRunningGameMode = GM_NONE;
  }
};

static BOOL LSLoadSplitScreen(CGameMenu *pgm, const CTString &fnm) {
  _fnGameToLoad = fnm;

  CSelectPlayersMenu &gmCurrent = _pGUIM->gmSelectPlayersMenu;
  gmCurrent.gm_bAllowDedicated = FALSE;
  gmCurrent.gm_bAllowObserving = FALSE;
  gmCurrent.gm_mgStart.mg_pCallbackFunction = &StartSplitScreenGameLoad;
  CSelectPlayersMenu::ChangeTo();
  return TRUE;
};

void StartSplitScreenQuickLoadMenu(void) {
  _gmMenuGameMode = GM_SPLIT_SCREEN;

  CLoadSaveMenu &gmCurrent = _pGUIM->gmLoadSaveMenu;
  gmCurrent.gm_mgTitle.SetText(TRANS("QUICK LOAD"));
  gmCurrent.gm_bAllowThumbnails = TRUE;
  gmCurrent.gm_iSortType = LSSORT_FILEDN;
  gmCurrent.gm_bSave = FALSE;
  gmCurrent.gm_bManage = TRUE;
  gmCurrent.gm_fnmDirectory = ExpandPath::ToUser("SaveGame\\SplitScreen\\Quick\\", TRUE); // [Cecil] From user data in a mod
  gmCurrent.gm_fnmSelected = CTString("");
  gmCurrent.gm_fnmExt = CTString(".sav");
  gmCurrent.gm_pAfterFileChosen = &LSLoadSplitScreen;
  extern void SetQuickLoadNotes(void);
  SetQuickLoadNotes();
  CLoadSaveMenu::ChangeTo();
};

void StartSplitScreenLoadMenu(void) {
  _gmMenuGameMode = GM_SPLIT_SCREEN;

  CLoadSaveMenu &gmCurrent = _pGUIM->gmLoadSaveMenu;
  gmCurrent.gm_mgTitle.SetText(TRANS("LOAD"));
  gmCurrent.gm_bAllowThumbnails = TRUE;
  gmCurrent.gm_iSortType = LSSORT_FILEDN;
  gmCurrent.gm_bSave = FALSE;
  gmCurrent.gm_bManage = TRUE;
  gmCurrent.gm_fnmDirectory = ExpandPath::ToUser("SaveGame\\SplitScreen\\", TRUE); // [Cecil] From user data in a mod
  gmCurrent.gm_fnmSelected = CTString("");
  gmCurrent.gm_fnmExt = CTString(".sav");
  gmCurrent.gm_pAfterFileChosen = &LSLoadSplitScreen;
  gmCurrent.gm_mgNotes.SetText("");
  CLoadSaveMenu::ChangeTo();
};

void CSplitScreenMenu::Initialize_t(void)
{
  // intialize split screen menu
  gm_mgTitle.mg_boxOnScreen = BoxTitle();
  gm_mgTitle.SetText(TRANS("SPLIT SCREEN"));
  AddChild(&gm_mgTitle);

  gm_mgStart.mg_bfsFontSize = BFS_LARGE;
  gm_mgStart.mg_boxOnScreen = BoxBigRow(0);
  gm_mgStart.mg_pmgUp = &gm_mgLoad;
  gm_mgStart.mg_pmgDown = &gm_mgQuickLoad;
  gm_mgStart.SetText(TRANS("NEW GAME"));
  gm_mgStart.mg_strTip = TRANS("start new split-screen game");
  AddChild(&gm_mgStart);
  gm_mgStart.mg_pCallbackFunction = &CSplitStartMenu::ChangeTo;

  gm_mgQuickLoad.mg_bfsFontSize = BFS_LARGE;
  gm_mgQuickLoad.mg_boxOnScreen = BoxBigRow(1);
  gm_mgQuickLoad.mg_pmgUp = &gm_mgStart;
  gm_mgQuickLoad.mg_pmgDown = &gm_mgLoad;
  gm_mgQuickLoad.SetText(TRANS("QUICK LOAD"));
  gm_mgQuickLoad.mg_strTip = TRANS("load a quick-saved game (F9)");
  AddChild(&gm_mgQuickLoad);
  gm_mgQuickLoad.mg_pCallbackFunction = &StartSplitScreenQuickLoadMenu;

  gm_mgLoad.mg_bfsFontSize = BFS_LARGE;
  gm_mgLoad.mg_boxOnScreen = BoxBigRow(2);
  gm_mgLoad.mg_pmgUp = &gm_mgQuickLoad;
  gm_mgLoad.mg_pmgDown = &gm_mgStart;
  gm_mgLoad.SetText(TRANS("LOAD"));
  gm_mgLoad.mg_strTip = TRANS("load a saved split-screen game");
  AddChild(&gm_mgLoad);
  gm_mgLoad.mg_pCallbackFunction = &StartSplitScreenLoadMenu;
}

void CSplitScreenMenu::StartMenu(void)
{
  CGameMenu::StartMenu();
}

// [Cecil] Change to the menu
void CSplitScreenMenu::ChangeTo(void) {
  ChangeToMenu(&_pGUIM->gmSplitScreenMenu);
};
