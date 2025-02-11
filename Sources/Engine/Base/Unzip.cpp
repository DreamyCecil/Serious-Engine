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

// unzip.cpp : Defines the entry point for the console application.
//

#include "StdH.h"

#include <Engine/Base/Unzip.h>

#include <zlib/zlib.h>
#pragma comment(lib, "zlib.lib")

// Critical section for accessing zlib functions
CTCriticalSection zip_csLock;

#pragma pack(1)

// before each file in the zip
#define SIGNATURE_LFH 0x04034b50
struct LocalFileHeader {
  SWORD lfh_swVersionToExtract;
  SWORD lfh_swGPBFlag;
  SWORD lfh_swCompressionMethod;
  SWORD lfh_swModFileTime;
  SWORD lfh_swModFileDate;
  SLONG lfh_slCRC32;
  SLONG lfh_slCompressedSize;
  SLONG lfh_slUncompressedSize;
  SWORD lfh_swFileNameLen;
  SWORD lfh_swExtraFieldLen;

// follows:
//	filename (variable size)
//  extra field (variable size)
};

// after file data, only if compressed from a non-seekable stream
// this exists only if bit 3 in GPB flag is set
#define SIGNATURE_DD 0x08074b50
struct DataDescriptor {
  SLONG dd_slCRC32;
  SLONG dd_slCompressedSize;
  SLONG dd_slUncompressedSize;
};

// one file in central dir
#define SIGNATURE_FH 0x02014b50
struct FileHeader {
  SWORD fh_swVersionMadeBy;
  SWORD fh_swVersionToExtract;
  SWORD fh_swGPBFlag;
  SWORD fh_swCompressionMethod;
  SWORD fh_swModFileTime;
  SWORD fh_swModFileDate;
  SLONG fh_slCRC32;
  SLONG fh_slCompressedSize;
  SLONG fh_slUncompressedSize;
  SWORD fh_swFileNameLen;
  SWORD fh_swExtraFieldLen;
  SWORD fh_swFileCommentLen;
  SWORD fh_swDiskNoStart;
  SWORD fh_swInternalFileAttributes;
  SLONG fh_swExternalFileAttributes;
  SLONG fh_slLocalHeaderOffset;

// follows:
//  filename (variable size)
//  extra field (variable size)
//  file comment (variable size)
};

// at the end of entire zip file
#define SIGNATURE_EOD 0x06054b50
struct EndOfDir {
  SWORD eod_swDiskNo;
  SWORD eod_swDirStartDiskNo;
  SWORD eod_swEntriesInDirOnThisDisk;
  SWORD eod_swEntriesInDir;
  SLONG eod_slSizeOfDir;
  SLONG eod_slDirOffsetInFile;
  SWORD eod_swCommentLenght;
// follows:
//  zipfile comment (variable size)
};

#pragma pack()

// [Cecil] Reworked interface for dealing with ZIP archives
namespace IZip {

void CEntry::SetArchive(const CTString *pfnm) {
  ze_pfnmArchive = pfnm;
};

void CEntry::SetFileName(const CTString &fnm) {
  ze_fnm = fnm;
};

void CEntry::SetCompressedSize(SLONG slSize) {
  ze_slCompressedSize = slSize;
};

void CEntry::SetUncompressedSize(SLONG slSize) {
  ze_slUncompressedSize = slSize;
};

void CEntry::SetDataOffset(SLONG slOffset) {
  ze_slDataOffset = slOffset;
};

void CEntry::SetCRC(ULONG ulCRC) {
  ze_ulCRC = ulCRC;
};

void CEntry::SetStored(BOOL bState) {
  ze_bStored = bState;
};

void CEntry::SetMod(BOOL bState) {
  ze_bMod = bState;
};

// ZIP file handle that manages a specific entry
class CHandle {
  public:
    BOOL zh_bOpen;          // set if the handle is used
    CEntry zh_zeEntry;      // the entry itself
    z_stream zh_zstream;    // zlib filestream for decompression
    FILE *zh_fFile;         // open handle of the archive
    UBYTE *zh_pubBufIn;     // input buffer

