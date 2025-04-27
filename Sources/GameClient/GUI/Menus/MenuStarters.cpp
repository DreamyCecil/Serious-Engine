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

/* This file contains starter fuctions for all menus. */

#include "StdH.h"

#include "MenuStarters.h"
#include "MenuStartersAF.h"
#include "MenuStuff.h"
#include "LevelInfo.h"

extern void(*_pAfterLevelChosen)(void);
extern BOOL _bPlayerMenuFromSinglePlayer;

extern CTString _strLastPlayerAppearance;
extern CTString sam_strNetworkSettings;

extern CTFileName _fnmModSelected;
extern CTString _strModURLSelected;
extern CTString _strModServerSelected;

void StartSinglePlayerNewMenuCustom(void)
{
  CSinglePlayerNewMenu::ChangeTo();
}

static void SetQuickLoadNotes(void)
{
  CLoadSaveMenu &gmCurrent = _pGUIM->gmLoadSaveMenu;

  if (_pShell->GetINDEX("gam_iQuickSaveSlots") <= 8) {
    gmCurrent.gm_mgNotes.SetText(TRANS(
      "In-game QuickSave shortcuts:\n"
      "F6 - save a new QuickSave\n"
      "F9 - load the last QuickSave\n"));
  } else {
    gmCurrent.gm_mgNotes.SetText("");
  }
}

extern CTString sam_strFirstLevel;

void StartSinglePlayerNewMenu(void)
{
  CSinglePlayerNewMenu &gmCurrent = _pGUIM->gmSinglePlayerNewMenu;

  _pGame->gam_strCustomLevel = sam_strFirstLevel;
  CSinglePlayerNewMenu::ChangeTo();
}

// game options var settings
void StartVarGameOptions(void)
{
  CVarMenu::ChangeTo(TRANS("GAME OPTIONS"), CTFILENAME("Scripts\\Menu\\GameOptions.cfg"));
}

void StartSinglePlayerGameOptions(void)
{
  CVarMenu::ChangeTo(TRANS("GAME OPTIONS"), CTFILENAME("Scripts\\Menu\\SPOptions.cfg"));
}

void StartGameOptionsFromNetwork(void)
{
  StartVarGameOptions();
}

void StartGameOptionsFromSplitScreen(void)
{
  StartVarGameOptions();
}

// rendering options var settings
void StartRenderingOptionsMenu(void)
{
  CVarMenu::ChangeTo(TRANS("RENDERING OPTIONS"), CTFILENAME("Scripts\\Menu\\RenderingOptions.cfg"));
}

void StartOptionsMenu(void)
{
  COptionsMenu::ChangeTo();
}

void StartCurrentLoadMenu()
{
  if (_gmRunningGameMode == GM_NETWORK) {
    void StartNetworkLoadMenu(void);
    StartNetworkLoadMenu();
  } else if (_gmRunningGameMode == GM_SPLIT_SCREEN) {
    void StartSplitScreenLoadMenu(void);
    StartSplitScreenLoadMenu();
  } else {
    void StartSinglePlayerLoadMenu(void);
    StartSinglePlayerLoadMenu();
  }
}

void StartCurrentSaveMenu()
{
  if (_gmRunningGameMode == GM_NETWORK) {
    void StartNetworkSaveMenu(void);
    StartNetworkSaveMenu();
  } else if (_gmRunningGameMode == GM_SPLIT_SCREEN) {
    void StartSplitScreenSaveMenu(void);
    StartSplitScreenSaveMenu();
  } else {
    void StartSinglePlayerSaveMenu(void);
    StartSinglePlayerSaveMenu();
  }
}

void StartCurrentQuickLoadMenu()
{
  if (_gmRunningGameMode == GM_NETWORK) {
    void StartNetworkQuickLoadMenu(void);
    StartNetworkQuickLoadMenu();
  } else if (_gmRunningGameMode == GM_SPLIT_SCREEN) {
    void StartSplitScreenQuickLoadMenu(void);
    StartSplitScreenQuickLoadMenu();
  } else {
    void StartSinglePlayerQuickLoadMenu(void);
    StartSinglePlayerQuickLoadMenu();
  }
}

void StartChangePlayerMenuFromOptions(void)
{
  _bPlayerMenuFromSinglePlayer = FALSE;
  _pGUIM->gmPlayerProfile.gm_piCurrentPlayer = &_pGame->gm_iSinglePlayer;
  CPlayerProfileMenu::ChangeTo();
}

