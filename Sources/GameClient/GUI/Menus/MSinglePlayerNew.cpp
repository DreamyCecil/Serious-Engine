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
#include "MSinglePlayerNew.h"

void StartSinglePlayerGame(void) {
  _pGame->gm_StartSplitScreenCfg = CGame::SSC_PLAY;
  _pGame->gm_aiStartLocalPlayers[0] = _pGame->gm_iSinglePlayer;

  for (INDEX iLocal = 1; iLocal < NET_MAXLOCALPLAYERS; iLocal++) {
    _pGame->gm_aiStartLocalPlayers[iLocal] = -1;
  }

  _pGame->gm_strNetworkProvider = "Local";

  CUniversalSessionProperties sp;
  _pGame->SetSinglePlayerSession(sp);

  if (_pGame->NewGame(_pGame->gam_strCustomLevel, _pGame->gam_strCustomLevel, sp)) {
    StopMenus();
    _gmRunningGameMode = GM_SINGLE_PLAYER;
  } else {
    _gmRunningGameMode = GM_NONE;
  }
};

static void StartSinglePlayerGame_Tourist(void) {
  _pShell->SetINDEX("gam_iStartDifficulty", CSessionProperties::GD_TOURIST);
  _pShell->SetINDEX("gam_iStartMode", CSessionProperties::GM_COOPERATIVE);
  StartSinglePlayerGame();
};

static void StartSinglePlayerGame_Easy(void) {
  _pShell->SetINDEX("gam_iStartDifficulty", CSessionProperties::GD_EASY);
  _pShell->SetINDEX("gam_iStartMode", CSessionProperties::GM_COOPERATIVE);
  StartSinglePlayerGame();
};

void StartSinglePlayerGame_Normal(void) {
  _pShell->SetINDEX("gam_iStartDifficulty", CSessionProperties::GD_NORMAL);
  _pShell->SetINDEX("gam_iStartMode", CSessionProperties::GM_COOPERATIVE);
  StartSinglePlayerGame();
};

static void StartSinglePlayerGame_Hard(void) {
  _pShell->SetINDEX("gam_iStartDifficulty", CSessionProperties::GD_HARD);
  _pShell->SetINDEX("gam_iStartMode", CSessionProperties::GM_COOPERATIVE);
  StartSinglePlayerGame();
};

static void StartSinglePlayerGame_Serious(void) {
  _pShell->SetINDEX("gam_iStartDifficulty", CSessionProperties::GD_EXTREME);
  _pShell->SetINDEX("gam_iStartMode", CSessionProperties::GM_COOPERATIVE);
  StartSinglePlayerGame();
};

static void StartSinglePlayerGame_Mental(void) {
  _pShell->SetINDEX("gam_iStartDifficulty", CSessionProperties::GD_EXTREME + 1);
  _pShell->SetINDEX("gam_iStartMode", CSessionProperties::GM_COOPERATIVE);
  StartSinglePlayerGame();
};

