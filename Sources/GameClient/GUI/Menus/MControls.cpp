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
#include "MControls.h"

extern CTFileName _fnmControlsToCustomize;

static BOOL LSLoadControls(CGameMenu *pgm, const CTString &fnm) {
  try {
    // [Cecil] Load user controls from user data
    CTString fnmControls = fnm;

    if (fnm.HasPrefix("Controls\\Controls")) {
      fnmControls.PrintF("Controls\\%s.ctl", fnm.FileName().ConstData());
      fnmControls = ExpandPath::ToUser(fnmControls);
    }

    ControlsMenuOn();
    _pGame->gm_ctrlControlsExtra.Load_t(fnmControls);
    ControlsMenuOff();

  } catch (char *strError) {
    CPutString(strError);
  }

  _pGUIM->MenuGoToParent();
  return TRUE;
};

static void StartControlsLoadMenu(void) {
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

void CControlsMenu::Initialize_t(void)
{
  // [Cecil] Create array of device slots
  const INDEX ctDeviceValues = _ctMaxInputDevices + 1;

  if (astrDeviceSlots == NULL) {
    astrDeviceSlots = new CTString[ctDeviceValues];
    astrDeviceSlots[0] = TRANS("Any slot");

    for (INDEX i = 1; i < ctDeviceValues; i++) {
      astrDeviceSlots[i].PrintF(TRANS("Slot %d"), i);
    }
  }

  // intialize player and controls menu
  gm_mgTitle.mg_boxOnScreen = BoxTitle();
  gm_mgTitle.SetText(TRANS("CONTROLS"));
  AddChild(&gm_mgTitle);

  gm_mgNameLabel.SetText("");
  gm_mgNameLabel.mg_boxOnScreen = BoxMediumRow(0.0);
  gm_mgNameLabel.mg_bfsFontSize = BFS_MEDIUM;
  gm_mgNameLabel.mg_iCenterI = -1;
  gm_mgNameLabel.mg_bEnabled = FALSE;
  gm_mgNameLabel.mg_bLabel = TRUE;
  AddChild(&gm_mgNameLabel);

  gm_mgButtons.SetText(TRANS("CUSTOMIZE BUTTONS"));
  gm_mgButtons.mg_boxOnScreen = BoxMediumRow(2.0);
  gm_mgButtons.mg_bfsFontSize = BFS_MEDIUM;
  gm_mgButtons.mg_iCenterI = 0;
  AddChild(&gm_mgButtons);
  gm_mgButtons.mg_pmgUp = &gm_mgPredefined;
  gm_mgButtons.mg_pmgDown = &gm_mgAdvanced;
  gm_mgButtons.mg_pCallbackFunction = &CCustomizeKeyboardMenu::ChangeTo;
  gm_mgButtons.mg_strTip = TRANS("customize buttons in current controls");

  gm_mgAdvanced.SetText(TRANS("ADVANCED JOYSTICK SETUP"));
  gm_mgAdvanced.mg_iCenterI = 0;
  gm_mgAdvanced.mg_boxOnScreen = BoxMediumRow(3);
  gm_mgAdvanced.mg_bfsFontSize = BFS_MEDIUM;
  AddChild(&gm_mgAdvanced);
  gm_mgAdvanced.mg_pmgUp = &gm_mgButtons;
  gm_mgAdvanced.mg_pmgDown = &gm_mgDeviceSlot;
  gm_mgAdvanced.mg_pCallbackFunction = &CCustomizeAxisMenu::ChangeTo;
  gm_mgAdvanced.mg_strTip = TRANS("adjust advanced settings for joystick axis");

  // [Cecil] Device slot selection
  TRIGGER_MG(gm_mgDeviceSlot, 4.5, gm_mgAdvanced, gm_mgSensitivity, TRANS("DEVICE SLOT"), astrDeviceSlots);
  gm_mgDeviceSlot.mg_ctTexts = ctDeviceValues; // [Cecil] Amount of device slot values
  gm_mgDeviceSlot.mg_strTip = TRANS("which slot to use when polling input from specific devices (for split screen)");

  gm_mgSensitivity.mg_boxOnScreen = BoxMediumRow(5.5);
  gm_mgSensitivity.SetText(TRANS("SENSITIVITY"));
  gm_mgSensitivity.mg_pmgUp = &gm_mgDeviceSlot;
  gm_mgSensitivity.mg_pmgDown = &gm_mgInvertTrigger;
  gm_mgSensitivity.mg_strTip = TRANS("sensitivity for all axis in this control set");
  AddChild(&gm_mgSensitivity);

  TRIGGER_MG(gm_mgInvertTrigger, 6.5, gm_mgSensitivity, gm_mgSmoothTrigger,
    TRANS("INVERT LOOK"), astrNoYes);
  gm_mgInvertTrigger.mg_strTip = TRANS("invert up/down looking");
  TRIGGER_MG(gm_mgSmoothTrigger, 7.5, gm_mgInvertTrigger, gm_mgAccelTrigger,
    TRANS("SMOOTH AXIS"), astrNoYes);
  gm_mgSmoothTrigger.mg_strTip = TRANS("smooth mouse/joystick movements");
  TRIGGER_MG(gm_mgAccelTrigger, 8.5, gm_mgSmoothTrigger, gm_mgIFeelTrigger,
    TRANS("MOUSE ACCELERATION"), astrNoYes);
  gm_mgAccelTrigger.mg_strTip = TRANS("allow mouse acceleration");
  TRIGGER_MG(gm_mgIFeelTrigger, 9.5, gm_mgAccelTrigger, gm_mgPredefined,
    TRANS("ENABLE IFEEL"), astrNoYes);
  gm_mgIFeelTrigger.mg_strTip = TRANS("enable support for iFeel tactile feedback mouse");

  gm_mgPredefined.SetText(TRANS("LOAD PREDEFINED SETTINGS"));
  gm_mgPredefined.mg_iCenterI = 0;
  gm_mgPredefined.mg_boxOnScreen = BoxMediumRow(11);
  gm_mgPredefined.mg_bfsFontSize = BFS_MEDIUM;
  AddChild(&gm_mgPredefined);
  gm_mgPredefined.mg_pmgUp = &gm_mgIFeelTrigger;
  gm_mgPredefined.mg_pmgDown = &gm_mgButtons;
  gm_mgPredefined.mg_pCallbackFunction = &StartControlsLoadMenu;
  gm_mgPredefined.mg_strTip = TRANS("load one of several predefined control settings");
}

void CControlsMenu::StartMenu(void)
{
  INDEX iPlayer = _pGame->gm_iSinglePlayer;
  if (_iLocalPlayer >= 0 && _iLocalPlayer < NET_MAXLOCALPLAYERS) {
    iPlayer = _pGame->gm_aiMenuLocalPlayers[_iLocalPlayer];
  }
  _fnmControlsToCustomize.PrintF("Controls\\Controls%d.ctl", iPlayer);
  _fnmControlsToCustomize = ExpandPath::ToUser(_fnmControlsToCustomize); // [Cecil] From user data

  ControlsMenuOn();

  gm_mgNameLabel.SetText(CTString(0, TRANS("CONTROLS FOR: %s"), _pGame->gm_apcPlayers[iPlayer].GetNameForPrinting().ConstData()));

  ObtainActionSettings();
  CGameMenu::StartMenu();
}

void CControlsMenu::EndMenu(void)
{
  ApplyActionSettings();

  ControlsMenuOff();

  CGameMenu::EndMenu();
}

void CControlsMenu::ObtainActionSettings(void)
{
  CControls &ctrls = _pGame->gm_ctrlControlsExtra;

  gm_mgSensitivity.mg_iMinPos = 0;
  gm_mgSensitivity.mg_iMaxPos = 50;
  gm_mgSensitivity.mg_iCurPos = ctrls.ctrl_fSensitivity / 2;
  gm_mgSensitivity.ApplyCurrentPosition();

  gm_mgInvertTrigger.mg_iSelected = ctrls.ctrl_bInvertLook ? 1 : 0;
  gm_mgSmoothTrigger.mg_iSelected = ctrls.ctrl_bSmoothAxes ? 1 : 0;
  gm_mgAccelTrigger.mg_iSelected = _pShell->GetINDEX("inp_bAllowMouseAcceleration") ? 1 : 0;
  gm_mgIFeelTrigger.mg_bEnabled = _pShell->GetINDEX("sys_bIFeelEnabled") ? 1 : 0;
  gm_mgIFeelTrigger.mg_iSelected = _pShell->GetFLOAT("inp_fIFeelGain")>0 ? 1 : 0;

  // [Cecil] Get device slot from the controls ("any" slot is -1 and the regular slots start from 0)
  gm_mgDeviceSlot.mg_iSelected = ctrls.ctrl_iDeviceSlot + 1;
  gm_mgDeviceSlot.ApplyCurrentSelection();

  gm_mgInvertTrigger.ApplyCurrentSelection();
  gm_mgSmoothTrigger.ApplyCurrentSelection();
  gm_mgAccelTrigger.ApplyCurrentSelection();
  gm_mgIFeelTrigger.ApplyCurrentSelection();
}

void CControlsMenu::ApplyActionSettings(void)
{
  CControls &ctrls = _pGame->gm_ctrlControlsExtra;

  // [Cecil] Set device slot for the controls ("any" slot is -1 and the regular slots start from 0)
  ctrls.ctrl_iDeviceSlot = gm_mgDeviceSlot.mg_iSelected - 1;

  FLOAT fSensitivity =
    FLOAT(gm_mgSensitivity.mg_iCurPos - gm_mgSensitivity.mg_iMinPos) /
    FLOAT(gm_mgSensitivity.mg_iMaxPos - gm_mgSensitivity.mg_iMinPos)*100.0f;

  BOOL bInvert = gm_mgInvertTrigger.mg_iSelected != 0;
  BOOL bSmooth = gm_mgSmoothTrigger.mg_iSelected != 0;
  BOOL bAccel = gm_mgAccelTrigger.mg_iSelected != 0;
  BOOL bIFeel = gm_mgIFeelTrigger.mg_iSelected != 0;

  if (INDEX(ctrls.ctrl_fSensitivity) != INDEX(fSensitivity)) {
    ctrls.ctrl_fSensitivity = fSensitivity;
  }
  ctrls.ctrl_bInvertLook = bInvert;
  ctrls.ctrl_bSmoothAxes = bSmooth;
  _pShell->SetINDEX("inp_bAllowMouseAcceleration", bAccel);
  _pShell->SetFLOAT("inp_fIFeelGain", bIFeel ? 1.0f : 0.0f);
  ctrls.CalculateInfluencesForAllAxis();
}

// [Cecil] Change to the menu
void CControlsMenu::ChangeTo(void) {
  _pGUIM->ChangeToMenu(&_pGUIM->gmControls);
};
