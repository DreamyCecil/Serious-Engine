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

#ifndef SE_INCL_GAME_MENU_LOADSAVE_H
#define SE_INCL_GAME_MENU_LOADSAVE_H
#ifdef PRAGMA_ONCE
  #pragma once
#endif

#include "GameMenu.h"
#include "GUI/Components/MGArrow.h"
#include "GUI/Components/MGButton.h"
#include "GUI/Components/MGFileButton.h"
#include "GUI/Components/MGTitle.h"

#define SAVELOAD_BUTTONS_CT 14

enum ELSSortType
{
  LSSORT_NONE,
  LSSORT_NAMEUP,
  LSSORT_NAMEDN,
  LSSORT_FILEUP,
  LSSORT_FILEDN,
};

class CLoadSaveMenu : public CGameMenu {
public:
  typedef BOOL (*FAfterChoosing)(CGameMenu *pgm, const CTString &fnm);

public:
  // settings adjusted before starting the menu
  CTFileName gm_fnmSelected;  // file that is selected initially
  CTFileName gm_fnmDirectory; // directory that should be read
  CTFileName gm_fnmBaseName;  // base file name for saving (numbers are auto-added)
  CTFileName gm_fnmExt;       // accepted file extension
  BOOL gm_bSave;              // set when chosing file for saving
  BOOL gm_bManage;            // set if managing (rename/delet is enabled)
  CTString gm_strSaveDes;     // default description (if saving)
  BOOL gm_bAllowThumbnails;   // set when chosing file for saving

  INDEX gm_iSortType;    // sort type

  // function to activate when file is chosen
  // return true if saving succeeded - description is saved automatically
  // always return true for loading
  FAfterChoosing gm_pAfterFileChosen;

  // internal properties
  CListHead gm_lhFileInfos;   // all file infos to list
  INDEX gm_iLastFile;         // index of last saved file in numbered format

  CMGTitle gm_mgTitle;
  CMGButton gm_mgNotes;
  CMGFileButton gm_amgButton[SAVELOAD_BUTTONS_CT];
  CMGArrow gm_mgArrowUp;
  CMGArrow gm_mgArrowDn;

  // [Cecil] Menu name for the mod interface
  virtual const char *GetName(void) const {
    return "LoadSave";
  };

  // [Cecil] Selected gadget by default
  virtual CMenuGadget *GetDefaultGadget(void) {
    CMenuGadget *pmg = FindListGadget(gm_iListWantedItem);
    return (pmg != NULL ? pmg : &gm_amgButton[0]);
  };

  // called to get info of a file from directory, or to skip it
  BOOL ParseFile(const CTFileName &fnm, CTString &strName);

  void Initialize_t(void);
  virtual void OnStart(void);
  virtual void OnEnd(void);
  void FillListItems(void);

  // [Cecil] Change to a generic list of files
  static void ChangeToFiles(const CTString &strTitle, ELSSortType eSorting, BOOL bThumbnails,
    const CTString &strDir, const CTString &strExt, const CTString &strSelected, const CTString &strNotes,
    CLoadSaveMenu::FAfterChoosing pCallback);

  // [Cecil] Change to a list of demos
  static void ChangeToDemos(BOOL bRecord, ELSSortType eSorting,
    const CTString &strSaveDes, CLoadSaveMenu::FAfterChoosing pCallback);

  // [Cecil] Change to saving a game
  static void ChangeToSave(ELSSortType eSorting, const CTString &strDir, const CTString &strBase,
    const CTString &strSaveDes, const CTString &strNotes, CLoadSaveMenu::FAfterChoosing pCallback);

  // [Cecil] Change to loading a game
  static void ChangeToLoad(BOOL bQuick, ELSSortType eSorting, const CTString &strDir,
    const CTString &strNotes, CLoadSaveMenu::FAfterChoosing pCallback);
};

#endif  /* include-once check. */
