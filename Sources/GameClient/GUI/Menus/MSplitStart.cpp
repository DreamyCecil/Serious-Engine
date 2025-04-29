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
#include "LevelInfo.h"
#include "MenuStuff.h"
#include "MSplitStart.h"

static void UpdateSplitLevel(CMenuGadget *pmg, INDEX iDummy) {
  CSplitStartMenu &gmSplitStart = *(CSplitStartMenu *)pmg->GetParentMenu();

  ValidateLevelForFlags(_pGame->gam_strCustomLevel, GetSpawnFlagsForGameType(gmSplitStart.gm_mgGameType.mg_iSelected));
  gmSplitStart.gm_mgLevel.SetText(FindLevelByFileName(_pGame->gam_strCustomLevel).li_strName);
};

static void StartSelectLevelFromSplit(CMenuGadget *pmg) {
  CSplitStartMenu &gmSplitStart = *(CSplitStartMenu *)pmg->GetParentMenu();

  extern void StartSelectLevel(ULONG ulFlags, BOOL bStartLevel);
  StartSelectLevel(GetSpawnFlagsForGameType(gmSplitStart.gm_mgGameType.mg_iSelected), FALSE);
};

extern void StartVarGameOptions(void);

static void StartSplitScreenGame(void) {
  _pGame->gm_StartSplitScreenCfg = _pGame->gm_MenuSplitScreenCfg;

  for (INDEX iLocal = 0; iLocal < NET_MAXLOCALPLAYERS; iLocal++) {
    _pGame->gm_aiStartLocalPlayers[iLocal] = _pGame->gm_aiMenuLocalPlayers[iLocal];
  }

  CTFileName fnWorld = _pGame->gam_strCustomLevel;

  _pGame->gm_strNetworkProvider = "Local";

  CUniversalSessionProperties sp;
  _pGame->SetMultiPlayerSession(sp);

  if (_pGame->NewGame(fnWorld.FileName(), fnWorld, sp)) {
    StopMenus();
    _gmRunningGameMode = GM_SPLIT_SCREEN;
  } else {
    _gmRunningGameMode = GM_NONE;
  }
};

static void StartSelectPlayersMenuFromSplit(void) {
  CSelectPlayersMenu &gmCurrent = _pGUIM->gmSelectPlayersMenu;
  gmCurrent.gm_bAllowDedicated = FALSE;
  gmCurrent.gm_bAllowObserving = FALSE;
  gmCurrent.gm_mgStart.mg_pCallbackFunction = &StartSplitScreenGame;
  CSelectPlayersMenu::ChangeTo();
};

void CSplitStartMenu::Initialize_t(void)
{
  // intialize split screen menu
  gm_mgTitle.mg_boxOnScreen = BoxTitle();
  gm_mgTitle.SetText(TRANS("START SPLIT SCREEN"));
  AddChild(&gm_mgTitle);

  // game type trigger
  TRIGGER_MG(gm_mgGameType, 0,
    gm_mgStart, gm_mgDifficulty, TRANS("Game type:"), astrGameTypeRadioTexts);
  gm_mgGameType.mg_ctTexts = ctGameTypeRadioTexts;
  gm_mgGameType.mg_strTip = TRANS("choose type of multiplayer game");
  gm_mgGameType.mg_pOnTriggerChange = &UpdateSplitLevel;

  // difficulty trigger
  TRIGGER_MG(gm_mgDifficulty, 1,
    gm_mgGameType, gm_mgLevel, TRANS("Difficulty:"), astrDifficultyRadioTexts);
  gm_mgDifficulty.mg_strTip = TRANS("choose difficulty level");

  // level name
  gm_mgLevel.SetText("");
  gm_mgLevel.SetName(TRANS("Level:"));
  gm_mgLevel.mg_boxOnScreen = BoxMediumRow(2);
  gm_mgLevel.mg_bfsFontSize = BFS_MEDIUM;
  gm_mgLevel.mg_iCenterI = -1;
  gm_mgLevel.mg_pmgUp = &gm_mgDifficulty;
  gm_mgLevel.mg_pmgDown = &gm_mgOptions;
  gm_mgLevel.mg_strTip = TRANS("choose the level to start");
  gm_mgLevel.mg_pActivatedFunction = &StartSelectLevelFromSplit;
  AddChild(&gm_mgLevel);

  // options button
  gm_mgOptions.SetText(TRANS("Game options"));
  gm_mgOptions.mg_boxOnScreen = BoxMediumRow(3);
  gm_mgOptions.mg_bfsFontSize = BFS_MEDIUM;
  gm_mgOptions.mg_iCenterI = 0;
  gm_mgOptions.mg_pmgUp = &gm_mgLevel;
  gm_mgOptions.mg_pmgDown = &gm_mgStart;
  gm_mgOptions.mg_strTip = TRANS("adjust game rules");
  gm_mgOptions.mg_pCallbackFunction = &StartVarGameOptions;
  AddChild(&gm_mgOptions);

  // start button
  gm_mgStart.mg_bfsFontSize = BFS_LARGE;
  gm_mgStart.mg_boxOnScreen = BoxBigRow(4);
  gm_mgStart.mg_pmgUp = &gm_mgOptions;
  gm_mgStart.mg_pmgDown = &gm_mgGameType;
  gm_mgStart.SetText(TRANS("START"));
  AddChild(&gm_mgStart);
  gm_mgStart.mg_pCallbackFunction = &StartSelectPlayersMenuFromSplit;
}

void CSplitStartMenu::StartMenu(void)
{
  extern INDEX sam_bMentalActivated;
  gm_mgDifficulty.mg_ctTexts = sam_bMentalActivated ? 6 : 5;

  gm_mgGameType.mg_iSelected = Clamp(_pShell->GetINDEX("gam_iStartMode"), 0L, ctGameTypeRadioTexts - 1L);
  gm_mgGameType.ApplyCurrentSelection();
  gm_mgDifficulty.mg_iSelected = _pShell->GetINDEX("gam_iStartDifficulty") + 1;
  gm_mgDifficulty.ApplyCurrentSelection();

  // clamp maximum number of players to at least 4
  _pShell->SetINDEX("gam_ctMaxPlayers", ClampDn(_pShell->GetINDEX("gam_ctMaxPlayers"), 4L));

  UpdateSplitLevel(&gm_mgGameType, 0);
  CGameMenu::StartMenu();
}

void CSplitStartMenu::EndMenu(void)
{
  _pShell->SetINDEX("gam_iStartDifficulty", gm_mgDifficulty.mg_iSelected - 1);
  _pShell->SetINDEX("gam_iStartMode", gm_mgGameType.mg_iSelected);

  CGameMenu::EndMenu();
}

// [Cecil] Change to the menu
void CSplitStartMenu::ChangeTo(void) {
  ChangeToMenu(&_pGUIM->gmSplitStartMenu);
};
