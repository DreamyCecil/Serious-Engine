/* Copyright (c) 2023 Dreamy Cecil
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

#include "FileSystem.h"

#if !SE1_WIN
  #include <sys/stat.h>
#endif

FileSystem::Search::Search()
{
#if SE1_WIN
  hFile = -1;
#else
  dDir = NULL;
  dent = NULL;
#endif
};

FileSystem::Search::~Search()
{
#if SE1_WIN
  if (hFile != -1) _findclose(hFile);
#else
  if (dDir != NULL) closedir(dDir);
#endif
};

// Find first file in a directory
BOOL FileSystem::Search::FindFirst(const char *strDir)
{
#if SE1_WIN
  hFile = _findfirst(strDir, &fdSearch);
  return (hFile != -1);

#else
  CTString strUnixDir = strDir;
  strUnixDir.ReplaceChar('\\', '/'); // [Cecil] NOTE: For opendir()

  dDir = opendir(strUnixDir.FileDir().ConstData());

  if (dDir == NULL) return FALSE;

  dent = readdir(dDir);
  return (dent != NULL);
#endif
};

// Find next file
BOOL FileSystem::Search::FindNext(void)
{
#if SE1_WIN
  ASSERT(hFile != -1);
  return (_findnext(hFile, &fdSearch) == 0);

#else
  ASSERT(dDir != NULL);

  dent = readdir(dDir);
  return (dent != NULL);
#endif
};

// Check if it's a dummy/parent directory
BOOL FileSystem::Search::IsDummy(void) const
{
  const char *strName = GetName();
  return (strcmp(strName, ".") == 0) || (strcmp(strName, "..") == 0);
};

// Get name of the current file/directory
const char *FileSystem::Search::GetName(void) const
{
#if SE1_WIN
  return fdSearch.name;

#else
  ASSERT(dent != NULL);
  return dent->d_name;
#endif
};

// Check if specified filename exists on disk
BOOL FileSystem::Exists(const char *strFilename)
{
#if SE1_WIN
  FILE *f = FileSystem::Open(strFilename, "rb");

  if (f != NULL) { 
    fclose(f);
    return TRUE;
  }

  return FALSE;

#else
  CTString strUnixFilename = strFilename;
  strUnixFilename.ReplaceChar('\\', '/'); // [Cecil] NOTE: For stat()

  struct stat s;
  return (stat(strUnixFilename.ConstData(), &s) != -1);
#endif
};

// Check if specified filename is a directory
BOOL FileSystem::IsDirectory(const char *strFilename)
{
#if SE1_WIN
  DWORD dwAttrib = GetFileAttributesA(strFilename);
  return (dwAttrib != -1 && (dwAttrib & FILE_ATTRIBUTE_DIRECTORY));

#else
  CTString strUnixFilename = strFilename;
  strUnixFilename.ReplaceChar('\\', '/'); // [Cecil] NOTE: For stat()

  struct stat s;
  if (stat(strUnixFilename.ConstData(), &s) == -1) return FALSE;

  return (S_ISDIR(s.st_mode) ? TRUE : FALSE);
#endif
};

// Check if some path matches a wildcard (should be prioritized over simple CTString::Matches() method)
BOOL FileSystem::PathMatches(CTString strPath, CTString strMatch) {
  // Match backward slashes for consistency
  strPath.ReplaceChar('/', '\\');
  strMatch.ReplaceSubstr("/", "\\\\");

  return strPath.Matches(strMatch);
};

// Universal method for opening files via fopen()
FILE *FileSystem::Open(CTString strFilename, const char *strMode) {
#if !SE1_WIN
  strFilename.ReplaceChar('\\', '/'); // [Cecil] NOTE: For fopen()
#endif

  return fopen(strFilename.ConstData(), strMode);
};
