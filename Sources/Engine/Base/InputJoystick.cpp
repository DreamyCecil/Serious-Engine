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

#include "StdH.h"

#include <Engine/Base/Input.h>

INDEX inp_bForceJoystickPolling = 0;
INDEX inp_ctJoysticksAllowed = MAX_JOYSTICKS;

GameController_t::GameController_t() : handle(NULL), iInfoSlot(-1)
{
};

GameController_t::~GameController_t() {
  Disconnect();
};

// Open a game controller under some slot
void GameController_t::Connect(SDL_JoystickID iDevice, INDEX iArraySlot) {
  ASSERT(handle == NULL && iInfoSlot == -1);

  handle = SDL_OpenGamepad(iDevice);
  iInfoSlot = iArraySlot + 1;
};

// Close an open game controller
void GameController_t::Disconnect(void) {
  if (handle != NULL) {
    SDL_Joystick *pJoystick = SDL_GetGamepadJoystick(handle);
    CPrintF(TRANS("Disconnected '%s' from controller slot %d\n"), SDL_GetJoystickName(pJoystick), iInfoSlot);

    SDL_CloseGamepad(handle);
  }

  handle = NULL;
  iInfoSlot = -1;
};

// Check if the controller is connected
BOOL GameController_t::IsConnected(void) {
  return (handle != NULL);
};

// [Cecil] Display info about current joysticks
void CInput::PrintJoysticksInfo(void) {
  if (_pInput == NULL) return;

  const INDEX ct = _pInput->inp_aControllers.Count();
  CPrintF(TRANS("%d controller slots:\n"), ct);

  for (INDEX i = 0; i < ct; i++) {
    GameController_t &gctrl = _pInput->inp_aControllers[i];
    CPrintF(" %d. ", i + 1);

    if (!gctrl.IsConnected()) {
      CPutString(TRANS("not connected\n"));
      continue;
    }

    SDL_Joystick *pJoystick = SDL_GetGamepadJoystick(gctrl.handle);
    const char *strName = SDL_GetJoystickName(pJoystick);

    if (strName == NULL) {
      strName = SDL_GetError();
    }

    CPrintF("'%s':", strName);
    CPrintF(TRANS(" %d axes, %d buttons, %d hats\n"), SDL_GetNumJoystickAxes(pJoystick),
      SDL_GetNumJoystickButtons(pJoystick), SDL_GetNumJoystickHats(pJoystick));
  }

  CPutString("-\n");
};

// [Cecil] Open a game controller under some device index
void CInput::OpenGameController(SDL_JoystickID iDevice)
{
  // Not a game controller
  if (!SDL_IsGamepad(iDevice)) return;

  // Check if this controller is already connected
  if (GetControllerSlotForDevice(iDevice) != -1) return;

  // Find an empty slot for opening a new controller
  GameController_t *pToOpen = NULL;
  INDEX iArraySlot = -1;
  const INDEX ct = inp_aControllers.Count();

  for (INDEX i = 0; i < ct; i++) {
    if (!inp_aControllers[i].IsConnected()) {
      // Remember controller and its slot
      pToOpen = &inp_aControllers[i];
      iArraySlot = i;
      break;
    }
  }

  // No slots left
  if (pToOpen == NULL || iArraySlot == -1) {
    CPrintF(TRANS("Cannot open another game controller due to all %d slots being occupied!\n"), ct);
    return;
  }

  pToOpen->Connect(iDevice, iArraySlot);

  if (!pToOpen->IsConnected()) {
    CPrintF(TRANS("Cannot open another game controller! SDL Error: %s\n"), SDL_GetError());
    return;
  }

  // Report controller info
  SDL_Joystick *pJoystick = SDL_GetGamepadJoystick(pToOpen->handle);
  const char *strName = SDL_GetJoystickName(pJoystick);

  if (strName == NULL) {
    strName = SDL_GetError();
  }

  CPrintF(TRANS("Connected '%s' to controller slot %d\n"), strName, pToOpen->iInfoSlot);

  int ctAxes = SDL_GetNumJoystickAxes(pJoystick);
  CPrintF(TRANS(" %d axes, %d buttons, %d hats\n"), ctAxes, SDL_GetNumJoystickButtons(pJoystick), SDL_GetNumJoystickHats(pJoystick));

  // Check whether all axes exist
  const INDEX iFirstAxis = FIRST_AXIS_ACTION + EIA_CONTROLLER_OFFSET + iArraySlot * SDL_GAMEPAD_AXIS_COUNT;

  for (INDEX iAxis = 0; iAxis < SDL_GAMEPAD_AXIS_COUNT; iAxis++) {
    InputDeviceAction &ida = inp_aInputActions[iFirstAxis + iAxis];
    ida.ida_bExists = (iAxis < ctAxes);
  }
};

