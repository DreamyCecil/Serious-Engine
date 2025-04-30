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
#include "MNetworkOpen.h"

extern CTFileName _fnmModSelected;
static CTString _strModURLSelected;
static CTString _strModServerSelected;

static void ExitAndSpawnExplorer(void) {
  _bRunning = FALSE;
  _bQuitScreen = FALSE;
  extern CTString _strURLToVisit;
  _strURLToVisit = _strModURLSelected;
};

static void ModNotInstalled(void) {
  CTString strLabel(0, TRANS("You don't have MOD '%s' installed.\nDo you want to visit its web site?"), _fnmModSelected.ConstData());
  CConfirmMenu::ChangeTo(strLabel, &ExitAndSpawnExplorer, NULL, FALSE);
};

static void ModConnect(void) {
  extern CTFileName _fnmModToLoad;
  extern CTString _strModServerJoin;
  _fnmModToLoad = _fnmModSelected;
  _strModServerJoin = _strModServerSelected;
};

static void ModConnectConfirm(void) {
  if (_fnmModSelected == " ") {
    _fnmModSelected = CTString("SeriousSam");
  }

  CTFileName fnmModPath = SE1_MODS_SUBDIR + _fnmModSelected + "\\";

  if (!FileExists(fnmModPath + "BaseWriteInclude.lst")  && !FileExists(fnmModPath + "BaseWriteExclude.lst")
   && !FileExists(fnmModPath + "BaseBrowseInclude.lst") && !FileExists(fnmModPath + "BaseBrowseExclude.lst")) {
    ModNotInstalled();
    return;
  }

  CPrintF(TRANS("Server is running a different MOD (%s).\nYou need to reload to connect.\n"), _fnmModSelected.ConstData());
  CConfirmMenu::ChangeTo(TRANS("CHANGE THE MOD?"), &ModConnect, NULL, TRUE);
};

void JoinNetworkGame(void) {
  _pGame->gm_StartSplitScreenCfg = _pGame->gm_MenuSplitScreenCfg;

  for (INDEX iLocal = 0; iLocal < NET_MAXLOCALPLAYERS; iLocal++) {
    _pGame->gm_aiStartLocalPlayers[iLocal] = _pGame->gm_aiMenuLocalPlayers[iLocal];
  }

  _pGame->gm_strNetworkProvider = "TCP/IP Client";

  if (_pGame->JoinGame(CNetworkSession(_pGame->gam_strJoinAddress))) {
    _pGUIM->StopMenus();
    _gmRunningGameMode = GM_NETWORK;
    return;
  }

  if (_pNetwork->ga_strRequiredMod != "") {
    char strModName[256] = { 0 };
    char strModURL[256] = { 0 };
    _pNetwork->ga_strRequiredMod.ScanF("%250[^\\]\\%s", &strModName, &strModURL);

    _fnmModSelected = CTString(strModName);
    _strModURLSelected = strModURL;

    if (_strModURLSelected == "") {
      _strModURLSelected = "http://www.croteam.com/mods/Old";
    }

    _strModServerSelected.PrintF("%s:%s", _pGame->gam_strJoinAddress.ConstData(), _pShell->GetValue("net_iPort").ConstData());
    ModConnectConfirm();
  }

  _gmRunningGameMode = GM_NONE;
};

void StartSelectPlayersMenuFromOpen(void) {
  CSelectPlayersMenu &gmCurrent = _pGUIM->gmSelectPlayersMenu;
  gmCurrent.gm_bAllowDedicated = FALSE;
  gmCurrent.gm_bAllowObserving = TRUE;
  gmCurrent.gm_mgStart.mg_pCallbackFunction = &JoinNetworkGame;
  CSelectPlayersMenu::ChangeTo();

  extern void StartNetworkSettingsMenu(void);
  StartNetworkSettingsMenu();
};

void CNetworkOpenMenu::Initialize_t(void)
{
  // intialize network join menu
  gm_mgTitle.mg_boxOnScreen = BoxTitle();
  gm_mgTitle.SetText(TRANS("JOIN"));
  AddChild(&gm_mgTitle);

  gm_mgAddressLabel.SetText(TRANS("Address:"));
  gm_mgAddressLabel.mg_boxOnScreen = BoxMediumLeft(1);
  gm_mgAddressLabel.mg_iCenterI = -1;
  AddChild(&gm_mgAddressLabel);

  gm_mgAddress.SetText(_pGame->gam_strJoinAddress);
  gm_mgAddress.mg_ctMaxStringLen = 20;
  gm_mgAddress.mg_pstrToChange = &_pGame->gam_strJoinAddress;
  gm_mgAddress.mg_boxOnScreen = BoxMediumMiddle(1);
  gm_mgAddress.mg_bfsFontSize = BFS_MEDIUM;
  gm_mgAddress.mg_iCenterI = -1;
  gm_mgAddress.mg_pmgUp = &gm_mgJoin;
  gm_mgAddress.mg_pmgDown = &gm_mgPort;
  gm_mgAddress.mg_strTip = TRANS("specify server address");
  AddChild(&gm_mgAddress);

  gm_mgPortLabel.SetText(TRANS("Port:"));
  gm_mgPortLabel.mg_boxOnScreen = BoxMediumLeft(2);
  gm_mgPortLabel.mg_iCenterI = -1;
  AddChild(&gm_mgPortLabel);

  gm_mgPort.SetText("");
  gm_mgPort.mg_ctMaxStringLen = 10;
  gm_mgPort.mg_pstrToChange = &gm_strPort;
  gm_mgPort.mg_boxOnScreen = BoxMediumMiddle(2);
  gm_mgPort.mg_bfsFontSize = BFS_MEDIUM;
  gm_mgPort.mg_iCenterI = -1;
  gm_mgPort.mg_pmgUp = &gm_mgAddress;
  gm_mgPort.mg_pmgDown = &gm_mgJoin;
  gm_mgPort.mg_strTip = TRANS("specify server address");
  AddChild(&gm_mgPort);

  gm_mgJoin.mg_boxOnScreen = BoxMediumMiddle(3);
  gm_mgJoin.mg_pmgUp = &gm_mgPort;
  gm_mgJoin.mg_pmgDown = &gm_mgAddress;
  gm_mgJoin.SetText(TRANS("Join"));
  AddChild(&gm_mgJoin);
  gm_mgJoin.mg_pCallbackFunction = &StartSelectPlayersMenuFromOpen;
}

void CNetworkOpenMenu::StartMenu(void)
{
  gm_strPort = _pShell->GetValue("net_iPort");
  gm_mgPort.SetText(gm_strPort);
}

void CNetworkOpenMenu::EndMenu(void)
{
  _pShell->SetValue("net_iPort", gm_strPort);
}

// [Cecil] Change to the menu
void CNetworkOpenMenu::ChangeTo(void) {
  _pGUIM->ChangeToMenu(&_pGUIM->gmNetworkOpenMenu);
};
