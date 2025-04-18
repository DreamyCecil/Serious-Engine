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
#include <Engine/Base/KeyNames.h>

// [Cecil] Maximum amount of supported devices of a specific type (mice, gamepads etc.)
const INDEX _ctMaxInputDevices = 8;

// [Cecil] Axis actions that continue the input action list after all the keys
enum EInputAxis {
  EIA_NONE = 0, // Invalid/no axis
  EIA_MOUSE_X,  // Mouse movement
  EIA_MOUSE_Y,
  EIA_MOUSE_Z,  // Mouse wheel

  // Amount of mouse axes / first controller axis
  EIA_MAX_MOUSE,
  EIA_CONTROLLER_OFFSET = EIA_MAX_MOUSE,

  // Amount of axes (mouse axes + all controller axes)
  EIA_MAX_ALL = (EIA_MAX_MOUSE + SDL_GAMEPAD_AXIS_COUNT),
};

// [Cecil] All possible input actions
#define MAX_INPUT_ACTIONS (KID_FIRST_AXIS + EIA_MAX_ALL)

// [Cecil] Mask of connected devices of a specific type, such as different mice (up to 8)
#define INPUTDEVICES_NONE        UBYTE(0)           // No devices
#define INPUTDEVICES_NUM(_Index) UBYTE(1 << _Index) // Device under a specific index (0-7)
#define INPUTDEVICES_ALL         UBYTE(0xFF)        // All devices

// Class responsible for dealing with input from various kinds of devices
class ENGINE_API CInput {
public:
  // Information about a single input action
  struct InputDeviceAction {
    CTString ida_strNameInt; // Internal name
    CTString ida_strNameTra; // Translated display name

    DOUBLE ida_fReading; // Current reading of the action (from -1 to +1)
    BOOL ida_bExists; // Whether this action (controller axis) can be used

    // Whether the action is active (button is held / controller stick is fully to the side)
    bool IsActive(DOUBLE fThreshold = 0.5) const;
  };

#if SE1_PREFER_SDL
  // [Cecil] Individual mouse
  struct MouseDevice_t {
    SDL_MouseID iID; // Device ID of a connected mouse
    INDEX iInfoSlot; // Used controller slot for info output

    // [Cecil] TEMP: Cached mouse device name for the SDL_EVENT_MOUSE_REMOVED event
    CTString strName;

    FLOAT vMotion[2]; // Movement relative to the last position (X, Y)
    FLOAT fScroll; // Accumulated wheel scroll in a certain direction (Z)

    // Specific data that's remembered between the input reads
    struct MouseInputData_t *pmid;

    MouseDevice_t();
    ~MouseDevice_t();

    __forceinline void ResetMotion(void) {
      vMotion[0] = vMotion[1] = fScroll = 0;
    };

    void Connect(SDL_MouseID iDevice, INDEX iArraySlot);
    void Disconnect(void);
    BOOL IsConnected(void) const;
  };
#endif

  // [Cecil] Individual game controller
  struct GameController_t {
    SDL_Gamepad *handle; // Opened controller
    INDEX iInfoSlot; // Used controller slot for info output

    GameController_t();
    ~GameController_t();

    void Connect(SDL_JoystickID iDevice, INDEX iArraySlot);
    void Disconnect(void);
    BOOL IsConnected(void) const;
  };

private:
  BOOL inp_bInputEnabled;
  BOOL inp_bPollJoysticks;
  FLOAT inp_aOldMousePos[2]; // Old mouse position

  // [Cecil] All possible actions that can be used as controls
  InputDeviceAction inp_aInputActions[MAX_INPUT_ACTIONS];

  // [Cecil] Connected game controllers
  CStaticArray<GameController_t> inp_aControllers;

#if SE1_PREFER_SDL
  // [Cecil] Connected mice
  CStaticArray<MouseDevice_t> inp_aMice;

#else
  SLONG inp_slScreenCenterX; // Screen center X in pixels
  SLONG inp_slScreenCenterY; // Screen center Y in pixels

  // System mouse settings
  struct MouseSpeedControl {
    int iThresholdX, iThresholdY, iSpeed;
  } inp_mscMouseSettings;
#endif

public:
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

  // [Cecil] Scan states of global input sources without distinguishing them between specific devices
  void GetGlobalInput(BOOL bPreScan);

  // [Cecil] Scan states of all input sources that are distinguished between specific devices
  // Even if all devices are included, it does not include "global" devices from GetGlobalInput()
  void GetInputFromDevices(BOOL bPreScan, ULONG ulDevices);

  // [Cecil] Scan states of all possible input sources from all devices (compatibility)
  __forceinline void GetInput(BOOL bPreScan) {
    GetGlobalInput(bPreScan);
    GetInputFromDevices(bPreScan, INPUTDEVICES_ALL);
  };