// [Cecil] Close a game controller under some device index
void CInput::CloseGameController(SDL_JoystickID iDevice)
{
  INDEX iSlot = GetControllerSlotForDevice(iDevice);

  if (iSlot != -1) {
    inp_aControllers[iSlot].Disconnect();
  }
};

// [Cecil] Find controller slot from its device index
INDEX CInput::GetControllerSlotForDevice(SDL_JoystickID iDevice) {
  INDEX i = inp_aControllers.Count();

  while (--i >= 0) {
    GameController_t &gctrl = inp_aControllers[i];

    // No open controller
    if (!gctrl.IsConnected()) continue;

    // Get device ID from the controller
    SDL_Joystick *pJoystick = SDL_GetGamepadJoystick(gctrl.handle);
    SDL_JoystickID id = SDL_GetJoystickID(pJoystick);

    // Found matching ID
    if (id == iDevice) {
      return i;
    }
  }

  // Couldn't find
  return -1;
};

// Toggle controller polling
void CInput::SetJoyPolling(BOOL bPoll) {
  inp_bPollJoysticks = bPoll;
};

// [Cecil] Update SDL joysticks manually (if SDL_PollEvent() isn't being used)
void CInput::UpdateJoysticks(void) {
  // Update SDL joysticks
  SDL_UpdateJoysticks();

  // Open all valid controllers
  int ctJoysticks;
  SDL_JoystickID *aJoysticks = SDL_GetJoysticks(&ctJoysticks);

  if (aJoysticks != NULL) {
    for (int iJoy = 0; iJoy < ctJoysticks; iJoy++) {
      OpenGameController(aJoysticks[iJoy]);
    }

    SDL_free(aJoysticks);
  }

  // Go through connected controllers
  FOREACHINSTATICARRAY(inp_aControllers, GameController_t, it) {
    if (!it->IsConnected()) continue;

    // Disconnect this controller if it has been detached
    SDL_Joystick *pJoystick = SDL_GetGamepadJoystick(it->handle);

    if (!SDL_JoystickConnected(pJoystick)) {
      it->Disconnect();
    }
  }
};

// [Cecil] Set names for joystick axes and buttons in a separate method
void CInput::SetJoystickNames(void) {
  const INDEX ct = inp_aControllers.Count();

  for (INDEX i = 0; i < ct; i++) {
    AddJoystickAbbilities(i);
  }
};

// [Cecil] Joystick setup on initialization
void CInput::StartupJoysticks(void) {
  // Create an empty array of controllers
  ASSERT(inp_aControllers.Count() == 0);
  inp_aControllers.New(MAX_JOYSTICKS);

  // Report on available controller amounts
  int ctJoysticks;
  SDL_JoystickID *aJoysticks = SDL_GetJoysticks(&ctJoysticks);

  if (aJoysticks != NULL) {
    CPrintF(TRANS("  joysticks found: %d\n"), ctJoysticks);
    SDL_free(aJoysticks);

  } else {
    CPrintF(TRANS("  Cannot retrieve an array of joysticks! SDL Error: %s\n"), SDL_GetError());
  }

  const INDEX ctAllowed = Min(inp_aControllers.Count(), inp_ctJoysticksAllowed);
  CPrintF(TRANS("  joysticks allowed: %d\n"), ctAllowed);
};

// [Cecil] Joystick cleanup on destruction
void CInput::ShutdownJoysticks(void) {
  // Should close all controllers automatically on array destruction
  inp_aControllers.Clear();
};

