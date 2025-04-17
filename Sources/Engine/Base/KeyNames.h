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

#ifndef SE_INCL_KEYNAMES_H
#define SE_INCL_KEYNAMES_H

#ifdef PRAGMA_ONCE
  #pragma once
#endif

// [Cecil] Organized everything in enums

// Types for all keys
enum EInputKeyID {
  // Reserved for "no key pressed"
  KID_NONE            = 0x00,

  // Numbers row
  KID_1               = 0x11,
  KID_2               = 0x12,
  KID_3               = 0x13,
  KID_4               = 0x14,
  KID_5               = 0x15,
  KID_6               = 0x16,
  KID_7               = 0x17,
  KID_8               = 0x18,
  KID_9               = 0x19,
  KID_0               = 0x1A,
  KID_MINUS           = 0x1B,
  KID_EQUALS          = 0x1C,

  // 1st alpha row
  KID_Q               = 0x20,
  KID_W               = 0x21,
  KID_E               = 0x22,
  KID_R               = 0x23,
  KID_T               = 0x24,
  KID_Y               = 0x25,
  KID_U               = 0x26,
  KID_I               = 0x27,
  KID_O               = 0x28,
  KID_P               = 0x29,
  KID_LBRACKET        = 0x2A,
  KID_RBRACKET        = 0x2B,
  KID_BACKSLASH       = 0x2C,

  // 2nd alpha row
  KID_A               = 0x30,
  KID_S               = 0x31,
  KID_D               = 0x32,
  KID_F               = 0x33,
  KID_G               = 0x34,
  KID_H               = 0x35,
  KID_J               = 0x36,
  KID_K               = 0x37,
  KID_L               = 0x38,
  KID_SEMICOLON       = 0x39,
  KID_APOSTROPHE      = 0x3A,

  // 3rd alpha row
  KID_Z               = 0x40,
  KID_X               = 0x41,
  KID_C               = 0x42,
  KID_V               = 0x43,
  KID_B               = 0x44,
  KID_N               = 0x45,
  KID_M               = 0x46,
  KID_COMMA           = 0x47,
  KID_PERIOD          = 0x48,
  KID_SLASH           = 0x49,

  // Row with F-keys
  KID_F1              = 0x51,
  KID_F2              = 0x52,
  KID_F3              = 0x53,
  KID_F4              = 0x54,
  KID_F5              = 0x55,
  KID_F6              = 0x56,
  KID_F7              = 0x57,
  KID_F8              = 0x58,
  KID_F9              = 0x59,
  KID_F10             = 0x5A,
  KID_F11             = 0x5B,
  KID_F12             = 0x5C,

  // Extra keys
  KID_ESCAPE          = 0x60,
  KID_TILDE           = 0x61,
  KID_BACKSPACE       = 0x62,
  KID_TAB             = 0x63,
  KID_CAPSLOCK        = 0x64,
  KID_ENTER           = 0x65,
  KID_SPACE           = 0x66,

  // Modifier keys
  KID_LSHIFT          = 0x70,
  KID_RSHIFT          = 0x71,
  KID_LCONTROL        = 0x72,
  KID_RCONTROL        = 0x73,
  KID_LALT            = 0x74,
  KID_RALT            = 0x75,

  // Navigation keys
  KID_ARROWUP         = 0x80,
  KID_ARROWDOWN       = 0x81,
  KID_ARROWLEFT       = 0x82,
  KID_ARROWRIGHT      = 0x83,
  KID_INSERT          = 0x84,
  KID_DELETE          = 0x85,
  KID_HOME            = 0x86,
  KID_END             = 0x87,
  KID_PAGEUP          = 0x88,
  KID_PAGEDOWN        = 0x89,
  KID_PRINTSCR        = 0x8A,
  KID_SCROLLLOCK      = 0x8B,
  KID_PAUSE           = 0x8C,

  // Numpad numbers
  KID_NUM0            = 0x90,
  KID_NUM1            = 0x91,
  KID_NUM2            = 0x92,
  KID_NUM3            = 0x93,
  KID_NUM4            = 0x94,
  KID_NUM5            = 0x95,
  KID_NUM6            = 0x96,
  KID_NUM7            = 0x97,
  KID_NUM8            = 0x98,
  KID_NUM9            = 0x99,
  KID_NUMDECIMAL      = 0x9A,

  // Numpad gray keys
  KID_NUMLOCK         = 0xA0,
  KID_NUMSLASH        = 0xA1,
  KID_NUMMULTIPLY     = 0xA2,
  KID_NUMMINUS        = 0xA3,
  KID_NUMPLUS         = 0xA4,
  KID_NUMENTER        = 0xA5,

