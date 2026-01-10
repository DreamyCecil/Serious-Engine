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
#include "MSinglePlayer.h"

#include "LevelInfo.h"

extern CTString sam_strFirstLevel;
extern CTString sam_strTechTestLevel;
extern CTString sam_strTrainingLevel;

// [Cecil] Open level or category selection screen
void StartSelectLevel(ULONG ulFlags, BOOL bStartLevel) {
  extern BOOL _bStartSelectedLevel;
  _bStartSelectedLevel = bStartLevel;

  CLevelsMenu::ChangeTo(ulFlags);
};

static void StartSinglePlayerNewMenu(void) {
  _pGame->gam_strCustomLevel = sam_strFirstLevel;
  CSinglePlayerNewMenu::ChangeTo();
};

static void StartSelectLevelFromSingle(void) {
  // [Cecil] Select from single player levels, regardless of gamemode
  StartSelectLevel(SPF_SINGLEPLAYER, TRUE);
};

CTString GetQuickLoadNotes(void) {
  if (_pShell->GetINDEX("gam_iQuickSaveSlots") <= 8) {
    return TRANS(
      "In-game QuickSave shortcuts:\n"
      "F6 - save a new QuickSave\n"
      "F9 - load the last QuickSave\n"
    );
  }

  return "";
};

BOOL LSLoadSinglePlayer(CGameMenu *pgm, const CTString &fnm) {
  _pGame->gm_StartSplitScreenCfg = CGame::SSC_PLAY;
  _pGame->gm_aiStartLocalPlayers[0] = _pGame->gm_iSinglePlayer;

  for (INDEX iLocal = 1; iLocal < NET_MAXLOCALPLAYERS; iLocal++) {
    _pGame->gm_aiStartLocalPlayers[iLocal] = -1;
  }

  _pGame->gm_strNetworkProvider = "Local";

  if (_pGame->LoadGame(fnm)) {
    _pGUIM->StopMenus();
    _gmRunningGameMode = GM_SINGLE_PLAYER;
  } else {
    _gmRunningGameMode = GM_NONE;
  }

  return TRUE;
};

void StartSinglePlayerQuickLoadMenu(void) {
  _gmMenuGameMode = GM_SINGLE_PLAYER;

  CTString strDir(0, "SaveGame\\Player%d\\Quick\\", _pGame->gm_iSinglePlayer);
  CLoadSaveMenu::ChangeToLoad(TRUE, LSSORT_FILEDN, strDir, GetQuickLoadNotes(), &LSLoadSinglePlayer);
};

void StartSinglePlayerLoadMenu(void) {
  _gmMenuGameMode = GM_SINGLE_PLAYER;

  CTString strDir(0, "SaveGame\\Player%d\\", _pGame->gm_iSinglePlayer);
  CLoadSaveMenu::ChangeToLoad(FALSE, LSSORT_FILEDN, strDir, "", &LSLoadSinglePlayer);
};

static void StartTraining(void) {
  _pGame->gam_strCustomLevel = sam_strTrainingLevel;
  CSinglePlayerNewMenu::ChangeTo();
};

extern void StartSinglePlayerGame_Normal(void);

static void StartTechTest(void) {
  _pGame->gam_strCustomLevel = sam_strTechTestLevel;
  StartSinglePlayerGame_Normal();
};

static void StartChangePlayerMenuFromSinglePlayer(void) {
  _iLocalPlayer = -1;
  CPlayerProfileMenu::ChangeTo(&_pGame->gm_iSinglePlayer, TRUE);
};

static void StartSinglePlayerGameOptions(void) {
  CVarMenu::ChangeTo(TRANS("GAME OPTIONS"), CTFILENAME("Scripts\\Menu\\SPOptions.cfg"));
};

