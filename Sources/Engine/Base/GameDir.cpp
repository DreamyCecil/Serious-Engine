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

#include "StdH.h"

#if SE1_WIN

#include <direct.h>

#else

#include <sys/stat.h>

// [Cecil] Stolen from https://github.com/icculus/Serious-Engine
// Stolen from SDL2/src/filesystem/unix/SDL_sysfilesystem.c
static char *readSymLink(const char *path) {
  char *retval = NULL;
  ssize_t len = 64;
  ssize_t rc = -1;

  while (1) {
    char *ptr = (char *)SDL_realloc(retval, (size_t)len);

    if (ptr == NULL) {
      SDL_OutOfMemory();
      break;
    }

    retval = ptr;
    rc = readlink(path, retval, len);

    if (rc == -1) {
      break; // not a symlink, i/o error, etc.

    } else if (rc < len) {
      retval[rc] = '\0'; // readlink doesn't null-terminate.

      // try to shrink buffer...
      ptr = (char *)SDL_realloc(retval, strlen(retval) + 1);

      if (ptr != NULL) {
        retval = ptr; // oh well if it failed.
      }

      return retval; // we're good to go.
    }

    len *= 2; // grow buffer, try again.
  }

  SDL_free(retval);
  return NULL;
};

#endif // SE1_WIN

// Global string with an absolute path to the main game directory
static CTFileName _fnmInternalAppPath;

// Global string with a relative path to the filename of the started application
static CTFileName _fnmInternalAppExe;

// References to internal variables
const CTFileName &_fnmApplicationPath = _fnmInternalAppPath;
const CTFileName &_fnmApplicationExe = _fnmInternalAppExe;

// Determine application paths for the first time
void DetermineAppPaths(CTString strSpecifiedRootDir) {
  // Get full path to the executable module
  // E.g. "C:\\SeriousSam\\Bin\\x64\\SeriousSam.exe"
  char strPathBuffer[1024];

#if SE1_WIN
  GetModuleFileNameA(NULL, strPathBuffer, sizeof(strPathBuffer));
#else
  char *strTempExePath = readSymLink("/proc/self/exe");
  strcpy(strPathBuffer, strTempExePath);
  SDL_free(strTempExePath);
#endif

  // Normalize path to the executable
  CTString strExePath(strPathBuffer);
  strExePath.NormalizePath();

  // Begin determining the root directory
  CTString strRootPath = strExePath;
  strRootPath.ReplaceChar('/', '\\');

  // Cut off module filename to end up with the directory
  // E.g. "C:\\SeriousSam\\Bin\\x64\\"
  strRootPath.Erase(strRootPath.RFind('\\') + 1);

  BOOL bSpecifiedRootDir = FALSE;

  // Check if the specified root directory is valid
  if (strSpecifiedRootDir != "") {
    const BOOL bAbsolute = strSpecifiedRootDir.SetFullDirectory();

    // Add executable path to the beginning of the relative directory and normalize it
    if (!bAbsolute) {
      strSpecifiedRootDir = strRootPath + strSpecifiedRootDir;
      strSpecifiedRootDir.NormalizePath();
    }

    // Use the specified directory if it's a part of the path to the executable
    if (strRootPath.HasPrefix(strSpecifiedRootDir)) {
      strRootPath.Erase(strSpecifiedRootDir.Length());
      bSpecifiedRootDir = TRUE;
    }
  }

  // Determine root directory from the Bin folder
  if (!bSpecifiedRootDir) {
    // Check if there's a Bin folder somewhere in the middle or at the end
    size_t iPos = strRootPath.RFind("\\Bin\\");

    if (iPos != CTString::npos) {
      strRootPath.Erase(iPos + 1);
    }
  }

  // Get cut-off position before the Bin directory
  // E.g. between "C:\\SeriousSam\\" and "Bin\\x64\\SeriousSam.exe"
  const size_t iBinDir = strRootPath.Length();

  // Copy absolute path to the game directory and relative path to the executable
  strExePath.Split((INDEX)iBinDir, _fnmInternalAppPath, _fnmInternalAppExe);
};

// Create a series of directories within the game folder
void CreateAllDirectories(CTString strPath) {
  // Make sure it's an absolute path
  strPath = ExpandPath::OnDisk(strPath);
  strPath.ReplaceChar('\\', '/'); // [Cecil] NOTE: For _mkdir()

  size_t iDir = 0;

  // Get next directory from the last position
  while ((iDir = strPath.Find('/', iDir)) != CTString::npos) {
    // Include the slash
    iDir++;

    // Create current subdirectory
    int iDummy = _mkdir(strPath.Substr(0, iDir).ConstData());
    (void)iDummy;
  }
};
