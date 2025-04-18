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
#include "MSelectPlayers.h"

#define ADD_GADGET( gd, box, up, dn, lf, rt, txt) \
  gd.mg_boxOnScreen = box; \
  gd.mg_pmgUp = up; \
  gd.mg_pmgDown = dn; \
  gd.mg_pmgLeft = lf; \
  gd.mg_pmgRight = rt; \
  gd.mg_strText = txt; \
  gm_lhGadgets.AddTail(gd.mg_lnNode);

extern CTString astrNoYes[2];
extern CTString *astrSplitScreenRadioTexts;
extern void SelectPlayersFillMenu(void);
extern void SelectPlayersApplyMenu(void);

void CSelectPlayersMenu::Initialize_t(void)
{
  // [Cecil] Create array of split screen values
  const INDEX ctSplitScreen = NET_MAXLOCALPLAYERS;

  if (astrSplitScreenRadioTexts == NULL) {
    astrSplitScreenRadioTexts = new CTString[ctSplitScreen];
    astrSplitScreenRadioTexts[0] = "1";

    for (INDEX i = 1; i < ctSplitScreen; i++) {
      astrSplitScreenRadioTexts[i].PrintF("%d - split screen", i + 1);
    }
  }

  // intialize split screen menu
  gm_mgTitle.mg_boxOnScreen = BoxTitle();
  gm_mgTitle.mg_strText = TRANS("SELECT PLAYERS");
  gm_lhGadgets.AddTail(gm_mgTitle.mg_lnNode);

  TRIGGER_MG(gm_mgDedicated, 0, gm_mgStart, gm_mgObserver, TRANS("Dedicated:"), astrNoYes);
  gm_mgDedicated.mg_strTip = TRANS("select to start dedicated server");
  gm_mgDedicated.mg_pOnTriggerChange = NULL;

  TRIGGER_MG(gm_mgObserver, 1, gm_mgDedicated, gm_mgSplitScreenCfg, TRANS("Observer:"), astrNoYes);
  gm_mgObserver.mg_strTip = TRANS("select to join in for observing, not for playing");
  gm_mgObserver.mg_pOnTriggerChange = NULL;

  // split screen config trigger
  TRIGGER_MG(gm_mgSplitScreenCfg, 2, gm_mgObserver, gm_amgChangePlayer[0], TRANS("Number of players:"), astrSplitScreenRadioTexts);
  gm_mgSplitScreenCfg.mg_ctTexts = ctSplitScreen; // [Cecil] Amount of split screen values

  gm_mgSplitScreenCfg.mg_strTip = TRANS("choose more than one player to play in split screen");
  gm_mgSplitScreenCfg.mg_pOnTriggerChange = NULL;

  for (INDEX iButton = 0; iButton < NET_MAXLOCALPLAYERS; iButton++) {
    gm_amgChangePlayer[iButton].mg_iCenterI = -1;
    gm_amgChangePlayer[iButton].mg_boxOnScreen = BoxMediumMiddle(4 + iButton);
    gm_amgChangePlayer[iButton].mg_strTip = TRANS("select profile for this player");
    gm_lhGadgets.AddTail(gm_amgChangePlayer[iButton].mg_lnNode);
  }

  gm_mgNotes.mg_boxOnScreen = BoxMediumRow(9.0);
  gm_mgNotes.mg_bfsFontSize = BFS_MEDIUM;
  gm_mgNotes.mg_iCenterI = -1;
  gm_mgNotes.mg_bEnabled = FALSE;
  gm_mgNotes.mg_bLabel = TRUE;
  gm_lhGadgets.AddTail(gm_mgNotes.mg_lnNode);
  gm_mgNotes.mg_strText = "";

  /*  // options button
  mgSplitOptions.mg_strText = TRANS("Game options");
  mgSplitOptions.mg_boxOnScreen = BoxMediumRow(3);
  mgSplitOptions.mg_bfsFontSize = BFS_MEDIUM;
  mgSplitOptions.mg_iCenterI = 0;
  mgSplitOptions.mg_pmgUp = &mgSplitLevel;
  mgSplitOptions.mg_pmgDown = &mgSplitStartStart;
  mgSplitOptions.mg_strTip = TRANS("adjust game rules");
  mgSplitOptions.mg_pActivatedFunction = &StartGameOptionsFromSplitScreen;
  gm_lhGadgets.AddTail( mgSplitOptions.mg_lnNode);*/

  /*  // start button
  mgSplitStartStart.mg_bfsFontSize = BFS_LARGE;
  mgSplitStartStart.mg_boxOnScreen = BoxBigRow(4);
  mgSplitStartStart.mg_pmgUp = &mgSplitOptions;
  mgSplitStartStart.mg_pmgDown = &mgSplitGameType;
  mgSplitStartStart.mg_strText = TRANS("START");
  gm_lhGadgets.AddTail( mgSplitStartStart.mg_lnNode);
  mgSplitStartStart.mg_pActivatedFunction = &StartSelectPlayersMenuFromSplit;
  */

  ADD_GADGET(gm_mgStart, BoxMediumRow(11), &gm_mgSplitScreenCfg, &gm_amgChangePlayer[0], NULL, NULL, TRANS("START"));
  gm_mgStart.mg_bfsFontSize = BFS_LARGE;
  gm_mgStart.mg_iCenterI = 0;
}

void CSelectPlayersMenu::StartMenu(void)
{
  CGameMenu::StartMenu();
  SelectPlayersFillMenu();
  SelectPlayersApplyMenu();
}

void CSelectPlayersMenu::EndMenu(void)
{
  SelectPlayersApplyMenu();
  CGameMenu::EndMenu();
}