void StartChangePlayerMenuFromSinglePlayer(void)
{
  _iLocalPlayer = -1;
  _bPlayerMenuFromSinglePlayer = TRUE;
  _pGUIM->gmPlayerProfile.gm_piCurrentPlayer = &_pGame->gm_iSinglePlayer;
  CPlayerProfileMenu::ChangeTo();
}

void StartControlsMenuFromPlayer(void)
{
  CControlsMenu::ChangeTo();
}

void StartControlsMenuFromOptions(void)
{
  CControlsMenu::ChangeTo();
}

void StartHighScoreMenu(void)
{
  CHighScoreMenu::ChangeTo();
}

void StartSplitScreenGame(void)
{
  //  _pGame->gm_MenuSplitScreenCfg = (enum CGame::SplitScreenCfg) mgSplitScreenCfg.mg_iSelected;
  _pGame->gm_StartSplitScreenCfg = _pGame->gm_MenuSplitScreenCfg;

  for (INDEX iLocal = 0; iLocal < NET_MAXLOCALPLAYERS; iLocal++) {
    _pGame->gm_aiStartLocalPlayers[iLocal] = _pGame->gm_aiMenuLocalPlayers[iLocal];
  }

  CTFileName fnWorld = _pGame->gam_strCustomLevel;

  _pGame->gm_strNetworkProvider = "Local";
  CUniversalSessionProperties sp;
  _pGame->SetMultiPlayerSession(sp);
  if (_pGame->NewGame(fnWorld.FileName(), fnWorld, sp))
  {
    StopMenus();
    _gmRunningGameMode = GM_SPLIT_SCREEN;
  } else {
    _gmRunningGameMode = GM_NONE;
  }
}

void StartNetworkGame(void)
{
  //  _pGame->gm_MenuSplitScreenCfg = (enum CGame::SplitScreenCfg) mgSplitScreenCfg.mg_iSelected;
  _pGame->gm_StartSplitScreenCfg = _pGame->gm_MenuSplitScreenCfg;

  for (INDEX iLocal = 0; iLocal < NET_MAXLOCALPLAYERS; iLocal++) {
    _pGame->gm_aiStartLocalPlayers[iLocal] = _pGame->gm_aiMenuLocalPlayers[iLocal];
  }

  CTFileName fnWorld = _pGame->gam_strCustomLevel;

  _pGame->gm_strNetworkProvider = "TCP/IP Server";
  CUniversalSessionProperties sp;
  _pGame->SetMultiPlayerSession(sp);
  if (_pGame->NewGame(_pGame->gam_strSessionName, fnWorld, sp))
  {
    StopMenus();
    _gmRunningGameMode = GM_NETWORK;
    // if starting a dedicated server
    if (_pGame->gm_MenuSplitScreenCfg == CGame::SSC_DEDICATED) {
      // pull down the console
      extern INDEX sam_bToggleConsole;
      sam_bToggleConsole = TRUE;
    }
  } else {
    _gmRunningGameMode = GM_NONE;
  }
}

void JoinNetworkGame(void)
{
  //  _pGame->gm_MenuSplitScreenCfg = (enum CGame::SplitScreenCfg) mgSplitScreenCfg.mg_iSelected;
  _pGame->gm_StartSplitScreenCfg = _pGame->gm_MenuSplitScreenCfg;

  for (INDEX iLocal = 0; iLocal < NET_MAXLOCALPLAYERS; iLocal++) {
    _pGame->gm_aiStartLocalPlayers[iLocal] = _pGame->gm_aiMenuLocalPlayers[iLocal];
  }

  _pGame->gm_strNetworkProvider = "TCP/IP Client";
  if (_pGame->JoinGame(CNetworkSession(_pGame->gam_strJoinAddress)))
  {
    StopMenus();
    _gmRunningGameMode = GM_NETWORK;
  } else {
    if (_pNetwork->ga_strRequiredMod != "") {
      extern CTFileName _fnmModToLoad;
      extern CTString _strModServerJoin;
      char strModName[256] = { 0 };
      char strModURL[256] = { 0 };
      _pNetwork->ga_strRequiredMod.ScanF("%250[^\\]\\%s", &strModName, &strModURL);
      _fnmModSelected = CTString(strModName);
      _strModURLSelected = strModURL;
      if (_strModURLSelected == "") {
        _strModURLSelected = "http://www.croteam.com/mods/Old";
      }
      _strModServerSelected.PrintF("%s:%s", _pGame->gam_strJoinAddress.ConstData(), _pShell->GetValue("net_iPort").ConstData());
      extern void ModConnectConfirm(void);
      ModConnectConfirm();
    }
    _gmRunningGameMode = GM_NONE;
  }
}

