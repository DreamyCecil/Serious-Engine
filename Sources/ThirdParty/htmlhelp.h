/* Copyright (c) 2025 Dreamy Cecil
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

#ifndef CECIL_INCL_HTMLHELP_COMPATIBILITY_H
#define CECIL_INCL_HTMLHELP_COMPATIBILITY_H

// Used commands
#define HH_DISPLAY_TOPIC 0x0000
#define HH_HELP_CONTEXT  0x000F

// Dynamically hook and access HtmlHelp()
inline void HtmlHelp(HWND hwndCaller, LPCSTR pszFile, UINT uCommand, DWORD dwData) {
  typedef HWND (WINAPI *FHtmlHelp)(HWND, LPCSTR, UINT, DWORD);

  // [Cecil] FIXME: Never freed!
  // Load the HTML Help ActiveX control
  static HMODULE hHtmlHelp = LoadLibraryA("hhctrl.ocx");
  static FHtmlHelp pHtmlHelpFunc = NULL;

  if (hHtmlHelp != NULL && pHtmlHelpFunc == NULL) {
    pHtmlHelpFunc = (FHtmlHelp)GetProcAddress(hHtmlHelp, "HtmlHelpA");
  }

  if (pHtmlHelpFunc != NULL) {
    pHtmlHelpFunc(hwndCaller, pszFile, uCommand, dwData);
  }
};

#endif // include-once check
