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
#include "MNetwork.h"

CTFileName _fnGameToLoad;

static void StartNetworkLoadGame(void) {
  _pGame->gm_StartSplitScreenCfg = _pGame->gm_MenuSplitScreenCfg;

  for (INDEX iLocal = 0; iLocal < NET_MAXLOCALPLAYERS; iLocal++) {
    _pGame->gm_aiStartLocalPlayers[iLocal] = _pGame->gm_aiMenuLocalPlayers[iLocal];
  }

  _pGame->gm_strNetworkProvider = "TCP/IP Server";

  if (_pGame->LoadGame(_fnGameToLoad)) {
    _pGUIM->StopMenus();
    _gmRunningGameMode = GM_NETWORK;
  } else {
    _gmRunningGameMode = GM_NONE;
  }
};

BOOL LSLoadNetwork(CGameMenu *pgm, const CTString &fnm) {
  // call local players menu
  _fnGameToLoad = fnm;
  CSelectPlayersMenu::ChangeTo(FALSE, TRUE, &StartNetworkLoadGame);
  return TRUE;
};

void StartNetworkQuickLoadMenu(void) {
  _gmMenuGameMode = GM_NETWORK;

  extern CTString GetQuickLoadNotes(void);
  CLoadSaveMenu::ChangeToLoad(TRUE, LSSORT_FILEDN, "SaveGame\\Network\\Quick\\", GetQuickLoadNotes(), &LSLoadNetwork);
};

void StartNetworkLoadMenu(void) {
  _gmMenuGameMode = GM_NETWORK;

  CLoadSaveMenu::ChangeToLoad(FALSE, LSSORT_FILEDN, "SaveGame\\Network\\", "", &LSLoadNetwork);
};

void CNetworkMenu::Initialize_t(void)
{
  // intialize network menu
  gm_mgTitle.mg_boxOnScreen = BoxTitle();
  gm_mgTitle.SetText(TRANS("NETWORK"));
  AddChild(&gm_mgTitle);

  gm_mgJoin.mg_bfsFontSize = BFS_LARGE;
  gm_mgJoin.mg_boxOnScreen = BoxBigRow(1.0f);
  gm_mgJoin.mg_pmgUp = &gm_mgLoad;
  gm_mgJoin.mg_pmgDown = &gm_mgStart;
  gm_mgJoin.SetText(TRANS("JOIN GAME"));
  gm_mgJoin.mg_strTip = TRANS("join a network game");
  AddChild(&gm_mgJoin);
  gm_mgJoin.mg_pCallbackFunction = &CNetworkJoinMenu::ChangeTo;

  gm_mgStart.mg_bfsFontSize = BFS_LARGE;
  gm_mgStart.mg_boxOnScreen = BoxBigRow(2.0f);
  gm_mgStart.mg_pmgUp = &gm_mgJoin;
  gm_mgStart.mg_pmgDown = &gm_mgQuickLoad;
  gm_mgStart.SetText(TRANS("START SERVER"));
  gm_mgStart.mg_strTip = TRANS("start a network game server");
  AddChild(&gm_mgStart);
  gm_mgStart.mg_pCallbackFunction = &CNetworkStartMenu::ChangeTo;

  gm_mgQuickLoad.mg_bfsFontSize = BFS_LARGE;
  gm_mgQuickLoad.mg_boxOnScreen = BoxBigRow(3.0f);
  gm_mgQuickLoad.mg_pmgUp = &gm_mgStart;
  gm_mgQuickLoad.mg_pmgDown = &gm_mgLoad;
  gm_mgQuickLoad.SetText(TRANS("QUICK LOAD"));
  gm_mgQuickLoad.mg_strTip = TRANS("load a quick-saved game (F9)");
  AddChild(&gm_mgQuickLoad);
  gm_mgQuickLoad.mg_pCallbackFunction = &StartNetworkQuickLoadMenu;

  gm_mgLoad.mg_bfsFontSize = BFS_LARGE;
  gm_mgLoad.mg_boxOnScreen = BoxBigRow(4.0f);
  gm_mgLoad.mg_pmgUp = &gm_mgQuickLoad;
  gm_mgLoad.mg_pmgDown = &gm_mgJoin;
  gm_mgLoad.SetText(TRANS("LOAD"));
  gm_mgLoad.mg_strTip = TRANS("start server and load a network game (server only)");
  AddChild(&gm_mgLoad);
  gm_mgLoad.mg_pCallbackFunction = &StartNetworkLoadMenu;
}

// [Cecil] Change to the menu
void CNetworkMenu::ChangeTo(void) {
  CGameMenu *pgm = new CNetworkMenu;
  pgm->Initialize_t();
  _pGUIM->ChangeToMenu(pgm);
};
