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
#include "GameMenu.h"

CGameMenu::CGameMenu(void) {
  gm_pmgFocused = NULL;
  gm_bActive = false;
  gm_boxSubArea = FLOATaabbox2D(FLOAT2D(0, 0), FLOAT2D(1, 1));

  gm_pmgArrowUp = NULL;
  gm_pmgArrowDn = NULL;
  gm_pmgListTop = NULL;
  gm_pmgListBottom = NULL;
  gm_iListOffset = 0;
  gm_ctListVisible = 0;
  gm_ctListTotal = 0;
}

// [Cecil] Focus a specific gadget
void CGameMenu::FocusGadget(CMenuGadget *pmg) {
  // Already focused
  if (gm_pmgFocused == pmg) return;

  if (gm_pmgFocused != NULL) {
    gm_pmgFocused->OnKillFocus();
  }

  gm_pmgFocused = pmg;
  gm_pmgFocused->OnSetFocus();
};

// [Cecil] Reset focus from specific gadget
void CGameMenu::UnfocusGadget(CMenuGadget *pmg) {
  // Already unfocused
  if (gm_pmgFocused != pmg) return;

  gm_pmgFocused->OnKillFocus();
  gm_pmgFocused = NULL;
};

// [Cecil] Reset focus from all gadgets (similar to the old KillAllFocuses() function)
void CGameMenu::ResetGadgetFocus(void) {
  // Already unfocused
  if (gm_pmgFocused == NULL) return;

  gm_pmgFocused->OnKillFocus();
  gm_pmgFocused = NULL;
};

// [Cecil] Find gadget in a list by its index
CMenuGadget *CGameMenu::FindListGadget(INDEX iInList) {
  FOREACHNODE(this, CAbstractMenuElement, itme) {
    if (itme->IsMenu()) continue;

    CMenuGadget &mg = (CMenuGadget &)itme.Current();

    if (mg.mg_iInList == iInList) {
      return &mg;
    }
  }

  return NULL;
};

// [Cecil] Retrieve the last possible active menu in the current hierarchy, including itself
CGameMenu *CGameMenu::GetLastMenu(void) {
  // Menu is inactive
  if (!gm_bActive) return NULL;

  // Find the last submenu
  CGameMenu *pgmSub = NULL;

  FOREACHNODE(this, CAbstractMenuElement, itme) {
    if (itme->IsMenu()) {
      pgmSub = (CGameMenu *)&itme.Current();
    }
  }

  // Go through that menu, if found
  if (pgmSub != NULL) {
    pgmSub = pgmSub->GetLastMenu();

    // Return this submenu if it exists
    if (pgmSub != NULL) return pgmSub;
  }

  // Otherwise this is the last menu
  return this;
};

// +-1 -> hit top/bottom when pressing up/down on keyboard
// +-2 -> pressed pageup/pagedown on keyboard
// +-3 -> pressed arrow up/down  button in menu
// +-4 -> scrolling with mouse wheel
void CGameMenu::ScrollList(INDEX iDir)
{
  // if not valid for scrolling
  if (gm_ctListTotal <= 0
    || gm_pmgArrowUp == NULL || gm_pmgArrowDn == NULL
    || gm_pmgListTop == NULL || gm_pmgListBottom == NULL) {
    // do nothing
    return;
  }

  INDEX iOldTopKey = gm_iListOffset;
  // change offset
  switch (iDir) {
    case -1:
      gm_iListOffset -= 1;
      break;

    case -4:
      gm_iListOffset -= 3;
      break;

    case -2:
    case -3:
      gm_iListOffset -= gm_ctListVisible;
      break;

    case +1:
      gm_iListOffset += 1;
      break;

    case +4:
      gm_iListOffset += 3;
      break;

    case +2:
    case +3:
      gm_iListOffset += gm_ctListVisible;
      break;

    default:
      ASSERT(FALSE);
      return;
  }

  if (gm_ctListTotal <= gm_ctListVisible) {
    gm_iListOffset = 0;
  } else {
    gm_iListOffset = Clamp(gm_iListOffset, INDEX(0), INDEX(gm_ctListTotal - gm_ctListVisible));
  }

  // set new names
  FillListItems();

  // if scroling with wheel
  if (iDir == +4 || iDir == -4) {
    // no focus changing
    return;
  }

  const INDEX iFirst = 0;
  const INDEX iLast = gm_ctListVisible - 1;

  // [Cecil] Set focus to another gadget in the menu
  switch (iDir) {
    case +1:
      FocusGadget(gm_pmgListBottom);
      break;

    case +2:
      if (gm_iListOffset != iOldTopKey) {
        FocusGadget(gm_pmgListTop);
      } else {
        FocusGadget(gm_pmgListBottom);
      }
      break;

    case +3:
      FocusGadget(gm_pmgArrowDn);
      break;

    case -1:
      FocusGadget(gm_pmgListTop);
      break;

    case -2:
      FocusGadget(gm_pmgListTop);
      break;

    case -3:
      FocusGadget(gm_pmgArrowUp);
      break;
  }
}

