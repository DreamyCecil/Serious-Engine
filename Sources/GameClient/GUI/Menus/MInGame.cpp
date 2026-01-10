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
#include "MInGame.h"

static void StartCurrentQuickLoadMenu(void) {
  if (_gmRunningGameMode == GM_NETWORK) {
    extern void StartNetworkQuickLoadMenu(void);
    StartNetworkQuickLoadMenu();

  } else if (_gmRunningGameMode == GM_SPLIT_SCREEN) {
    extern void StartSplitScreenQuickLoadMenu(void);
    StartSplitScreenQuickLoadMenu();

  } else {
    extern void StartSinglePlayerQuickLoadMenu(void);
    StartSinglePlayerQuickLoadMenu();
  }
};

// start load/save menus depending on type of game running
static void QuickSaveFromMenu(void) {
  _pShell->SetINDEX("gam_bQuickSave", 2); // force save with reporting
  _pGUIM->StopMenus();
};

void StartCurrentLoadMenu(void) {
  if (_gmRunningGameMode == GM_NETWORK) {
    extern void StartNetworkLoadMenu(void);
    StartNetworkLoadMenu();

  } else if (_gmRunningGameMode == GM_SPLIT_SCREEN) {
    extern void StartSplitScreenLoadMenu(void);
    StartSplitScreenLoadMenu();

  } else {
    extern void StartSinglePlayerLoadMenu(void);
    StartSinglePlayerLoadMenu();
  }
};

// same function for saving in singleplay, network and splitscreen
static BOOL LSSaveAnyGame(CGameMenu *pgm, const CTString &fnm) {
  if (_pGame->SaveGame(fnm)) {
    _pGUIM->StopMenus();
    return TRUE;
  }

  return FALSE;
};

static void StartNetworkSaveMenu(void) {
  if (_gmRunningGameMode != GM_NETWORK) return;
  _gmMenuGameMode = GM_NETWORK;

  CLoadSaveMenu::ChangeToSave(LSSORT_FILEDN, "SaveGame\\Network\\", "SaveGame", _pGame->GetDefaultGameDescription(TRUE), "", &LSSaveAnyGame);
};

static void StartSplitScreenSaveMenu(void) {
  if (_gmRunningGameMode != GM_SPLIT_SCREEN) return;
  _gmMenuGameMode = GM_SPLIT_SCREEN;

  CLoadSaveMenu::ChangeToSave(LSSORT_FILEDN, "SaveGame\\SplitScreen\\", "SaveGame", _pGame->GetDefaultGameDescription(TRUE), "", &LSSaveAnyGame);
};

static void StartSinglePlayerSaveMenu(void) {
  if (_gmRunningGameMode != GM_SINGLE_PLAYER) return;

  // Do nothing if no live players
  if (_pGame->GetPlayersCount() > 0 && _pGame->GetLivePlayersCount() <= 0) {
    return;
  }

  _gmMenuGameMode = GM_SINGLE_PLAYER;

  CTString strDir(0, "SaveGame\\Player%d\\", _pGame->gm_iSinglePlayer);
  CLoadSaveMenu::ChangeToSave(LSSORT_FILEDN, strDir, "SaveGame", _pGame->GetDefaultGameDescription(TRUE), "", &LSSaveAnyGame);
};

void StartCurrentSaveMenu(void) {
  if (_gmRunningGameMode == GM_NETWORK) {
    StartNetworkSaveMenu();

  } else if (_gmRunningGameMode == GM_SPLIT_SCREEN) {
    StartSplitScreenSaveMenu();

  } else {
    StartSinglePlayerSaveMenu();
  }
};

static BOOL LSSaveDemo(CGameMenu *pgm, const CTString &fnm) {
  // save the demo
  if (_pGame->StartDemoRec(fnm)) {
    _pGUIM->StopMenus();
    return TRUE;
  }

  return FALSE;
};

static void StartDemoSaveMenu(CMenuGadget *pmg) {
  if (_gmRunningGameMode == GM_NONE) return;
  _gmMenuGameMode = GM_DEMO;

  CLoadSaveMenu::ChangeToDemos(TRUE, LSSORT_FILEUP, _pGame->GetDefaultGameDescription(FALSE), &LSSaveDemo);
};

static void SetDemoStartStopRecText(CMenuGadget *pmg);

static void StopRecordingDemo(CMenuGadget *pmg) {
  _pNetwork->StopDemoRec();
  SetDemoStartStopRecText(pmg);
};

