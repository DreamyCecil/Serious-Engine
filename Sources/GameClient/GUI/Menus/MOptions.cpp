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
#include "MOptions.h"

extern CTString sam_strNetworkSettings;

BOOL _bPlayerMenuFromSinglePlayer = FALSE;

static void StartChangePlayerMenuFromOptions(void) {
  _bPlayerMenuFromSinglePlayer = FALSE;
  _pGUIM->gmPlayerProfile.gm_piCurrentPlayer = &_pGame->gm_iSinglePlayer;
  CPlayerProfileMenu::ChangeTo();
};

static BOOL LSLoadNetSettings(CGameMenu *pgm, const CTString &fnm) {
  sam_strNetworkSettings = fnm;
  CTString strCmd;
  strCmd.PrintF("include \"%s\"", sam_strNetworkSettings.ConstData());
  _pShell->Execute(strCmd);

  _pGUIM->MenuGoToParent();
  return TRUE;
};

void StartNetworkSettingsMenu(void) {
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
};

static BOOL LSLoadCustom(CGameMenu *pgm, const CTString &fnm) {
  CVarMenu::ChangeTo(TRANS("ADVANCED OPTIONS"), fnm);
  return TRUE;
};

static void StartCustomLoadMenu(void) {
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
};

static BOOL LSLoadAddon(CGameMenu *pgm, const CTString &fnm) {
  extern INDEX _iAddonExecState;
  extern CTFileName _fnmAddonToExec;
  _iAddonExecState = 1;
  _fnmAddonToExec = fnm;
  return TRUE;
};

static void StartAddonsLoadMenu(void) {
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
};

void COptionsMenu::Initialize_t(void)
{
  // intialize options menu
  gm_mgTitle.mg_boxOnScreen = BoxTitle();
  gm_mgTitle.SetText(TRANS("OPTIONS"));
  AddChild(&gm_mgTitle);

  gm_mgVideoOptions.mg_bfsFontSize = BFS_LARGE;
  gm_mgVideoOptions.mg_boxOnScreen = BoxBigRow(0.0f);
  gm_mgVideoOptions.mg_pmgUp = &gm_mgAddonOptions;
  gm_mgVideoOptions.mg_pmgDown = &gm_mgAudioOptions;
  gm_mgVideoOptions.SetText(TRANS("VIDEO OPTIONS"));
  gm_mgVideoOptions.mg_strTip = TRANS("set video mode and driver");
  AddChild(&gm_mgVideoOptions);
  gm_mgVideoOptions.mg_pCallbackFunction = &CVideoOptionsMenu::ChangeTo;

  gm_mgAudioOptions.mg_bfsFontSize = BFS_LARGE;
  gm_mgAudioOptions.mg_boxOnScreen = BoxBigRow(1.0f);
  gm_mgAudioOptions.mg_pmgUp = &gm_mgVideoOptions;
  gm_mgAudioOptions.mg_pmgDown = &gm_mgPlayerProfileOptions;
  gm_mgAudioOptions.SetText(TRANS("AUDIO OPTIONS"));
  gm_mgAudioOptions.mg_strTip = TRANS("set audio quality and volume");
  AddChild(&gm_mgAudioOptions);
  gm_mgAudioOptions.mg_pCallbackFunction = &CAudioOptionsMenu::ChangeTo;

  gm_mgPlayerProfileOptions.mg_bfsFontSize = BFS_LARGE;
  gm_mgPlayerProfileOptions.mg_boxOnScreen = BoxBigRow(2.0f);
  gm_mgPlayerProfileOptions.mg_pmgUp = &gm_mgAudioOptions;
  gm_mgPlayerProfileOptions.mg_pmgDown = &gm_mgNetworkOptions;
  gm_mgPlayerProfileOptions.SetText(TRANS("PLAYERS AND CONTROLS"));
  gm_mgPlayerProfileOptions.mg_strTip = TRANS("change currently active player or adjust controls");
  AddChild(&gm_mgPlayerProfileOptions);
  gm_mgPlayerProfileOptions.mg_pCallbackFunction = &StartChangePlayerMenuFromOptions;

  gm_mgNetworkOptions.mg_bfsFontSize = BFS_LARGE;
  gm_mgNetworkOptions.mg_boxOnScreen = BoxBigRow(3);
  gm_mgNetworkOptions.mg_pmgUp = &gm_mgPlayerProfileOptions;
  gm_mgNetworkOptions.mg_pmgDown = &gm_mgCustomOptions;
  gm_mgNetworkOptions.SetText(TRANS("NETWORK CONNECTION"));
  gm_mgNetworkOptions.mg_strTip = TRANS("choose your connection parameters");
  AddChild(&gm_mgNetworkOptions);
  gm_mgNetworkOptions.mg_pCallbackFunction = &StartNetworkSettingsMenu;

  gm_mgCustomOptions.mg_bfsFontSize = BFS_LARGE;
  gm_mgCustomOptions.mg_boxOnScreen = BoxBigRow(4);
  gm_mgCustomOptions.mg_pmgUp = &gm_mgNetworkOptions;
  gm_mgCustomOptions.mg_pmgDown = &gm_mgAddonOptions;
  gm_mgCustomOptions.SetText(TRANS("ADVANCED OPTIONS"));
  gm_mgCustomOptions.mg_strTip = TRANS("for advanced users only");
  AddChild(&gm_mgCustomOptions);
  gm_mgCustomOptions.mg_pCallbackFunction = &StartCustomLoadMenu;

  gm_mgAddonOptions.mg_bfsFontSize = BFS_LARGE;
  gm_mgAddonOptions.mg_boxOnScreen = BoxBigRow(5);
  gm_mgAddonOptions.mg_pmgUp = &gm_mgCustomOptions;
  gm_mgAddonOptions.mg_pmgDown = &gm_mgVideoOptions;
  gm_mgAddonOptions.SetText(TRANS("EXECUTE ADDON"));
  gm_mgAddonOptions.mg_strTip = TRANS("choose from list of addons to execute");
  AddChild(&gm_mgAddonOptions);
  gm_mgAddonOptions.mg_pCallbackFunction = &StartAddonsLoadMenu;
}

// [Cecil] Change to the menu
void COptionsMenu::ChangeTo(void) {
  _pGUIM->ChangeToMenu(&_pGUIM->gmOptionsMenu);
};
