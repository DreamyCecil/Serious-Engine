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

/*
 *  Game library
 *  Copyright (c) 1997-1999, CroTeam. 
 */

#include "StdAfx.h"

/*
 * Default constructor
 */
CControls::CControls(void)
{
  // buttons are all none anyway (empty list)
  // switch axes actions to defaults
  SwitchAxesToDefaults();
}

/*
 * Default destructor
 */
CControls::~CControls(void)
{
  // for each action in the original list
  FORDELETELIST(CButtonAction, ba_lnNode, ctrl_lhButtonActions, itButtonAction)
  {
    // delete button action
    delete( &*itButtonAction);
  }
}

// Assignment operator.
CControls &CControls::operator=(CControls &ctrlOriginal)
{
  // remove old button actions
  {FORDELETELIST(CButtonAction, ba_lnNode, ctrl_lhButtonActions, itButtonAction) {
    delete &*itButtonAction;
  }}
  // for each action in the original list
  {FOREACHINLIST(CButtonAction, ba_lnNode, ctrlOriginal.ctrl_lhButtonActions, itButtonAction) {
    // create and copy button action
    AddButtonAction() = *itButtonAction;
  }}

  // for all axis-type actions
  for( INDEX iAxisAction=0; iAxisAction<AXIS_ACTIONS_CT; iAxisAction++) {
    // copy it
    ctrl_aaAxisActions[iAxisAction] = ctrlOriginal.ctrl_aaAxisActions[iAxisAction];
  }

  // copy global settings
  ctrl_fSensitivity  = ctrlOriginal.ctrl_fSensitivity;
  ctrl_bInvertLook   = ctrlOriginal.ctrl_bInvertLook;
  ctrl_bSmoothAxes   = ctrlOriginal.ctrl_bSmoothAxes;
  ctrl_iDeviceSlot   = ctrlOriginal.ctrl_iDeviceSlot; // [Cecil]

  return *this;
}

/*
 * Switches button and axis action mounters to defaults
 */
void CControls::SwitchAxesToDefaults(void)
{
  // for all axis-type actions
  for( INDEX iAxisAction=0; iAxisAction<AXIS_ACTIONS_CT; iAxisAction++)
  {
    // set axis action none
    ctrl_aaAxisActions[ iAxisAction].aa_iAxisAction = EIA_NONE;
    ctrl_aaAxisActions[ iAxisAction].aa_fSensitivity = 50;
    ctrl_aaAxisActions[ iAxisAction].aa_fDeadZone = 0;
    ctrl_aaAxisActions[ iAxisAction].aa_bInvert = FALSE;
    ctrl_aaAxisActions[ iAxisAction].aa_bRelativeControler = TRUE;
    ctrl_aaAxisActions[ iAxisAction].aa_bSmooth = FALSE;
    ctrl_aaAxisActions[ iAxisAction].aa_fLastReading = 0.0f;
  }
  // set default controlers for some axis-type actions
  // mouse left/right motion
  ctrl_aaAxisActions[ AXIS_TURN_LR].aa_iAxisAction = EIA_MOUSE_X;
  ctrl_aaAxisActions[ AXIS_TURN_LR].aa_fSensitivity = 45;
  ctrl_aaAxisActions[ AXIS_TURN_LR].aa_bInvert = FALSE;
  ctrl_aaAxisActions[ AXIS_TURN_LR].aa_bRelativeControler = TRUE;
  // mouse up/down motion
  ctrl_aaAxisActions[ AXIS_TURN_UD].aa_iAxisAction = EIA_MOUSE_Y;
  ctrl_aaAxisActions[ AXIS_TURN_UD].aa_fSensitivity = 45;
  ctrl_aaAxisActions[ AXIS_TURN_UD].aa_bInvert = TRUE;
  ctrl_aaAxisActions[ AXIS_TURN_UD].aa_bRelativeControler = TRUE;

  ctrl_fSensitivity = 50;
  ctrl_bInvertLook = FALSE;
  ctrl_bSmoothAxes = TRUE;
  ctrl_iDeviceSlot = -1; // [Cecil] Any slot
}

void CControls::SwitchToDefaults(void)
{
  // copy controls from initial player
  try
  {
    CControls ctrlDefaultControls;
    ctrlDefaultControls.Load_t( CTString("Data\\Defaults\\InitialControls.ctl"));
    *this = ctrlDefaultControls;
  }
  catch( char *strError)
  {
    (void) strError;
  }
}

/*
 * Depending on axis attributes and type (rotation or translation),
 * calculates axis influence factors for all axis actions
 */
