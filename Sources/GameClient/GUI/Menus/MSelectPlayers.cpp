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
  gd.SetText(txt); \
  AddChild(&gd);

extern CTString astrNoYes[2];
extern CTString *astrSplitScreenRadioTexts;

static INDEX FindUnusedPlayer(void) {
  INDEX *ai = _pGame->gm_aiMenuLocalPlayers;
  INDEX iPlayer = 0;

  for (; iPlayer < MAX_PLAYER_PROFILES; iPlayer++) {
    BOOL bUsed = FALSE;

    for (INDEX iLocal = 0; iLocal < NET_MAXLOCALPLAYERS; iLocal++) {
      if (ai[iLocal] == iPlayer) {
        bUsed = TRUE;
        break;
      }
    }

    if (!bUsed) {
      return iPlayer;
    }
  }

  ASSERT(FALSE);
  return iPlayer;
};

static void SelectPlayersFillMenu(CSelectPlayersMenu &gmCurrent) {
  INDEX iLocal;
  INDEX *ai = _pGame->gm_aiMenuLocalPlayers;

  for (iLocal = 0; iLocal < NET_MAXLOCALPLAYERS; iLocal++) {
    gmCurrent.gm_amgChangePlayer[iLocal].mg_iLocalPlayer = iLocal;
  }

  if (gmCurrent.gm_bAllowDedicated && _pGame->gm_MenuSplitScreenCfg == CGame::SSC_DEDICATED) {
    gmCurrent.gm_mgDedicated.mg_iSelected = 1;
  } else {
    gmCurrent.gm_mgDedicated.mg_iSelected = 0;
  }

  gmCurrent.gm_mgDedicated.ApplyCurrentSelection();

  if (gmCurrent.gm_bAllowObserving && _pGame->gm_MenuSplitScreenCfg == CGame::SSC_OBSERVER) {
    gmCurrent.gm_mgObserver.mg_iSelected = 1;
  } else {
    gmCurrent.gm_mgObserver.mg_iSelected = 0;
  }

  gmCurrent.gm_mgObserver.ApplyCurrentSelection();

  if (_pGame->gm_MenuSplitScreenCfg >= CGame::SSC_PLAY) {
    gmCurrent.gm_mgSplitScreenCfg.mg_iSelected = _pGame->gm_MenuSplitScreenCfg;
    gmCurrent.gm_mgSplitScreenCfg.ApplyCurrentSelection();
  }

  BOOL bHasDedicated = gmCurrent.gm_bAllowDedicated;
  BOOL bHasObserver = gmCurrent.gm_bAllowObserving;
  BOOL bHasPlayers = TRUE;

  if (bHasDedicated && gmCurrent.gm_mgDedicated.mg_iSelected) {
    bHasObserver = FALSE;
    bHasPlayers = FALSE;
  }

  if (bHasObserver && gmCurrent.gm_mgObserver.mg_iSelected) {
    bHasPlayers = FALSE;
  }

  const INDEX ctButtons = 4 + NET_MAXLOCALPLAYERS;

  CMenuGadget *apmg[ctButtons];
  memset(apmg, 0, sizeof(apmg));
  INDEX i = 0;

  if (bHasDedicated) {
    gmCurrent.gm_mgDedicated.Appear();
    apmg[i++] = &gmCurrent.gm_mgDedicated;
  } else {
    gmCurrent.gm_mgDedicated.Disappear();
  }

  if (bHasObserver) {
    gmCurrent.gm_mgObserver.Appear();
    apmg[i++] = &gmCurrent.gm_mgObserver;
  } else {
    gmCurrent.gm_mgObserver.Disappear();
  }

  for (INDEX iLocal = 0; iLocal < NET_MAXLOCALPLAYERS; iLocal++) {
    if (ai[iLocal] < 0 || ai[iLocal] >= MAX_PLAYER_PROFILES) {
      ai[iLocal] = 0;
    }
    for (INDEX iCopy = 0; iCopy<iLocal; iCopy++) {
      if (ai[iCopy] == ai[iLocal]) {
        ai[iLocal] = FindUnusedPlayer();
      }
    }
  }

  for (iLocal = 0; iLocal < NET_MAXLOCALPLAYERS; iLocal++) {
    gmCurrent.gm_amgChangePlayer[iLocal].Disappear();
  }

  if (bHasPlayers) {
    gmCurrent.gm_mgSplitScreenCfg.Appear();

    apmg[i++] = &gmCurrent.gm_mgSplitScreenCfg;
    gmCurrent.gm_amgChangePlayer[0].Appear();
    apmg[i++] = &gmCurrent.gm_amgChangePlayer[0];

    for (iLocal = 1; iLocal < NET_MAXLOCALPLAYERS; iLocal++) {
      if (gmCurrent.gm_mgSplitScreenCfg.mg_iSelected >= iLocal) {
        gmCurrent.gm_amgChangePlayer[iLocal].Appear();
        apmg[i++] = &gmCurrent.gm_amgChangePlayer[iLocal];
      }
    }

  } else {
    gmCurrent.gm_mgSplitScreenCfg.Disappear();
  }
  apmg[i++] = &gmCurrent.gm_mgStart;

  // relink
  for (INDEX img = 0; img < ctButtons; img++) {
    if (apmg[img] == NULL) {
      continue;
    }
    INDEX imgPred = (img + ctButtons - 1) % ctButtons;
    for (; imgPred != img; imgPred = (imgPred + ctButtons - 1) % ctButtons) {
      if (apmg[imgPred] != NULL) {
        break;
      }
    }
    INDEX imgSucc = (img + 1) % ctButtons;
    for (; imgSucc != img; imgSucc = (imgSucc + 1) % ctButtons) {
      if (apmg[imgSucc] != NULL) {
        break;
      }
    }
    apmg[img]->mg_pmgUp = apmg[imgPred];
    apmg[img]->mg_pmgDown = apmg[imgSucc];
  }

  for (iLocal = 0; iLocal < NET_MAXLOCALPLAYERS; iLocal++) {
    gmCurrent.gm_amgChangePlayer[iLocal].SetPlayerText();
  }

  if (bHasPlayers && gmCurrent.gm_mgSplitScreenCfg.mg_iSelected >= 1) {
    gmCurrent.gm_mgNotes.SetText(TRANS("Make sure you set different controls for each player!"));
  } else {
    gmCurrent.gm_mgNotes.SetText("");
  }
};

