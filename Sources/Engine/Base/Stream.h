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


#ifndef SE_INCL_STREAM_H
#define SE_INCL_STREAM_H
#ifdef PRAGMA_ONCE
  #pragma once
#endif

#include <Engine/Base/Translation.h>
#include <Engine/Base/Lists.h>
#include <Engine/Base/CTString.h>
#include <Engine/Base/FileName.h>
#include <Engine/Base/ErrorReporting.h>
#include <Engine/Base/Unzip.h>
#include <Engine/Templates/DynamicStackArray.h>
#include <Engine/Templates/DynamicContainer.h>
#include <Engine/Templates/NameTable.h>

// [Cecil] Exception handling for streams only on Windows OS
#if SE1_WIN

#define CTSTREAM_BEGIN CTStream::EnableStreamHandling(); __try
#define CTSTREAM_END __except( CTStream::ExceptionFilter( GetExceptionCode(),\
                                                          GetExceptionInformation()) )\
  {\
     CTStream::ExceptionFatalError();\
  }; CTStream::DisableStreamHandling();

#else
  #define CTSTREAM_BEGIN CTStream::EnableStreamHandling();
  #define CTSTREAM_END   CTStream::DisableStreamHandling();
#endif

/*
 * Chunk ID class
 */
#define CID_LENGTH 4
class ENGINE_API CChunkID {
public:
  char cid_ID[CID_LENGTH+1];
public:
  /* Default constructor and string constructor. */
  inline CChunkID(const char *strString = "    ");
  /* Comparison operator. */
  inline int operator==(const CChunkID &cidOther) const;
  inline int operator!=(const CChunkID &cidOther) const;
  inline operator const char *(void) const { return cid_ID; }
  inline operator char *(void) { return cid_ID; }
};

// inline implementations
/* Default constructor and string constructor. */
inline CChunkID::CChunkID(const char *strString /*= "    "*/) {
  ASSERT(strlen(strString)==CID_LENGTH);
  memcpy(cid_ID, strString, CID_LENGTH+1);
};
/* Comparison operator. */
inline int CChunkID::operator==(const CChunkID &cidOther) const {
  return (*((ULONG *)&cid_ID[0]) == *((ULONG *)&cidOther.cid_ID[0]));
};
inline int CChunkID::operator!=(const CChunkID &cidOther) const {
  return (*((ULONG *)&cid_ID[0]) != *((ULONG *)&cidOther.cid_ID[0]));
};

/*
 * CroTeam stream class -- abstract base class
 */
class ENGINE_API CTStream {
public:
  CListNode strm_lnListNode;  // for linking into main library's list of opened streams
public:
  CTString strm_strStreamDescription; // descriptive string

  enum DictionaryMode {
    DM_NONE,        // no dictionary on this file (yet)
    DM_ENABLED,     // dictionary is enabled, reading/writing rest of file
    DM_PROCESSING,  // reading/writing the dictionary itself
  } strm_dmDictionaryMode;    // dictionary mode of operation on this file
  SLONG strm_slDictionaryPos; // dictionary position in file (0 for no dictionary)
  INDEX strm_ctDictionaryImported;  // how many filenames were imported
  CNameTable<CTFileName> &strm_ntDictionary;  // name table for the dictionary
  CDynamicStackArray<CTFileName> strm_afnmDictionary; // dictionary is stored here
  CDynamicContainer<CSerial> strm_cserPreloaded; // [Cecil] Replacement for 'fnm_pserPreloaded' from old CTFileName

  /* Throw an exception of formatted string. */
  void Throw_t(const char *strFormat, ...); // throw char *
  // read the dictionary from given offset in file (internal function)
  void ReadDictionary_intenal_t(SLONG slOffset);
  // copy filename dictionary from another stream
  void CopyDictionary(CTStream &strmOther);
public:
  // modes for opening streams
  enum OpenMode {
    // [Cecil] Removed obsolete OM_READTEXT, OM_WRITETEXT, OM_READBINARY & OM_WRITEBINARY
    // Their values were equal to the following OM_READ and OM_WRITE values anyway
    OM_READ  = 1,
    OM_WRITE = 2,
  };