  // [Cecil] Get input from a specific mouse (-1 for any)
  void GetMouseInput(BOOL bPreScan, INDEX iMouse);

  // [Cecil] Clear states of all buttons (as if they are all released)
  void ClearButtonInput(BOOL bKeyboards, BOOL bMice, BOOL bJoysticks);

  // [Cecil] Clear movements of all axes (as if they are still)
  void ClearAxisInput(BOOL bMice, BOOL bJoysticks);

  // [Cecil] Old method for compatibility
  __forceinline void ClearInput(void) {
    ClearButtonInput(TRUE, TRUE, TRUE);
    ClearAxisInput(TRUE, TRUE);
  };

// [Cecil] Mouse interface
public:

#if SE1_PREFER_SDL
  // [Cecil] Display info about current mice
  static void PrintMiceInfo(void);

  // [Cecil] Open a mouse under some device index
  void OpenMouse(SDL_MouseID iDevice);

  // [Cecil] Close a mouse under some device index
  void CloseMouse(SDL_MouseID iDevice);

  // [Cecil] Retrieve amount of mouse slots
  inline INDEX GetMouseCount(void) const {
    return inp_aMice.Count();
  };

  // [Cecil] Retrieve a mouse device by its device index and optionally return its slot index
  MouseDevice_t *GetMouseByID(SDL_MouseID iDevice, INDEX *piSlot = NULL);

  // [Cecil] Retrieve a mouse device by its slot index
  MouseDevice_t *GetMouseFromSlot(INDEX iSlot);

  // [Cecil] Find mouse slot from its device index
  INDEX GetMouseSlotForDevice(SDL_MouseID iDevice) const;
#endif

private:

  // [Cecil] Mouse setup on initialization
  void StartupMice(void);

  // [Cecil] Mouse cleanup on destruction
  void ShutdownMice(void);

// [Cecil] Joystick interface
public:

  // [Cecil] Display info about current joysticks
  static void PrintJoysticksInfo(void);

  // [Cecil] Open a game controller under some device index
  void OpenGameController(SDL_JoystickID iDevice);

  // [Cecil] Close a game controller under some device index
  void CloseGameController(SDL_JoystickID iDevice);

  // [Cecil] Retrieve amount of game controller slots
  inline INDEX GetControllerCount(void) const {
    return inp_aControllers.Count();
  };

  // [Cecil] Retrieve a game controller by its device index and optionally return its slot index
  GameController_t *GetControllerByID(SDL_JoystickID iDevice, INDEX *piSlot = NULL);

  // [Cecil] Retrieve a game controller by its slot index
  GameController_t *GetControllerFromSlot(INDEX iSlot);

  // [Cecil] Find controller slot from its device index
  INDEX GetControllerSlotForDevice(SDL_JoystickID iDevice) const;

  // Toggle controller polling
  void SetJoyPolling(BOOL bPoll);

  // [Cecil] Update SDL joysticks manually (if SDL_PollEvent() isn't being used)
  void UpdateJoysticks(void);

private:

  // [Cecil] Joystick setup on initialization
  void StartupJoysticks(void);

  // [Cecil] Joystick cleanup on destruction
  void ShutdownJoysticks(void);

  // Adds axis and buttons for joysticks
  void AddJoystickAbbilities(void);

  // Scans axis and buttons for given joystick
  void ScanJoystick(INDEX iJoy);

  // [Cecil] Get input from joysticks
  void PollJoysticks(ULONG ulDevices);

public:

  // Get amount of available axes
  __forceinline const INDEX GetMaxInputAxes(void) const {
    return EIA_MAX_ALL;
  };

  // Get amount of available buttons
  __forceinline const INDEX GetMaxInputButtons(void) const {
    return KID_MAX_ALL;
  };

  // [Cecil] Get amount of available actions (buttons + axes)
  __forceinline const INDEX GetMaxInputActions(void) const {
    return MAX_INPUT_ACTIONS;
  };

  // Get name of given axis
  inline const CTString &GetAxisName(INDEX iAxisNo) const {
    // [Cecil] Start past the button actions for compatibility
    return inp_aInputActions[KID_FIRST_AXIS + iAxisNo].ida_strNameInt;
  };

  // Get translated name of given axis
  inline const CTString &GetAxisTransName(INDEX iAxisNo) const {
    return inp_aInputActions[KID_FIRST_AXIS + iAxisNo].ida_strNameTra;
  };

  // Get current position of given axis
  inline FLOAT GetAxisValue(INDEX iAxisNo) const {
    return inp_aInputActions[KID_FIRST_AXIS + iAxisNo].ida_fReading;
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

  // [Cecil] Find axis action index by its name
  EInputAxis FindAxisByName(const CTString &strName);

  // [Cecil] Find input action index by its name
  INDEX FindActionByName(const CTString &strName);
};

// pointer to global input object
ENGINE_API extern CInput *_pInput;


#endif  /* include-once check. */

