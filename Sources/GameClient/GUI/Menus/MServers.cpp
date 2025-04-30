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
#include "MServers.h"

static void SortByColumn(CMenuGadget *pmg, int i) {
  CServersMenu &gmServers = *(CServersMenu *)pmg->GetParentMenu();

  if (gmServers.gm_mgList.mg_iSort == i) {
    gmServers.gm_mgList.mg_bSortDown = !gmServers.gm_mgList.mg_bSortDown;
  } else {
    gmServers.gm_mgList.mg_bSortDown = FALSE;
  }

  gmServers.gm_mgList.mg_iSort = i;
};

static void SortByServer(CMenuGadget *pmg)  { SortByColumn(pmg, 0); }
static void SortByMap(CMenuGadget *pmg)     { SortByColumn(pmg, 1); }
static void SortByPing(CMenuGadget *pmg)    { SortByColumn(pmg, 2); }
static void SortByPlayers(CMenuGadget *pmg) { SortByColumn(pmg, 3); }
static void SortByGame(CMenuGadget *pmg)    { SortByColumn(pmg, 4); }
static void SortByMod(CMenuGadget *pmg)     { SortByColumn(pmg, 5); }
static void SortByVer(CMenuGadget *pmg)     { SortByColumn(pmg, 6); }

static void RefreshServerList(CMenuGadget *pmg) {
  CServersMenu &gmServers = *(CServersMenu *)pmg->GetParentMenu();
  _pNetwork->EnumSessions(gmServers.m_bInternet);
};

void CServersMenu::Initialize_t(void)
{
  gm_mgTitle.mg_boxOnScreen = BoxTitle();
  gm_mgTitle.SetText(TRANS("CHOOSE SERVER"));
  AddChild(&gm_mgTitle);

  gm_mgList.mg_boxOnScreen = FLOATaabbox2D(FLOAT2D(0, 0), FLOAT2D(1, 1));
  gm_mgList.mg_pmgLeft = &gm_mgList;  // make sure it can get focus
  gm_mgList.mg_bEnabled = TRUE;
  AddChild(&gm_mgList);

  for (INDEX i = 0; i < SERVER_MENU_COLUMNS; i++) {
    gm_amgServerColumn[i].SetText("");
    gm_amgServerColumn[i].mg_boxOnScreen = BoxPlayerEdit(5.0);
    gm_amgServerColumn[i].mg_bfsFontSize = BFS_SMALL;
    gm_amgServerColumn[i].mg_iCenterI = -1;
    gm_amgServerColumn[i].mg_pmgUp = &gm_mgList;
    gm_amgServerColumn[i].mg_pmgDown = &gm_amgServerFilter[i];
    AddChild(&gm_amgServerColumn[i]);

    gm_amgServerFilter[i].mg_ctMaxStringLen = 25;
    gm_amgServerFilter[i].mg_boxOnScreen = BoxPlayerEdit(5.0);
    gm_amgServerFilter[i].mg_bfsFontSize = BFS_SMALL;
    gm_amgServerFilter[i].mg_iCenterI = -1;
    gm_amgServerFilter[i].mg_pmgUp = &gm_amgServerColumn[i];
    gm_amgServerFilter[i].mg_pmgDown = &gm_mgList;
    AddChild(&gm_amgServerFilter[i]);
    gm_amgServerFilter[i].mg_pstrToChange = &gm_astrServerFilter[i];
    gm_amgServerFilter[i].SetText(*gm_amgServerFilter[i].mg_pstrToChange);
  }

  gm_mgRefresh.SetText(TRANS("REFRESH"));
  gm_mgRefresh.mg_boxOnScreen = BoxLeftColumn(15.0);
  gm_mgRefresh.mg_bfsFontSize = BFS_SMALL;
  gm_mgRefresh.mg_iCenterI = -1;
  gm_mgRefresh.mg_pmgDown = &gm_mgList;
  gm_mgRefresh.mg_pActivatedFunction = &RefreshServerList;
  AddChild(&gm_mgRefresh);

  gm_amgServerColumn[0].SetText(TRANS("Server"));
  gm_amgServerColumn[1].SetText(TRANS("Map"));
  gm_amgServerColumn[2].SetText(TRANS("Ping"));
  gm_amgServerColumn[3].SetText(TRANS("Players"));
  gm_amgServerColumn[4].SetText(TRANS("Game"));
  gm_amgServerColumn[5].SetText(TRANS("Mod"));
  gm_amgServerColumn[6].SetText(TRANS("Ver"));
  gm_amgServerColumn[0].mg_pActivatedFunction = SortByServer;
  gm_amgServerColumn[1].mg_pActivatedFunction = SortByMap;
  gm_amgServerColumn[2].mg_pActivatedFunction = SortByPing;
  gm_amgServerColumn[3].mg_pActivatedFunction = SortByPlayers;
  gm_amgServerColumn[4].mg_pActivatedFunction = SortByGame;
  gm_amgServerColumn[5].mg_pActivatedFunction = SortByMod;
  gm_amgServerColumn[6].mg_pActivatedFunction = SortByVer;
  gm_amgServerColumn[0].mg_strTip = TRANS("sort by server");
  gm_amgServerColumn[1].mg_strTip = TRANS("sort by map");
  gm_amgServerColumn[2].mg_strTip = TRANS("sort by ping");
  gm_amgServerColumn[3].mg_strTip = TRANS("sort by players");
  gm_amgServerColumn[4].mg_strTip = TRANS("sort by game");
  gm_amgServerColumn[5].mg_strTip = TRANS("sort by mod");
  gm_amgServerColumn[6].mg_strTip = TRANS("sort by version");
  gm_amgServerFilter[0].mg_strTip = TRANS("filter by server");
  gm_amgServerFilter[1].mg_strTip = TRANS("filter by map");
  gm_amgServerFilter[2].mg_strTip = TRANS("filter by ping (ie. <200)");
  gm_amgServerFilter[3].mg_strTip = TRANS("filter by players (ie. >=2)");
  gm_amgServerFilter[4].mg_strTip = TRANS("filter by game (ie. coop)");
  gm_amgServerFilter[5].mg_strTip = TRANS("filter by mod");
  gm_amgServerFilter[6].mg_strTip = TRANS("filter by version");
}

void CServersMenu::StartMenu(void)
{
  RefreshServerList(&gm_mgRefresh);
  CGameMenu::StartMenu();
}

void CServersMenu::Think(void)
{
  if (!_pNetwork->ga_bEnumerationChange) {
    return;
  }
  _pNetwork->ga_bEnumerationChange = FALSE;
}

// [Cecil] Change to the menu
void CServersMenu::ChangeTo(void) {
  _pGUIM->ChangeToMenu(&_pGUIM->gmServersMenu);
};
