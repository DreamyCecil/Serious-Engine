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

#ifndef SE_INCL_GAME_MENU_CONFIRM_H
#define SE_INCL_GAME_MENU_CONFIRM_H
#ifdef PRAGMA_ONCE
  #pragma once
#endif

#include "GameMenu.h"
#include "GUI/Components/MGButton.h"


class CConfirmMenu : public CGameMenu {
public:
  typedef void (*CConfirmFunc)(void);

public:
  CMGButton gm_mgConfirmLabel;
  CMGButton gm_mgConfirmYes;
  CMGButton gm_mgConfirmNo;

  CConfirmFunc gm_pConfirmedYes;
  CConfirmFunc gm_pConfirmedNo;

  CConfirmMenu() : CGameMenu(),
    gm_pConfirmedYes(NULL), gm_pConfirmedNo(NULL)
  {
  };

  // [Cecil] Menu name for the mod interface
  virtual const char *GetName(void) const {
    return "Confirm";
  };

  // [Cecil] Selected gadget by default
  virtual CMenuGadget *GetDefaultGadget(void) {
    return &gm_mgConfirmYes;
  };

  void Initialize_t(void);
  // return TRUE if handled
  BOOL OnKeyDown(PressedMenuButton pmb);

  void BeLarge(void);
  void BeSmall(void);

  // [Cecil] Change to the menu
  static void ChangeTo(const CTString &strLabel, CConfirmFunc pFuncYes, CConfirmFunc pFuncNo, BOOL bBigLabel);
};

#endif  /* include-once check. */