// Adds axis and buttons for given joystick
void CInput::AddJoystickAbbilities(INDEX iSlot) {
  const CTString strJoystickNameInt(0, "[C%d] ", iSlot + 1);
  const CTString strJoystickNameTra(0, TRANS("[C%d] "), iSlot + 1);

  // Set proper names for axes
  const INDEX iFirstAxis = FIRST_AXIS_ACTION + EIA_CONTROLLER_OFFSET + iSlot * SDL_GAMEPAD_AXIS_COUNT;

  #define SET_AXIS_NAMES(_Axis, _Name, _Translated) \
    inp_aInputActions[iFirstAxis + _Axis].ida_strNameInt = strJoystickNameInt + _Name; \
    inp_aInputActions[iFirstAxis + _Axis].ida_strNameTra = strJoystickNameTra + _Translated;

  // Set default names for all axes
  for (INDEX iAxis = 0; iAxis < SDL_GAMEPAD_AXIS_COUNT; iAxis++) {
    SET_AXIS_NAMES(iAxis, CTString(0, "Unknown %d", iAxis), CTString(0, TRANS("Unknown %d"), iAxis));
  }

  // Set proper names for axes
  SET_AXIS_NAMES(SDL_GAMEPAD_AXIS_LEFTX,         "L Stick X", TRANS("L Stick X"));
  SET_AXIS_NAMES(SDL_GAMEPAD_AXIS_LEFTY,         "L Stick Y", TRANS("L Stick Y"));
  SET_AXIS_NAMES(SDL_GAMEPAD_AXIS_RIGHTX,        "R Stick X", TRANS("R Stick X"));
  SET_AXIS_NAMES(SDL_GAMEPAD_AXIS_RIGHTY,        "R Stick Y", TRANS("R Stick Y"));
  SET_AXIS_NAMES(SDL_GAMEPAD_AXIS_LEFT_TRIGGER,  "L2",              "L2");
  SET_AXIS_NAMES(SDL_GAMEPAD_AXIS_RIGHT_TRIGGER, "R2",              "R2");

  const INDEX iFirstButton = FIRST_JOYBUTTON + iSlot * SDL_GAMEPAD_BUTTON_COUNT;

  #define SET_BUTTON_NAMES(_Button, _Name, _Translated) \
    inp_aInputActions[iFirstButton + _Button].ida_strNameInt = strJoystickNameInt + _Name; \
    inp_aInputActions[iFirstButton + _Button].ida_strNameTra = strJoystickNameTra + _Translated;

  // Set default names for all buttons
  for (INDEX iButton = 0; iButton < SDL_GAMEPAD_BUTTON_COUNT; iButton++) {
    SET_BUTTON_NAMES(iButton, CTString(0, "Unknown %d", iButton), CTString(0, TRANS("Unknown %d"), iButton));
  }

  // Set proper names for buttons
  SET_BUTTON_NAMES(SDL_GAMEPAD_BUTTON_SOUTH,          "A",                 "A");
  SET_BUTTON_NAMES(SDL_GAMEPAD_BUTTON_EAST,           "B",                 "B");
  SET_BUTTON_NAMES(SDL_GAMEPAD_BUTTON_WEST,           "X",                 "X");
  SET_BUTTON_NAMES(SDL_GAMEPAD_BUTTON_NORTH,          "Y",                 "Y");
  SET_BUTTON_NAMES(SDL_GAMEPAD_BUTTON_BACK,           "Back",        TRANS("Back"));
  SET_BUTTON_NAMES(SDL_GAMEPAD_BUTTON_GUIDE,          "Guide",       TRANS("Guide"));
  SET_BUTTON_NAMES(SDL_GAMEPAD_BUTTON_START,          "Start",       TRANS("Start"));
  SET_BUTTON_NAMES(SDL_GAMEPAD_BUTTON_LEFT_STICK,     "L3",                "L3");
  SET_BUTTON_NAMES(SDL_GAMEPAD_BUTTON_RIGHT_STICK,    "R3",                "R3");
  SET_BUTTON_NAMES(SDL_GAMEPAD_BUTTON_LEFT_SHOULDER,  "L1",                "L1");
  SET_BUTTON_NAMES(SDL_GAMEPAD_BUTTON_RIGHT_SHOULDER, "R1",                "R1");
  SET_BUTTON_NAMES(SDL_GAMEPAD_BUTTON_DPAD_UP,        "D-pad Up",    TRANS("D-pad Up"));
  SET_BUTTON_NAMES(SDL_GAMEPAD_BUTTON_DPAD_DOWN,      "D-pad Down",  TRANS("D-pad Down"));
  SET_BUTTON_NAMES(SDL_GAMEPAD_BUTTON_DPAD_LEFT,      "D-pad Left",  TRANS("D-pad Left"));
  SET_BUTTON_NAMES(SDL_GAMEPAD_BUTTON_DPAD_RIGHT,     "D-pad Right", TRANS("D-pad Right"));
  SET_BUTTON_NAMES(SDL_GAMEPAD_BUTTON_MISC1,          "Misc 1",      TRANS("Misc 1"));
  SET_BUTTON_NAMES(SDL_GAMEPAD_BUTTON_RIGHT_PADDLE1,  "R4",                "R4");
  SET_BUTTON_NAMES(SDL_GAMEPAD_BUTTON_LEFT_PADDLE1,   "L4",                "L4");
  SET_BUTTON_NAMES(SDL_GAMEPAD_BUTTON_RIGHT_PADDLE2,  "R5",                "R5");
  SET_BUTTON_NAMES(SDL_GAMEPAD_BUTTON_LEFT_PADDLE2,   "L5",                "L5");
  SET_BUTTON_NAMES(SDL_GAMEPAD_BUTTON_MISC2,          "Misc 2",      TRANS("Misc 2"));
  SET_BUTTON_NAMES(SDL_GAMEPAD_BUTTON_MISC3,          "Misc 3",      TRANS("Misc 3"));
  SET_BUTTON_NAMES(SDL_GAMEPAD_BUTTON_MISC4,          "Misc 4",      TRANS("Misc 4"));
  SET_BUTTON_NAMES(SDL_GAMEPAD_BUTTON_MISC5,          "Misc 5",      TRANS("Misc 5"));
  SET_BUTTON_NAMES(SDL_GAMEPAD_BUTTON_MISC6,          "Misc 6",      TRANS("Misc 6"));
};