  // direction for seeking
  enum SeekDir {
    SD_BEG = SEEK_SET,
    SD_END = SEEK_END,
    SD_CUR = SEEK_CUR,
  };

  /* Static function enable stream handling. */
  static void EnableStreamHandling(void);
  /* Static function disable stream handling. */
  static void DisableStreamHandling(void);

#if SE1_WIN /* rcg10042001 !!! FIXME */
  /* Static function to filter exceptions and intercept access violation */
  static int ExceptionFilter(DWORD dwCode, _EXCEPTION_POINTERS *pExceptionInfoPtrs);

  /* Static function to report fatal exception error. */
  static void ExceptionFatalError(void);
#endif

  /* Default constructor. */
  CTStream(void);
  /* Destruction. */
  virtual ~CTStream(void);

  /* Check if the stream can be read. -- used mainly for assertions */
  virtual BOOL IsReadable(void) = 0;
  /* Check if the stream can be written. -- used mainly for assertions */
  virtual BOOL IsWriteable(void) = 0;
  /* Check if the stream can be seeked. -- used mainly for assertions */
  virtual BOOL IsSeekable(void) = 0;

  /* Read a block of data from stream. */
  virtual void Read_t(void *pvBuffer, size_t slSize) = 0; // throw char *
  /* Write a block of data to stream. */
  virtual void Write_t(const void *pvBuffer, size_t slSize) = 0; // throw char *

  /* Seek in stream. */
  virtual void Seek_t(SLONG slOffset, enum SeekDir sd) = 0; // throw char *
  /* Set absolute position in stream. */
  virtual void SetPos_t(SLONG slPosition) = 0; // throw char *
  /* Get absolute position in stream. */
  virtual SLONG GetPos_t(void) = 0; // throw char *
  /* Get size of stream */
  virtual SLONG GetStreamSize(void) = 0;
  /* Get CRC32 of stream */
  virtual ULONG GetStreamCRC32_t(void) = 0;
  /* Check if file position points to the EOF */
  virtual BOOL AtEOF(void) = 0;

  /* Get description of this stream (e.g. filename for a CFileStream). */
  inline CTString &GetDescription(void) { return strm_strStreamDescription; };

  /* Read an object from stream. */
  inline CTStream &operator>>(UQUAD &ull) { Read_t(&ull, sizeof(ull)); return *this; } // [Cecil]
  inline CTStream &operator>>(SQUAD &sll) { Read_t(&sll, sizeof(sll)); return *this; } // [Cecil]
  inline CTStream &operator>>(float  &f) { Read_t( &f, sizeof( f)); return *this; } // throw char *
  inline CTStream &operator>>(double &d) { Read_t( &d, sizeof( d)); return *this; } // throw char *
  inline CTStream &operator>>(ULONG &ul) { Read_t(&ul, sizeof(ul)); return *this; } // throw char *
  inline CTStream &operator>>(UWORD &uw) { Read_t(&uw, sizeof(uw)); return *this; } // throw char *
  inline CTStream &operator>>(UBYTE &ub) { Read_t(&ub, sizeof(ub)); return *this; } // throw char *
  inline CTStream &operator>>(SLONG &sl) { Read_t(&sl, sizeof(sl)); return *this; } // throw char *
  inline CTStream &operator>>(SWORD &sw) { Read_t(&sw, sizeof(sw)); return *this; } // throw char *
  inline CTStream &operator>>(SBYTE &sb) { Read_t(&sb, sizeof(sb)); return *this; } // throw char *
#if SE1_WIN
  inline CTStream &operator>>(BOOL   &b) { Read_t( &b, sizeof( b)); return *this; } // throw char *
#endif
  /* Write an object into stream. */
  inline CTStream &operator<<(const UQUAD &ull) { Write_t(&ull, sizeof(ull)); return *this; } // [Cecil]
  inline CTStream &operator<<(const SQUAD &sll) { Write_t(&sll, sizeof(sll)); return *this; } // [Cecil]
  inline CTStream &operator<<(const float  &f) { Write_t( &f, sizeof( f)); return *this; } // throw char *
  inline CTStream &operator<<(const double &d) { Write_t( &d, sizeof( d)); return *this; } // throw char *
  inline CTStream &operator<<(const ULONG &ul) { Write_t(&ul, sizeof(ul)); return *this; } // throw char *
  inline CTStream &operator<<(const UWORD &uw) { Write_t(&uw, sizeof(uw)); return *this; } // throw char *
  inline CTStream &operator<<(const UBYTE &ub) { Write_t(&ub, sizeof(ub)); return *this; } // throw char *
  inline CTStream &operator<<(const SLONG &sl) { Write_t(&sl, sizeof(sl)); return *this; } // throw char *
  inline CTStream &operator<<(const SWORD &sw) { Write_t(&sw, sizeof(sw)); return *this; } // throw char *
  inline CTStream &operator<<(const SBYTE &sb) { Write_t(&sb, sizeof(sb)); return *this; } // throw char *
#if SE1_WIN
  inline CTStream &operator<<(const BOOL   &b) { Write_t( &b, sizeof( b)); return *this; } // throw char *
#endif

