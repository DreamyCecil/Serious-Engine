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
        || iCtrl == SDL_GAMEPAD_BUTTON_EAST || iCtrl == SDL_GAMEPAD_BUTTON_BACK;
  };

  // Apply / Enter the next menu
  inline bool Apply(BOOL bMouse) {
    return iKey == SE1K_RETURN || (bMouse && iMouse == SDL_BUTTON_LEFT)
        || iCtrl == SDL_GAMEPAD_BUTTON_SOUTH || iCtrl == SDL_GAMEPAD_BUTTON_START;
  };

  // Decrease value
  inline bool Decrease(void) {
    return iKey == SE1K_BACKSPACE || iKey == SE1K_LEFT
        || iCtrl == SDL_GAMEPAD_BUTTON_DPAD_LEFT;
  };

  // Increase value
  inline bool Increase(void) {
    return iKey == SE1K_RETURN || iKey == SE1K_RIGHT
        || iCtrl == SDL_GAMEPAD_BUTTON_DPAD_RIGHT;
  };

  inline INDEX ChangeValue(void) {
    // Weak
    if (Decrease()) return -1;
    if (Increase()) return +1;

    // Strong
    if (iCtrl == SDL_GAMEPAD_BUTTON_WEST) return -5;
    if (iCtrl == SDL_GAMEPAD_BUTTON_SOUTH) return +5;

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
  inline bool Up(void)    { return iKey == SE1K_UP    || iCtrl == SDL_GAMEPAD_BUTTON_DPAD_UP; };
  inline bool Down(void)  { return iKey == SE1K_DOWN  || iCtrl == SDL_GAMEPAD_BUTTON_DPAD_DOWN; };
  inline bool Left(void)  { return iKey == SE1K_LEFT  || iCtrl == SDL_GAMEPAD_BUTTON_DPAD_LEFT; };
  inline bool Right(void) { return iKey == SE1K_RIGHT || iCtrl == SDL_GAMEPAD_BUTTON_DPAD_RIGHT; };

  inline INDEX ScrollPower(void) {
    // Weak
    if (iKey == SE1K_PAGEUP   || iCtrl == SDL_GAMEPAD_BUTTON_LEFT_SHOULDER)  return -1;
    if (iKey == SE1K_PAGEDOWN || iCtrl == SDL_GAMEPAD_BUTTON_RIGHT_SHOULDER) return +1;

    // Strong
    if (iMouse == MOUSEWHEEL_UP) return -2;
    if (iMouse == MOUSEWHEEL_DN) return +2;

    // None
    return 0;
  };
};

#include "MAudioOptions.h"
#include "MConfirm.h"
#include "MControls.h"
#include "MCustomizeAxis.h"
#include "MCustomizeKeyboard.h"
#include "MHighScore.h"
#include "MInGame.h"
#include "MLevels.h"
#include "MLoadSave.h"
#include "MMain.h"
#include "MNetwork.h"
#include "MNetworkJoin.h"
#include "MNetworkOpen.h"
#include "MNetworkStart.h"
#include "MOptions.h"
#include "MPlayerProfile.h"
#include "MSelectPlayers.h"
#include "MServers.h"
#include "MSinglePlayer.h"
#include "MSinglePlayerNew.h"
#include "MSplitScreen.h"
#include "MSplitStart.h"
#include "MVar.h"
#include "MVideoOptions.h"

// [Cecil] Moved here out of MenuManager.h
class CMenuManager {
  public:
    // Menu sound types
    enum EMenuSound {
      E_MSNG_SELECT,
      E_MSND_PRESS,
      // New types
      E_MSND_RETURN,
      E_MSND_DISABLED,
    };

  private:
    // Current menu states
    BOOL m_bMenuActive;
    BOOL m_bMenuRendering;

    // Mouse cursor position
    FLOAT m_aCursorPos[2];
    FLOAT m_aCursorExternPos[2];

    BOOL m_bMouseUsedLast;
    CMenuGadget *m_pmgUnderCursor;

    // [Cecil] List of previously visited menus with the last one being the current one
    // Each time the "Back" button is pressed, it pops the current menu and switches to
    // the previous one, otherwise returns to game (if it's active) or to the main menu
    CStaticStackArray<CGameMenu *> m_aVisitedMenus;

    // Thumbnail for showing in the menu
    CTextureObject m_toThumbnail;
    BOOL m_bThumbnailOn;

    // Menu sounds
    CSoundObject m_soMenuSound;
    CSoundData *m_psdSelect;
    CSoundData *m_psdPress;
    CSoundData *m_psdReturn;
    CSoundData *m_psdDisabled;

    // Extra menu textures
    CTextureObject m_toLogoMenuA;
    CTextureObject m_toLogoMenuB;
    CTextureObject m_toLogoCT;
    CTextureObject m_toLogoODI;
    CTextureObject m_toLogoEAX;

  public:
    // Fonts used in the menu
    CFontData m_fdBig;
    CFontData m_fdMedium;
    CFontData m_fdTitle;

