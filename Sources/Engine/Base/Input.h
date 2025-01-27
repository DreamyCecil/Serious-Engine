/* Copyright (c) 2002-2012 Croteam Ltd.
   Copyright (c) 2024 Dreamy Cecil
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

#ifndef SE_INCL_INPUT_H
#define SE_INCL_INPUT_H
#ifdef PRAGMA_ONCE
  #pragma once
#endif

#include <Engine/Base/CTString.h>
#include <Engine/Graphics/ViewPort.h>
#include <Engine/Templates/StaticArray.h>

// Maximum amount of supported game controllers
#define MAX_JOYSTICKS 8

// Number of reserved key IDs (in KeyNames.h)
#define KID_TOTALCOUNT 256

#define FIRST_JOYBUTTON (KID_TOTALCOUNT)
#define MAX_OVERALL_BUTTONS (KID_TOTALCOUNT + MAX_JOYSTICKS * SDL_CONTROLLER_BUTTON_MAX)
#define FIRST_AXIS_ACTION (MAX_OVERALL_BUTTONS)

enum EInputAxis {
  EIA_NONE = 0, // Invalid/no axis
  EIA_MOUSE_X,  // Mouse movement
  EIA_MOUSE_Y,
  EIA_MOUSE_Z,  // Mouse wheel
  EIA_MOUSE2_X, // Second mouse movement
  EIA_MOUSE2_Y,

  // Amount of mouse axes / first controller axis
  EIA_MAX_MOUSE,
  EIA_CONTROLLER_OFFSET = EIA_MAX_MOUSE,

  // Amount of axes (mouse axes + all controller axes * all controllers)
  EIA_MAX_ALL = (EIA_MAX_MOUSE + SDL_CONTROLLER_AXIS_MAX * MAX_JOYSTICKS),
};

// All possible input actions
#define MAX_INPUT_ACTIONS (MAX_OVERALL_BUTTONS + EIA_MAX_ALL)

// Information about a single input action
struct InputDeviceAction {
  CTString ida_strNameInt; // Internal name
  CTString ida_strNameTra; // Translated display name

  DOUBLE ida_fReading; // Current reading of the action (from -1 to +1)
  BOOL ida_bExists; // Whether this action (controller axis) can be used

  // Whether the action is active (button is held / controller stick is fully to the side)
  bool IsActive(DOUBLE fThreshold = 0.5) const;
};

// [Cecil] Individual game controller
struct GameController_t {
  SDL_GameController *handle; // Opened controller
  INDEX iInfoSlot; // Used controller slot for info output

  GameController_t();
  ~GameController_t();

  void Connect(INDEX iConnectSlot, INDEX iArraySlot);
  void Disconnect(void);
  BOOL IsConnected(void);
};

/*
 * Class responsible for dealing with DirectInput
 */
class ENGINE_API CInput {
public:
// Attributes

  BOOL inp_bLastPrescan;
  BOOL inp_bInputEnabled;
  BOOL inp_bPollJoysticks;

  // [Cecil] All possible actions that can be used as controls
  InputDeviceAction inp_aInputActions[MAX_INPUT_ACTIONS];

  // [Cecil] Game controllers
  CStaticArray<GameController_t> inp_aControllers;

#if !SE1_PREFER_SDL
  SLONG inp_slScreenCenterX; // Screen center X in pixels
  SLONG inp_slScreenCenterY; // Screen center Y in pixels
  int   inp_aOldMousePos[2]; // Old mouse position

  // System mouse settings
  struct MouseSpeedControl {
    int iThresholdX, iThresholdY, iSpeed;
  } inp_mscMouseSettings;
#endif

public:
// Operations
  CInput();
  ~CInput();

  // Sets name for every key
  void SetKeyNames(void);

  // Initializes all available devices and enumerates available controls
  void Initialize(void);

