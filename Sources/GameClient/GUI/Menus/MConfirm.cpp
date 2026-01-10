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
#include "MConfirm.h"

static void ConfirmYes(CMenuGadget *pmg) {
  CConfirmMenu &gmConfirm = *(CConfirmMenu *)pmg->GetParentMenu();

  if (gmConfirm.gm_pConfirmedYes != NULL) {
    gmConfirm.gm_pConfirmedYes();
  }

  gmConfirm.EndMenu();
};

static void ConfirmNo(CMenuGadget *pmg) {
  CConfirmMenu &gmConfirm = *(CConfirmMenu *)pmg->GetParentMenu();

  if (gmConfirm.gm_pConfirmedNo != NULL) {
    gmConfirm.gm_pConfirmedNo();
  }

  gmConfirm.EndMenu();
};

void CConfirmMenu::Initialize_t(void)
{
  gm_boxSubArea = BoxPopup();
  const FLOAT fHeightRatio = 1.0f / gm_boxSubArea.Size()(2);

  gm_mgConfirmLabel.SetText("");
  AddChild(&gm_mgConfirmLabel);
  gm_mgConfirmLabel.mg_boxOnScreen = BoxPopupLabel();
  gm_mgConfirmLabel.mg_iCenterI = 0;
  gm_mgConfirmLabel.mg_bfsFontSize = BFS_LARGE;
  gm_mgConfirmLabel.mg_fTextScale = fHeightRatio;

  gm_mgConfirmYes.SetText(TRANS("YES"));
  AddChild(&gm_mgConfirmYes);
  gm_mgConfirmYes.mg_boxOnScreen = BoxPopupYesLarge();
  gm_mgConfirmYes.mg_pActivatedFunction = &ConfirmYes;
  gm_mgConfirmYes.mg_pmgLeft =
    gm_mgConfirmYes.mg_pmgRight = &gm_mgConfirmNo;
  gm_mgConfirmYes.mg_iCenterI = 1;
  gm_mgConfirmYes.mg_bfsFontSize = BFS_LARGE;
  gm_mgConfirmYes.mg_fTextScale = fHeightRatio;

  gm_mgConfirmNo.SetText(TRANS("NO"));
  AddChild(&gm_mgConfirmNo);
  gm_mgConfirmNo.mg_boxOnScreen = BoxPopupNoLarge();
  gm_mgConfirmNo.mg_pActivatedFunction = &ConfirmNo;
  gm_mgConfirmNo.mg_pmgLeft =
    gm_mgConfirmNo.mg_pmgRight = &gm_mgConfirmYes;
  gm_mgConfirmNo.mg_iCenterI = -1;
  gm_mgConfirmNo.mg_bfsFontSize = BFS_LARGE;
  gm_mgConfirmNo.mg_fTextScale = fHeightRatio;

  gm_pConfirmedYes = NULL;
  gm_pConfirmedNo = NULL;
}

// [Cecil] End menu
void CConfirmMenu::OnEnd(void) {
  // Detach from the current menu
  Expunge();
};

void CConfirmMenu::BeLarge(void)
{
  gm_mgConfirmLabel.mg_bfsFontSize = BFS_LARGE;
  gm_mgConfirmYes.mg_bfsFontSize = BFS_LARGE;
  gm_mgConfirmNo.mg_bfsFontSize = BFS_LARGE;

  gm_mgConfirmLabel.mg_iCenterI = 0;
  gm_mgConfirmYes.mg_boxOnScreen = BoxPopupYesLarge();
  gm_mgConfirmNo.mg_boxOnScreen = BoxPopupNoLarge();
}

void CConfirmMenu::BeSmall(void)
{
  gm_mgConfirmLabel.mg_bfsFontSize = BFS_MEDIUM;
  gm_mgConfirmYes.mg_bfsFontSize = BFS_MEDIUM;
  gm_mgConfirmNo.mg_bfsFontSize = BFS_MEDIUM;

  gm_mgConfirmLabel.mg_iCenterI = -1;
  gm_mgConfirmYes.mg_boxOnScreen = BoxPopupYesSmall();
  gm_mgConfirmNo.mg_boxOnScreen = BoxPopupNoSmall();
}

// return TRUE if handled
BOOL CConfirmMenu::OnKeyDown(PressedMenuButton pmb)
{
  if (pmb.Back(TRUE)) {
    if (gm_mgConfirmNo.mg_pActivatedFunction != NULL || gm_mgConfirmNo.mg_pCallbackFunction != NULL) {
      gm_mgConfirmNo.OnActivate();
      return TRUE;
    }
  }

  return CGameMenu::OnKeyDown(pmb);
}

// [Cecil] Change to the menu
void CConfirmMenu::ChangeTo(const CTString &strLabel, FConfirm pFuncYes, FConfirm pFuncNo, BOOL bBigLabel)
{
  CConfirmMenu &gm = _pGUIM->gmConfirmMenu;

  gm.gm_pConfirmedYes = pFuncYes;
  gm.gm_pConfirmedNo = pFuncNo;
  gm.gm_mgConfirmLabel.SetText(strLabel);

  if (bBigLabel) {
    gm.BeLarge();
  } else {
    gm.BeSmall();
  }

  // Attach to the current menu
  _pGUIM->GetCurrentMenu()->AddChild(&gm);
  gm.StartMenu();
};
