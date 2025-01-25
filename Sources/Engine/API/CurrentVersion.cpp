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

#include "StdH.h"

// Get current engine build version as a readable string
CTString SE_GetBuildVersion(void) {
  static CTString _strBuildVer = "????-??-?? ??:??:?? | UNKNOWN";
  static bool _bConstructBuildVersion = true;

  // Construct the build version string only once
  if (_bConstructBuildVersion) {
    _bConstructBuildVersion = false;

    // Start with the build time with a space between the date and the time
    _strBuildVer = SE1_BUILD_DATETIME;
    _strBuildVer.ReplaceChar('T', ' ');

    // Append only the first 7 characters of the commit hash
    CTString strHash = SE1_CURRENT_COMMIT_HASH;
    _strBuildVer += " | " + strHash.Erase(7);
  }

  return _strBuildVer;
};