// -------- Servers Menu Functions
void StartSelectServerLAN(void)
{
  CServersMenu &gmCurrent = _pGUIM->gmServersMenu;

  gmCurrent.m_bInternet = FALSE;
  CServersMenu::ChangeTo();
}

void StartSelectServerNET(void)
{
  CServersMenu &gmCurrent = _pGUIM->gmServersMenu;

  gmCurrent.m_bInternet = TRUE;
  CServersMenu::ChangeTo();
}

// [Cecil] Open level or category selection screen
static void StartSelectLevel(ULONG ulFlags, void (*pAfterChosen)(void), CGameMenu *pgmParent) {
  FilterLevels(ulFlags);

  _pAfterLevelChosen = pAfterChosen;

  // [Cecil] Rewind visited menus to the parent
  extern CGameMenu *_pgmRewindToAfterLevelChosen;
  _pgmRewindToAfterLevelChosen = pgmParent;

  CLevelsMenu::ChangeTo();
};

// -------- Levels Menu Functions
void StartSelectLevelFromSingle(void)
{
  StartSelectLevel(GetSpawnFlagsForGameType(-1), &StartSinglePlayerNewMenuCustom, &_pGUIM->gmSinglePlayerMenu);
}

void StartSelectLevelFromSplit(void)
{
  StartSelectLevel(GetSpawnFlagsForGameType(_pGUIM->gmSplitStartMenu.gm_mgGameType.mg_iSelected),
    &CSplitStartMenu::ChangeTo, &_pGUIM->gmSplitStartMenu);
}

void StartSelectLevelFromNetwork(void)
{
  StartSelectLevel(GetSpawnFlagsForGameType(_pGUIM->gmNetworkStartMenu.gm_mgGameType.mg_iSelected),
    &CNetworkStartMenu::ChangeTo, &_pGUIM->gmNetworkStartMenu);
}

// -------- Players Selection Menu Functions
void StartSelectPlayersMenuFromSplit(void)
{
  CSelectPlayersMenu &gmCurrent = _pGUIM->gmSelectPlayersMenu;

  gmCurrent.gm_bAllowDedicated = FALSE;
  gmCurrent.gm_bAllowObserving = FALSE;
  gmCurrent.gm_mgStart.mg_pActivatedFunction = &StartSplitScreenGame;
  CSelectPlayersMenu::ChangeTo();
}

void StartSelectPlayersMenuFromNetwork(void)
{
  CSelectPlayersMenu &gmCurrent = _pGUIM->gmSelectPlayersMenu;

  gmCurrent.gm_bAllowDedicated = TRUE;
  gmCurrent.gm_bAllowObserving = TRUE;
  gmCurrent.gm_mgStart.mg_pActivatedFunction = &StartNetworkGame;
  CSelectPlayersMenu::ChangeTo();
}

void StartSelectPlayersMenuFromNetworkLoad(void)
{
  CSelectPlayersMenu &gmCurrent = _pGUIM->gmSelectPlayersMenu;

  gmCurrent.gm_bAllowDedicated = FALSE;
  gmCurrent.gm_bAllowObserving = TRUE;
  gmCurrent.gm_mgStart.mg_pActivatedFunction = &StartNetworkLoadGame;
  CSelectPlayersMenu::ChangeTo();
}

void StartSelectPlayersMenuFromSplitScreenLoad(void)
{
  CSelectPlayersMenu &gmCurrent = _pGUIM->gmSelectPlayersMenu;

  gmCurrent.gm_bAllowDedicated = FALSE;
  gmCurrent.gm_bAllowObserving = FALSE;
  gmCurrent.gm_mgStart.mg_pActivatedFunction = &StartSplitScreenGameLoad;
  CSelectPlayersMenu::ChangeTo();
}

