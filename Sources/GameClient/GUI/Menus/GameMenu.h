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

#ifndef SE_INCL_GAME_MENU_H
#define SE_INCL_GAME_MENU_H
#ifdef PRAGMA_ONCE
  #pragma once
#endif

#include <Engine/Base/LinkedNode.h>

class CMenuGadget;

class CGameMenu : public CLinkedNode {
public:
  BOOL gm_bPopup;
  CMenuGadget *gm_pmgArrowUp;
  CMenuGadget *gm_pmgArrowDn;
  CMenuGadget *gm_pmgListTop;
  CMenuGadget *gm_pmgListBottom;
  INDEX gm_iListOffset;
  INDEX gm_iListWantedItem;   // item you want to focus initially
  INDEX gm_ctListVisible;
  INDEX gm_ctListTotal;
  CGameMenu(void);

  // [Cecil] Menu name for the mod interface (used to be a gm_strName field)
  virtual const char *GetName(void) const = 0;

  // [Cecil] Selected gadget by default (used to be a gm_pmgSelectedByDefault field)
  virtual CMenuGadget *GetDefaultGadget(void) = 0;

  // [Cecil] Find gadget in a list by its index
  CMenuGadget *FindListGadget(INDEX iInList);

  void ScrollList(INDEX iDir);
  void KillAllFocuses(void);
  virtual void Initialize_t(void);
  virtual void Destroy(void);
  virtual void StartMenu(void);
  virtual void FillListItems(void);
  virtual void EndMenu(void);
  // return TRUE if handled
  virtual BOOL OnKeyDown(PressedMenuButton pmb);
  virtual BOOL OnChar(const OS::SE1Event &event);
  virtual void Think(void);
};

#endif  /* include-once check. */