void CControls::CalculateInfluencesForAllAxis(void)
{
  FLOAT fSensitivityGlobal = (FLOAT)pow(1.2, (ctrl_fSensitivity-50.0)*1.0/5.0);
  FLOAT fBaseDelta; // different for rotations and translations
  // for all axis actions
  for( INDEX iAxisAction=0; iAxisAction<AXIS_ACTIONS_CT; iAxisAction++)
  {
    fBaseDelta = 1.0f;
    // apply invert factor
    if( ctrl_aaAxisActions[ iAxisAction].aa_bInvert || 
      ( (iAxisAction==AXIS_TURN_UD||iAxisAction==AXIS_LOOK_UD) && ctrl_bInvertLook) ) {
      // negative factor
      fBaseDelta = -fBaseDelta;
    }

    FLOAT fSensitivityLocal = (FLOAT)pow(2.0, (ctrl_aaAxisActions[ iAxisAction].aa_fSensitivity-50.0)*1.0/5.0);

    // calculate influence for current axis
    ctrl_aaAxisActions[ iAxisAction].aa_fAxisInfluence = fBaseDelta * fSensitivityGlobal * fSensitivityLocal;
  }
}

CTString ReadTextLine(CTStream &strm, const CTString &strKeyword, BOOL bTranslate)
{
  CTString strLine;
  strm.GetLine_t(strLine);
  strLine.TrimSpacesLeft();
  if (!strLine.RemovePrefix(strKeyword)) {
    return "???";
  }
  strLine.TrimSpacesLeft();
  if (bTranslate) {
    strLine.RemovePrefix("TTRS");
  }
  strLine.TrimSpacesLeft();
  strLine.TrimSpacesRight();

  return strLine;
}