void StartSelectPlayersMenuFromOpen(void)
{
  CSelectPlayersMenu &gmCurrent = _pGUIM->gmSelectPlayersMenu;

  gmCurrent.gm_bAllowDedicated = FALSE;
  gmCurrent.gm_bAllowObserving = TRUE;
  gmCurrent.gm_mgStart.mg_pActivatedFunction = &JoinNetworkGame;
  CSelectPlayersMenu::ChangeTo();

  /*if (sam_strNetworkSettings=="")*/ {
    void StartNetworkSettingsMenu(void);
    StartNetworkSettingsMenu();
    _pGUIM->gmLoadSaveMenu.gm_bNoEscape = TRUE;
  }
}

void StartSelectPlayersMenuFromServers(void)
{
  CSelectPlayersMenu &gmCurrent = _pGUIM->gmSelectPlayersMenu;

  gmCurrent.gm_bAllowDedicated = FALSE;
  gmCurrent.gm_bAllowObserving = TRUE;
  gmCurrent.gm_mgStart.mg_pActivatedFunction = &JoinNetworkGame;
  CSelectPlayersMenu::ChangeTo();

  /*if (sam_strNetworkSettings=="")*/ {
    void StartNetworkSettingsMenu(void);
    StartNetworkSettingsMenu();
    _pGUIM->gmLoadSaveMenu.gm_bNoEscape = TRUE;
  }
}

// -------- Save/Load Menu Calling Functions
void StartPlayerModelLoadMenu(void)
{
  CLoadSaveMenu &gmCurrent = _pGUIM->gmLoadSaveMenu;

  gmCurrent.gm_mgTitle.SetText(TRANS("CHOOSE MODEL"));
  gmCurrent.gm_bAllowThumbnails = TRUE;
  gmCurrent.gm_iSortType = LSSORT_FILEUP;
  gmCurrent.gm_bSave = FALSE;
  gmCurrent.gm_bManage = FALSE;
  gmCurrent.gm_fnmDirectory = CTString("Models\\Player\\");
  gmCurrent.gm_fnmSelected = _strLastPlayerAppearance;
  gmCurrent.gm_fnmExt = CTString(".amc");
  gmCurrent.gm_pAfterFileChosen = &LSLoadPlayerModel;
  gmCurrent.gm_mgNotes.SetText("");
  CLoadSaveMenu::ChangeTo();
}

void StartControlsLoadMenu(void)
{
  CLoadSaveMenu &gmCurrent = _pGUIM->gmLoadSaveMenu;

  gmCurrent.gm_mgTitle.SetText(TRANS("LOAD CONTROLS"));
  gmCurrent.gm_bAllowThumbnails = FALSE;
  gmCurrent.gm_iSortType = LSSORT_FILEUP;
  gmCurrent.gm_bSave = FALSE;
  gmCurrent.gm_bManage = FALSE;
  // [Cecil] NOTE: It needs to list controls presets from the main directory but it will still
  // load user controls from the user data directory when selected (see LSLoadControls() function)
  gmCurrent.gm_fnmDirectory = CTString("Controls\\");
  gmCurrent.gm_fnmSelected = CTString("");
  gmCurrent.gm_fnmExt = CTString(".ctl");
  gmCurrent.gm_pAfterFileChosen = &LSLoadControls;
  gmCurrent.gm_mgNotes.SetText("");
  CLoadSaveMenu::ChangeTo();
}

void StartCustomLoadMenu(void)
{
  CLoadSaveMenu &gmCurrent = _pGUIM->gmLoadSaveMenu;

  gmCurrent.gm_mgTitle.SetText(TRANS("ADVANCED OPTIONS"));
  gmCurrent.gm_bAllowThumbnails = FALSE;
  gmCurrent.gm_iSortType = LSSORT_NAMEUP;
  gmCurrent.gm_bSave = FALSE;
  gmCurrent.gm_bManage = FALSE;
  gmCurrent.gm_fnmDirectory = CTString("Scripts\\CustomOptions\\");
  gmCurrent.gm_fnmSelected = CTString("");
  gmCurrent.gm_fnmExt = CTString(".cfg");
  gmCurrent.gm_pAfterFileChosen = &LSLoadCustom;
  gmCurrent.gm_mgNotes.SetText("");
  CLoadSaveMenu::ChangeTo();
}