// Scans axis and buttons for given joystick
void CInput::ScanJoystick(INDEX iSlot, BOOL bPreScan) {
  SDL_Gamepad *pController = inp_aControllers[iSlot].handle;

  const INDEX iFirstAxis = FIRST_AXIS_ACTION + EIA_CONTROLLER_OFFSET + iSlot * SDL_GAMEPAD_AXIS_COUNT;

  // For each available axis
  for (INDEX iAxis = 0; iAxis < SDL_GAMEPAD_AXIS_COUNT; iAxis++) {
    InputDeviceAction &ida = inp_aInputActions[iFirstAxis + iAxis];

    // If the axis is not present
    if (!ida.ida_bExists) {
      // Read as zero and skip to the next one
      ida.ida_fReading = 0.0;
      continue;
    }

    // [Cecil] FIXME: I cannot put a bigger emphasis on why it's important to reset readings during pre-scanning here.
    // If this isn't done and the sticks are being used for the camera rotation, the rotation speed changes drastically
    // depending on the current FPS, either making it suddenly too fast or too slow.
    // It doesn't affect the speed only when the maximum FPS is limited, either by using sam_iMaxFPSActive command or
    // by building with SE1_PREFER_SDL, which always seems to lock framerate at the monitor's refresh rate.
    //
    // But even when you reset it, there's still a noticable choppiness that sometimes happens during view rotation if
    // the maximum FPS is set too high because the reading is only being set once every CTimer::TickQuantum seconds.
    //
    // I have tried multiple methods to try and solve it, even multipling the reading by the time difference between
    // calling this function (per axis), but it always ended up broken. This is the most stable fix I could figure out.
    if (bPreScan) {
      ida.ida_fReading = 0.0;
      continue;
    }

    // Read its state
    SLONG slAxisReading = SDL_GetGamepadAxis(pController, (SDL_GamepadAxis)iAxis);

    // Set current axis value from -1 to +1
    const DOUBLE fCurrentValue = DOUBLE(slAxisReading - SDL_JOYSTICK_AXIS_MIN);
    const DOUBLE fMaxValue = DOUBLE(SDL_JOYSTICK_AXIS_MAX - SDL_JOYSTICK_AXIS_MIN);

    ida.ida_fReading = fCurrentValue / fMaxValue * 2.0 - 1.0;
  }

  if (!bPreScan) {
    const INDEX iFirstButton = FIRST_JOYBUTTON + iSlot * SDL_GAMEPAD_BUTTON_COUNT;

    // For each available button
    for (INDEX iButton = 0; iButton < SDL_GAMEPAD_BUTTON_COUNT; iButton++) {
      // Test if the button is pressed
      const BOOL bJoyButtonPressed = SDL_GetGamepadButton(pController, (SDL_GamepadButton)iButton);

      if (bJoyButtonPressed) {
        inp_aInputActions[iFirstButton + iButton].ida_fReading = 1;
      } else {
        inp_aInputActions[iFirstButton + iButton].ida_fReading = 0;
      }
    }
  }
};

// [Cecil] Get input from joysticks
void CInput::PollJoysticks(BOOL bPreScan) {
  // Only if joystick polling is enabled or forced
  if (!inp_bPollJoysticks && !inp_bForceJoystickPolling) return;

  // Scan states of all available joysticks
  const INDEX ct = inp_aControllers.Count();

  for (INDEX i = 0; i < ct; i++) {
    if (!inp_aControllers[i].IsConnected() || i >= inp_ctJoysticksAllowed) continue;

    ScanJoystick(i, bPreScan);
  }
};
