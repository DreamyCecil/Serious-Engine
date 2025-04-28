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

#include "MenuStuff.h"

#define RADIOTRANS(str) ("ETRS" str)

CTString astrNoYes[] = {
  RADIOTRANS("No"),
  RADIOTRANS("Yes"),
};

CTString astrComputerInvoke[] = {
  RADIOTRANS("Use"),
  RADIOTRANS("Double-click use"),
};

CTString astrWeapon[] = {
  RADIOTRANS("Only if new"),
  RADIOTRANS("Never"),
  RADIOTRANS("Always"),
  RADIOTRANS("Only if stronger"),
};

CTString astrCrosshair[] = {
  "",
  "Textures\\Interface\\Crosshairs\\Crosshair1.tex",
  "Textures\\Interface\\Crosshairs\\Crosshair2.tex",
  "Textures\\Interface\\Crosshairs\\Crosshair3.tex",
  "Textures\\Interface\\Crosshairs\\Crosshair4.tex",
  "Textures\\Interface\\Crosshairs\\Crosshair5.tex",
  "Textures\\Interface\\Crosshairs\\Crosshair6.tex",
  "Textures\\Interface\\Crosshairs\\Crosshair7.tex",
};

CTString *astrDeviceSlots = NULL; // [Cecil]

CTString astrMaxPlayersRadioTexts[] = {
  RADIOTRANS("2"),
  RADIOTRANS("3"),
  RADIOTRANS("4"),
  RADIOTRANS("5"),
  RADIOTRANS("6"),
  RADIOTRANS("7"),
  RADIOTRANS("8"),
  RADIOTRANS("9"),
  RADIOTRANS("10"),
  RADIOTRANS("11"),
  RADIOTRANS("12"),
  RADIOTRANS("13"),
  RADIOTRANS("14"),
  RADIOTRANS("15"),
  RADIOTRANS("16"),
};
// here, we just reserve space for up to 16 different game types
// actual names are added later
CTString astrGameTypeRadioTexts[] = {
  "", "", "", "", "",
  "", "", "", "", "",
  "", "", "", "", "",
  "", "", "", "", "",
};

INDEX ctGameTypeRadioTexts = 1;

CTString astrDifficultyRadioTexts[] = {
  RADIOTRANS("Tourist"),
  RADIOTRANS("Easy"),
  RADIOTRANS("Normal"),
  RADIOTRANS("Hard"),
  RADIOTRANS("Serious"),
  RADIOTRANS("Mental"),
};

// [Cecil] Dynamically created array
CTString *astrSplitScreenRadioTexts = NULL;

CTString astrDisplayPrefsRadioTexts[] = {
  RADIOTRANS("Speed"),
  RADIOTRANS("Normal"),
  RADIOTRANS("Quality"),
  RADIOTRANS("Custom"),
};

// [Cecil] Dynamically created array
CTString *astrDisplayAPIRadioTexts = NULL;

CTString astrBitsPerPixelRadioTexts[] = {
  RADIOTRANS("Desktop"),
  RADIOTRANS("16 BPP"),
  RADIOTRANS("32 BPP"),
};

CTString astrFrequencyRadioTexts[] = {
  RADIOTRANS("No sound"),
  RADIOTRANS("11kHz"),
  RADIOTRANS("22kHz"),
  RADIOTRANS("44kHz"),
};

// [Cecil] Dynamically created array
CTString *astrSoundAPIRadioTexts = NULL;

// initialize game type strings table
void InitGameTypes(void)
{
  // for each mode
  for (ctGameTypeRadioTexts = 0; ctGameTypeRadioTexts<ARRAYCOUNT(astrGameTypeRadioTexts); ctGameTypeRadioTexts++) {
    // get the text
    CTString strMode = GetGameTypeName(ctGameTypeRadioTexts);
    // if no mode modes
    if (strMode == "") {
      // stop
      break;
    }
    // add that mode
    astrGameTypeRadioTexts[ctGameTypeRadioTexts] = strMode;
  }
}

int qsort_CompareFileInfos_NameUp(const void *elem1, const void *elem2)
{
  const CFileInfo &fi1 = **(CFileInfo **)elem1;
  const CFileInfo &fi2 = **(CFileInfo **)elem2;
  return strcmp(fi1.fi_strName.ConstData(), fi2.fi_strName.ConstData());
}

int qsort_CompareFileInfos_NameDn(const void *elem1, const void *elem2)
{
  const CFileInfo &fi1 = **(CFileInfo **)elem1;
  const CFileInfo &fi2 = **(CFileInfo **)elem2;
  return -strcmp(fi1.fi_strName.ConstData(), fi2.fi_strName.ConstData());
}

int qsort_CompareFileInfos_FileUp(const void *elem1, const void *elem2)
{
  const CFileInfo &fi1 = **(CFileInfo **)elem1;
  const CFileInfo &fi2 = **(CFileInfo **)elem2;
  return strcmp(fi1.fi_fnFile.ConstData(), fi2.fi_fnFile.ConstData());
}

int qsort_CompareFileInfos_FileDn(const void *elem1, const void *elem2)
{
  const CFileInfo &fi1 = **(CFileInfo **)elem1;
  const CFileInfo &fi2 = **(CFileInfo **)elem2;
  return -strcmp(fi1.fi_fnFile.ConstData(), fi2.fi_fnFile.ConstData());
}

INDEX APIToSwitch(enum GfxAPIType gat)
{
  switch (gat) {
  case GAT_OGL: return 0;
#if SE1_DIRECT3D
  case GAT_D3D: return 1;
#endif // SE1_DIRECT3D
  default: ASSERT(FALSE); return 0;
  }
}

enum GfxAPIType SwitchToAPI(INDEX i)
{
  switch (i) {
  case 0: return GAT_OGL;
#if SE1_DIRECT3D
  case 1: return GAT_D3D;
#endif // SE1_DIRECT3D
  default: ASSERT(FALSE); return GAT_OGL;
  }
}

INDEX DepthToSwitch(enum DisplayDepth dd)
{
  switch (dd) {
  case DD_DEFAULT: return 0;
  case DD_16BIT: return 1;
  case DD_32BIT: return 2;
  default: ASSERT(FALSE); return 0;
  }
}

enum DisplayDepth SwitchToDepth(INDEX i)
{
  switch (i) {
  case 0: return DD_DEFAULT;
  case 1: return DD_16BIT;
  case 2: return DD_32BIT;
  default: ASSERT(FALSE); return DD_DEFAULT;
  }
}

// controls that are currently customized
CTFileName _fnmControlsToCustomize = CTString("");

void ControlsMenuOn()
{
  _pGame->SavePlayersAndControls();
  try {
    _pGame->gm_ctrlControlsExtra.Load_t(_fnmControlsToCustomize);
  }
  catch (char *strError) {
    WarningMessage(strError);
  }
}

void ControlsMenuOff()
{
  try {
    if (_pGame->gm_ctrlControlsExtra.ctrl_lhButtonActions.Count()>0) {
      _pGame->gm_ctrlControlsExtra.Save_t(_fnmControlsToCustomize);
    }
  }
  catch (char *strError) {
    FatalError(strError);
  }
  FORDELETELIST(CButtonAction, ba_lnNode, _pGame->gm_ctrlControlsExtra.ctrl_lhButtonActions, itAct) {
    delete &itAct.Current();
  }
  _pGame->LoadPlayersAndControls();
}