  public:
    CHandle(void);

    void Clear(void);
    void ThrowZLIBError_t(int ierr, const CTString &strDescription);
};

const size_t _ctHandleBufferSize = 1024;

CHandle::CHandle(void) {
  zh_bOpen = FALSE;
  zh_fFile = NULL;
  zh_pubBufIn = NULL;
  memset(&zh_zstream, 0, sizeof(zh_zstream));
};

void CHandle::Clear(void) {
  zh_bOpen = FALSE;
  zh_zeEntry.Clear();

  // Clear the zlib stream
  CTSingleLock slZip(&zip_csLock, TRUE);
  inflateEnd(&zh_zstream);
  memset(&zh_zstream, 0, sizeof(zh_zstream));

  // Free the buffer
  if (zh_pubBufIn != NULL) {
    FreeMemory(zh_pubBufIn);
    zh_pubBufIn = NULL;
  }

  // Close the ZIP archive itself
  if (zh_fFile != NULL) {
    fclose(zh_fFile);
    zh_fFile = NULL;
  }
};

void CHandle::ThrowZLIBError_t(int ierr, const CTString &strDescription) {
  CTString strZlibError;

  switch (ierr) {
    case Z_OK:            strZlibError = TRANS("Z_OK           "); break;
    case Z_STREAM_END:    strZlibError = TRANS("Z_STREAM_END   "); break;
    case Z_NEED_DICT:     strZlibError = TRANS("Z_NEED_DICT    "); break;
    case Z_STREAM_ERROR:  strZlibError = TRANS("Z_STREAM_ERROR "); break;
    case Z_DATA_ERROR:    strZlibError = TRANS("Z_DATA_ERROR   "); break;
    case Z_MEM_ERROR:     strZlibError = TRANS("Z_MEM_ERROR    "); break;
    case Z_BUF_ERROR:     strZlibError = TRANS("Z_BUF_ERROR    "); break;
    case Z_VERSION_ERROR: strZlibError = TRANS("Z_VERSION_ERROR"); break;

    case Z_ERRNO: strZlibError.PrintF(TRANS("Z_ERRNO: %s"), strerror(errno)); break;
    default: strZlibError.PrintF(TRANS("Unknown ZLIB error: %d"), ierr); break;
  }

  ThrowF_t(TRANS("(%s/%s) %s - ZLIB error: %s - %s"),
    zh_zeEntry.GetArchive().ConstData(), zh_zeEntry.GetFileName().ConstData(),
    strDescription.ConstData(), strZlibError.ConstData(), zh_zstream.msg);
};

// All files in all active archives
static CStaticStackArray<CEntry> _azeFiles;

// Handles of currently opened files
static CStaticStackArray<CHandle> _azhHandles;

// Filenames of active archives
static CStaticStackArray<CTString> _afnmArchives;

// Add one ZIP archive to current set
void AddArchive(const CTString &fnm) {
  _afnmArchives.Add(fnm);
};

// Read directory of a ZIP archive and add all files from it
static void ReadZIPDirectory_t(const CTString *pfnmZip) {
  const char *strZip = pfnmZip->ConstData();
  FILE *f = FileSystem::Open(strZip, "rb");

  if (f == NULL) {
    ThrowF_t(TRANS("%s: Cannot open file (%s)"), strZip, strerror(errno));
  }

  // Start at the end of the file, minus expected minimum overhead
  fseek(f, 0, SEEK_END);
  int iPos = ftell(f) - sizeof(int) - sizeof(EndOfDir) + 2;

  // Do not search more than 128kb (should be around 65kb at most)
  int iMinPos = ClampDn(iPos - 128 * 1024, 0);

  EndOfDir eod;
  BOOL bEODFound = FALSE;

  for (; iPos > iMinPos; iPos--) {
    // Read signature
    fseek(f, iPos, SEEK_SET);

    int slSig;
    fread(&slSig, sizeof(slSig), 1, f);

    // If this is the signature
    if (slSig == SIGNATURE_EOD) {
      // Read directory end
      fread(&eod, sizeof(eod), 1, f);

      // Check multi-volume archives
      if (eod.eod_swDiskNo != 0 || eod.eod_swDirStartDiskNo != 0
       || eod.eod_swEntriesInDirOnThisDisk != eod.eod_swEntriesInDir) {
        ThrowF_t(TRANS("%s: Multi-volume zips are not supported"), strZip);
      }

      // Check empty archives
      if (eod.eod_swEntriesInDir <= 0) {
        ThrowF_t(TRANS("%s: Empty zip"), strZip);
      }

      bEODFound = TRUE;
      break;
    }
  }

  // Not found
  if (!bEODFound) {
    ThrowF_t(TRANS("%s: Cannot find 'end of central directory'"), strZip);
  }

  // Check if the archive is from a mod
  const BOOL bMod = pfnmZip->HasPrefix(_fnmApplicationPath + SE1_MODS_SUBDIR);

  // Go to the beginning of the central dir
  fseek(f, eod.eod_slDirOffsetInFile, SEEK_SET);
  INDEX ctFiles = 0;

  for (INDEX iFile=0; iFile < eod.eod_swEntriesInDir; iFile++) {
    // Read the signature
    int slSig;
    fread(&slSig, sizeof(slSig), 1, f);

    // Unexpected signature
    if (slSig != SIGNATURE_FH) {
      ThrowF_t(TRANS("%s: Wrong signature for 'file header' number %d'"), strZip, iFile);
    }

    // Read its header
    FileHeader fh;
    fread(&fh, sizeof(fh), 1, f);

    // Read the filename
    const SLONG slMaxFileName = 512;

    CTString strBuffer;
    strBuffer.Fill(slMaxFileName, '\0');

    if (fh.fh_swFileNameLen > slMaxFileName) {
      ThrowF_t(TRANS("%s: Too long filepath in zip"), strZip);
    }

    if (fh.fh_swFileNameLen <= 0) {
      ThrowF_t(TRANS("%s: Invalid filepath length in zip"), strZip);
    }

    fread(strBuffer.Data(), fh.fh_swFileNameLen, 1, f);

    // Skip eventual comment and extra fields
    if (fh.fh_swFileCommentLen + fh.fh_swExtraFieldLen > 0) {
      fseek(f, fh.fh_swFileCommentLen + fh.fh_swExtraFieldLen, SEEK_CUR);
    }

    // If it's a directory
    if (strBuffer[strBuffer.Length() - 1] == '/') {
      // Check size
      if (fh.fh_slUncompressedSize != 0
       || fh.fh_slCompressedSize != 0) {
        ThrowF_t(TRANS("%s/%s: Invalid directory"), strZip, strBuffer.ConstData());
      }

    // If it's a real file
    } else {
      // Create a new entry
      CEntry &ze = _azeFiles.Push();
      ctFiles++;

      // Convert slashes
      strBuffer.ReplaceChar('/', '\\');
      ze.SetFileName(strBuffer);

      ze.SetArchive(pfnmZip);
      ze.SetCompressedSize(fh.fh_slCompressedSize);
      ze.SetUncompressedSize(fh.fh_slUncompressedSize);
      ze.SetDataOffset(fh.fh_slLocalHeaderOffset);
      ze.SetCRC(fh.fh_slCRC32);
      ze.SetMod(bMod);

      // Check for compression
      if (fh.fh_swCompressionMethod == 0) {
        ze.SetStored(TRUE);

      } else if (fh.fh_swCompressionMethod == 8) {
        ze.SetStored(FALSE);

      } else {
        ThrowF_t(TRANS("%s/%s: Only 'deflate' compression is supported"),
          ze.GetArchive().ConstData(), ze.GetFileName().ConstData());
      }
    }
  }

  // Report an error
  if (ferror(f)) {
    ThrowF_t(TRANS("%s: Error reading central directory"), strZip);
  }

  // Report that the file has been read
  CPrintF(TRANS("  %s: %d files\n"), strZip, ctFiles++);
};

// Read directory of one archive
static void ReadOneArchiveDir_t(const CTString &fnm) {
  // Remember current number of files
  const INDEX ctLastFiles = _azeFiles.Count();

  // Try to read the directory and add all files
  try {
    ReadZIPDirectory_t(&fnm);

  // If failed with some files being added
  } catch (char *) {
    if (ctLastFiles < _azeFiles.Count()) {
      // Remove them
      if (ctLastFiles == 0) {
        _azeFiles.PopAll();
      } else {
        _azeFiles.PopUntil(ctLastFiles - 1);
      }
    }

    // Cascade the error
    throw;
  }
};

// [Cecil] Get priority for a specific archive
static INDEX ArchiveDirPriority(CTString fnm)
{
  #define PRI_MOD   10002 // Mod subdirectory
  #define PRI_EXTRA 10001 // Extra content directories
  #define PRI_ROOT  10000 // Main game directory
  #define PRI_GAMES     0 // Extra game directories (PRI_GAMES + directory index inside _aContentDirs)

  // Current game (overrides other games)
  if (fnm.RemovePrefix(_fnmApplicationPath)) {
    // Check for mod (overrides everything)
    return fnm.HasPrefix(SE1_MODS_SUBDIR) ? PRI_MOD : PRI_ROOT;
  }

  // Other game paths
  const INDEX ctDirs = _aContentDirs.Count();

  for (INDEX iDir = 0; iDir < ctDirs; iDir++) {
    const ExtraContentDir_t &dir = _aContentDirs[iDir];
    if (!dir.bGame) continue;

    if (dir.fnmPath != "" && fnm.HasPrefix(dir.fnmPath)) {
      // Sort by list index (doesn't override other files)
      return PRI_GAMES + iDir;
    }
  }

  // None of the above - extra content directory (overrides current game)
  return PRI_EXTRA;
};

// [Cecil] Compare two ZIP file entries
static int qsort_CompareContentDir(const void *pElement1, const void *pElement2)
{
  // Get the entries
  const CTString &fnm1 = *(const CTString *)pElement1;
  const CTString &fnm2 = *(const CTString *)pElement2;

  // Sort archive directories with priority
  INDEX iPriority1 = ArchiveDirPriority(fnm1);
  INDEX iPriority2 = ArchiveDirPriority(fnm2);

  if (iPriority1 < iPriority2) {
    return +1;
  } else if (iPriority1 > iPriority2) {
    return -1;
  }

  // Sort archives in reverse alphabetical order
  return -stricmp(fnm1.ConstData(), fnm2.ConstData());
};

// Read directories of all currently added archives in reverse alphabetical order
void ReadDirectoriesReverse_t(void) {
  // No archives
  const INDEX ctArchives = _afnmArchives.Count();
  if (ctArchives == 0) return;

  // [Cecil] Sort archives by content directory. Order after sorting:
  // 1. From mod
  // 2. From extra content directories
  // 3. From the game itself
  // 4. From other game directories
  qsort(&_afnmArchives[0], ctArchives, sizeof(CTString), qsort_CompareContentDir);

  CTString strAllErrors = "";

  for (INDEX iArchive = 0; iArchive < ctArchives; iArchive++) {
    // Try to read archive's directory
    try {
      ReadOneArchiveDir_t(_afnmArchives[iArchive]);

    // Otherwise remember the error
    } catch (char *strError) {
      strAllErrors += strError;
      strAllErrors += "\n";
    }
  }

  // Report any errors
  if (strAllErrors != "") strAllErrors.Throw_t();
};

// Get amount of all ZIP entries
INDEX GetEntryCount(void) {
  return _azeFiles.Count();
};

// [Cecil] Get ZIP file entry at a specific position
const CEntry *GetEntry(INDEX i) {
  if (i < 0 || i >= _azeFiles.Count()) {
    ASSERTALWAYS("ZIP entry index out of range!");
    return NULL;
  }

  return &_azeFiles[i];
};

// Get ZIP file entry from its handle
const CEntry *GetEntry(Handle_t pHandle) {
  if (pHandle == NULL || !pHandle->zh_bOpen) {
    ASSERT(FALSE);
    return NULL;
  }

  return &pHandle->zh_zeEntry;
};

// Try to find ZIP file entry by its file path
const CEntry *FindEntry(const CTString &fnm) {
  const INDEX ctFiles = _azeFiles.Count();

  for (INDEX iFile = 0; iFile < ctFiles; iFile++) {
    if (_azeFiles[iFile].GetFileName() == fnm) {
      return &_azeFiles[iFile];
    }
  }

  return NULL;
};

// Open a ZIP file for reading
Handle_t Open_t(const CTString &fnm) {
  // Find an entry with this filename
  const CEntry *pze = FindEntry(fnm);

  if (pze == NULL) ThrowF_t(TRANS("File not found: %s"), fnm.ConstData());

  // Try to find an unused handle in the stack
  Handle_t pHandle = NULL;
  const INDEX ctHandles = _azhHandles.Count();

  for (INDEX iHandle = 1; iHandle < ctHandles; iHandle++) {
    if (!_azhHandles[iHandle].zh_bOpen) {
      pHandle = &_azhHandles[iHandle];
      break;
    }
  }

  // Create a new handle if none found
  if (pHandle == NULL) {
    pHandle = &_azhHandles.Push();
  }

  // Get the handle
  CHandle &zh = *pHandle;
  ASSERT(!zh.zh_bOpen);

  zh.zh_zeEntry = *pze;

  // Open the archive for reading
  zh.zh_fFile = FileSystem::Open(zh.zh_zeEntry.GetArchive(), "rb");

  // Failed to open it
  if (zh.zh_fFile == NULL) {
    zh.Clear();
    ThrowF_t(TRANS("Cannot open '%s': %s"), zh.zh_zeEntry.GetArchive().ConstData(), strerror(errno));
  }

  // Go to the local header of the entry
  fseek(zh.zh_fFile, zh.zh_zeEntry.GetDataOffset(), SEEK_SET);

  // Read the signature
  int slSig;
  fread(&slSig, sizeof(slSig), 1, zh.zh_fFile);

  // Unexpected signature
  if (slSig != SIGNATURE_LFH) {
    ThrowF_t(TRANS("%s/%s: Wrong signature for 'local file header'"),
      zh.zh_zeEntry.GetArchive().ConstData(), zh.zh_zeEntry.GetFileName().ConstData());
  }

  // Read the header
  LocalFileHeader lfh;
  fread(&lfh, sizeof(lfh), 1, zh.zh_fFile);

  // Determine the exact compressed data position
  const SLONG slOffset = ftell(zh.zh_fFile) + lfh.lfh_swFileNameLen + lfh.lfh_swExtraFieldLen;
  zh.zh_zeEntry.SetDataOffset(slOffset);

  // And go there
  fseek(zh.zh_fFile, slOffset, SEEK_SET);

  // Allocate the buffer
  zh.zh_pubBufIn = (UBYTE *)AllocMemory(_ctHandleBufferSize);

  // Initialize zlib stream
  CTSingleLock slZip(&zip_csLock, TRUE);

  zh.zh_zstream.next_out  = NULL;
  zh.zh_zstream.avail_out = 0;
  zh.zh_zstream.next_in   = NULL;
  zh.zh_zstream.avail_in  = 0;
  zh.zh_zstream.zalloc = (alloc_func)Z_NULL;
  zh.zh_zstream.zfree = (free_func)Z_NULL;

  int err = inflateInit2(&zh.zh_zstream, -15); // 32k windows

  // If failed
  if (err != Z_OK) {
    // Clean up everything
    FreeMemory(zh.zh_pubBufIn);
    zh.zh_pubBufIn  = NULL;

    fclose(zh.zh_fFile);
    zh.zh_fFile = NULL;

    // And throw an error
    zh.ThrowZLIBError_t(err, TRANS("Cannot init inflation"));
  }

  // Return the handle successfully
  zh.zh_bOpen = TRUE;
  return pHandle;
};

// Read a block of data from a ZIP file
void ReadBlock_t(Handle_t pHandle, UBYTE *pub, SLONG slStart, SLONG slLen) {
  if (pHandle == NULL || !pHandle->zh_bOpen) {
    ASSERT(FALSE);
    return;
  }

  CHandle &zh = *pHandle;
  CEntry &ze = zh.zh_zeEntry;

  // Nothing to read
  if (slStart >= ze.GetUncompressedSize()) return;

  // Clamp length to the end of the entry data
  slLen = Min(slLen, ze.GetUncompressedSize() - slStart);

  // If not compressed
  if (ze.IsStored()) {
    // Just read from the file
    fseek(zh.zh_fFile, ze.GetDataOffset() + slStart, SEEK_SET);
    fread(pub, 1, slLen, zh.zh_fFile);
    return;
  }

  CTSingleLock slZip(&zip_csLock, TRUE);

  // If behind the current pointer
  if (slStart < zh.zh_zstream.total_out) {
    // Reset zlib stream to the beginning
    inflateReset(&zh.zh_zstream);
    zh.zh_zstream.avail_in = 0;
    zh.zh_zstream.next_in = NULL;

    // Seek to the start of the ZIP entry data inside the archive
    fseek(zh.zh_fFile, ze.GetDataOffset(), SEEK_SET);
  }

  // While ahead of the current pointer
  while (slStart > zh.zh_zstream.total_out)
  {
    // If zlib has no more input
    while (zh.zh_zstream.avail_in == 0) {
      // Read more into it
      size_t slRead = fread(zh.zh_pubBufIn, 1, _ctHandleBufferSize, zh.zh_fFile);
      if (slRead == 0) return; // !!!!

      // Tell zlib that there is more to read
      zh.zh_zstream.next_in = zh.zh_pubBufIn;
      zh.zh_zstream.avail_in = (uInt)slRead;
    }

    // Read dummy data from the output
    const SLONG slDummySize = 256;
    UBYTE aubDummy[slDummySize];

    // Decode to output
    zh.zh_zstream.avail_out = Min(slStart - zh.zh_zstream.total_out, slDummySize);
    zh.zh_zstream.next_out = aubDummy;

    int ierr = inflate(&zh.zh_zstream, Z_SYNC_FLUSH);

    if (ierr != Z_OK && ierr != Z_STREAM_END) {
      zh.ThrowZLIBError_t(ierr, TRANS("Error seeking in zip"));
    }
  }

  // If not streaming continuously
  if (slStart != zh.zh_zstream.total_out) {
    // This shouldn't happen, so read nothing
    ASSERT(FALSE);
    memset(pub, 0, slLen);
    return;
  }

  // Setup zlib for writing into the block
  zh.zh_zstream.avail_out = slLen;
  zh.zh_zstream.next_out = pub;

  // While there is something to write to the given block
  while (zh.zh_zstream.avail_out > 0)
  {
    // If zlib has no more input
    while (zh.zh_zstream.avail_in == 0) {
      // Read more into it
      size_t slRead = fread(zh.zh_pubBufIn, 1, _ctHandleBufferSize, zh.zh_fFile);
      if (slRead == 0) return; // !!!!

      // Tell zlib that there is more to read
      zh.zh_zstream.next_in = zh.zh_pubBufIn;
      zh.zh_zstream.avail_in = (uInt)slRead;
    }

    // Decode to output
    int ierr = inflate(&zh.zh_zstream, Z_SYNC_FLUSH);

    if (ierr != Z_OK && ierr != Z_STREAM_END) {
      zh.ThrowZLIBError_t(ierr, TRANS("Error reading from zip"));
    }
  }
};

// Close a ZIP file
void Close(Handle_t pHandle) {
  if (pHandle == NULL || !pHandle->zh_bOpen) {
    ASSERT(FALSE);
    return;
  }

  // clear it
  pHandle->Clear();
};

}; // namespace