void CControls::Load_t( CTFileName fnFile)
{
  char achrLine[ 1024];
  char achrName[ 1024];
  char achrID[ 1024];
  char achrActionName[ 1024];
  
  // open script file for reading
  CTFileStream strmFile;
  strmFile.Open_t( fnFile);				

  // if file can be opened for reading remove old button actions
  {FORDELETELIST(CButtonAction, ba_lnNode, ctrl_lhButtonActions, itButtonAction) {
    delete &*itButtonAction;
  }}

  do
  {
    achrLine[0] = 0;
    achrID[0] = 0;
    strmFile.GetLine_t( achrLine, 1024);
    sscanf( achrLine, "%s", achrID);

    const CTString strProperty = achrID;

    // if name
    if (strProperty == "Name") {
      // name is obsolete, just skip it
      sscanf( achrLine, "%*[^\"]\"%1024[^\"]\"", achrName);
    
    // if this is button action
    } else if (strProperty == "Button") {
      // create and read button action
      CButtonAction &baNew = AddButtonAction();
      baNew.ba_strName = ReadTextLine(strmFile, "Name:", TRUE);

      baNew.ba_iFirstKey = _pInput->FindActionByName(ReadTextLine(strmFile, "Key1:", FALSE));
      baNew.ba_iSecondKey = _pInput->FindActionByName(ReadTextLine(strmFile, "Key2:", FALSE));

      baNew.ba_strCommandLineWhenPressed = ReadTextLine(strmFile, "Pressed:", FALSE);
      baNew.ba_strCommandLineWhenReleased = ReadTextLine(strmFile, "Released:", FALSE);

    // if this is axis action
    } else if (strProperty == "Axis") {
      char achrAxis[ 1024];
      achrAxis[ 0] = 0;
      char achrIfInverted[ 1024];
      achrIfInverted[ 0] = 0;
      char achrIfRelative[ 1024];
      achrIfRelative[ 0] = 0;
      achrActionName[ 0] = 0;
      FLOAT fSensitivity = 50;
      FLOAT fDeadZone = 0;
      sscanf( achrLine, "%*[^\"]\"%1024[^\"]\"%*[^\"]\"%1024[^\"]\" %g %g %1024s %1024s",
              achrActionName, achrAxis, &fSensitivity, &fDeadZone, achrIfInverted, achrIfRelative);

      // find action axis
      INDEX iActionAxisNo = -1;

      for (INDEX iAxis = 0; iAxis < AXIS_ACTIONS_CT; iAxis++) {
        if (_pGame->gm_astrAxisNames[iAxis] == achrActionName) {
          iActionAxisNo = iAxis;
          break;
        }
      }

      // find controller axis
      EInputAxis eCtrlAxis = _pInput->FindAxisByName(achrAxis);

      // if valid axis found
      if (iActionAxisNo != -1) {
        // set it
        ctrl_aaAxisActions[ iActionAxisNo].aa_iAxisAction = eCtrlAxis;
        ctrl_aaAxisActions[ iActionAxisNo].aa_fSensitivity = fSensitivity;
        ctrl_aaAxisActions[ iActionAxisNo].aa_fDeadZone = fDeadZone;
        ctrl_aaAxisActions[ iActionAxisNo].aa_bInvert = ( CTString( "Inverted") == achrIfInverted);
        ctrl_aaAxisActions[ iActionAxisNo].aa_bRelativeControler = ( CTString( "Relative") == achrIfRelative);
        ctrl_aaAxisActions[ iActionAxisNo].aa_bSmooth = ( CTString( "Smooth") == achrIfRelative);
      }
    // read global parameters
    } else if (strProperty == "GlobalInvertLook") {
      ctrl_bInvertLook = TRUE;
    } else if (strProperty == "GlobalDontInvertLook") {
      ctrl_bInvertLook = FALSE;
    } else if (strProperty == "GlobalSmoothAxes") {
      ctrl_bSmoothAxes = TRUE;
    } else if (strProperty == "GlobalDontSmoothAxes") {
      ctrl_bSmoothAxes = FALSE;
    } else if (strProperty == "GlobalSensitivity") {
      sscanf(achrLine, "GlobalSensitivity %g", &ctrl_fSensitivity);

    // [Cecil] Read device slot
    } else if (strProperty == "DeviceSlot") {
      sscanf(achrLine, "DeviceSlot %d", &ctrl_iDeviceSlot);
      ctrl_iDeviceSlot = Clamp(ctrl_iDeviceSlot, -1, 7);
    }
  }
  while( !strmFile.AtEOF());

/*
  // search for talk button
  BOOL bHasTalk = FALSE;
  BOOL bHasT = FALSE;
  FOREACHINLIST( CButtonAction, ba_lnNode, ctrl_lhButtonActions, itba) {
    CButtonAction &ba = *itba;
    if (ba.ba_strName=="Talk") {
      bHasTalk = TRUE;
    }
    if (ba.ba_iFirstKey==KID_T) {
      bHasT = TRUE;
    }
    if (ba.ba_iSecondKey==KID_T) {
      bHasT = TRUE;
    }
  }
  // if talk button not found
  if (!bHasTalk) {
    // add it
    CButtonAction &baNew = AddButtonAction();
    baNew.ba_strName = "Talk";
    baNew.ba_iFirstKey = KID_NONE;
    baNew.ba_iSecondKey = KID_NONE;
    baNew.ba_strCommandLineWhenPressed = "  con_bTalk=1;";
    baNew.ba_strCommandLineWhenReleased = "";
    // if T key is not bound to anything
    if (!bHasT) {
      // bind it to talk
      baNew.ba_iFirstKey = KID_T;
    // if we couldn't bind it
    } else {
      // put it to the top of the list
      baNew.ba_lnNode.Remove();
      ctrl_lhButtonActions.AddHead(baNew.ba_lnNode);
    }
  }
  */

  CalculateInfluencesForAllAxis();
}

