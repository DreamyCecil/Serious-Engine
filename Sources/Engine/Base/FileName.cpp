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

#include "StdH.h"

#include <Engine/Base/FileName.h>

#include <list>

// [Cecil] Check if there's a path separator character at some position
bool CTString::PathSeparatorAt(size_t i) const {
  return str_String[i] == '/' || str_String[i] == '\\';
};

// [Cecil] Remove directory from the filename
CTString CTString::NoDir(void) const {
  return Substr(FindLastOf("/\\") + 1);
};

// [Cecil] Remove extension from the filename
CTString CTString::NoExt(void) const {
  const size_t iPeriodPos(RFind('.'));
  const size_t iLastDir(FindLastOf("/\\"));

  // No period found or it's before the last directory
  if (iPeriodPos == npos || (iLastDir != npos && iPeriodPos < iLastDir)) {
    return *this;
  }

  return Substr(0, iPeriodPos);
};

// [Cecil] Get name of the file
CTString CTString::FileName(void) const {
  return NoDir().NoExt();
};

// [Cecil] Get path to the file
CTString CTString::FileDir(void) const {
  const size_t iLastDirectory(FindLastOf("/\\") + 1);
  return Substr(0, iLastDirectory);
};

// [Cecil] Get file extension with the period
CTString CTString::FileExt(void) const {
  const size_t iPeriodPos(RFind('.'));
  const size_t iLastDir(FindLastOf("/\\"));

  // No period found or it's before the last directory
  if (iPeriodPos == npos || (iLastDir != npos && iPeriodPos < iLastDir)) {
    return "";
  }

  return Substr(iPeriodPos);
};

// [Cecil] Go up the path until a certain directory
size_t CTString::GoUpUntilDir(CTString strDirName) const {
  // Convert every string in the same case
  CTString strPath(*this);
  strPath.ToLower();
  strDirName.ToLower();

  // Make consistent slashes
  strPath.ReplaceChar('\\', '/'); // [Cecil] NOTE: Internally for GoUpUntilDir()

  // Absolute path, e.g. "abc/strDirName/qwe"
  size_t iDir(strPath.RFind("/" + strDirName + "/"));
  if (iDir != npos) return iDir + 1;

  // Relative down to the desired directory, e.g. "abc/qwe/strDirName"
  iDir = strPath.RFind("/" + strDirName) + 1;
  if (iDir == strPath.Length() - strDirName.Length()) return iDir;

  // Relative up to the desired directory, e.g. "strDirName/abc/qwe"
  iDir = strPath.Find(strDirName + "/");
  if (iDir == 0) return 0;

  // No extra directories up or down the path, must be the same
  if (strPath == strDirName) {
    return 0;
  }

  return npos;
};

// [Cecil] Normalize the path taking "backward" and "current" directories into consideration
// E.g. "abc\\sub1\\../sub2/./qwe" -> "abc\\sub2\\qwe"
void CTString::NormalizePath(void) {
  CTString strPath(*this);
  strPath.ReplaceChar('/', '\\');

  // Gather parts of the entire path
  std::list<CTString> aParts;
  strPath.CharSplit('\\', aParts);

  std::list<CTString> aFinalPath;
  std::list<CTString>::const_iterator it;

  // Iterate through the list of directories
  for (it = aParts.begin(); it != aParts.end(); ++it) {
    const CTString &strPart = *it;

    // Ignore current directories
    if (strPart == ".") continue;

    // If encountered a "backward" directory and there are some directories written
    // AND it's not another "backward" directory (that has been written because it can't go up any more)
    if (strPart == ".." && !aFinalPath.empty() && aFinalPath.back() != "..") {
      // Remove the last directory (go up one directory) and skip to the next one
      aFinalPath.pop_back();
      continue;
    }

    // Add directory to the final path
    aFinalPath.push_back(strPart);
  }

  // Reset current path
  *this = "";

  // No path to compose
  if (aFinalPath.size() == 0) return;

  // Compose the final path
  std::list<CTString>::const_iterator itLast = --aFinalPath.end();

  for (it = aFinalPath.begin(); it != aFinalPath.end(); ++it) {
    *this += *it;

    // Add separators between the directories
    if (it != itLast) *this += "\\";
  }
};

