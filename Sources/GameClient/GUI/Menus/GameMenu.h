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

#include "GUI/MenuElement.h"

class CMenuGadget;

class CGameMenu : public CAbstractMenuElement {
private:
  // [Cecil] Currently focused gadget in this menu (instead of managing CMenuGadget::mg_bFocused per gadget)
  CMenuGadget *gm_pmgFocused;

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

  // Constructor
  CGameMenu();

  // [Cecil] Destructor for derived menus
  virtual ~CGameMenu() {
    Destroy();
  };

  // [Cecil] This is a menu
  virtual bool IsMenu(void) const {
    return true;
  };

  // [Cecil] Menu name for the mod interface (used to be a gm_strName field)
  virtual const char *GetName(void) const = 0;

  // [Cecil] Selected gadget by default (used to be a gm_pmgSelectedByDefault field)
  virtual CMenuGadget *GetDefaultGadget(void) = 0;

  // [Cecil] Get currently focused gadget
  inline CMenuGadget *GetFocused(void) {
    return gm_pmgFocused;
  };

  // [Cecil] Focus on a specific gadget
  void FocusGadget(CMenuGadget *pmg);

  // [Cecil] Reset focus from specific gadget
  void UnfocusGadget(CMenuGadget *pmg);

  // [Cecil] Reset focus from all gadgets (similar to the old KillAllFocuses() function)
  void ResetGadgetFocus(void);

  // [Cecil] Find gadget in a list by its index
  CMenuGadget *FindListGadget(INDEX iInList);

  // [Cecil] Retrieve the last possible menu in the current hierarchy
  CGameMenu *GetLastMenu(void);

  void ScrollList(INDEX iDir);

  virtual void Initialize_t(void) {};
  virtual void Destroy(void) {};
  virtual void Think(void) {};

  virtual void FillListItems(void) {
    ASSERTALWAYS("FillListItems() isn't defined for this menu!");
  };

  // return TRUE if handled
  virtual BOOL OnKeyDown(PressedMenuButton pmb);
  virtual BOOL OnChar(const OS::SE1Event &event);

  virtual void StartMenu(void);
  virtual void EndMenu(void);

  // [Cecil] Reload this menu, if needed
  inline void ReloadMenu(void) {
    EndMenu();
    StartMenu();
  };

  // [Cecil] Render the menu in its entirety and optionally find a gadget under the cursor
  void Render(CDrawPort *pdp, CMenuGadget **ppmgUnderCursor);
};

#endif  /* include-once check. */