BOOL CGameMenu::OnChar(const OS::SE1Event &event)
{
  CMenuGadget *pmgActive = GetFocused();

  // if none focused
  if (pmgActive == NULL) {
    // do nothing
    return FALSE;
  }

  // if active gadget handles it
  if (pmgActive->OnChar(event)) {
    // key is handled
    return TRUE;
  }

  // key is not handled
  return FALSE;
}

// return TRUE if handled
BOOL CGameMenu::OnKeyDown(PressedMenuButton pmb) {
  // [Cecil] Ignore button press if it came from a mouse hovering over nothing
  if (_pGUIM->MouseUsedOverNothing()) return FALSE;

  CMenuGadget *pmgActive = GetFocused();

  // if none focused
  if (pmgActive == NULL) {
    // do nothing
    return FALSE;
  }

  // if active gadget handles it
  if (pmgActive->OnKeyDown(pmb)) {
    // key is handled
    return TRUE;
  }

  // [Cecil] Scroll in some direction
  const INDEX iScroll = pmb.ScrollPower();

  if (iScroll != 0) {
    ScrollList(iScroll * 2);
    return TRUE;
  }

  // [Cecil] Go up in the menu
  if (pmb.Up()) {
    // if this is top button in list
    if (pmgActive == gm_pmgListTop) {
      // scroll list up
      ScrollList(-1);
      // key is handled
      return TRUE;
    }
    // if we can go up
    if (pmgActive->mg_pmgUp != NULL && pmgActive->mg_pmgUp->mg_bVisible) {
      // [Cecil] Focus on the new gadget in the menu
      FocusGadget(pmgActive->mg_pmgUp);
      // key is handled
      return TRUE;
    }
  }

  // [Cecil] Go down in the menu
  if (pmb.Down()) {
    // if this is bottom button in list
    if (pmgActive == gm_pmgListBottom) {
      // scroll list down
      ScrollList(+1);
      // key is handled
      return TRUE;
    }
    // if we can go down
    if (pmgActive->mg_pmgDown != NULL && pmgActive->mg_pmgDown->mg_bVisible) {
      // [Cecil] Focus on the new gadget in the menu
      FocusGadget(pmgActive->mg_pmgDown);
      // key is handled
      return TRUE;
    }
  }

  CMenuGadget *pmgDefault = GetDefaultGadget();

  // [Cecil] Go left in the menu
  if (pmb.Left()) {
    // if we can go left
    if (pmgActive->mg_pmgLeft != NULL) {
      // [Cecil] Focus on the new gadget in the menu
      if (!pmgActive->mg_pmgLeft->mg_bVisible && pmgDefault != NULL) {
        pmgActive = pmgDefault;
      } else {
        pmgActive = pmgActive->mg_pmgLeft;
      }
      FocusGadget(pmgActive);
      // key is handled
      return TRUE;
    }
  }

  // [Cecil] Go right in the menu
  if (pmb.Right()) {
    // if we can go right
    if (pmgActive->mg_pmgRight != NULL) {
      // [Cecil] Focus on the new gadget in the menu
      if (!pmgActive->mg_pmgRight->mg_bVisible && pmgDefault != NULL) {
        pmgActive = pmgDefault;
      } else {
        pmgActive = pmgActive->mg_pmgRight;
      }
      FocusGadget(pmgActive);
      // key is handled
      return TRUE;
    }
  }

  // key is not handled
  return FALSE;
}

void CGameMenu::StartMenu(void) {
  // [Cecil] Activate the menu
  ASSERT(!gm_bActive);
  gm_bActive = true;

  // Show all menu gadgets by default
  FOREACHNODE(this, CAbstractMenuElement, itme) {
    if (itme->IsMenu()) continue;

    CMenuGadget &mg = (CMenuGadget &)itme.Current();
    mg.Appear();
  }

  // [Cecil] Execute custom callback
  OnStart();

  // if there is a list
  if (gm_pmgListTop != NULL) {
    // scroll it so that the wanted tem is centered
    gm_iListOffset = gm_iListWantedItem - gm_ctListVisible / 2;
    // clamp the scrolling
    gm_iListOffset = Clamp(gm_iListOffset, 0L, Max(0L, gm_ctListTotal - gm_ctListVisible));

    // fill the list
    FillListItems();

    // Show and hide respective gadgets in a list
    FOREACHNODE(this, CAbstractMenuElement, itme) {
      if (itme->IsMenu()) continue;

      CMenuGadget &mg = (CMenuGadget &)itme.Current();

      // In a list, but disabled
      if (mg.mg_iInList == -2) {
        mg.mg_bVisible = FALSE;

      // In a list
      } else if (mg.mg_iInList >= 0) {
        mg.mg_bVisible = TRUE;
      }
    }
  }

  // [Cecil] Start all submenus
  FOREACHNODE(this, CAbstractMenuElement, itme) {
    if (!itme->IsMenu()) continue;

    CGameMenu &gm = (CGameMenu &)itme.Current();
    gm.StartMenu();
  }

  // [Cecil] Update focus to the default gadget
  CMenuGadget *pmgDefault = GetDefaultGadget();

  if (pmgDefault != NULL) {
    FocusGadget(pmgDefault);
  }
};