    // Menu instances
    CConfirmMenu gmConfirmMenu;
    CMainMenu gmMainMenu;
    CInGameMenu gmInGameMenu;
    CSinglePlayerMenu gmSinglePlayerMenu;
    CSinglePlayerNewMenu gmSinglePlayerNewMenu;
    CLevelsMenu gmLevelsMenu;
    CVarMenu gmVarMenu;
    CPlayerProfileMenu gmPlayerProfile;
    CControlsMenu gmControls;
    CLoadSaveMenu gmLoadSaveMenu;
    CHighScoreMenu gmHighScoreMenu;
    CCustomizeKeyboardMenu gmCustomizeKeyboardMenu;
    CServersMenu gmServersMenu;
    CCustomizeAxisMenu gmCustomizeAxisMenu;
    COptionsMenu gmOptionsMenu;
    CVideoOptionsMenu gmVideoOptionsMenu;
    CAudioOptionsMenu gmAudioOptionsMenu;
    CNetworkMenu gmNetworkMenu;
    CNetworkJoinMenu gmNetworkJoinMenu;
    CNetworkStartMenu gmNetworkStartMenu;
    CNetworkOpenMenu gmNetworkOpenMenu;
    CSplitScreenMenu gmSplitScreenMenu;
    CSplitStartMenu gmSplitStartMenu;
    CSelectPlayersMenu gmSelectPlayersMenu;

    // [Cecil] Global back button from the global scope
    CMGButton m_mgBack;

    // Menu interactions
    BOOL m_bDefiningKey;
    BOOL m_bEditingString;

  public:
    // Constructor (used to be InitializeMenus() method)
    CMenuManager();

    // Destructor (used to be DestroyMenus() method)
    ~CMenuManager();

    // Check if the mouse cursor is inside some box
    inline bool IsCursorInside(const PIXaabbox2D &box) {
      return box >= PIX2D(m_aCursorPos[0], m_aCursorPos[1]);
    };

    // Get ratio of the cursor inside some box
    inline FLOAT2D CursorRatio(const PIXaabbox2D &box) {
      FLOAT2D vRatio(
        (m_aCursorPos[0] - box.Min()(1)) / box.Size()(1),
        (m_aCursorPos[1] - box.Min()(2)) / box.Size()(2)
      );
      return vRatio;
    };

    // Get amount of visited menus
    inline INDEX GetMenuCount(void) {
      return m_aVisitedMenus.Count();
    };

    // Get a visited menu from the stack
    inline CGameMenu *GetMenu(INDEX i) {
      const INDEX ct = GetMenuCount();

      ASSERT(i >= 0 && i < ct);
      if (i < 0 || i >= ct) return NULL;

      return m_aVisitedMenus[i];
    };

    // Get the currently active menu
    inline CGameMenu *GetCurrentMenu(void) {
      const INDEX ct = GetMenuCount();
      if (ct == 0) return NULL;

      return m_aVisitedMenus[ct - 1];
    };

    // Push a new menu on top, making it the current one
    inline void PushMenu(CGameMenu *pgm) {
      m_aVisitedMenus.Add(pgm);
    };

    // Pop the current menu from top, making the previous menu the current one
    inline void PopMenu(void) {
      m_aVisitedMenus.Pop();
    };

    // Pop the menus from top until a specific one
    inline void PopMenusUntil(INDEX i) {
      m_aVisitedMenus.PopUntil(i);
    };

    // Pop all visited menus
    inline void ClearVisitedMenus(void) {
      m_aVisitedMenus.PopAll();
    };

    // Automaticaly manage input enable/disable toggling
    void UpdateInputEnabledState(void);

    // Automaticaly manage pause toggling
    void UpdatePauseState(void);

    // Separate method for letting the menu manage the game client
    void Process(void);

    // Do the main game loop and render the screen
    void DoGame(void);

  // [Cecil] These methods have been moved from global scope
  public:

    // Play a specific menu sound, replacing the previous sound if needed
    void PlayMenuSound(EMenuSound eSound, BOOL bOverOtherSounds = TRUE);

    // Set new thumbnail
    void SetThumbnail(const CTString &fnm);

    // Remove thumbnail
    void ClearThumbnail(void);

    // Process pressed buttons from various devices
    void MenuOnKeyDown(PressedMenuButton pmb);

    // Process character input
    void MenuOnChar(const OS::SE1Event &event);

    // Process mouse movement
    void MenuOnMouseMove(PIX pixI, PIX pixJ);

    // Select menu gadget under the mouse cursor
    void MenuUpdateMouseFocus(void);

    // Render mouse cursor if needed
    void RenderMouseCursor(CDrawPort *pdp);

    // Returns TRUE if any menu is still active
    BOOL DoMenu(CDrawPort *pdp);

    // Open a specific game menu
    void StartMenus(const CTString &str = "");

    // Close the current menu
    void StopMenus(BOOL bGoToRoot = TRUE);

    // Check if it's a root menu
    BOOL IsMenuRoot(CGameMenu *pgm);

    // Go to a specific menu
    void ChangeToMenu(CGameMenu *pgmNew);

    // Go to the previous menu or back to the game
    static void MenuGoToParent(void);

    // Configure the "back" button for a specific menu
    void FixupBackButton(CGameMenu *pgm);
};

// [Cecil] Declared here
extern CMenuManager *_pGUIM;

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

#include "GameMenu.h"

#include "MLoadSave.h"
#include "MPlayerProfile.h"
#include "MSelectPlayers.h"

#endif  /* include-once check. */
