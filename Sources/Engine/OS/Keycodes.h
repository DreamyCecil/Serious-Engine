/* Copyright (c) 2024 Dreamy Cecil
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

// [Cecil] This header defines cross-platform virtual keycodes for input handling
#ifndef SE_INCL_KEYCODES_H
#define SE_INCL_KEYCODES_H

#if SE1_PREFER_SDL

// These exist purely for rerouting logic from SDL_GetKeyboardState() to SDL_GetMouseState()
// and should NOT be used akin to Windows' VK_*BUTTON macros!
#define SE1K_LBUTTON      0x1
#define SE1K_RBUTTON      0x2
#define SE1K_MBUTTON      0x3
#define SE1K_XBUTTON1     0x4
#define SE1K_XBUTTON2     0x5

#define SE1K_1            SDLK_1
#define SE1K_2            SDLK_2
#define SE1K_3            SDLK_3
#define SE1K_4            SDLK_4
#define SE1K_5            SDLK_5
#define SE1K_6            SDLK_6
#define SE1K_7            SDLK_7
#define SE1K_8            SDLK_8
#define SE1K_9            SDLK_9
#define SE1K_0            SDLK_0
#define SE1K_a            SDLK_A
#define SE1K_b            SDLK_B
#define SE1K_c            SDLK_C
#define SE1K_d            SDLK_D
#define SE1K_e            SDLK_E
#define SE1K_f            SDLK_F
#define SE1K_g            SDLK_G
#define SE1K_h            SDLK_H
#define SE1K_i            SDLK_I
#define SE1K_j            SDLK_J
#define SE1K_k            SDLK_K
#define SE1K_l            SDLK_L
#define SE1K_m            SDLK_M
#define SE1K_n            SDLK_N
#define SE1K_o            SDLK_O
#define SE1K_p            SDLK_P
#define SE1K_q            SDLK_Q
#define SE1K_r            SDLK_R
#define SE1K_s            SDLK_S
#define SE1K_t            SDLK_T
#define SE1K_u            SDLK_U
#define SE1K_v            SDLK_V
#define SE1K_w            SDLK_W
#define SE1K_x            SDLK_X
#define SE1K_y            SDLK_Y
#define SE1K_z            SDLK_Z
#define SE1K_MINUS        SDLK_MINUS
#define SE1K_EQUALS       SDLK_EQUALS
#define SE1K_COMMA        SDLK_COMMA
#define SE1K_PERIOD       SDLK_PERIOD
#define SE1K_SLASH        SDLK_SLASH
#define SE1K_SEMICOLON    SDLK_SEMICOLON
#define SE1K_QUOTE        SDLK_APOSTROPHE
#define SE1K_ESCAPE       SDLK_ESCAPE
#define SE1K_TAB          SDLK_TAB
#define SE1K_CAPSLOCK     SDLK_CAPSLOCK
#define SE1K_RETURN       SDLK_RETURN
#define SE1K_BACKSPACE    SDLK_BACKSPACE
#define SE1K_SPACE        SDLK_SPACE
#define SE1K_UP           SDLK_UP
#define SE1K_DOWN         SDLK_DOWN
#define SE1K_LEFT         SDLK_LEFT
#define SE1K_RIGHT        SDLK_RIGHT
#define SE1K_INSERT       SDLK_INSERT
#define SE1K_DELETE       SDLK_DELETE
#define SE1K_HOME         SDLK_HOME
#define SE1K_END          SDLK_END
#define SE1K_PAGEUP       SDLK_PAGEUP
#define SE1K_PAGEDOWN     SDLK_PAGEDOWN
#define SE1K_PRINTSCREEN  SDLK_PRINTSCREEN
#define SE1K_SCROLLLOCK   SDLK_SCROLLLOCK
#define SE1K_PAUSE        SDLK_PAUSE
#define SE1K_F1           SDLK_F1
#define SE1K_F2           SDLK_F2
#define SE1K_F3           SDLK_F3
#define SE1K_F4           SDLK_F4
#define SE1K_F5           SDLK_F5
#define SE1K_F6           SDLK_F6
#define SE1K_F7           SDLK_F7
#define SE1K_F8           SDLK_F8
#define SE1K_F9           SDLK_F9
#define SE1K_F10          SDLK_F10
#define SE1K_F11          SDLK_F11
#define SE1K_F12          SDLK_F12
#define SE1K_GRAVE        SDLK_GRAVE
#define SE1K_LEFTBRACKET  SDLK_LEFTBRACKET
#define SE1K_RIGHTBRACKET SDLK_RIGHTBRACKET
#define SE1K_BACKSLASH    SDLK_BACKSLASH
#define SE1K_LSHIFT       SDLK_LSHIFT
#define SE1K_RSHIFT       SDLK_RSHIFT
#define SE1K_LCTRL        SDLK_LCTRL
#define SE1K_RCTRL        SDLK_RCTRL
#define SE1K_LALT         SDLK_LALT
#define SE1K_RALT         SDLK_RALT
#define SE1K_KP_0         SDLK_KP_0
#define SE1K_KP_1         SDLK_KP_1
#define SE1K_KP_2         SDLK_KP_2
#define SE1K_KP_3         SDLK_KP_3
#define SE1K_KP_4         SDLK_KP_4
#define SE1K_KP_5         SDLK_KP_5
#define SE1K_KP_6         SDLK_KP_6
#define SE1K_KP_7         SDLK_KP_7
#define SE1K_KP_8         SDLK_KP_8
#define SE1K_KP_9         SDLK_KP_9
#define SE1K_KP_PERIOD    SDLK_KP_PERIOD
#define SE1K_NUMLOCKCLEAR SDLK_NUMLOCKCLEAR
#define SE1K_KP_DIVIDE    SDLK_KP_DIVIDE
#define SE1K_KP_MULTIPLY  SDLK_KP_MULTIPLY
#define SE1K_KP_MINUS     SDLK_KP_MINUS
#define SE1K_KP_PLUS      SDLK_KP_PLUS
#define SE1K_KP_ENTER     SDLK_KP_ENTER

#else

// These exist purely for recognizing mouse buttons in Win32 input logic
// and should NOT be used akin to Windows' VK_*BUTTON macros!
#define SE1K_LBUTTON      VK_LBUTTON
#define SE1K_RBUTTON      VK_RBUTTON
#define SE1K_MBUTTON      VK_MBUTTON
#define SE1K_XBUTTON1     VK_XBUTTON1
#define SE1K_XBUTTON2     VK_XBUTTON2

#define SE1K_1            '1'
#define SE1K_2            '2'
#define SE1K_3            '3'
#define SE1K_4            '4'
#define SE1K_5            '5'
#define SE1K_6            '6'
#define SE1K_7            '7'
#define SE1K_8            '8'
#define SE1K_9            '9'
#define SE1K_0            '0'
#define SE1K_a            'A'
#define SE1K_b            'B'
#define SE1K_c            'C'
#define SE1K_d            'D'
#define SE1K_e            'E'
#define SE1K_f            'F'
#define SE1K_g            'G'
#define SE1K_h            'H'
#define SE1K_i            'I'
#define SE1K_j            'J'
#define SE1K_k            'K'
#define SE1K_l            'L'
#define SE1K_m            'M'
#define SE1K_n            'N'
#define SE1K_o            'O'
#define SE1K_p            'P'
#define SE1K_q            'Q'
#define SE1K_r            'R'
#define SE1K_s            'S'
#define SE1K_t            'T'
#define SE1K_u            'U'
#define SE1K_v            'V'
#define SE1K_w            'W'
#define SE1K_x            'X'
#define SE1K_y            'Y'
#define SE1K_z            'Z'
#define SE1K_MINUS        VK_OEM_MINUS
#define SE1K_EQUALS       VK_OEM_PLUS
#define SE1K_COMMA        VK_OEM_COMMA
#define SE1K_PERIOD       VK_OEM_PERIOD
#define SE1K_SLASH        VK_OEM_2
#define SE1K_SEMICOLON    VK_OEM_1
#define SE1K_QUOTE        VK_OEM_7
#define SE1K_ESCAPE       VK_ESCAPE
#define SE1K_TAB          VK_TAB
#define SE1K_CAPSLOCK     VK_CAPITAL
#define SE1K_RETURN       VK_RETURN
#define SE1K_BACKSPACE    VK_BACK
#define SE1K_SPACE        VK_SPACE
#define SE1K_UP           VK_UP
#define SE1K_DOWN         VK_DOWN
#define SE1K_LEFT         VK_LEFT
#define SE1K_RIGHT        VK_RIGHT
#define SE1K_INSERT       VK_INSERT
#define SE1K_DELETE       VK_DELETE
#define SE1K_HOME         VK_HOME
#define SE1K_END          VK_END
#define SE1K_PAGEUP       VK_PRIOR
#define SE1K_PAGEDOWN     VK_NEXT
#define SE1K_PRINTSCREEN  VK_SNAPSHOT
#define SE1K_SCROLLLOCK   VK_SCROLL
#define SE1K_PAUSE        VK_PAUSE
#define SE1K_F1           VK_F1
#define SE1K_F2           VK_F2
#define SE1K_F3           VK_F3
#define SE1K_F4           VK_F4
#define SE1K_F5           VK_F5
#define SE1K_F6           VK_F6
#define SE1K_F7           VK_F7
#define SE1K_F8           VK_F8
#define SE1K_F9           VK_F9
#define SE1K_F10          VK_F10
#define SE1K_F11          VK_F11
#define SE1K_F12          VK_F12
#define SE1K_GRAVE        VK_OEM_3
#define SE1K_LEFTBRACKET  VK_OEM_4
#define SE1K_RIGHTBRACKET VK_OEM_6
#define SE1K_BACKSLASH    VK_OEM_102
#define SE1K_LSHIFT       VK_LSHIFT
#define SE1K_RSHIFT       VK_RSHIFT
#define SE1K_LCTRL        VK_LCONTROL
#define SE1K_RCTRL        VK_RCONTROL
#define SE1K_LALT         VK_LMENU
#define SE1K_RALT         VK_RMENU
#define SE1K_KP_0         VK_NUMPAD0
#define SE1K_KP_1         VK_NUMPAD1
#define SE1K_KP_2         VK_NUMPAD2
#define SE1K_KP_3         VK_NUMPAD3
#define SE1K_KP_4         VK_NUMPAD4
#define SE1K_KP_5         VK_NUMPAD5
#define SE1K_KP_6         VK_NUMPAD6
#define SE1K_KP_7         VK_NUMPAD7
#define SE1K_KP_8         VK_NUMPAD8
#define SE1K_KP_9         VK_NUMPAD9
#define SE1K_KP_PERIOD    VK_DECIMAL
#define SE1K_NUMLOCKCLEAR VK_NUMLOCK
#define SE1K_KP_DIVIDE    VK_DIVIDE
#define SE1K_KP_MULTIPLY  VK_MULTIPLY
#define SE1K_KP_MINUS     VK_SUBTRACT
#define SE1K_KP_PLUS      VK_ADD
#define SE1K_KP_ENTER     VK_SEPARATOR

#endif // !SE1_PREFER_SDL

#endif // include-once check
