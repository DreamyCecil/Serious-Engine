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

#ifndef SE_INCL_INPUT_H
#define SE_INCL_INPUT_H
#ifdef PRAGMA_ONCE
  #pragma once
#endif

#include <Engine/Base/CTString.h>
#include <Engine/Templates/StaticArray.h>

// Maximum amount of supported game controllers
#define MAX_JOYSTICKS 8

// Number of reserved key IDs (in KeyNames.h)
#define KID_TOTALCOUNT 256

#define FIRST_JOYBUTTON (KID_TOTALCOUNT)
#define MAX_OVERALL_BUTTONS (KID_TOTALCOUNT + MAX_JOYSTICKS * SDL_CONTROLLER_BUTTON_MAX)

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

/*
 *  Mouse speed control structure
 */
struct MouseSpeedControl
{
  int msc_iThresholdX;
  int msc_iThresholdY;
  int msc_iSpeed;
};

// Information about a single axis
struct ControlAxisInfo {
  CTString cai_strNameInt; // Internal name
  CTString cai_strNameTra; // Translated display name

  FLOAT cai_fReading; // Current reading of the axis
  BOOL cai_bExisting; // Whether a controller has this axis
};

// [Cecil] Individual game controller
struct GameController_t {
  SDL_GameController *handle; // Opened controller
  INDEX iInfoSlot; // Used controller slot for info output

  GameController_t();
  ~GameController_t();

  void Connect(INDEX iSetSlot);
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
  ControlAxisInfo inp_caiAllAxisInfo[EIA_MAX_ALL]; // info for all available axis
  CTString inp_strButtonNames[ MAX_OVERALL_BUTTONS];// individual button names
  CTString inp_strButtonNamesTra[ MAX_OVERALL_BUTTONS];// individual button names (translated)
  UBYTE inp_ubButtonsBuffer[ MAX_OVERALL_BUTTONS];  // statuses for all buttons (KEY & 128 !=0)

  // [Cecil] Game controllers
  CStaticArray<GameController_t> inp_aControllers;

#if !SE1_PREFER_SDL
  SLONG inp_slScreenCenterX;                        // screen center X in pixels
  SLONG inp_slScreenCenterY;                        // screen center Y in pixels
  int inp_aOldMousePos[2];                          // old mouse position
  struct MouseSpeedControl inp_mscMouseSettings;    // system mouse settings
#endif

public:
// Operations
  CInput();
  ~CInput();

  // Sets name for every key
  void SetKeyNames(void);
  // Initializes all available devices and enumerates available controls
  void Initialize(void);
  // Enable input inside one viewport, or window
  void EnableInput(CViewPort *pvp);
  void EnableInput(OS::Window hWnd);
  // Disable input
  void DisableInput(void);
  // Test input activity
  BOOL IsInputEnabled( void) const { return inp_bInputEnabled; };
  // Scan states of all available input sources
  void GetInput(BOOL bPreScan);
  // Clear all input states (keys become not pressed, axes are reset to zero)
  void ClearInput( void);

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
  void OpenGameController(INDEX iSlot);

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
    return MAX_OVERALL_BUTTONS;
  };

  // Get name of given axis
  inline const CTString &GetAxisName(INDEX iAxisNo) const {
    return inp_caiAllAxisInfo[iAxisNo].cai_strNameInt;
  };

  // Get translated name of given axis
  const CTString &GetAxisTransName(INDEX iAxisNo) const {
    return inp_caiAllAxisInfo[iAxisNo].cai_strNameTra;
  };

  // Get current position of given axis
  inline FLOAT GetAxisValue(INDEX iAxisNo) const {
    return inp_caiAllAxisInfo[iAxisNo].cai_fReading;
  };

  // Get given button's name
  inline const CTString &GetButtonName(INDEX iButtonNo) const {
    return inp_strButtonNames[iButtonNo];
  };

  // Get given button's name translated
  inline const CTString &GetButtonTransName(INDEX iButtonNo) const {
    return inp_strButtonNamesTra[iButtonNo];
  };

  // Get given button's current state
  inline BOOL GetButtonState(INDEX iButtonNo) const {
    return (inp_ubButtonsBuffer[iButtonNo] & 128) != 0;
  };
};

// pointer to global input object
ENGINE_API extern CInput *_pInput;


#endif  /* include-once check. */

