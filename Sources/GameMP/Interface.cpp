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

#include "StdAfx.h"

extern INDEX gam_iQuickStartDifficulty;
extern INDEX gam_iQuickStartMode;
extern INDEX gam_iStartDifficulty;
extern INDEX gam_iStartMode;

// initialize game and load settings
void CGame::Initialize(const CTFileName &fnGameSettings)
{
  gm_fnSaveFileName = fnGameSettings;
  InitInternal();
}

// save settings and cleanup
void CGame::End(void)
{
  EndInternal();
}

// automaticaly manage input enable/disable toggling
static BOOL _bInputEnabled = FALSE;
void UpdateInputEnabledState(CViewPort *pvp)
{
  // input should be enabled if application is active
  // and no menu is active and no console is active
  BOOL bShouldBeEnabled = _pGame->gm_csConsoleState==CS_OFF && _pGame->gm_csComputerState==CS_OFF;

  // if should be turned off
  if (!bShouldBeEnabled && _bInputEnabled) {
    // disable it
    _pInput->DisableInput(pvp);

    // remember new state
    _bInputEnabled = FALSE;
  }

  // if should be turned on
  if (bShouldBeEnabled && !_bInputEnabled) {
    // enable it
    _pInput->EnableInput(pvp);

    // remember new state
    _bInputEnabled = TRUE;
  }
}

// automaticaly manage pause toggling
static void UpdatePauseState(void)
{
  BOOL bShouldPause = 
     _pGame->gm_csConsoleState ==CS_ON || _pGame->gm_csConsoleState ==CS_TURNINGON || _pGame->gm_csConsoleState ==CS_TURNINGOFF ||
     _pGame->gm_csComputerState==CS_ON || _pGame->gm_csComputerState==CS_TURNINGON || _pGame->gm_csComputerState==CS_TURNINGOFF;

  _pNetwork->SetLocalPause(bShouldPause);
}

// [Cecil] Pass key presses to console and computer
void CGame::HandleConsoleAndComputer(const OS::SE1Event &event) {
  extern INDEX con_bTalk;

  // [Cecil] Pass controller presses to computer
  if (event.type == WM_CTRLBUTTONDOWN) {
    // Only if console isn't in the way
    if (gm_csConsoleState != CS_ON) ComputerKeyDown(event);

  // Pass key presses to console and computer
  } else if (event.type == WM_KEYDOWN) {
    ConsoleKeyDown(event);

    // Only if console isn't in the way
    if (gm_csConsoleState != CS_ON) ComputerKeyDown(event);

  // Close chat after pressing Enter
  } else if (event.type == WM_KEYUP) {
    if (event.key.code == SE1K_RETURN && gm_csConsoleState == CS_TALK) {
      con_bTalk = FALSE;
      gm_csConsoleState = CS_OFF;
    }

  // Type characters in console
  } else if (event.type == WM_CHAR) {
    ConsoleChar(event);
  }

  // Pass mouse buttons to computer
  if (event.type == WM_LBUTTONDOWN || event.type == WM_LBUTTONUP
   || event.type == WM_RBUTTONDOWN || event.type == WM_RBUTTONUP
   || event.type == WM_MBUTTONDOWN || event.type == WM_MBUTTONUP
   || event.type == WM_XBUTTONDOWN || event.type == WM_XBUTTONUP) {
    // Only if console isn't in the way
    if (gm_csConsoleState != CS_ON) ComputerKeyDown(event);
  }

  // Open chat on command
  if (con_bTalk && gm_csConsoleState == CS_OFF) {
    con_bTalk = FALSE;
    gm_csConsoleState = CS_TALK;
  }
};

// [Cecil] Manually toggle in-game pause
void CGame::HandlePause(const OS::SE1Event &event) {
  // Pressed Pause button with console and computer closed
  if (event.type == WM_KEYDOWN && event.key.code == SE1K_PAUSE
   && gm_csConsoleState == CS_OFF && gm_csComputerState == CS_OFF)
  {
    _pNetwork->TogglePause();
  }
};

// [Cecil] Should the application stop running or not
BOOL CGame::ShouldStopRunning(const OS::SE1Event &event, BOOL bOnDeactivation) {
  BOOL bStop = (event.type == WM_QUIT || event.type == WM_CLOSE);

#if SE1_WIN
  // Check for deactivations and lost focus too
  if (bOnDeactivation) {
    bStop |= (event.type == WM_ACTIVATE || event.type == WM_ACTIVATEAPP
           || event.type == WM_KILLFOCUS || event.type == WM_CANCELMODE);
  }
#endif

  return bStop;
};

// [Cecil] Check if pressed the key to go back
BOOL CGame::IsEscapeKeyPressed(const OS::SE1Event &event) {
  return (event.type == WM_KEYDOWN && event.key.code == SE1K_ESCAPE)
      || (event.type == WM_CTRLBUTTONDOWN && event.ctrl.action == SDL_GAMEPAD_BUTTON_START);
};