  // [Cecil] Serialize strings as filenames using special methods instead of friend operators
  void ReadFileName(CTString &fnmFileName);
  void WriteFileName(const CTString &fnmFileName);

  /* Put a line of text into stream. */
  virtual void PutLine_t(const char *strBuffer); // throw char *
  virtual void PutString_t(const char *strString); // throw char *

#if !SE1_EXF_VERIFY_VA_IN_PRINTF
  virtual void FPrintF_t(const char *strFormat, ...) SE1_FORMAT_FUNC(2, 3); // throw char *
#else
  EXF_VERIFY_VA_FUNC(FPrintF_t); // [Cecil] See 'SE1_EXF_VERIFY_VA_IN_PRINTF' definition
#endif

  /* Get a line of text from stream. */
  virtual void GetLine_t(char *strBuffer, SLONG slBufferSize, char cDelimiter='\n'); // throw char *
  virtual void GetLine_t(CTString &strLine, char cDelimiter='\n'); // throw char *

  virtual CChunkID GetID_t(void); // throw char *
  virtual CChunkID PeekID_t(void); // throw char *
  virtual void ExpectID_t(const CChunkID &cidExpected); // throw char *
  virtual void ExpectKeyword_t(const CTString &strKeyword); // throw char *
  virtual SLONG GetSize_t(void); // throw char *
  virtual void ReadRawChunk_t(void *pvBuffer, SLONG slSize); // throw char *
  virtual void ReadChunk_t(void *pvBuffer, SLONG slExpectedSize); // throw char *
  virtual void ReadFullChunk_t(const CChunkID &cidExpected, void *pvBuffer, SLONG slExpectedSize); // throw char *
  virtual void *ReadChunkAlloc_t(SLONG slSize=0); // throw char *
  virtual void ReadStream_t(CTStream &strmOther); // throw char *

  virtual void WriteID_t(const CChunkID &cidSave); // throw char *
  virtual void WriteSize_t(SLONG slSize); // throw char *
  virtual void WriteRawChunk_t(void *pvBuffer, SLONG slSize); // throw char *  // doesn't write length
  virtual void WriteChunk_t(void *pvBuffer, SLONG slSize); // throw char *
  virtual void WriteFullChunk_t(const CChunkID &cidSave, void *pvBuffer, SLONG slSize); // throw char *
  virtual void WriteStream_t(CTStream &strmOther); // throw char *

  // whether or not the given pointer is coming from this stream (mainly used for exception handling)
  virtual BOOL PointerInStream(void* pPointer);

  // filename dictionary operations

  // start writing a with a dictionary
  void DictionaryWriteBegin_t(const CTFileName &fnmImportFrom, SLONG slImportOffset);
  // stop writing a with a dictionary
  void DictionaryWriteEnd_t(void);
  // start reading a with a dictionary
  SLONG DictionaryReadBegin_t(void);  // returns offset of dictionary for cross-file importing
  // stop reading a with a dictionary
  void DictionaryReadEnd_t(void);
  // preload all files mentioned in the dictionary
  void DictionaryPreload_t(void);
};

