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

#ifndef SE_INCL_CRCTABLE_H
#define SE_INCL_CRCTABLE_H
#ifdef PRAGMA_ONCE
  #pragma once
#endif

// [Cecil] Reworked interface for dealing with CRC tables
namespace ICRCTable {

extern BOOL _bGatherCRCs;  // set while gathering CRCs of all loaded files

// init CRC table
void Init(void);

// add one file to active list
void AddFile_t(const CTFileName &fnm); // throw char *

// check if a file is added
BOOL IsFileAdded(const CTFileName &fnm);

// reset all files to not active
void ResetActiveList(void);

// free all memory used by the crc cache
void Clear(void);

// dump list of all active files to the stream
void MakeFileList_t(CTStream &strmFiles); // throw char *

// dump checksums for all files from the list
ULONG MakeCRCForFiles_t(CTStream &strmFiles); // throw char *

};

#endif  /* include-once check. */

