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

#ifndef SE_INCL_MENU_GADGET_H
#define SE_INCL_MENU_GADGET_H
#ifdef PRAGMA_ONCE
  #pragma once
#endif

#include "GUI/MenuElement.h"
#include "GUI/Menus/MenuPrinting.h"

#define DOING_NOTHING 0
#define PRESS_KEY_WAITING 1
#define RELEASE_RETURN_WAITING 2

#define EMPTYSLOTSTRING TRANS("<save a new one>")

class CGameMenu;

class CMenuGadget : public CAbstractMenuElement {
public:
  FLOATaabbox2D mg_boxOnScreen;
  BOOL mg_bVisible;
  BOOL mg_bEnabled;
  BOOL mg_bLabel;
  INDEX mg_iInList; // for scrollable gadget lists

  CTString mg_strTip;
  CMenuGadget *mg_pmgLeft;
  CMenuGadget *mg_pmgRight;
  CMenuGadget *mg_pmgUp;
  CMenuGadget *mg_pmgDown;

  // Constructor
  CMenuGadget();

  // [Cecil] This is not a menu
  virtual bool IsMenu(void) const {
    return false;
  };

  // [Cecil] Get parent menu
  inline CGameMenu *GetParentMenu(void) const {
    CAbstractMenuElement *pParent = GetParentElement();
    if (pParent == NULL) return NULL;

    ASSERT(pParent->IsMenu());
    return (CGameMenu *)pParent;
  };

  // [Cecil] Check if the gadget is currently focused
  BOOL IsFocused(void);

  // [Cecil] Get gadget name/label
  virtual const CTString &GetName(void) const {
    ASSERTALWAYS("CMenuGadget::GetName() isn't redefined for this gadget!");
    return me_strDummyString;
  };

  // [Cecil] Set gadget name/label
  virtual void SetName(const CTString &strNew) {
    ASSERTALWAYS("CMenuGadget::SetName() isn't redefined for this gadget!");
  };

  // [Cecil] Get gadget text
  virtual const CTString &GetText(void) const {
    ASSERTALWAYS("CMenuGadget::GetText() isn't redefined for this gadget!");
    return me_strDummyString;
  };

  // [Cecil] Set gadget text
  virtual void SetText(const CTString &strNew) {
    ASSERTALWAYS("CMenuGadget::SetText() isn't redefined for this gadget!");
  };

  // return TRUE if handled
  virtual BOOL OnKeyDown(PressedMenuButton pmb);
  virtual BOOL OnChar(const OS::SE1Event &event) { return FALSE; };
  virtual void OnActivate(void) {};

  // [Cecil] NOTE: These callbacks are to be called *only* from FocusGadget(), UnfocusGadget() & ResetGadgetFocus() of CGameMenu!
  virtual void OnSetFocus(void);
  virtual void OnKillFocus(void) {};

  virtual void Appear(void);
  virtual void Disappear(void);
  virtual void Think(void) {};
  virtual void OnMouseOver(PIX pixI, PIX pixJ) {};

  // [Cecil] Manual flag for whether the color is of a focused gadget or not
  COLOR GetCurrentColor(BOOL bFocused);

  // [Cecil] Wrapper for convenience
  inline COLOR GetCurrentColor(void) {
    return GetCurrentColor(IsFocused());
  };

  virtual void Render(CDrawPort *pdp) {};
  virtual BOOL IsSeparator(void) { return FALSE; };
};

enum ButtonFontSize {
  BFS_SMALL = 0,
  BFS_MEDIUM = 1,
  BFS_LARGE = 2,
};

#endif  /* include-once check. */