/*
 * CroTeam file stream class
 */
class ENGINE_API CTFileStream : public CTStream {
public:
  // [Cecil] Global overrideable flags for the CTFileStream::Open_t() wrapper method
  static ULONG ulFileStreamOpenFlags;

  // [Cecil] Global overrideable flags for the CTFileStream::Create_t() wrapper method
  static ULONG ulFileStreamCreateFlags;

private:
  FILE *fstrm_pFile;    // ptr to opened file

  IZip::Handle_t fstrm_pZipHandle; // handle of zip-file entry
  INDEX fstrm_iZipLocation; // location in zip-file entry
  UBYTE* fstrm_pubZipBuffer; // buffer for zip-file entry
  SLONG fstrm_slZipSize; // size of the zip-file entry

  BOOL fstrm_bReadOnly;  // set if file is opened in read-only mode

public:
  /* Default constructor. */
  CTFileStream(void);
  /* Destructor. */
  virtual ~CTFileStream(void);

  // [Cecil] Open an existing file with some flags
  void OpenEx_t(const CTString &fnm, ULONG ulFlags, CTStream::OpenMode om = CTStream::OM_READ);

  // [Cecil] Create a new file with some flags
  void CreateEx_t(const CTString &fnm, ULONG ulFlags);

  // [Cecil] Wrappers for compatibility
  __forceinline void Open_t(const CTString &fnm, CTStream::OpenMode om = CTStream::OM_READ) {
    OpenEx_t(fnm, ulFileStreamOpenFlags, om);
  };

  __forceinline void Create_t(const CTString &fnm) {
    CreateEx_t(fnm, ulFileStreamCreateFlags);
  };

  // [Cecil] Set EDirListFlags flags for CTFileStream::Open_t()
  static inline void SetOpenFlags(ULONG ulFlags);

  // [Cecil] Set flags for CTFileStream::Open_t() to the default value
  static inline void ResetOpenFlags(void);

  // [Cecil] Set EDirListFlags flags for CTFileStream::Create_t()
  static inline void SetCreateFlags(ULONG ulFlags);

  // [Cecil] Set flags for CTFileStream::Create_t() to the default value
  static inline void ResetCreateFlags(void);

  /* Close an open file. */
  void Close(void);
  /* Get CRC32 of stream */
  ULONG GetStreamCRC32_t(void);

  /* Read a block of data from stream. */
  void Read_t(void *pvBuffer, size_t slSize); // throw char *
  /* Write a block of data to stream. */
  void Write_t(const void *pvBuffer, size_t slSize); // throw char *

  /* Seek in stream. */
  void Seek_t(SLONG slOffset, enum SeekDir sd); // throw char *
  /* Set absolute position in stream. */
  void SetPos_t(SLONG slPosition); // throw char *
  /* Get absolute position in stream. */
  SLONG GetPos_t(void); // throw char *
  /* Get size of stream */
  SLONG GetStreamSize(void);
  /* Check if file position points to the EOF */
  BOOL AtEOF(void);

  // whether or not the given pointer is coming from this stream (mainly used for exception handling)
  virtual BOOL PointerInStream(void* pPointer);

  // from CTStream
  inline virtual BOOL IsWriteable(void){ return !fstrm_bReadOnly;};
  inline virtual BOOL IsReadable(void){ return TRUE;};
  inline virtual BOOL IsSeekable(void){ return TRUE;};
};

/*
 * CroTeam memory stream class
 */
class ENGINE_API CTMemoryStream : public CTStream {
public:
  BOOL mstrm_bReadable;      // set if stream is readable
  BOOL mstrm_bWriteable;     // set if stream is writeable
  INDEX mstrm_ctLocked;      // counter for buffer locking