void StartAddonsLoadMenu(void)
{
  CLoadSaveMenu &gmCurrent = _pGUIM->gmLoadSaveMenu;

  gmCurrent.gm_mgTitle.SetText(TRANS("EXECUTE ADDON"));
  gmCurrent.gm_bAllowThumbnails = FALSE;
  gmCurrent.gm_iSortType = LSSORT_NAMEUP;
  gmCurrent.gm_bSave = FALSE;
  gmCurrent.gm_bManage = FALSE;
  gmCurrent.gm_fnmDirectory = CTString("Scripts\\Addons\\");
  gmCurrent.gm_fnmSelected = CTString("");
  gmCurrent.gm_fnmExt = CTString(".ini");
  gmCurrent.gm_pAfterFileChosen = &LSLoadAddon;
  gmCurrent.gm_mgNotes.SetText("");
  CLoadSaveMenu::ChangeTo();
}

void StartModsLoadMenu(void)
{
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
}

void StartNetworkSettingsMenu(void)
{
  CLoadSaveMenu &gmCurrent = _pGUIM->gmLoadSaveMenu;

  gmCurrent.gm_mgTitle.SetText(TRANS("CONNECTION SETTINGS"));
  gmCurrent.gm_bAllowThumbnails = FALSE;
  gmCurrent.gm_iSortType = LSSORT_FILEUP;
  gmCurrent.gm_bSave = FALSE;
  gmCurrent.gm_bManage = FALSE;
  gmCurrent.gm_fnmDirectory = CTString("Scripts\\NetSettings\\");
  gmCurrent.gm_fnmSelected = sam_strNetworkSettings;
  gmCurrent.gm_fnmExt = CTString(".ini");
  gmCurrent.gm_pAfterFileChosen = &LSLoadNetSettings;

  if (sam_strNetworkSettings == "") {
    gmCurrent.gm_mgNotes.SetText(TRANS(
      "Before joining a network game,\n"
      "you have to adjust your connection parameters.\n"
      "Choose one option from the list.\n"
      "If you have problems with connection, you can adjust\n"
      "these parameters again from the Options menu.\n"));
  } else {
    gmCurrent.gm_mgNotes.SetText("");
  }

  CLoadSaveMenu::ChangeTo();
}


void StartSinglePlayerQuickLoadMenu(void)
{
  CLoadSaveMenu &gmCurrent = _pGUIM->gmLoadSaveMenu;

  _gmMenuGameMode = GM_SINGLE_PLAYER;

  gmCurrent.gm_mgTitle.SetText(TRANS("QUICK LOAD"));
  gmCurrent.gm_bAllowThumbnails = TRUE;
  gmCurrent.gm_iSortType = LSSORT_FILEDN;
  gmCurrent.gm_bSave = FALSE;
  gmCurrent.gm_bManage = TRUE;
  gmCurrent.gm_fnmDirectory.PrintF("SaveGame\\Player%d\\Quick\\", _pGame->gm_iSinglePlayer);
  gmCurrent.gm_fnmDirectory = ExpandPath::ToUser(gmCurrent.gm_fnmDirectory, TRUE); // [Cecil] From user data in a mod
  gmCurrent.gm_fnmSelected = CTString("");
  gmCurrent.gm_fnmExt = CTString(".sav");
  gmCurrent.gm_pAfterFileChosen = &LSLoadSinglePlayer;
  SetQuickLoadNotes();
  CLoadSaveMenu::ChangeTo();
}

void StartSinglePlayerLoadMenu(void)
{
  CLoadSaveMenu &gmCurrent = _pGUIM->gmLoadSaveMenu;

  _gmMenuGameMode = GM_SINGLE_PLAYER;

  gmCurrent.gm_mgTitle.SetText(TRANS("LOAD"));
  gmCurrent.gm_bAllowThumbnails = TRUE;
  gmCurrent.gm_iSortType = LSSORT_FILEDN;
  gmCurrent.gm_bSave = FALSE;
  gmCurrent.gm_bManage = TRUE;
  gmCurrent.gm_fnmDirectory.PrintF("SaveGame\\Player%d\\", _pGame->gm_iSinglePlayer);
  gmCurrent.gm_fnmDirectory = ExpandPath::ToUser(gmCurrent.gm_fnmDirectory, TRUE); // [Cecil] From user data in a mod
  gmCurrent.gm_fnmSelected = CTString("");
  gmCurrent.gm_fnmExt = CTString(".sav");
  gmCurrent.gm_pAfterFileChosen = &LSLoadSinglePlayer;
  gmCurrent.gm_mgNotes.SetText("");
  CLoadSaveMenu::ChangeTo();
}

