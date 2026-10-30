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

#include <Engine/Base/CRCTable.h>
#include <Engine/Base/FileName.h>
#include <Engine/Base/Stream.h>
#include <Engine/Base/Console.h>
#include <Engine/Base/CRC.h>
#include <Engine/Templates/DynamicStackArray.cpp>
#include <Engine/Templates/NameTable.h>

extern INDEX net_bReportCRC;

class CCRCEntry {
public:
  // [Cecil] CRC entry flags
  enum ECRCEntryFlags {
    CRCEF_NONE   = 0,
    CRCEF_ACTIVE = (1UL << 0), // Whether the file is now active for CRC checking (replacement for old ce_bActive field)
    CRCEF_IGNORE = (1UL << 1), // Whether to ignore file's CRC value in calculations (instead of setting ce_ulCRC to 0x12345678 like before)
  };

public:
  CTFileName ce_fnmFile; // the file that CRC is for
  ULONG ce_ulCRC; // CRC of the file
  ULONG ce_ulFlags; // [Cecil] Behavior flags

public:
  CCRCEntry() : ce_ulCRC(0), ce_ulFlags(CRCEF_NONE) {};

  // filename is its name (used for storing in nametable)
  inline const CTFileName &GetName(void) { return ce_fnmFile; };

  void Clear(void) {
    ce_fnmFile.Clear(); 
    ce_ulCRC = 0;
    ce_ulFlags = CRCEF_NONE;
  };
};

extern CDynamicStackArray<CTFileName> _afnmNoCRC;
static CDynamicStackArray<CCRCEntry> _aceEntries;
static CNameTable<CCRCEntry> _ntEntries;

// [Cecil] Reworked interface for dealing with CRC tables
namespace ICRCTable {

BOOL _bGatherCRCs = FALSE;  // set while gathering CRCs of all loaded files

// init CRC table
void Init(void)
{
  _ntEntries.SetAllocationParameters(50, 10, 10);
}

// check if a file is added
BOOL IsFileAdded(const CTFileName &fnm)
{
  return _ntEntries.Find(fnm)!=NULL;
}

// add one file to active list and get its crc
void AddFile_t(const CTFileName &fnm) // throw char *
{
  // Not gathering CRCs now
  if (!_bGatherCRCs) return;

  // try to find it in table
  CCRCEntry *pce = _ntEntries.Find(fnm);

  BOOL bNew = FALSE;
  // if found
  if (pce!=NULL) {
    // just activate it
    bNew = !(pce->ce_ulFlags & CCRCEntry::CRCEF_ACTIVE);

  // if not found
  } else {
    // add to the table
    pce = &_aceEntries.Push();

    // [Cecil] TODO: Maybe hardcode the files from the default NoCRC list to always be ignored no matter what?
    // Because vanilla gameplay isn't affected in any way by the contents of, let's say, textures and sounds.
    // [Cecil] FIXME: By always calculating CRC for every file instead of ignoring it if it's in the NoCRC list,
    // the game may become slower than usual in multiplayer because GetFileCRC32_t() is executed on every file now.

    // calculate checksum
    pce->ce_ulCRC = GetFileCRC32_t(fnm);

    if (FileMatchesList(_afnmNoCRC, fnm)) {
      // [Cecil] Instead of overwriting CRC value as 0x12345678, mark it as ignored
      pce->ce_ulFlags |= CCRCEntry::CRCEF_IGNORE;
    }

    pce->ce_fnmFile = fnm;
    _ntEntries.Add(pce);
    bNew = TRUE;
  }

  // [Cecil] Mark entry as active if it's not ignored
  if (!(pce->ce_ulFlags & CCRCEntry::CRCEF_IGNORE)) {
    pce->ce_ulFlags |= CCRCEntry::CRCEF_ACTIVE;
  }

  if (bNew && net_bReportCRC) {
    CPrintF("CRC %08x: '%s'\n", pce->ce_ulCRC, pce->ce_fnmFile.ConstData());
  }
}

// free all memory used by the crc cache
void Clear(void)
{
  _ntEntries.Clear();
  _aceEntries.Clear();
}

// reset all files to not active
void ResetActiveList(void)
{
  for(INDEX ice=0; ice<_aceEntries.Count(); ice++) {
    _aceEntries[ice].ce_ulFlags &= ~CCRCEntry::CRCEF_ACTIVE;
  }
}

static INDEX GetNumberOfActiveEntries(void)
{
  INDEX ctActive = 0;
  for(INDEX ice=0; ice<_aceEntries.Count(); ice++) {
    CCRCEntry &ce = _aceEntries[ice];

    if (ce.ce_ulFlags & CCRCEntry::CRCEF_ACTIVE) {
      ctActive++;
    }
  }
  return ctActive;
}

// dump list of all active files to the stream
void MakeFileList_t(CTStream &strmFiles)  // throw char *
{
  // save number of active entries
  INDEX ctActive = GetNumberOfActiveEntries();
  strmFiles<<ctActive;
  // for each active entry
  for(INDEX ice=0; ice<_aceEntries.Count(); ice++) {
    CCRCEntry &ce = _aceEntries[ice];

    if (ce.ce_ulFlags & CCRCEntry::CRCEF_ACTIVE) {
      // save name to stream
      strmFiles << ce.ce_fnmFile;
    }
  }
}

// dump checksums for all files from the list
ULONG MakeCRCForFiles_t(CTStream &strmFiles)  // throw char *
{
  const BOOL bOld = _bGatherCRCs;
  _bGatherCRCs = TRUE;

  ULONG ulCRC;
  CRC_Start(ulCRC);
  // read number of active files
  INDEX ctFiles;
  strmFiles>>ctFiles;
  // for each one
  for(INDEX i=0; i<ctFiles; i++) {
    // read the name
    CTString strName;
    strmFiles>>strName;
    // try to find it in table
    CCRCEntry *pce = _ntEntries.Find(strName);
    // if not there
    if (pce==NULL) {
      AddFile_t(strName);
      // add it now
      pce = _ntEntries.Find(strName);
    }
    // add the crc
    CRC_AddLONG(ulCRC, pce->ce_ulCRC);
  }
  _bGatherCRCs = bOld;
  CRC_Finish(ulCRC);
  return ulCRC;
}

}; // namespace