  // Mouse buttons
  KID_MOUSE1          = 0xC0,
  KID_MOUSE2          = 0xC1,
  KID_MOUSE3          = 0xC2,
  KID_MOUSE4          = 0xC3,
  KID_MOUSE5          = 0xC4,
  KID_MOUSEWHEELUP    = 0xC5,
  KID_MOUSEWHEELDOWN  = 0xC6,

  // [Cecil] Number of reserved key IDs
  KID_MAX = 0x100,

  // [Cecil] Special helper types
  KID_FIRST_KEYBOARD = KID_NONE + 1,
  KID_FIRST_MOUSE    = KID_MOUSE1,
  KID_LAST_KEYBOARD  = KID_FIRST_MOUSE - 1,
  KID_LAST_MOUSE     = KID_MAX - 1,
};

// [Cecil] NOTE: These values are unused
enum EInputDeviceControlID {
  // Mouse device controls
  CID_MOUSE_AXIS_XP    =  0,
  CID_MOUSE_AXIS_XN    =  1,
  CID_MOUSE_AXIS_YP    =  2,
  CID_MOUSE_AXIS_YN    =  3,
  CID_MOUSE_WHEEL_UP   =  4,
  CID_MOUSE_WHEEL_DOWN =  5,
  CID_MOUSE_BUTTON1    =  6,
  CID_MOUSE_BUTTON2    =  7,
  CID_MOUSE_BUTTON3    =  8,
  CID_MOUSE_BUTTON4    =  9,
  CID_MOUSE_BUTTON5    = 10,

  // Second mouse device controls
  CID_MOUSE2_AXIS_XP = 0,
  CID_MOUSE2_AXIS_XN = 1,
  CID_MOUSE2_AXIS_YP = 2,
  CID_MOUSE2_AXIS_YN = 3,
  CID_MOUSE2_BUTTON1 = 4,
  CID_MOUSE2_BUTTON2 = 5,
  CID_MOUSE2_BUTTON3 = 6,

  // Joystick controls
  CID_JOY_AXIS_XP  =  0,
  CID_JOY_AXIS_XN  =  1,
  CID_JOY_AXIS_YP  =  2,
  CID_JOY_AXIS_YN  =  3,
  CID_JOY_AXIS_ZP  =  4,
  CID_JOY_AXIS_ZN  =  5,
  CID_JOY_AXIS_RP  =  6,
  CID_JOY_AXIS_RN  =  7,
  CID_JOY_AXIS_UP  =  8,
  CID_JOY_AXIS_UN  =  9,
  CID_JOY_AXIS_VP  = 10,
  CID_JOY_AXIS_VN  = 11,

  CID_JOY_BUTTON1  = 12,
  CID_JOY_BUTTON2  = 13,
  CID_JOY_BUTTON3  = 14,
  CID_JOY_BUTTON4  = 15,
  CID_JOY_BUTTON5  = 16,
  CID_JOY_BUTTON6  = 17,
  CID_JOY_BUTTON7  = 18,
  CID_JOY_BUTTON8  = 19,
  CID_JOY_BUTTON9  = 20,
  CID_JOY_BUTTON10 = 21,
  CID_JOY_BUTTON11 = 22,
  CID_JOY_BUTTON12 = 23,
  CID_JOY_BUTTON13 = 24,
  CID_JOY_BUTTON14 = 25,
  CID_JOY_BUTTON15 = 26,
  CID_JOY_BUTTON16 = 27,
  CID_JOY_BUTTON17 = 28,
  CID_JOY_BUTTON18 = 29,
  CID_JOY_BUTTON19 = 30,
  CID_JOY_BUTTON20 = 31,
  CID_JOY_BUTTON21 = 32,
  CID_JOY_BUTTON22 = 33,
  CID_JOY_BUTTON23 = 34,
  CID_JOY_BUTTON24 = 35,
  CID_JOY_BUTTON25 = 36,
  CID_JOY_BUTTON26 = 37,
  CID_JOY_BUTTON27 = 38,
  CID_JOY_BUTTON28 = 39,
  CID_JOY_BUTTON29 = 40,
  CID_JOY_BUTTON30 = 41,
  CID_JOY_BUTTON31 = 42,
  CID_JOY_BUTTON32 = 43,

  CID_JOY_POV_N    = 44,
  CID_JOY_POV_E    = 45,
  CID_JOY_POV_S    = 46,
  CID_JOY_POV_W    = 47,
};

#endif  /* include-once check. */
