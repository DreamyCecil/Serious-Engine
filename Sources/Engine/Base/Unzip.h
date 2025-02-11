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

#ifndef SE_INCL_UNZIP_H
#define SE_INCL_UNZIP_H
#ifdef PRAGMA_ONCE
  #pragma once
#endif

// [Cecil] Reworked interface for dealing with ZIP archives
namespace IZip {

// Add one ZIP archive to current set
void AddArchive(const CTString &fnm);

// Read directories of all currently added archives in reverse alphabetical order
void ReadDirectoriesReverse_t(void);

// File entry in a ZIP archive
class CEntry {
  private:
    const CTString *ze_pfnmArchive; // Path of the archive
    CTString ze_fnm;             // File name with path inside archive
    SLONG ze_slCompressedSize;   // Size of file in the archive
    SLONG ze_slUncompressedSize; // Size when uncompressed
    SLONG ze_slDataOffset;       // Position of compressed data inside archive
    ULONG ze_ulCRC;              // Checksum of the file
    BOOL ze_bStored;             // Set if file is not compressed, but stored
    BOOL ze_bMod;                // Set if from a mod's archive

  public:
    inline void Clear(void) {
      ze_pfnmArchive = NULL;
      ze_fnm.Clear();
    };

    void SetArchive(const CTString *pfnm);
    void SetFileName(const CTString &fnm);
    void SetCompressedSize(SLONG slSize);
    void SetUncompressedSize(SLONG slSize);
    void SetDataOffset(SLONG slOffset);
    void SetCRC(ULONG ulCRC);
    void SetStored(BOOL bState);
    void SetMod(BOOL bState);

    // Get path to the archive where this entry is from
    inline const CTString &GetArchive(void) const { return *ze_pfnmArchive; };

    // Get full path to the file inside the archive
    inline const CTString &GetFileName(void) const { return ze_fnm; };

    // Get compressed size of the file
    inline SLONG GetCompressedSize(void) const { return ze_slCompressedSize; };

    // Get real size of the file
    inline SLONG GetUncompressedSize(void) const { return ze_slUncompressedSize; };

    // Get offset of the file inside the archive
    inline SLONG GetDataOffset(void) const { return ze_slDataOffset; };

    // Get file's CRC value
    inline ULONG GetCRC(void) const { return ze_ulCRC; };

    // Check if the file is stored as is with no compression
    inline BOOL IsStored(void) const { return ze_bStored; };

    // Check if this file is from an archive of an active mod
    inline BOOL IsMod(void) const { return ze_bMod; };
};

// ZIP file handle that manages a specific entry
typedef class CHandle *Handle_t;

// Get amount of all ZIP entries
INDEX GetEntryCount(void);

// Get ZIP file entry at a specific position in the [0, GetEntryCount() - 1] range
const CEntry *GetEntry(INDEX i);

// Get ZIP file entry from its handle
const CEntry *GetEntry(Handle_t pHandle);

// Try to find ZIP file entry by its file path
const CEntry *FindEntry(const CTString &fnm);

// Open a ZIP file for reading
Handle_t Open_t(const CTString &fnm);

// Read a block of data from a ZIP file
void ReadBlock_t(Handle_t pHandle, UBYTE *pub, SLONG slStart, SLONG slLen);

// Close a ZIP file
void Close(Handle_t pHandle);

}; // namespace

#endif  /* include-once check. */