// [Cecil] Check if pressed any of the console opening keys
BOOL CGame::IsConsoleKeyPressed(const OS::SE1Event &event) {
  return (event.type == WM_KEYDOWN && (event.key.code == SE1K_F1 || event.key.code == SE1K_GRAVE));
};

// [Cecil] Toggle console state and return TRUE if console is being opened
BOOL CGame::ToggleConsole(void) {
  if (gm_csConsoleState == CS_OFF || gm_csConsoleState == CS_TURNINGOFF) {
    gm_csConsoleState = CS_TURNINGON;
    return TRUE;

  } else if (gm_csConsoleState == CS_ON || gm_csConsoleState == CS_TURNINGON) {
    gm_csConsoleState = CS_TURNINGOFF;
    return FALSE;
  }

  // Reset on edge case
  gm_csConsoleState = CS_OFF;
  return FALSE;
};

// run a quicktest game from within editor
void CGame::QuickTest(const CTFileName &fnMapName, 
  CDrawPort *pdp, CViewPort *pvp)
{
#if SE1_WIN
  const UINT uiMessengerMsg = RegisterWindowMessageA("Croteam Messenger: Incoming Message");
#endif

  EnableLoadingHook(pdp);

  // quick start game with the world
  gm_strNetworkProvider = "Local";
  gm_aiStartLocalPlayers[0] = gm_iWEDSinglePlayer;
  gm_aiStartLocalPlayers[1] = -1;
  gm_aiStartLocalPlayers[2] = -1;
  gm_aiStartLocalPlayers[3] = -1;
  gm_CurrentSplitScreenCfg = CGame::SSC_PLAY1;

  // set properties for a quick start session
  CSessionProperties sp;
  SetQuickStartSession(sp);

  // start the game
  if( !NewGame( fnMapName, fnMapName, sp)) {
    DisableLoadingHook();
    return;
  }

  // enable input
  _pInput->EnableInput(pvp);

  // initialy, game is running
  BOOL bRunning = TRUE;
  // while it is still running
  while( bRunning)
  {
  #if SE1_SINGLE_THREAD
    // [Cecil] Run timer logic in the same thread
    _pTimer->HandleTimerHandlers();
  #endif

    // [Cecil] Cross-platform events
    OS::SE1Event event;

    // While there are any messages in the message queue
    while (OS::PollEvent(event))
    {
      // Stop running the simulation
      if (IsEscapeKeyPressed(event) || ShouldStopRunning(event, TRUE)) {
        bRunning = FALSE;
        break;
      }

    #if SE1_WIN
      // If received a custom message
      if (event.type == uiMessengerMsg) {
        // Pause the game
        if (!_pNetwork->IsPaused()) _pNetwork->TogglePause();

        // Navigate to the system's TEMP folder
        char *pachrTemp = getenv("TEMP");

        if (pachrTemp != NULL) {
          // Read message from the file and output it in console
          FILE *fileMsg = FileSystem::Open(CTString(pachrTemp) + "Messenger.msg", "r");

          if (fileMsg != NULL) {
            char achrMessage[1024];
            char *pachrMessage = fgets(achrMessage, 1024 - 1, fileMsg);

            if (pachrMessage != NULL) CPutString(pachrMessage);
            fclose(fileMsg);
          }
        }
      }
    #endif

      // [Cecil] Abstracted
      if (IsConsoleKeyPressed(event)) ToggleConsole();
      HandleConsoleAndComputer(event);
      HandlePause(event);
    }

    // get real cursor position
    if (gm_csComputerState != CS_OFF) {
      float fMouseX, fMouseY;
      OS::GetMouseState(&fMouseX, &fMouseY);
      ComputerMouseMove(fMouseX, fMouseY);
    }

    UpdatePauseState();
    UpdateInputEnabledState(pvp);
      
    // if playing a demo and it is finished
    if (_pNetwork->IsDemoPlayFinished()) {
      // stop running
      bRunning = FALSE;
    }

    // do the main game loop
    GameMainLoop();
    
    // redraw the view
    if (pdp->Lock()) {
      // if current view preferences will not clear the background, clear it here
      if( _wrpWorldRenderPrefs.GetPolygonsFillType() == CWorldRenderPrefs::FT_NONE) {
        // clear background
        pdp->Fill(C_BLACK| CT_OPAQUE);
        pdp->FillZBuffer(ZBUF_BACK);
      }
      // redraw view
      if (gm_csComputerState != CS_ON) {
        GameRedrawView(pdp, (gm_csConsoleState==CS_ON)?0:GRV_SHOWEXTRAS);
      }
      ComputerRender(pdp);
      ConsoleRender(pdp);
      pdp->Unlock();
      // show it
      pvp->SwapBuffers();
    }
  }

  if (gm_csConsoleState != CS_OFF) {
    gm_csConsoleState = CS_TURNINGOFF;
  }
  if (gm_csComputerState != CS_OFF) {
    gm_csComputerState = CS_TURNINGOFF;
    cmp_ppenPlayer = NULL;
  }

  _pInput->DisableInput(pvp);
  StopGame();
  DisableLoadingHook();
}