  // Enable input inside one window
  void EnableInput(OS::Window hwnd);
  __forceinline void EnableInput(CViewPort *pvp) { EnableInput(pvp->vp_hWnd); };

  // [Cecil] Disable input inside one window instead of generally
  void DisableInput(OS::Window hwnd);
  __forceinline void DisableInput(CViewPort *pvp) { DisableInput(pvp->vp_hWnd); };

  // Test input activity
  BOOL IsInputEnabled( void) const { return inp_bInputEnabled; };

  // Scan states of all available input sources
  void GetInput(BOOL bPreScan);

  // Clear all input states (keys become not pressed, axes are reset to zero)
  void ClearInput(void);

// [Cecil] Second mouse interface
public:

  void Mouse2_Clear(void);
  void Mouse2_Startup(void);
  void Mouse2_Shutdown(void);
  void Mouse2_Update(void);

// [Cecil] Joystick interface
public:

  // [Cecil] Display info about current joysticks
  static void PrintJoysticksInfo(void);

  // [Cecil] Open a game controller under some slot
  // Slot index always ranges from 0 to SDL_NumJoysticks()-1
  void OpenGameController(INDEX iConnectSlot);

  // [Cecil] Close a game controller under some device index
  // This device index is NOT the same as a slot and it's always unique for each added controller
  // Use GetControllerSlotForDevice() to retrieve a slot from a device index, if there's any
  void CloseGameController(SDL_JoystickID iDevice);

  // [Cecil] Find controller slot from its device index
  INDEX GetControllerSlotForDevice(SDL_JoystickID iDevice);

  // Toggle controller polling
  void SetJoyPolling(BOOL bPoll);

  // [Cecil] Update SDL joysticks manually (if SDL_PollEvent() isn't being used)
  void UpdateJoysticks(void);

private:

  // [Cecil] Set names for joystick axes and buttons in a separate method
  void SetJoystickNames(void);

  // [Cecil] Joystick setup on initialization
  void StartupJoysticks(void);

  // [Cecil] Joystick cleanup on destruction
  void ShutdownJoysticks(void);

  // Adds axis and buttons for given joystick
  void AddJoystickAbbilities(INDEX iSlot);

  // Scans axis and buttons for given joystick
  void ScanJoystick(INDEX iSlot, BOOL bPreScan);

  // [Cecil] Get input from joysticks
  void PollJoysticks(BOOL bPreScan);

public:

  // Get count of available axis
  inline const INDEX GetAvailableAxisCount(void) const {
    return EIA_MAX_ALL;
  };

  // Get count of available buttons
  inline const INDEX GetAvailableButtonsCount(void) const {
    // [Cecil] Include axes with buttons
    return MAX_INPUT_ACTIONS;
  };

  // Get name of given axis
  inline const CTString &GetAxisName(INDEX iAxisNo) const {
    // [Cecil] Start past the button actions for compatibility
    return inp_aInputActions[FIRST_AXIS_ACTION + iAxisNo].ida_strNameInt;
  };

  // Get translated name of given axis
  inline const CTString &GetAxisTransName(INDEX iAxisNo) const {
    return inp_aInputActions[FIRST_AXIS_ACTION + iAxisNo].ida_strNameTra;
  };

  // Get current position of given axis
  inline FLOAT GetAxisValue(INDEX iAxisNo) const {
    return inp_aInputActions[FIRST_AXIS_ACTION + iAxisNo].ida_fReading;
  };

  // Get given button's name
  inline const CTString &GetButtonName(INDEX iButtonNo) const {
    return inp_aInputActions[iButtonNo].ida_strNameInt;
  };

  // Get given button's name translated
  inline const CTString &GetButtonTransName(INDEX iButtonNo) const {
    return inp_aInputActions[iButtonNo].ida_strNameTra;
  };

  // Get given button's current state
  BOOL GetButtonState(INDEX iButtonNo) const;
};

// pointer to global input object
ENGINE_API extern CInput *_pInput;


#endif  /* include-once check. */

