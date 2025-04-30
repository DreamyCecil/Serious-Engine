/* C/* Copyright (c) 2002-2012 Croteam Ltd. 
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

#include "MGChangePlayer.h"

void CMGChangePlayer::OnActivate(void)
{
  _pGUIM->PlayMenuSound(CMenuManager::E_MSND_PRESS);
  _iLocalPlayer = mg_iLocalPlayer;

  if (_pGame->gm_aiMenuLocalPlayers[mg_iLocalPlayer] < 0) {
    _pGame->gm_aiMenuLocalPlayers[mg_iLocalPlayer] = 0;
  }

  CPlayerProfileMenu::ChangeTo(&_pGame->gm_aiMenuLocalPlayers[mg_iLocalPlayer], FALSE);
}

void CMGChangePlayer::SetPlayerText(void)
{
  INDEX iPlayer = _pGame->gm_aiMenuLocalPlayers[mg_iLocalPlayer];
  CPlayerCharacter &pc = _pGame->gm_apcPlayers[iPlayer];

  if (iPlayer < 0 || iPlayer >= MAX_PLAYER_PROFILES) {
    SetText("????");
  } else {
    CTString str(0, TRANS("Player %d: %s\n"), mg_iLocalPlayer + 1, pc.GetNameForPrinting().ConstData());
    SetText(str);
  }
}