void StartSinglePlayerSaveMenu(void)
{
  if (_gmRunningGameMode != GM_SINGLE_PLAYER) return;

  // if no live players
  if (_pGame->GetPlayersCount()>0 && _pGame->GetLivePlayersCount() <= 0) {
    // do nothing
    return;
  }

  CLoadSaveMenu &gmCurrent = _pGUIM->gmLoadSaveMenu;

  _gmMenuGameMode = GM_SINGLE_PLAYER;

  gmCurrent.gm_mgTitle.SetText(TRANS("SAVE"));
  gmCurrent.gm_bAllowThumbnails = TRUE;
  gmCurrent.gm_iSortType = LSSORT_FILEDN;
  gmCurrent.gm_bSave = TRUE;
  gmCurrent.gm_bManage = TRUE;
  gmCurrent.gm_fnmDirectory.PrintF("SaveGame\\Player%d\\", _pGame->gm_iSinglePlayer);
  gmCurrent.gm_fnmDirectory = ExpandPath::ToUser(gmCurrent.gm_fnmDirectory, TRUE); // [Cecil] From user data in a mod
  gmCurrent.gm_fnmSelected = CTString("");
  gmCurrent.gm_fnmBaseName = CTString("SaveGame");
  gmCurrent.gm_fnmExt = CTString(".sav");
  gmCurrent.gm_pAfterFileChosen = &LSSaveAnyGame;
  gmCurrent.gm_mgNotes.SetText("");
  gmCurrent.gm_strSaveDes = _pGame->GetDefaultGameDescription(TRUE);
  CLoadSaveMenu::ChangeTo();
}

void StartDemoLoadMenu(void)
{
  CLoadSaveMenu &gmCurrent = _pGUIM->gmLoadSaveMenu;

  _gmMenuGameMode = GM_DEMO;

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
}

void StartDemoSaveMenu(void)
{
  CLoadSaveMenu &gmCurrent = _pGUIM->gmLoadSaveMenu;

  if (_gmRunningGameMode == GM_NONE) return;
  _gmMenuGameMode = GM_DEMO;

  gmCurrent.gm_mgTitle.SetText(TRANS("RECORD DEMO"));
  gmCurrent.gm_bAllowThumbnails = TRUE;
  gmCurrent.gm_iSortType = LSSORT_FILEUP;
  gmCurrent.gm_bSave = TRUE;
  gmCurrent.gm_bManage = TRUE;
  gmCurrent.gm_fnmDirectory = CTString("Demos\\");
  gmCurrent.gm_fnmSelected = CTString("");
  gmCurrent.gm_fnmBaseName = CTString("Demo");
  gmCurrent.gm_fnmExt = CTString(".dem");
  gmCurrent.gm_pAfterFileChosen = &LSSaveDemo;
  gmCurrent.gm_mgNotes.SetText("");
  gmCurrent.gm_strSaveDes = _pGame->GetDefaultGameDescription(FALSE);
  CLoadSaveMenu::ChangeTo();
}

void StartNetworkQuickLoadMenu(void)
{
  CLoadSaveMenu &gmCurrent = _pGUIM->gmLoadSaveMenu;

  _gmMenuGameMode = GM_NETWORK;

  gmCurrent.gm_mgTitle.SetText(TRANS("QUICK LOAD"));
  gmCurrent.gm_bAllowThumbnails = TRUE;
  gmCurrent.gm_iSortType = LSSORT_FILEDN;
  gmCurrent.gm_bSave = FALSE;
  gmCurrent.gm_bManage = TRUE;
  gmCurrent.gm_fnmDirectory = ExpandPath::ToUser("SaveGame\\Network\\Quick\\", TRUE); // [Cecil] From user data in a mod
  gmCurrent.gm_fnmSelected = CTString("");
  gmCurrent.gm_fnmExt = CTString(".sav");
  gmCurrent.gm_pAfterFileChosen = &LSLoadNetwork;
  SetQuickLoadNotes();
  CLoadSaveMenu::ChangeTo();
}