void SetDemoStartStopRecText(CMenuGadget *pmg) {
  CMGButton &mgRec = *(CMGButton *)pmg;

  if (_pNetwork->IsRecordingDemo()) {
    mgRec.SetText(TRANS("STOP RECORDING"));
    mgRec.mg_strTip = TRANS("stop current recording");
    mgRec.mg_pActivatedFunction = &StopRecordingDemo;
  } else {
    mgRec.SetText(TRANS("RECORD DEMO"));
    mgRec.mg_strTip = TRANS("start recording current game");
    mgRec.mg_pActivatedFunction = &StartDemoSaveMenu;
  }
};

static void StopCurrentGame(void) {
  _pGame->StopGame();
  _gmRunningGameMode = GM_NONE;
  _pGUIM->StopMenus();
  _pGUIM->StartMenus();
};

static void StopConfirm(void) {
  CConfirmMenu::ChangeTo(TRANS("ARE YOU SERIOUS?"), &StopCurrentGame, NULL, TRUE);
};

extern void ExitConfirm(void);

void CInGameMenu::Initialize_t(void)
{
  // intialize main menu
  gm_mgTitle.SetText(TRANS("GAME"));
  gm_mgTitle.mg_boxOnScreen = BoxTitle();
  AddChild(&gm_mgTitle);

  gm_mgLabel1.SetText("");
  gm_mgLabel1.mg_boxOnScreen = BoxMediumRow(-2.0);
  gm_mgLabel1.mg_bfsFontSize = BFS_MEDIUM;
  gm_mgLabel1.mg_iCenterI = -1;
  gm_mgLabel1.mg_bEnabled = FALSE;
  gm_mgLabel1.mg_bLabel = TRUE;
  AddChild(&gm_mgLabel1);

  gm_mgLabel2.SetText("");
  gm_mgLabel2.mg_boxOnScreen = BoxMediumRow(-1.0);
  gm_mgLabel2.mg_bfsFontSize = BFS_MEDIUM;
  gm_mgLabel2.mg_iCenterI = -1;
  gm_mgLabel2.mg_bEnabled = FALSE;
  gm_mgLabel2.mg_bLabel = TRUE;
  AddChild(&gm_mgLabel2);

  gm_mgQuickLoad.SetText(TRANS("QUICK LOAD"));
  gm_mgQuickLoad.mg_bfsFontSize = BFS_LARGE;
  gm_mgQuickLoad.mg_boxOnScreen = BoxBigRow(0.0f);
  gm_mgQuickLoad.mg_strTip = TRANS("load a quick-saved game (F9)");
  AddChild(&gm_mgQuickLoad);
  gm_mgQuickLoad.mg_pmgUp = &gm_mgQuit;
  gm_mgQuickLoad.mg_pmgDown = &gm_mgQuickSave;
  gm_mgQuickLoad.mg_pCallbackFunction = &StartCurrentQuickLoadMenu;

  gm_mgQuickSave.SetText(TRANS("QUICK SAVE"));
  gm_mgQuickSave.mg_bfsFontSize = BFS_LARGE;
  gm_mgQuickSave.mg_boxOnScreen = BoxBigRow(1.0f);
  gm_mgQuickSave.mg_strTip = TRANS("quick-save current game (F6)");
  AddChild(&gm_mgQuickSave);
  gm_mgQuickSave.mg_pmgUp = &gm_mgQuickLoad;
  gm_mgQuickSave.mg_pmgDown = &gm_mgLoad;
  gm_mgQuickSave.mg_pCallbackFunction = &QuickSaveFromMenu;

  gm_mgLoad.SetText(TRANS("LOAD"));
  gm_mgLoad.mg_bfsFontSize = BFS_LARGE;
  gm_mgLoad.mg_boxOnScreen = BoxBigRow(2.0f);
  gm_mgLoad.mg_strTip = TRANS("load a saved game");
  AddChild(&gm_mgLoad);
  gm_mgLoad.mg_pmgUp = &gm_mgQuickSave;
  gm_mgLoad.mg_pmgDown = &gm_mgSave;
  gm_mgLoad.mg_pCallbackFunction = &StartCurrentLoadMenu;

  gm_mgSave.SetText(TRANS("SAVE"));
  gm_mgSave.mg_bfsFontSize = BFS_LARGE;
  gm_mgSave.mg_boxOnScreen = BoxBigRow(3.0f);
  gm_mgSave.mg_strTip = TRANS("save current game (each player has own slots!)");
  AddChild(&gm_mgSave);
  gm_mgSave.mg_pmgUp = &gm_mgLoad;
  gm_mgSave.mg_pmgDown = &gm_mgDemoRec;
  gm_mgSave.mg_pCallbackFunction = &StartCurrentSaveMenu;

  gm_mgDemoRec.mg_boxOnScreen = BoxBigRow(4.0f);
  gm_mgDemoRec.mg_bfsFontSize = BFS_LARGE;
  gm_mgDemoRec.mg_pmgUp = &gm_mgSave;
  gm_mgDemoRec.mg_pmgDown = &gm_mgHighScore;
  gm_mgDemoRec.SetText("Text not set");
  AddChild(&gm_mgDemoRec);
  gm_mgDemoRec.mg_pActivatedFunction = NULL;

  gm_mgHighScore.SetText(TRANS("HIGH SCORES"));
  gm_mgHighScore.mg_bfsFontSize = BFS_LARGE;
  gm_mgHighScore.mg_boxOnScreen = BoxBigRow(5.0f);
  gm_mgHighScore.mg_strTip = TRANS("view list of top ten best scores");
  AddChild(&gm_mgHighScore);
  gm_mgHighScore.mg_pmgUp = &gm_mgDemoRec;
  gm_mgHighScore.mg_pmgDown = &gm_mgOptions;
  gm_mgHighScore.mg_pCallbackFunction = &CHighScoreMenu::ChangeTo;

  gm_mgOptions.SetText(TRANS("OPTIONS"));
  gm_mgOptions.mg_bfsFontSize = BFS_LARGE;
  gm_mgOptions.mg_boxOnScreen = BoxBigRow(6.0f);
  gm_mgOptions.mg_strTip = TRANS("adjust video, audio and input options");
  AddChild(&gm_mgOptions);
  gm_mgOptions.mg_pmgUp = &gm_mgHighScore;
  gm_mgOptions.mg_pmgDown = &gm_mgStop;
  gm_mgOptions.mg_pCallbackFunction = &COptionsMenu::ChangeTo;

  gm_mgStop.SetText(TRANS("STOP GAME"));
  gm_mgStop.mg_bfsFontSize = BFS_LARGE;
  gm_mgStop.mg_boxOnScreen = BoxBigRow(7.0f);
  gm_mgStop.mg_strTip = TRANS("stop currently running game");
  AddChild(&gm_mgStop);
  gm_mgStop.mg_pmgUp = &gm_mgOptions;
  gm_mgStop.mg_pmgDown = &gm_mgQuit;
  gm_mgStop.mg_pCallbackFunction = &StopConfirm;

  gm_mgQuit.SetText(TRANS("QUIT"));
  gm_mgQuit.mg_bfsFontSize = BFS_LARGE;
  gm_mgQuit.mg_boxOnScreen = BoxBigRow(8.0f);
  gm_mgQuit.mg_strTip = TRANS("exit game immediately");
  AddChild(&gm_mgQuit);
  gm_mgQuit.mg_pmgUp = &gm_mgStop;
  gm_mgQuit.mg_pmgDown = &gm_mgQuickLoad;
  gm_mgQuit.mg_pCallbackFunction = &ExitConfirm;
}