  UBYTE* mstrm_pubBuffer;    // buffer of the stream
  UBYTE* mstrm_pubBufferEnd; // pointer to the end of the stream buffer
  SLONG mstrm_slLocation;    // location in the stream
  UBYTE* mstrm_pubBufferMax; // furthest that the stream location has ever gotten
public:
  /* Create dynamically resizing stream for reading/writing. */
  CTMemoryStream(void);
  /* Create static stream from given buffer. */
  CTMemoryStream(void *pvBuffer, SLONG slSize, CTStream::OpenMode om = CTStream::OM_READ);
  /* Destructor. */
  virtual ~CTMemoryStream(void);

  /* Lock the buffer contents and it's size. */
  void LockBuffer(void **ppvBuffer, SLONG *pslSize);
    /* Unlock buffer. */
  void UnlockBuffer(void);

  /* Read a block of data from stream. */
  void Read_t(void *pvBuffer, size_t slSize); // throw char *
  /* Write a block of data to stream. */
  void Write_t(const void *pvBuffer, size_t slSize); // throw char *

  /* Seek in stream. */
  void Seek_t(SLONG slOffset, enum SeekDir sd); // throw char *
  /* Set absolute position in stream. */
  void SetPos_t(SLONG slPosition); // throw char *
  /* Get absolute position in stream. */
  SLONG GetPos_t(void); // throw char *
  /* Get size of stream. */
  SLONG GetSize_t(void); // throw char *
  /* Get size of stream */
  SLONG GetStreamSize(void);
  /* Get CRC32 of stream */
  ULONG GetStreamCRC32_t(void);
  /* Check if file position points to the EOF */
  BOOL AtEOF(void);

  // whether or not the given pointer is coming from this stream (mainly used for exception handling)
  virtual BOOL PointerInStream(void* pPointer);

  // memory stream can be opened only for reading and writing
  virtual BOOL IsWriteable(void);
  virtual BOOL IsReadable(void);
  virtual BOOL IsSeekable(void);
};

// [Cecil] Directory for separate user-made modifications
#define SE1_MODS_SUBDIR "Mods\\"

// [Cecil] Exported from the engine; makes a string copy of the path to match
ENGINE_API BOOL FileMatchesList(CDynamicStackArray<CTFileName> &afnm, CTString fnm);

// Test if a file exists.
ENGINE_API BOOL FileExists(const CTFileName &fnmFile);
// Test if a file exists for writing. 
// (this is can be diferent than normal FileExists() if a mod uses basewriteexclude.lst
ENGINE_API BOOL FileExistsForWriting(const CTFileName &fnmFile);
// Get file timestamp
ENGINE_API SLONG GetFileTimeStamp_t(const CTFileName &fnmFile); // throw char *
// Get CRC32 of a file
ENGINE_API ULONG GetFileCRC32_t(const CTFileName &fnmFile); // throw char *
// Test if a file is read only (also returns FALSE if file does not exist)
ENGINE_API BOOL IsFileReadOnly(const CTFileName &fnmFile);
// Delete a file (called 'remove' to avid name clashes with win32)
ENGINE_API BOOL RemoveFile(const CTFileName &fnmFile);

// [Cecil] New flags for using specific paths in specified directories using various methods that accept flags
enum EDirListFlags {
  DLI_RECURSIVE   = (1 << 0), // Look into subdirectories
  DLI_SEARCHGAMES = (1 << 1), // Search directories of other games
  DLI_SEARCHEXTRA = (1 << 2), // Search extra content directories
  DLI_REUSELIST   = (1 << 3), // Reuse existing entries in the provided list
  DLI_ONLYMOD     = (1 << 4), // List files exclusively from the mod directory
  DLI_ONLYGRO     = (1 << 5), // List files exclusively from GRO packages
  DLI_IGNOREMOD   = (1 << 6), // Ignore extra files from the mod
  DLI_IGNORELISTS = (1 << 7), // Ignore include/exclude lists, if playing a mod
  DLI_IGNOREGRO   = (1 << 8), // Ignore files from GRO packages
};

// [Cecil] Set EDirListFlags flags for CTFileStream::Open_t()
void CTFileStream::SetOpenFlags(ULONG ulFlags) {
  ulFileStreamOpenFlags = ulFlags;
};