void CSinglePlayerMenu::Initialize_t(void)
{
  // intialize single player menu
  gm_mgTitle.SetText(TRANS("SINGLE PLAYER"));
  gm_mgTitle.mg_boxOnScreen = BoxTitle();
  AddChild(&gm_mgTitle);

  gm_mgPlayerLabel.mg_boxOnScreen = BoxBigRow(-1.0f);
  gm_mgPlayerLabel.mg_bfsFontSize = BFS_MEDIUM;
  gm_mgPlayerLabel.mg_iCenterI = -1;
  gm_mgPlayerLabel.mg_bEnabled = FALSE;
  gm_mgPlayerLabel.mg_bLabel = TRUE;
  AddChild(&gm_mgPlayerLabel);

  gm_mgNewGame.SetText(TRANS("NEW GAME"));
  gm_mgNewGame.mg_bfsFontSize = BFS_LARGE;
  gm_mgNewGame.mg_boxOnScreen = BoxBigRow(0.0f);
  gm_mgNewGame.mg_strTip = TRANS("start new game with current player");
  AddChild(&gm_mgNewGame);
  gm_mgNewGame.mg_pmgUp = &gm_mgOptions;
  gm_mgNewGame.mg_pmgDown = &gm_mgCustom;
  gm_mgNewGame.mg_pCallbackFunction = &StartSinglePlayerNewMenu;

  gm_mgCustom.SetText(TRANS("CUSTOM LEVEL"));
  gm_mgCustom.mg_bfsFontSize = BFS_LARGE;
  gm_mgCustom.mg_boxOnScreen = BoxBigRow(1.0f);
  gm_mgCustom.mg_strTip = TRANS("start new game on a custom level");
  AddChild(&gm_mgCustom);
  gm_mgCustom.mg_pmgUp = &gm_mgNewGame;
  gm_mgCustom.mg_pmgDown = &gm_mgQuickLoad;
  gm_mgCustom.mg_pCallbackFunction = &StartSelectLevelFromSingle;

  gm_mgQuickLoad.SetText(TRANS("QUICK LOAD"));
  gm_mgQuickLoad.mg_bfsFontSize = BFS_LARGE;
  gm_mgQuickLoad.mg_boxOnScreen = BoxBigRow(2.0f);
  gm_mgQuickLoad.mg_strTip = TRANS("load a quick-saved game (F9)");
  AddChild(&gm_mgQuickLoad);
  gm_mgQuickLoad.mg_pmgUp = &gm_mgCustom;
  gm_mgQuickLoad.mg_pmgDown = &gm_mgLoad;
  gm_mgQuickLoad.mg_pCallbackFunction = &StartSinglePlayerQuickLoadMenu;

  gm_mgLoad.SetText(TRANS("LOAD"));
  gm_mgLoad.mg_bfsFontSize = BFS_LARGE;
  gm_mgLoad.mg_boxOnScreen = BoxBigRow(3.0f);
  gm_mgLoad.mg_strTip = TRANS("load a saved game of current player");
  AddChild(&gm_mgLoad);
  gm_mgLoad.mg_pmgUp = &gm_mgQuickLoad;
  gm_mgLoad.mg_pmgDown = &gm_mgTraining;
  gm_mgLoad.mg_pCallbackFunction = &StartSinglePlayerLoadMenu;

  gm_mgTraining.SetText(TRANS("TRAINING"));
  gm_mgTraining.mg_bfsFontSize = BFS_LARGE;
  gm_mgTraining.mg_boxOnScreen = BoxBigRow(4.0f);
  gm_mgTraining.mg_strTip = TRANS("start training level");
  AddChild(&gm_mgTraining);
  gm_mgTraining.mg_pmgUp = &gm_mgLoad;
  gm_mgTraining.mg_pmgDown = &gm_mgTechTest;
  gm_mgTraining.mg_pCallbackFunction = &StartTraining;

  gm_mgTechTest.SetText(TRANS("TECHNOLOGY TEST"));
  gm_mgTechTest.mg_bfsFontSize = BFS_LARGE;
  gm_mgTechTest.mg_boxOnScreen = BoxBigRow(5.0f);
  gm_mgTechTest.mg_strTip = TRANS("start technology testing level");
  AddChild(&gm_mgTechTest);
  gm_mgTechTest.mg_pmgUp = &gm_mgTraining;
  gm_mgTechTest.mg_pmgDown = &gm_mgPlayersAndControls;
  gm_mgTechTest.mg_pCallbackFunction = &StartTechTest;

  gm_mgPlayersAndControls.mg_bfsFontSize = BFS_LARGE;
  gm_mgPlayersAndControls.mg_boxOnScreen = BoxBigRow(6.0f);
  gm_mgPlayersAndControls.mg_pmgUp = &gm_mgTechTest;
  gm_mgPlayersAndControls.mg_pmgDown = &gm_mgOptions;
  gm_mgPlayersAndControls.SetText(TRANS("PLAYERS AND CONTROLS"));
  gm_mgPlayersAndControls.mg_strTip = TRANS("change currently active player or adjust controls");
  AddChild(&gm_mgPlayersAndControls);
  gm_mgPlayersAndControls.mg_pCallbackFunction = &StartChangePlayerMenuFromSinglePlayer;

  gm_mgOptions.SetText(TRANS("GAME OPTIONS"));
  gm_mgOptions.mg_bfsFontSize = BFS_LARGE;
  gm_mgOptions.mg_boxOnScreen = BoxBigRow(7.0f);
  gm_mgOptions.mg_strTip = TRANS("adjust miscellaneous game options");
  AddChild(&gm_mgOptions);
  gm_mgOptions.mg_pmgUp = &gm_mgPlayersAndControls;
  gm_mgOptions.mg_pmgDown = &gm_mgNewGame;
  gm_mgOptions.mg_pCallbackFunction = &StartSinglePlayerGameOptions;
}

void CSinglePlayerMenu::OnStart(void) {
  // [Cecil] Simply toggle the training button if the level exists
  gm_mgTraining.mg_bEnabled = FileExists(sam_strTrainingLevel);
  gm_mgTechTest.mg_bEnabled = IsMenuEnabled("Technology Test");

  CPlayerCharacter &pc = _pGame->gm_apcPlayers[_pGame->gm_iSinglePlayer];
  gm_mgPlayerLabel.SetText(CTString(0, TRANS("Player: %s\n"), pc.GetNameForPrinting().ConstData()));
};

// [Cecil] Change to the menu
void CSinglePlayerMenu::ChangeTo(void) {
  CGameMenu *pgm = new CSinglePlayerMenu;
  pgm->Initialize_t();
  _pGUIM->ChangeToMenu(pgm);
};