void CInGameMenu::OnStart(void) {
  gm_mgQuickLoad.mg_bEnabled = _pNetwork->IsServer();
  gm_mgQuickSave.mg_bEnabled = _pNetwork->IsServer();
  gm_mgLoad.mg_bEnabled = _pNetwork->IsServer();
  gm_mgSave.mg_bEnabled = _pNetwork->IsServer();

  SetDemoStartStopRecText(&gm_mgDemoRec);

  if (_gmRunningGameMode == GM_SINGLE_PLAYER) {
    CPlayerCharacter &pc = _pGame->gm_apcPlayers[_pGame->gm_iSinglePlayer];
    gm_mgLabel1.SetText(CTString(0, TRANS("Player: %s"), pc.GetNameForPrinting().ConstData()));
    gm_mgLabel2.SetText("");

  } else {
    if (_pNetwork->IsServer()) {

      CTString strHost, strAddress;
      CTString strHostName;
      _pNetwork->GetHostName(strHost, strAddress);
      if (strHost == "") {
        strHostName = TRANS("<not started yet>");
      }
      else {
        strHostName = strHost + " (" + strAddress + ")";
      }

      gm_mgLabel1.SetText(TRANS("Address: ") + strHostName);
      gm_mgLabel2.SetText("");

    } else {

      CTString strConfig;
      strConfig = TRANS("<not adjusted>");
      extern CTString sam_strNetworkSettings;
      if (sam_strNetworkSettings != "") {
        LoadStringVar(CTFileName(sam_strNetworkSettings).NoExt() + ".des", strConfig);
        strConfig.OnlyFirstLine();
      }

      gm_mgLabel1.SetText(TRANS("Connected to: ") + _pGame->gam_strJoinAddress);
      gm_mgLabel2.SetText(TRANS("Connection: ") + strConfig);
    }
  }
};

// [Cecil] Change to the menu
void CInGameMenu::ChangeTo(void) {
  CGameMenu *pgm = new CInGameMenu;
  pgm->Initialize_t();
  _pGUIM->ChangeToMenu(pgm);
};