// [Cecil] Get length of the root name, if there's any
size_t CTString::RootNameLength() const {
  const size_t ctLen = Length();

#if SE1_WIN
  // Starts with a drive letter and a colon on Windows (e.g. "C:")
  const char chUpper = (char)toupper(static_cast<UBYTE>(str_String[0]));

  if (ctLen >= 2 && chUpper >= 'A' && chUpper <= 'Z' && str_String[1] == ':') {
    return 2;
  }
#endif

  // Starts with a double separator and has any directory right after (e.g. "//abc")
  if (ctLen > 2
   && PathSeparatorAt(0) && PathSeparatorAt(1) && !PathSeparatorAt(2)
   && isprint(static_cast<UBYTE>(str_String[2])))
  {
    // Find the next separator, if there's any
    size_t iNextSep = FindFirstOf("/\\", 3);
    return (iNextSep == npos ? ctLen : iNextSep);
  }

  return 0;
};

/*
 * Remove application path from a file name and returns TRUE if it's a relative path.
 */
BOOL CTString::RemoveApplicationPath_t(void) // throws char *
{
  CTString fnmApp = _fnmApplicationPath;
  fnmApp.NormalizePath();
  // remove the path string from beginning of the string
  BOOL bIsRelative = RemovePrefix(fnmApp);
  if (_fnmMod!="") {
    RemovePrefix(_fnmApplicationPath+_fnmMod);
  }
  return bIsRelative;
}

// [Cecil] Convert from a relative path to an absolute path and add missing backslashes
// Returns TRUE if the path becomes absolute as a result
BOOL CTString::SetFullDirectory(void) {
  CTString &str = *this;
  INDEX iLength = str.Length();

  // Add missing backslash at the end
  if (iLength > 0 && !str.PathSeparatorAt(iLength - 1)) {
    str += "\\";
  }

  // Convert relative path into absolute path
  BOOL bAbsolute = IsAbsolute();

  if (!bAbsolute && _fnmApplicationPath != "") {
    str = _fnmApplicationPath + str;
    bAbsolute = TRUE;
  }

  // Normalize the rest of the path (leave relative path as is)
  if (bAbsolute) str.NormalizePath();

  return bAbsolute;
};

// [Cecil] Read string as a filename from a stream
void CTStream::ReadFileName(CTString &fnmFileName)
{
  // if dictionary is enabled
  if (strm_dmDictionaryMode == CTStream::DM_ENABLED) {
    // read the index in dictionary
    INDEX iFileName;
    *this >> iFileName;
    // get that file from the dictionary
    fnmFileName = strm_afnmDictionary[iFileName];

  // if dictionary is processing or not active
  } else {
    char strTag[] = "_FNM"; strTag[0] = 'D';  // must create tag at run-time!
    // skip dependency catcher header
    ExpectID_t(strTag);

    // read the string
    *this >> fnmFileName;
  }
};

// [Cecil] Write string as a filename into a stream
void CTStream::WriteFileName(const CTString &fnmFileName)
{
  // if dictionary is enabled
  if (strm_dmDictionaryMode == CTStream::DM_ENABLED) {
    // try to find the filename in dictionary
    CTString *pfnmExisting = strm_ntDictionary.Find(fnmFileName);
    // if not existing
    if (pfnmExisting==NULL) {
      // add it
      pfnmExisting = &strm_afnmDictionary.Push();
      *pfnmExisting = fnmFileName;
      strm_ntDictionary.Add(pfnmExisting);
    }
    // write its index
    *this << strm_afnmDictionary.Index(pfnmExisting);

  // if dictionary is processing or not active
  } else {
    char strTag[] = "_FNM"; strTag[0] = 'D';  // must create tag at run-time!
    // write dependency catcher header
    WriteID_t(strTag);

    // write the string
    *this << fnmFileName;
  }
};