void StartNetworkLoadMenu(void)
{
  CLoadSaveMenu &gmCurrent = _pGUIM->gmLoadSaveMenu;

  _gmMenuGameMode = GM_NETWORK;

  gmCurrent.gm_mgTitle.SetText(TRANS("LOAD"));
  gmCurrent.gm_bAllowThumbnails = TRUE;
  gmCurrent.gm_iSortType = LSSORT_FILEDN;
  gmCurrent.gm_bSave = FALSE;
  gmCurrent.gm_bManage = TRUE;
  gmCurrent.gm_fnmDirectory = ExpandPath::ToUser("SaveGame\\Network\\", TRUE); // [Cecil] From user data in a mod
  gmCurrent.gm_fnmSelected = CTString("");
  gmCurrent.gm_fnmExt = CTString(".sav");
  gmCurrent.gm_pAfterFileChosen = &LSLoadNetwork;
  gmCurrent.gm_mgNotes.SetText("");
  CLoadSaveMenu::ChangeTo();
}

void StartNetworkSaveMenu(void)
{
  CLoadSaveMenu &gmCurrent = _pGUIM->gmLoadSaveMenu;

  if (_gmRunningGameMode != GM_NETWORK) return;
  _gmMenuGameMode = GM_NETWORK;

  gmCurrent.gm_mgTitle.SetText(TRANS("SAVE"));
  gmCurrent.gm_bAllowThumbnails = TRUE;
  gmCurrent.gm_iSortType = LSSORT_FILEDN;
  gmCurrent.gm_bSave = TRUE;
  gmCurrent.gm_bManage = TRUE;
  gmCurrent.gm_fnmDirectory = ExpandPath::ToUser("SaveGame\\Network\\", TRUE); // [Cecil] From user data in a mod
  gmCurrent.gm_fnmSelected = CTString("");
  gmCurrent.gm_fnmBaseName = CTString("SaveGame");
  gmCurrent.gm_fnmExt = CTString(".sav");
  gmCurrent.gm_pAfterFileChosen = &LSSaveAnyGame;
  gmCurrent.gm_mgNotes.SetText("");
  gmCurrent.gm_strSaveDes = _pGame->GetDefaultGameDescription(TRUE);
  CLoadSaveMenu::ChangeTo();
}

void StartSplitScreenQuickLoadMenu(void)
{
  CLoadSaveMenu &gmCurrent = _pGUIM->gmLoadSaveMenu;

  _gmMenuGameMode = GM_SPLIT_SCREEN;

  gmCurrent.gm_mgTitle.SetText(TRANS("QUICK LOAD"));
  gmCurrent.gm_bAllowThumbnails = TRUE;
  gmCurrent.gm_iSortType = LSSORT_FILEDN;
  gmCurrent.gm_bSave = FALSE;
  gmCurrent.gm_bManage = TRUE;
  gmCurrent.gm_fnmDirectory = ExpandPath::ToUser("SaveGame\\SplitScreen\\Quick\\", TRUE); // [Cecil] From user data in a mod
  gmCurrent.gm_fnmSelected = CTString("");
  gmCurrent.gm_fnmExt = CTString(".sav");
  gmCurrent.gm_pAfterFileChosen = &LSLoadSplitScreen;
  SetQuickLoadNotes();
  CLoadSaveMenu::ChangeTo();
}

void StartSplitScreenLoadMenu(void)
{
  CLoadSaveMenu &gmCurrent = _pGUIM->gmLoadSaveMenu;

  _gmMenuGameMode = GM_SPLIT_SCREEN;

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
}

void StartSplitScreenSaveMenu(void)
{
  CLoadSaveMenu &gmCurrent = _pGUIM->gmLoadSaveMenu;

  if (_gmRunningGameMode != GM_SPLIT_SCREEN) return;
  _gmMenuGameMode = GM_SPLIT_SCREEN;

  gmCurrent.gm_mgTitle.SetText(TRANS("SAVE"));
  gmCurrent.gm_bAllowThumbnails = TRUE;
  gmCurrent.gm_iSortType = LSSORT_FILEDN;
  gmCurrent.gm_bSave = TRUE;
  gmCurrent.gm_bManage = TRUE;
  gmCurrent.gm_fnmDirectory = ExpandPath::ToUser("SaveGame\\SplitScreen\\", TRUE); // [Cecil] From user data in a mod
  gmCurrent.gm_fnmSelected = CTString("");
  gmCurrent.gm_fnmBaseName = CTString("SaveGame");
  gmCurrent.gm_fnmExt = CTString(".sav");
  gmCurrent.gm_pAfterFileChosen = &LSSaveAnyGame;
  gmCurrent.gm_mgNotes.SetText("");
  gmCurrent.gm_strSaveDes = _pGame->GetDefaultGameDescription(TRUE);
  CLoadSaveMenu::ChangeTo();
}