// [Cecil] Set flags for CTFileStream::Open_t() to the default value
void CTFileStream::ResetOpenFlags(void) {
  ulFileStreamOpenFlags = DLI_SEARCHGAMES;
};

// [Cecil] Set EDirListFlags flags for CTFileStream::Create_t()
void CTFileStream::SetCreateFlags(ULONG ulFlags) {
  ulFileStreamCreateFlags = ulFlags;
};

// [Cecil] Set flags for CTFileStream::Create_t() to the default value
void CTFileStream::ResetCreateFlags(void) {
  ulFileStreamCreateFlags = 0;
};

// make a list of all files in a directory
ENGINE_API void MakeDirList(
  CDynamicStackArray<CTFileName> &adeDir, 
  const CTFileName &fnmDir,     // directory to list
  const CTString &strPattern,   // pattern for each file to match ("" matches all)
  ULONG ulFlags                 // additional flags
);

// [Cecil] Structure that expands a relative path to the absolute path for writing/reading files
struct ENGINE_API ExpandPath {
  CTString fnmExpanded; // Absolute path to the file

  enum EType {
    E_PATH_INVALID = 0,
    E_PATH_ABSOLUTE, // Already an absolute path that can be inside any directory (including other types)
    E_PATH_GAME,     // Path inside any game directory
    E_PATH_MOD,      // Path inside the current mod directory
  };

  EType eType; // Inside which directory this path is
  BOOL bArchive; // Marks a file from an archive

  __forceinline ExpandPath() : eType(E_PATH_INVALID), bArchive(FALSE) {};

  // Get a potential substitution for some file
  static BOOL GetSub(CTString &fnm);

  // Get full path on disk from any relative/absolute path
  static CTString OnDisk(CTString fnmFile);

  // Expand some path to the directory for temporary files
  static CTString ToTemp(const CTString &fnmRelative);

  // Expand some path to the directory for personal user data
  static CTString ToUser(const CTString &fnmRelative, BOOL bMod = FALSE);

  // Get full path for writing a file on disk
  // Accepted flags: DLI_ONLYMOD/DLI_IGNOREMOD, DLI_IGNORELISTS
  BOOL ForWriting(const CTString &fnmFile, ULONG ulFlags);

  // Get full path for reading a file from disk or from some archive
  // Accepted flags: DLI_SEARCHGAMES, DLI_ONLYMOD/DLI_IGNOREMOD, DLI_ONLYGRO/DLI_IGNOREGRO
  BOOL ForReading(const CTString &fnmFile, ULONG ulFlags);

  // Like ForReading() but without any substitutions for non-existing files
  // Accepted flags: DLI_SEARCHGAMES, DLI_ONLYMOD/DLI_IGNOREMOD, DLI_ONLYGRO/DLI_IGNOREGRO
  BOOL ForReading_internal(const CTString &fnmFile, ULONG ulFlags);
};

// [Cecil] Turned paths into constant references to internal variables
// global string with application path
ENGINE_API extern const CTFileName &_fnmApplicationPath;
// global string with filename of the started application
ENGINE_API extern const CTFileName &_fnmApplicationExe;

// [Cecil] Full path to the launched executable (essentially '_fnmApplicationPath + _fnmApplicationExe')
ENGINE_API extern const CTFileName &_fnmFullExecutablePath;

// global string with current MOD path
ENGINE_API extern CTFileName _fnmMod;
// global string with current name (the parameter that is passed on cmdline)
ENGINE_API extern CTString _strModName;
// global string with url to be shown to users that don't have the mod installed
// (should be set by game.dll)
ENGINE_API extern CTString _strModURL;

// [Cecil] Extra content directory
struct ExtraContentDir_t {
  CTString fnmPath; // Absolute path to the directory
  BOOL bGame; // Whether it's another game installation or a simple collection of packages

  ExtraContentDir_t() {
    Clear();
  };

  inline void Clear(void) {
    fnmPath = "";
    bGame = FALSE;
  };
};

// [Cecil] List of extra content directories
ENGINE_API extern CDynamicStackArray<ExtraContentDir_t> _aContentDirs;

#endif  /* include-once check. */