void CGameMenu::EndMenu(void) {
  // [Cecil] Deactivate the menu
  ASSERT(gm_bActive);
  gm_bActive = false;

  // [Cecil] Execute custom callback
  OnEnd();

  CDynamicContainer<CGameMenu> cMenus;

  FOREACHNODE(this, CAbstractMenuElement, itme) {
    // [Cecil] Remember submenus
    if (itme->IsMenu()) {
      CGameMenu &gm = (CGameMenu &)itme.Current();
      cMenus.Add(&gm);

    // Hide all gadgets
    } else {
      CMenuGadget &mg = (CMenuGadget &)itme.Current();
      mg.Disappear();
    }
  }

  // [Cecil] End submenus separately
  FOREACHINDYNAMICCONTAINER(cMenus, CGameMenu, itgm) {
    itgm->EndMenu();
  }

  // [Cecil] Reset focused gadget
  gm_pmgFocused = NULL;
};

// [Cecil] Render menu background
void CGameMenu::RenderBackground(CDrawPort *pdp, bool bSubmenu) {
  // Submenu background
  if (bSubmenu) {
    _pGame->LCDRenderClouds1();
    _pGame->LCDRenderGrid();
    //_pGame->LCDRenderClouds2();
    _pGame->LCDScreenBox(_pGame->LCDGetColor(C_GREEN | 0xFF, "popup box"));

  // Regular background
  } else {
    _pGame->LCDRenderClouds1();
    _pGame->LCDRenderGrid();
    _pGame->LCDRenderClouds2();
  }
};

// [Cecil] Render the menu in its entirety and optionally find a gadget under the cursor
BOOL CGameMenu::Render(CDrawPort *pdp, CMenuGadget **ppmgUnderCursor) {
  // Menu is inactive
  if (!gm_bActive) return FALSE;

  // Clear gadget from the previous menu to only focus on the submenu gadgets
  if (ppmgUnderCursor != NULL) {
    *ppmgUnderCursor = NULL;
  }

  const bool bSubmenu = (gm_boxSubArea.Size() != FLOAT2D(1, 1));

  // Darken the parent menu
  if (bSubmenu) {
    pdp->Fill(C_BLACK | 0x7F);
  }

  // Create render subarea for this menu
  pdp->Unlock();
  CDrawPort dpSub(pdp, FloatBoxToPixBox(pdp, gm_boxSubArea));
  dpSub.Lock();

  // Clear background
  _pGame->LCDSetDrawport(&dpSub);
  dpSub.Fill(C_BLACK | 0xFF);

  // Render menu background
  RenderBackground(&dpSub, bSubmenu);

  // Begin rendering gadgets of the current menu
  _pGame->MenuPreRenderMenu(GetName());

  BOOL bDrawnAnything = FALSE;

  FOREACHNODE(this, CAbstractMenuElement, itme) {
    if (itme->IsMenu()) continue;

    CMenuGadget &mg = (CMenuGadget &)itme.Current();

    if (mg.mg_bVisible) {
      bDrawnAnything = TRUE;
      mg.Render(&dpSub);

      // Check if this gadget is under the cursor
      if (ppmgUnderCursor != NULL) {
        PIXaabbox2D boxGadget = FloatBoxToPixBox(&dpSub, mg.mg_boxOnScreen);
        boxGadget += PIX2D(dpSub.dp_MinI, dpSub.dp_MinJ);

        if (_pGUIM->IsCursorInside(boxGadget)) {
          *ppmgUnderCursor = &mg;
        }
      }
    }
  }

  // Finish rendering gadgets of the current menu
  _pGame->MenuPostRenderMenu(GetName());

  // Render submenus
  FOREACHNODE(this, CAbstractMenuElement, itme) {
    if (!itme->IsMenu()) continue;

    CGameMenu &gm = (CGameMenu &)itme.Current();
    bDrawnAnything |= gm.Render(&dpSub, ppmgUnderCursor);
  }

  // Restore the parent render area
  dpSub.Unlock();
  pdp->Lock();

  return bDrawnAnything;
};