static void SelectPlayersApplyMenu(CSelectPlayersMenu &gmCurrent) {
  if (gmCurrent.gm_bAllowDedicated && gmCurrent.gm_mgDedicated.mg_iSelected) {
    _pGame->gm_MenuSplitScreenCfg = CGame::SSC_DEDICATED;
    return;
  }

  if (gmCurrent.gm_bAllowObserving && gmCurrent.gm_mgObserver.mg_iSelected) {
    _pGame->gm_MenuSplitScreenCfg = CGame::SSC_OBSERVER;
    return;
  }

  _pGame->gm_MenuSplitScreenCfg = (enum CGame::SplitScreenCfg) gmCurrent.gm_mgSplitScreenCfg.mg_iSelected;
};

static void UpdateSelectPlayers(CMenuGadget *pmg, INDEX i) {
  SelectPlayersApplyMenu(*(CSelectPlayersMenu *)pmg->GetParentMenu());
  SelectPlayersFillMenu(*(CSelectPlayersMenu *)pmg->GetParentMenu());
};

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

  gm_bAllowDedicated = FALSE;
  gm_bAllowObserving = FALSE;

  // intialize split screen menu
  gm_mgTitle.mg_boxOnScreen = BoxTitle();
  gm_mgTitle.SetText(TRANS("SELECT PLAYERS"));
  AddChild(&gm_mgTitle);

  TRIGGER_MG(gm_mgDedicated, 0, gm_mgStart, gm_mgObserver, TRANS("Dedicated:"), astrNoYes);
  gm_mgDedicated.mg_strTip = TRANS("select to start dedicated server");
  gm_mgDedicated.mg_pOnTriggerChange = &UpdateSelectPlayers;

  TRIGGER_MG(gm_mgObserver, 1, gm_mgDedicated, gm_mgSplitScreenCfg, TRANS("Observer:"), astrNoYes);
  gm_mgObserver.mg_strTip = TRANS("select to join in for observing, not for playing");
  gm_mgObserver.mg_pOnTriggerChange = &UpdateSelectPlayers;

  // split screen config trigger
  TRIGGER_MG(gm_mgSplitScreenCfg, 2, gm_mgObserver, gm_amgChangePlayer[0], TRANS("Number of players:"), astrSplitScreenRadioTexts);
  gm_mgSplitScreenCfg.mg_ctTexts = ctSplitScreen; // [Cecil] Amount of split screen values

  gm_mgSplitScreenCfg.mg_strTip = TRANS("choose more than one player to play in split screen");
  gm_mgSplitScreenCfg.mg_pOnTriggerChange = &UpdateSelectPlayers;

  for (INDEX iButton = 0; iButton < NET_MAXLOCALPLAYERS; iButton++) {
    gm_amgChangePlayer[iButton].mg_iCenterI = -1;
    gm_amgChangePlayer[iButton].mg_boxOnScreen = BoxMediumMiddle(4 + iButton);
    gm_amgChangePlayer[iButton].mg_strTip = TRANS("select profile for this player");
    AddChild(&gm_amgChangePlayer[iButton]);
  }

  gm_mgNotes.mg_boxOnScreen = BoxMediumRow(9.0);
  gm_mgNotes.mg_bfsFontSize = BFS_MEDIUM;
  gm_mgNotes.mg_iCenterI = -1;
  gm_mgNotes.mg_bEnabled = FALSE;
  gm_mgNotes.mg_bLabel = TRUE;
  AddChild(&gm_mgNotes);
  gm_mgNotes.SetText("");

  ADD_GADGET(gm_mgStart, BoxMediumRow(11), &gm_mgSplitScreenCfg, &gm_amgChangePlayer[0], NULL, NULL, TRANS("START"));
  gm_mgStart.mg_bfsFontSize = BFS_LARGE;
  gm_mgStart.mg_iCenterI = 0;
}

void CSelectPlayersMenu::OnStart(void) {
  SelectPlayersFillMenu(*this);
  SelectPlayersApplyMenu(*this);
};

void CSelectPlayersMenu::OnEnd(void) {
  SelectPlayersApplyMenu(*this);
};

// [Cecil] Change to the menu
void CSelectPlayersMenu::ChangeTo(BOOL bAllowDedicated, BOOL bAllowObserving, CMGButton::FCallback pCallback) {
  CSelectPlayersMenu *pgm = new CSelectPlayersMenu;
  pgm->Initialize_t();
  pgm->gm_bAllowDedicated = bAllowDedicated;
  pgm->gm_bAllowObserving = bAllowObserving;
  pgm->gm_mgStart.mg_pCallbackFunction = pCallback;
  _pGUIM->ChangeToMenu(pgm);
};
