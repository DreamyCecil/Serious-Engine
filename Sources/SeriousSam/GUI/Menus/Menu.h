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

#ifndef SE_INCL_MENU_H
#define SE_INCL_MENU_H
#ifdef PRAGMA_ONCE
  #pragma once
#endif

// [Cecil] One pressed menu button (keyboard, mouse or controller)
struct PressedMenuButton {
  int iKey; // Keyboard key
  int iMouse; // Mouse button
  int iCtrl; // Controller button

  PressedMenuButton(int iSetKey, int iSetMouse, int iSetCtrl) :
    iKey(iSetKey), iMouse(iSetMouse), iCtrl(iSetCtrl) {};

  // Cancel / Go back to the previous menu
  inline bool Back(BOOL bMouse) {
    return iKey == SE1K_ESCAPE || (bMouse && iMouse == SDL_BUTTON_RIGHT)
        || iCtrl == SDL_CONTROLLER_BUTTON_B || iCtrl == SDL_CONTROLLER_BUTTON_BACK;
  };

  // Apply / Enter the next menu
  inline bool Apply(BOOL bMouse) {
    return iKey == SE1K_RETURN || (bMouse && iMouse == SDL_BUTTON_LEFT)
        || iCtrl == SDL_CONTROLLER_BUTTON_A || iCtrl == SDL_CONTROLLER_BUTTON_START;
  };

  // Decrease value
  inline bool Decrease(void) {
    return iKey == SE1K_BACKSPACE || iKey == SE1K_LEFT
        || iCtrl == SDL_CONTROLLER_BUTTON_DPAD_LEFT;
  };

  // Increase value
  inline bool Increase(void) {
    return iKey == SE1K_RETURN || iKey == SE1K_RIGHT
        || iCtrl == SDL_CONTROLLER_BUTTON_DPAD_RIGHT;
  };

  inline INDEX ChangeValue(void) {
    // Weak
    if (Decrease()) return -1;
    if (Increase()) return +1;

    // Strong
    if (iCtrl == SDL_CONTROLLER_BUTTON_X) return -5;
    if (iCtrl == SDL_CONTROLLER_BUTTON_A) return +5;

    // None
    return 0;
  };

  // Select previous value
  inline bool Prev(void) {
    return Decrease() || iMouse == SDL_BUTTON_RIGHT;
  };

  // Select next value
  inline bool Next(void) {
    return Increase() || iMouse == SDL_BUTTON_LEFT;
  };

  // Directions
  inline bool Up(void)    { return iKey == SE1K_UP    || iCtrl == SDL_CONTROLLER_BUTTON_DPAD_UP; };
  inline bool Down(void)  { return iKey == SE1K_DOWN  || iCtrl == SDL_CONTROLLER_BUTTON_DPAD_DOWN; };
  inline bool Left(void)  { return iKey == SE1K_LEFT  || iCtrl == SDL_CONTROLLER_BUTTON_DPAD_LEFT; };
  inline bool Right(void) { return iKey == SE1K_RIGHT || iCtrl == SDL_CONTROLLER_BUTTON_DPAD_RIGHT; };

  inline INDEX ScrollPower(void) {
    // Weak
    if (iKey == SE1K_PAGEUP   || iCtrl == SDL_CONTROLLER_BUTTON_LEFTSHOULDER)  return -1;
    if (iKey == SE1K_PAGEDOWN || iCtrl == SDL_CONTROLLER_BUTTON_RIGHTSHOULDER) return +1;

    // Strong
    if (iMouse == MOUSEWHEEL_UP) return -2;
    if (iMouse == MOUSEWHEEL_DN) return +2;

    // None
    return 0;
  };
};

// set new thumbnail
void SetThumbnail(CTFileName fn);
// remove thumbnail
void ClearThumbnail(void);

void InitializeMenus( void);
void DestroyMenus( void);
void MenuOnKeyDown(PressedMenuButton pmb); // [Cecil] Handle mouse buttons separately from keys
void MenuOnChar(const OS::SE1Event &event);
void MenuOnMouseMove(PIX pixI, PIX pixJ);
BOOL DoMenu( CDrawPort *pdp); // returns TRUE if still active, FALSE if should quit
void StartMenus(const CTString &str = "");
void StopMenus(BOOL bGoToRoot =TRUE);
BOOL IsMenusInRoot(void);
void ChangeToMenu( class CGameMenu *pgmNew);
extern void PlayMenuSound(CSoundData *psd);

#define KEYS_ON_SCREEN 14
#define LEVELS_ON_SCREEN 16
#define SERVERS_ON_SCREEN 15
#define VARS_ON_SCREEN 14

extern CListHead _lhServers;

extern INDEX _iLocalPlayer;

enum GameMode {
  GM_NONE = 0,
  GM_SINGLE_PLAYER,
  GM_NETWORK,
  GM_SPLIT_SCREEN,
  GM_DEMO,
  GM_INTRO,
};
extern GameMode _gmMenuGameMode;
extern GameMode _gmRunningGameMode;

extern CGameMenu *pgmCurrentMenu;

#include "GameMenu.h"

#include "MLoadSave.h"
#include "MPlayerProfile.h"
#include "MSelectPlayers.h"


#endif  /* include-once check. */