void CSinglePlayerNewMenu::Initialize_t(void)
{
  // intialize single player new menu
  gm_mgTitle.SetText(TRANS("NEW GAME"));
  gm_mgTitle.mg_boxOnScreen = BoxTitle();
  gm_lhGadgets.AddTail(gm_mgTitle.mg_lnNode);

  gm_mgTourist.SetText(TRANS("TOURIST"));
  gm_mgTourist.mg_bfsFontSize = BFS_LARGE;
  gm_mgTourist.mg_boxOnScreen = BoxBigRow(0.0f);
  gm_mgTourist.mg_strTip = TRANS("for non-FPS players");
  gm_lhGadgets.AddTail(gm_mgTourist.mg_lnNode);
  gm_mgTourist.mg_pmgUp = &gm_mgSerious;
  gm_mgTourist.mg_pmgDown = &gm_mgEasy;
  gm_mgTourist.mg_pActivatedFunction = &StartSinglePlayerGame_Tourist;

  gm_mgEasy.SetText(TRANS("EASY"));
  gm_mgEasy.mg_bfsFontSize = BFS_LARGE;
  gm_mgEasy.mg_boxOnScreen = BoxBigRow(1.0f);
  gm_mgEasy.mg_strTip = TRANS("for unexperienced FPS players");
  gm_lhGadgets.AddTail(gm_mgEasy.mg_lnNode);
  gm_mgEasy.mg_pmgUp = &gm_mgTourist;
  gm_mgEasy.mg_pmgDown = &gm_mgMedium;
  gm_mgEasy.mg_pActivatedFunction = &StartSinglePlayerGame_Easy;

  gm_mgMedium.SetText(TRANS("NORMAL"));
  gm_mgMedium.mg_bfsFontSize = BFS_LARGE;
  gm_mgMedium.mg_boxOnScreen = BoxBigRow(2.0f);
  gm_mgMedium.mg_strTip = TRANS("for experienced FPS players");
  gm_lhGadgets.AddTail(gm_mgMedium.mg_lnNode);
  gm_mgMedium.mg_pmgUp = &gm_mgEasy;
  gm_mgMedium.mg_pmgDown = &gm_mgHard;
  gm_mgMedium.mg_pActivatedFunction = &StartSinglePlayerGame_Normal;

  gm_mgHard.SetText(TRANS("HARD"));
  gm_mgHard.mg_bfsFontSize = BFS_LARGE;
  gm_mgHard.mg_boxOnScreen = BoxBigRow(3.0f);
  gm_mgHard.mg_strTip = TRANS("for experienced Serious Sam players");
  gm_lhGadgets.AddTail(gm_mgHard.mg_lnNode);
  gm_mgHard.mg_pmgUp = &gm_mgMedium;
  gm_mgHard.mg_pmgDown = &gm_mgSerious;
  gm_mgHard.mg_pActivatedFunction = &StartSinglePlayerGame_Hard;

  gm_mgSerious.SetText(TRANS("SERIOUS"));
  gm_mgSerious.mg_bfsFontSize = BFS_LARGE;
  gm_mgSerious.mg_boxOnScreen = BoxBigRow(4.0f);
  gm_mgSerious.mg_strTip = TRANS("are you serious?");
  gm_lhGadgets.AddTail(gm_mgSerious.mg_lnNode);
  gm_mgSerious.mg_pmgUp = &gm_mgHard;
  gm_mgSerious.mg_pmgDown = &gm_mgTourist;
  gm_mgSerious.mg_pActivatedFunction = &StartSinglePlayerGame_Serious;

  gm_mgMental.SetText(TRANS("MENTAL"));
  gm_mgMental.mg_bfsFontSize = BFS_LARGE;
  gm_mgMental.mg_boxOnScreen = BoxBigRow(5.0f);
  gm_mgMental.mg_strTip = TRANS("you are not serious!");
  gm_lhGadgets.AddTail(gm_mgMental.mg_lnNode);
  gm_mgMental.mg_pmgUp = &gm_mgSerious;
  gm_mgMental.mg_pmgDown = &gm_mgTourist;
  gm_mgMental.mg_pActivatedFunction = &StartSinglePlayerGame_Mental;
  gm_mgMental.mg_bMental = TRUE;


}
void CSinglePlayerNewMenu::StartMenu(void)
{
  CGameMenu::StartMenu();
  extern INDEX sam_bMentalActivated;
  if (sam_bMentalActivated) {
    gm_mgMental.Appear();
    gm_mgSerious.mg_pmgDown = &gm_mgMental;
    gm_mgTourist.mg_pmgUp = &gm_mgMental;
  } else {
    gm_mgMental.Disappear();
    gm_mgSerious.mg_pmgDown = &gm_mgTourist;
    gm_mgTourist.mg_pmgUp = &gm_mgSerious;
  }
}

// [Cecil] Change to the menu
void CSinglePlayerNewMenu::ChangeTo(void) {
  ChangeToMenu(&_pGUIM->gmSinglePlayerNewMenu);
};