void CControls::Save_t( CTFileName fnFile)
{
  CTString strLine;
  // create file
  CTFileStream strmFile;
  strmFile.Create_t(fnFile);

  // write button actions
  FOREACHINLIST( CButtonAction, ba_lnNode, ctrl_lhButtonActions, itba)
  {
    strLine.PrintF("Button\n Name: TTRS %s\n Key1: %s\n Key2: %s",
      itba->ba_strName.ConstData(),
      _pInput->GetButtonName(itba->ba_iFirstKey).ConstData(),
      _pInput->GetButtonName(itba->ba_iSecondKey).ConstData());
    strmFile.PutLine_t(strLine.ConstData());

    // export pressed command
    strLine.PrintF(" Pressed:  %s", itba->ba_strCommandLineWhenPressed.ConstData());
    {for( INDEX iLetter = 0; strLine[ iLetter] != 0; iLetter++)
    {
      // delete EOL-s
      if( (strLine[ iLetter] == 0x0d) || (strLine[ iLetter] == 0x0a) )
      {
        strLine.Data()[iLetter] = ' ';
      }
    }}
    strmFile.PutLine_t(strLine.ConstData());

    // export released command
    strLine.PrintF(" Released: %s", itba->ba_strCommandLineWhenReleased.ConstData());
    {for( INDEX iLetter = 0; strLine[ iLetter] != 0; iLetter++)
    {
      // delete EOL-s
      if( (strLine[ iLetter] == 0x0d) || (strLine[ iLetter] == 0x0a) )
      {
        strLine.Data()[iLetter] = ' ';
      }
    }}
    strmFile.PutLine_t(strLine.ConstData());
  }
  // write axis actions
  for( INDEX iAxis=0; iAxis<AXIS_ACTIONS_CT; iAxis++)
  {
    CTString strIfInverted;
    CTString strIfRelative;
    CTString strIfSmooth;

    if( ctrl_aaAxisActions[iAxis].aa_bInvert) {
      strIfInverted = "Inverted";
    } else {
      strIfInverted = "NotInverted";
    }
    if( ctrl_aaAxisActions[iAxis].aa_bRelativeControler) {
      strIfRelative = "Relative";
    } else {
      strIfRelative = "Absolute";
    }
    if( ctrl_aaAxisActions[iAxis].aa_bSmooth) {
      strIfSmooth = "Smooth";
    } else {
      strIfSmooth = "NotSmooth";
    }
    

    strLine.PrintF("Axis \"%s\" \"%s\" %g %g %s %s %s",
      _pGame->gm_astrAxisNames[iAxis].ConstData(), 
      _pInput->GetAxisName(ctrl_aaAxisActions[iAxis].aa_iAxisAction).ConstData(),
      ctrl_aaAxisActions[ iAxis].aa_fSensitivity,
      ctrl_aaAxisActions[ iAxis].aa_fDeadZone,
      strIfInverted.ConstData(),
      strIfRelative.ConstData(),
      strIfSmooth.ConstData());
    strmFile.PutLine_t(strLine.ConstData());
  }

  // write global parameters
  if (ctrl_bInvertLook) {
    strmFile.PutLine_t( "GlobalInvertLook");
  } else {
    strmFile.PutLine_t( "GlobalDontInvertLook");
  }
  if (ctrl_bSmoothAxes) {
    strmFile.PutLine_t( "GlobalSmoothAxes");
  } else {
    strmFile.PutLine_t( "GlobalDontSmoothAxes");
  }
  strmFile.FPrintF_t("GlobalSensitivity %g\n", ctrl_fSensitivity);

  // [Cecil] Write device slot
  strmFile.FPrintF_t("DeviceSlot %d\n", (INDEX)Clamp(ctrl_iDeviceSlot, -1, 7));
}

// check if these controls use any joystick
BOOL CControls::UsesJoystick(void) {
  // Buttons
  FOREACHINLIST(CButtonAction, ba_lnNode, ctrl_lhButtonActions, itba) {
    CButtonAction &ba = *itba;

    if ((ba.ba_iFirstKey  >= KID_FIRST_GAMEPAD && ba.ba_iFirstKey  <= KID_LAST_GAMEPAD)
     || (ba.ba_iSecondKey >= KID_FIRST_GAMEPAD && ba.ba_iSecondKey <= KID_LAST_GAMEPAD)) {
      return TRUE;
    }
  }

  // Axes
  for (INDEX iAxis = 0; iAxis < AXIS_ACTIONS_CT; iAxis++) {
    if (ctrl_aaAxisActions[iAxis].aa_iAxisAction > EIA_CONTROLLER_OFFSET
     && ctrl_aaAxisActions[iAxis].aa_iAxisAction < EIA_MAX_ALL) {
      return TRUE;
    }
  }

  return FALSE;
}

// [Cecil] Check if these controls use any mouse
BOOL CControls::UsesMouse(void) {
  // Buttons
  FOREACHINLIST(CButtonAction, ba_lnNode, ctrl_lhButtonActions, itba) {
    CButtonAction &ba = *itba;

    if ((ba.ba_iFirstKey  >= KID_FIRST_MOUSE && ba.ba_iFirstKey  <= KID_LAST_MOUSE)
     || (ba.ba_iSecondKey >= KID_FIRST_MOUSE && ba.ba_iSecondKey <= KID_LAST_MOUSE)) {
      return TRUE;
    }
  }

  // Axes
  for (INDEX iAxis = 0; iAxis < AXIS_ACTIONS_CT; iAxis++) {
    if (ctrl_aaAxisActions[iAxis].aa_iAxisAction > EIA_NONE
     && ctrl_aaAxisActions[iAxis].aa_iAxisAction < EIA_MAX_MOUSE) {
      return TRUE;
    }
  }

  return FALSE;
};
