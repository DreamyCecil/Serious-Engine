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

#ifndef SE_INCL_GAME_MENU_SERVERS_H
#define SE_INCL_GAME_MENU_SERVERS_H
#ifdef PRAGMA_ONCE
  #pragma once
#endif

#include "GameMenu.h"
#include "GUI/Components/MGButton.h"
#include "GUI/Components/MGEdit.h"
#include "GUI/Components/MGServerList.h"
#include "GUI/Components/MGTitle.h"

#define SERVER_MENU_COLUMNS 7

class CServersMenu : public CGameMenu {
public:
  BOOL m_bInternet;

  CMGTitle gm_mgTitle;
  CMGServerList gm_mgList;
  CMGButton gm_mgRefresh;

  CTString gm_astrServerFilter[SERVER_MENU_COLUMNS];
  CMGButton gm_amgServerColumn[SERVER_MENU_COLUMNS];
  CMGEdit gm_amgServerFilter[SERVER_MENU_COLUMNS];

  // [Cecil] Menu name for the mod interface
  virtual const char *GetName(void) const {
    return "Servers";
  };

  // [Cecil] Selected gadget by default
  virtual CMenuGadget *GetDefaultGadget(void) {
    return &gm_mgList;
  };

  void Initialize_t(void);
  virtual void OnStart(void);
  void Think(void);

  // [Cecil] Change to the menu
  static void ChangeTo(void);
};

#endif  /* include-once check. */
