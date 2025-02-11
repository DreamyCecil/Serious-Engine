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

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#if SE1_WIN && !SE1_OLD_COMPILER
  #include <DbgHelp.h>

  #pragma comment(lib, "DbgHelp.lib")
#endif

#include <Engine/Base/Protection.h>

#include <Engine/Base/Stream.h>

#include <Engine/Base/Memory.h>
#include <Engine/Base/Console.h>
#include <Engine/Base/ErrorReporting.h>
#include <Engine/Base/ListIterator.inl>
#include <Engine/Base/ProgressHook.h>
#include <Engine/Base/Unzip.h>
#include <Engine/Base/CRC.h>
#include <Engine/Base/Shell.h>
#include <Engine/Templates/StaticArray.cpp>
#include <Engine/Templates/DynamicStackArray.cpp>

#include <Engine/Templates/Stock_CTextureData.h>
#include <Engine/Templates/Stock_CModelData.h>

// maximum lenght of file that can be saved (default: 128Mb)
ULONG _ulMaxLenghtOfSavingFile = (1UL<<20)*128;
INDEX fil_bPreferZips = FALSE;

// set if current thread has currently enabled stream handling
static SE1_THREADLOCAL BOOL _bThreadCanHandleStreams = FALSE;
// list of currently opened streams
static SE1_THREADLOCAL CListHead *_plhOpenedStreams = NULL;

ULONG _ulVirtuallyAllocatedSpace = 0;
ULONG _ulVirtuallyAllocatedSpaceTotal = 0;

// global string with current MOD path
CTFileName _fnmMod;
// global string with current name (the parameter that is passed on cmdline)
CTString _strModName;
// global string with url to be shown to users that don't have the mod installed
// (should be set by game.dll)
CTString _strModURL;

// [Cecil] List of extra content directories
CDynamicStackArray<ExtraContentDir_t> _aContentDirs;

// [Cecil] Included/excluded "base" lists have been replaced with included "mod" lists
CDynamicStackArray<CTFileName> _afnmModWrite;
CDynamicStackArray<CTFileName> _afnmModRead;

// list of paths or patterns that are not included when making CRCs for network connection
// this is used to enable connection between different localized versions
CDynamicStackArray<CTFileName> _afnmNoCRC;

// load a filelist
static BOOL LoadFileList(CDynamicStackArray<CTFileName> &afnm, const CTFileName &fnmList)
{
  afnm.PopAll();
  try {
    CTFileStream strm;
    strm.Open_t(fnmList);
    while(!strm.AtEOF()) {
      CTString strLine;
      strm.GetLine_t(strLine);
      strLine.TrimSpacesLeft();
      strLine.TrimSpacesRight();
      if (strLine!="") {
        afnm.Push() = strLine;
      }
    }
    return TRUE;
  } catch(char *strError) {
    CPrintF("%s\n", strError);
    return FALSE;
  }
}

// [Cecil] Makes a string copy of the path to match
BOOL FileMatchesList(CDynamicStackArray<CTFileName> &afnm, CTString fnm) {
  // [Cecil] Fix slashes for consistency
  fnm.ReplaceChar('/', '\\');

  for (INDEX i = 0; i < afnm.Count(); i++) {
    CTString strEntry = afnm[i];

    // [Cecil] Match backward slashes for consistency
    strEntry.ReplaceChar('/', '\\');

    // Try matching wildcards first and only then check if the path starts with it
    if (fnm.Matches(strEntry) || fnm.HasPrefix(strEntry)) {
      return TRUE;
    }
  }

  return FALSE;
};

// [Cecil] Load ZIP packages under an absolute directory that match a filename mask
static void LoadPackages(const CTString &strDirectory, const CTString &strMatchFiles) {
  FileSystem::Search search;
  CTString fnmBasePath = (strDirectory + strMatchFiles);

  // Find first file in the directory
  BOOL bOK = search.FindFirst(fnmBasePath.ConstData());

  while (bOK) {
    const char *strFile = search.GetName();

    // Add file to the active set if the name matches the mask
    if (FileSystem::PathMatches(strFile, strMatchFiles)) {
      IZip::AddArchive(strDirectory + strFile);
    }

    bOK = search.FindNext();
  }
};

// [Cecil] Load packages from extra content directories
static void LoadExtraPackages(const CTString &fnmDirList, BOOL bGameDir) {
  CDynamicStackArray<CTString> afnmDirs;
  LoadFileList(afnmDirs, fnmDirList);

  const INDEX ctDirs = afnmDirs.Count();

  for (INDEX iDir = 0; iDir < ctDirs; iDir++) {
    // Make directory into a full path
    CTString &fnmDir = afnmDirs[iDir];
    fnmDir.SetFullDirectory();

    if (FileSystem::IsDirectory(fnmDir.ConstData())) {
      // Load packages from the directory and add it to the list
      LoadPackages(fnmDir, "*.gro");

      ExtraContentDir_t &dir = _aContentDirs.Push();
      dir.fnmPath = fnmDir;
      dir.bGame = bGameDir;
    }
  }
};

void InitStreams(void)
{
  // if no mod defined yet
  if (_fnmMod=="") {
    // check for 'default mod' file
    LoadStringVar(CTString("DefaultMod.txt"), _fnmMod);
  }

  CPrintF(TRANS("Current mod: %s\n"), _fnmMod == "" ? TRANS("<none>") : _fnmMod.ConstData());
  // if there is a mod active
  if (_fnmMod!="") {
    // load mod's include/exclude lists
    CPrintF(TRANS("Loading mod include/exclude lists...\n"));
    BOOL bOK = FALSE;

    // [Cecil] TEMP: Load old vanilla lists and construct actual new lists below
    // that consist only of specific directories that can be browsed/written into
    CDynamicStackArray<CTString> aWriteList;
    CDynamicStackArray<CTString> aReadList;
    bOK |= LoadFileList(aWriteList, "BaseWriteExclude.lst");
    bOK |= LoadFileList(aReadList, "BaseBrowseExclude.lst");
    // [Cecil] TEMP: Simply check for include lists, in case exclude lists aren't there
    bOK |= FileExists("BaseWriteInclude.lst");
    bOK |= FileExists("BaseBrowseInclude.lst");

    // if none found
    if (!bOK) {
      // the mod is not valid
      _fnmMod = CTString("");
      CPrintF(TRANS("Error: MOD not found!\n"));
    // if mod is ok
    } else {
      // remember mod name (the parameter that is passed on cmdline)
      _strModName = _fnmMod;
      _strModName.DeleteChar(_strModName.Length()-1);
      _strModName = CTFileName(_strModName).FileName();

      // [Cecil] Try to match specific kinds of files
      // Should match such entries: 'Levels', 'Levels\\', 'Levels\\LevelsMP', 'Levels\\LevelsMP\\'
      const BOOL bLevels = (FileMatchesList(aReadList, "Levels\\testmatch.wld")
                         || FileMatchesList(aReadList, "Levels\\LevelsMP\\testmatch.wld"));
      // Should match such entries: 'Savegame', 'savegame\\', 'SaveGame*'
      const BOOL bSaves = (FileMatchesList(aWriteList, "SaveGame\\Player0\\testmatch.sav")
                        || FileMatchesList(aReadList,  "SaveGame\\Player0\\testmatch.sav"));
      // Should match such entries: 'Models', 'Models\\Player', 'modelsmp', 'modelsmp\\player'
      const BOOL bModels = (FileMatchesList(aReadList, "Models\\Player\\testmatch.amc")
                         || FileMatchesList(aReadList, "ModelsMP\\Player\\testmatch.amc"));

      BOOL bSavesWithLevels = FALSE;

      // [Cecil] If mod has exclusive levels or exclusive saves, make all level-related content exclusive
      if (bLevels || bSaves) {
        // Record and list mod demos
        _afnmModWrite.Push() = "Demos";
        _afnmModRead.Push() = "Demos";
        // List mod levels and create .vis files for them
        _afnmModWrite.Push() = "Levels";
        _afnmModRead.Push() = "Levels";
        // Write and read mod saves
        _afnmModWrite.Push() = "UserData\\SaveGame";
        _afnmModRead.Push() = "UserData\\SaveGame";
        bSavesWithLevels = TRUE;
      }

      // [Cecil] If mod has exclusive saves but not levels, add them separately
      if (!bSavesWithLevels && bSaves) {
        // Write and read mod saves
        _afnmModWrite.Push() = "UserData\\SaveGame";
        _afnmModRead.Push() = "UserData\\SaveGame";
      }

      // [Cecil] If mod has exclusive models, add player models specifically
      if (bModels) {
        // Read AMC files
        _afnmModRead.Push() = "Models\\Player";
      }
    }
  }

  CPrintF(TRANS("Loading group files...\n"));

  // for each group file in base directory
  LoadPackages(_fnmApplicationPath, "*.gro");

  // if there is a mod active
  if (_fnmMod!="") {
    // for each group file in mod directory
    LoadPackages(_fnmApplicationPath + _fnmMod, "*.gro");
  }

  // [Cecil] Load packages from extra content directories
  LoadExtraPackages(CTFILENAME("UserData\\ContentDirs.lst"), FALSE); // Simple GRO packages
  LoadExtraPackages(CTFILENAME("UserData\\GameDirs.lst"), TRUE); // Entire games

  // try to
  try {
    // read the zip directories
    IZip::ReadDirectoriesReverse_t();
  // if failed
  } catch( char *strError) {
    // report warning
    CPrintF( TRANS("There were group file errors:\n%s"), strError);
  }
  CPrintF("\n");

  LoadFileList(_afnmNoCRC, CTFILENAME("Data\\NoCRC.lst"));

  _pShell->SetINDEX(CTString("sys")+"_iCPU"+"Misc", 1);
}

void EndStreams(void)
{
}

/////////////////////////////////////////////////////////////////////////////
// Helper functions

/* Static function enable stream handling. */
void CTStream::EnableStreamHandling(void)
{
  ASSERT(!_bThreadCanHandleStreams && _plhOpenedStreams == NULL);

  _bThreadCanHandleStreams = TRUE;
  _plhOpenedStreams = new CListHead;
}

/* Static function disable stream handling. */
void CTStream::DisableStreamHandling(void)
{
  ASSERT(_bThreadCanHandleStreams && _plhOpenedStreams != NULL);

  _bThreadCanHandleStreams = FALSE;
  delete _plhOpenedStreams;
  _plhOpenedStreams = NULL;
}

#if SE1_WIN

int CTStream::ExceptionFilter(DWORD dwCode, _EXCEPTION_POINTERS *pExceptionInfoPtrs)
{
  // If the exception is not a page fault, exit.
  if( dwCode != EXCEPTION_ACCESS_VIOLATION)
  {
    return EXCEPTION_CONTINUE_SEARCH;
  }

  // obtain access violation virtual address
  UBYTE *pIllegalAdress = (UBYTE *)pExceptionInfoPtrs->ExceptionRecord->ExceptionInformation[1];

  CTStream *pstrmAccessed = NULL;

  // search for stream that was accessed
  FOREACHINLIST( CTStream, strm_lnListNode, (*_plhOpenedStreams), itStream)
  {
    // if access violation happened inside curently testing stream
    if(itStream.Current().PointerInStream(pIllegalAdress))
    {
      // remember accesed stream ptr
      pstrmAccessed = &itStream.Current();
      // stream found, stop searching
      break;
    }
  }

  // if none of our streams was accessed, real access violation occured
  if( pstrmAccessed == NULL)
  {
    // so continue default exception handling
    return EXCEPTION_CONTINUE_SEARCH;
  }

  // Continue execution where the page fault occurred
  return EXCEPTION_CONTINUE_EXECUTION;
}

/*
 * Static function to report fatal exception error.
 */
void CTStream::ExceptionFatalError(void)
{
  FatalError("%s", GetWindowsError(GetLastError()).ConstData());
}

#endif // SE1_WIN

/*
 * Throw an exception of formatted string.
 */
void CTStream::Throw_t(const char *strFormat, ...)  // throws char *
{
  const SLONG slBufferSize = 256;
  char strFormatBuffer[slBufferSize];

  // [Cecil] Need to keep the message in memory (on Linux) or else
  // it will cause undefined behavior on subsequent "throw;" statements
  static char *strBuffer = NULL;

  if (strBuffer != NULL) delete[] strBuffer;
  strBuffer = new char[slBufferSize + 1];

  // add the stream description to the format string
  _snprintf(strFormatBuffer, slBufferSize, "%s (%s)", strFormat, strm_strStreamDescription.ConstData());
  // format the message in buffer
  va_list arg;
  va_start(arg, strFormat); // variable arguments start after this argument
  _vsnprintf(strBuffer, slBufferSize, strFormatBuffer, arg);
  va_end(arg);
  throw strBuffer;
}

/////////////////////////////////////////////////////////////////////////////
// Binary access methods

/* Get CRC32 of stream */
ULONG CTStream::GetStreamCRC32_t(void)
{
  // remember where stream is now
  SLONG slOldPos = GetPos_t();
  // go to start of file
  SetPos_t(0);
  // get size of file
  SLONG slFileSize = GetStreamSize();

  ULONG ulCRC;
  CRC_Start(ulCRC);

  // for each block in file
  const SLONG slBlockSize = 4096;
  for(SLONG slPos=0; slPos<slFileSize; slPos+=slBlockSize) {
    // read the block
    UBYTE aubBlock[slBlockSize];
    SLONG slThisBlockSize = Min(slFileSize-slPos, slBlockSize);
    Read_t(aubBlock, slThisBlockSize);
    // checksum it
    CRC_AddBlock(ulCRC, aubBlock, slThisBlockSize);
  }

  // restore position
  SetPos_t(slOldPos);

  CRC_Finish(ulCRC);
  return ulCRC;
}

/////////////////////////////////////////////////////////////////////////////
// Text access methods

/* Get a line of text from file. */
// throws char *
void CTStream::GetLine_t(char *strBuffer, SLONG slBufferSize, char cDelimiter /*='\n'*/ )
{
  // Check parameters and that the stream can be read
  ASSERT(strBuffer != NULL && slBufferSize > 0 && IsReadable());

  // letters slider
  INDEX iLetters = 0;
  // test if EOF reached
  if(AtEOF()) {
    ThrowF_t(TRANS("EOF reached, file %s"), strm_strStreamDescription.ConstData());
  }
  // get line from istream
  FOREVER
  {
    char c;
    Read_t(&c, 1);

    // [Cecil] Just skip instead of entering the block when it's not that
    if (c == '\r') continue;

    strBuffer[iLetters] = c;

    // Stop reading after the delimiter
    if (strBuffer[iLetters] == cDelimiter) {
      strBuffer[iLetters] = '\0';
      return;
    }

    // Go to the next destination letter
    iLetters++;

    // [Cecil] Cut off after actually setting the character
    if (AtEOF()) {
      strBuffer[iLetters] = '\0';
      return;
    }

    // test if maximum buffer lenght reached
    if( iLetters==slBufferSize) {
      return;
    }
  }
}

void CTStream::GetLine_t(CTString &strLine, char cDelimiter/*='\n'*/) // throw char *
{
  char strBuffer[1024];
  GetLine_t(strBuffer, sizeof(strBuffer)-1, cDelimiter);
  strLine = strBuffer;
}


/* Put a line of text into file. */
void CTStream::PutLine_t(const char *strBuffer) // throws char *
{
  // check parameters
  ASSERT(strBuffer!=NULL);
  // check that the stream is writteable
  ASSERT(IsWriteable());
  // get string length
  size_t iStringLength = strlen(strBuffer);
  // put line into stream
  Write_t(strBuffer, iStringLength);
  // write "\r\n" into stream
  Write_t("\r\n", 2);
}

void CTStream::PutString_t(const char *strString) // throw char *
{
  // check parameters
  ASSERT(strString!=NULL);
  // check that the stream is writteable
  ASSERT(IsWriteable());
  // get string length
  INDEX iStringLength = (INDEX)strlen(strString);
  // put line into stream
  for( INDEX iLetter=0; iLetter<iStringLength; iLetter++)
  {
    if (*strString=='\n') {
      // write "\r\n" into stream
      Write_t("\r\n", 2);
      strString++;
    } else {
      Write_t(strString++, 1);
    }
  }
}

#if !SE1_EXF_VERIFY_VA_IN_PRINTF

void CTStream::FPrintF_t(const char *strFormat, ...) // throw char *
{
  const SLONG slBufferSize = 2048;
  char strBuffer[slBufferSize];
  // format the message in buffer
  va_list arg;
  va_start(arg, strFormat); // variable arguments start after this argument
  _vsnprintf(strBuffer, slBufferSize, strFormat, arg);
  va_end(arg);
  // print the buffer
  PutString_t(strBuffer);
}

#endif // SE1_EXF_VERIFY_VA_IN_PRINTF

/////////////////////////////////////////////////////////////////////////////
// Chunk reading/writing methods

CChunkID CTStream::GetID_t(void) // throws char *
{
	CChunkID cidToReturn;
	Read_t( &cidToReturn.cid_ID[0], CID_LENGTH);
	return( cidToReturn);
}

CChunkID CTStream::PeekID_t(void) // throw char *
{
  // read the chunk id
	CChunkID cidToReturn;
	Read_t( &cidToReturn.cid_ID[0], CID_LENGTH);
  // return the stream back
  Seek_t(-CID_LENGTH, SD_CUR);
	return( cidToReturn);
}

void CTStream::ExpectID_t(const CChunkID &cidExpected) // throws char *
{
	CChunkID cidToCompare;

	Read_t( &cidToCompare.cid_ID[0], CID_LENGTH);
	if( !(cidToCompare == cidExpected))
	{
		ThrowF_t(TRANS("Chunk ID validation failed.\nExpected ID \"%s\" but found \"%s\"\n"),
      cidExpected.cid_ID, cidToCompare.cid_ID);
	}
}
void CTStream::ExpectKeyword_t(const CTString &strKeyword) // throw char *
{
  // check that the keyword is present
  const INDEX ctChars = strKeyword.Length();

  for (INDEX iKeywordChar = 0; iKeywordChar < ctChars; iKeywordChar++) {
    SBYTE chKeywordChar;
    (*this)>>chKeywordChar;
    if (chKeywordChar!=strKeyword[iKeywordChar]) {
      ThrowF_t(TRANS("Expected keyword %s not found"), strKeyword.ConstData());
    }
  }
}


SLONG CTStream::GetSize_t(void) // throws char *
{
	SLONG chunkSize;

	Read_t( (char *) &chunkSize, sizeof( SLONG));
	return( chunkSize);
}

void CTStream::ReadRawChunk_t(void *pvBuffer, SLONG slSize)  // throws char *
{
	Read_t((char *)pvBuffer, slSize);
}

void CTStream::ReadChunk_t(void *pvBuffer, SLONG slExpectedSize) // throws char *
{
	if( slExpectedSize != GetSize_t())
		throw TRANS("Chunk size not equal as expected size");
	Read_t((char *)pvBuffer, slExpectedSize);
}

void CTStream::ReadFullChunk_t(const CChunkID &cidExpected, void *pvBuffer,
                             SLONG slExpectedSize) // throws char *
{
	ExpectID_t( cidExpected);
	ReadChunk_t( pvBuffer, slExpectedSize);
};

void* CTStream::ReadChunkAlloc_t(SLONG slSize) // throws char *
{
	UBYTE *buffer;
	SLONG chunkSize;

	if( slSize != 0)
		chunkSize = slSize;
	else
		chunkSize = GetSize_t(); // throws char *
	buffer = (UBYTE *) AllocMemory( chunkSize);
	if( buffer == NULL)
		throw TRANS("ReadChunkAlloc: Unable to allocate needed amount of memory.");
	Read_t((char *)buffer, chunkSize); // throws char *
	return buffer;
}
void CTStream::ReadStream_t(CTStream &strmOther) // throw char *
{
  // implement this !!!! @@@@
}

void CTStream::WriteID_t(const CChunkID &cidSave) // throws char *
{
	Write_t( &cidSave.cid_ID[0], CID_LENGTH);
}

void CTStream::WriteSize_t(SLONG slSize) // throws char *
{
	Write_t( (char *)&slSize, sizeof( SLONG));
}

void CTStream::WriteRawChunk_t(void *pvBuffer, SLONG slSize) // throws char *
{
	Write_t( (char *)pvBuffer, slSize);
}

void CTStream::WriteChunk_t(void *pvBuffer, SLONG slSize) // throws char *
{
	WriteSize_t( slSize);
	WriteRawChunk_t( pvBuffer, slSize);
}

void CTStream::WriteFullChunk_t(const CChunkID &cidSave, void *pvBuffer,
                              SLONG slSize) // throws char *
{
	WriteID_t( cidSave); // throws char *
	WriteChunk_t( pvBuffer, slSize); // throws char *
}
void CTStream::WriteStream_t(CTStream &strmOther) // throw char *
{
  // implement this !!!! @@@@
}

// whether or not the given pointer is coming from this stream (mainly used for exception handling)
BOOL CTStream::PointerInStream(void* pPointer)
{
  // safe to return FALSE, we're using virtual functions anyway
  return FALSE;
}

/////////////////////////////////////////////////////////////////////////////
// filename dictionary operations

// enable dictionary in writable file from this point
void CTStream::DictionaryWriteBegin_t(const CTFileName &fnmImportFrom, SLONG slImportOffset)
{
  ASSERT(strm_slDictionaryPos==0);
  ASSERT(strm_dmDictionaryMode == DM_NONE);
  strm_ntDictionary.SetAllocationParameters(100, 5, 5);
  strm_ctDictionaryImported = 0;

  // if importing an existing dictionary to start with
  if (fnmImportFrom!="") {
    // open that file
    CTFileStream strmOther;
    strmOther.Open_t(fnmImportFrom);
    // read the dictionary in that stream
    strmOther.ReadDictionary_intenal_t(slImportOffset);
    // copy the dictionary here
    CopyDictionary(strmOther);
    // write dictionary importing data
    WriteID_t("DIMP");  // dictionary import
    WriteFileName(fnmImportFrom);
    *this << slImportOffset;
    // remember how many filenames were imported
    strm_ctDictionaryImported = strm_afnmDictionary.Count();
  }

  // write dictionary position chunk id
  WriteID_t("DPOS");  // dictionary position
  // remember where position will be placed
  strm_slDictionaryPos = GetPos_t();
  // leave space for position
  *this<<SLONG(0);

  // start dictionary
  strm_dmDictionaryMode = DM_ENABLED;
}

// write the dictionary (usually at the end of file)
void CTStream::DictionaryWriteEnd_t(void)
{
  ASSERT(strm_dmDictionaryMode == DM_ENABLED);
  ASSERT(strm_slDictionaryPos>0);
  // remember the dictionary position chunk position
  SLONG slDictPos = strm_slDictionaryPos;
  // mark that now saving dictionary
  strm_slDictionaryPos = -1;
  // remember where dictionary begins
  SLONG slDictBegin = GetPos_t();
  // start dictionary processing
  strm_dmDictionaryMode = DM_PROCESSING;

  WriteID_t("DICT");  // dictionary
  // write number of used filenames
  INDEX ctFileNames = strm_afnmDictionary.Count();
  INDEX ctFileNamesNew = ctFileNames-strm_ctDictionaryImported;
  *this<<ctFileNamesNew;
  // for each filename
  for(INDEX iFileName=strm_ctDictionaryImported; iFileName<ctFileNames; iFileName++) {
    // write it to disk
    WriteFileName(strm_afnmDictionary[iFileName]);
  }
  WriteID_t("DEND");  // dictionary end

  // remember where is end of dictionary
  SLONG slContinue = GetPos_t();

  // write the position back to dictionary position chunk
  SetPos_t(slDictPos);
  *this<<slDictBegin;

  // stop dictionary processing
  strm_dmDictionaryMode = DM_NONE;
  strm_ntDictionary.Clear();
  strm_afnmDictionary.Clear();
  strm_cserPreloaded.Clear(); // [Cecil]

  // return to end of dictionary
  SetPos_t(slContinue);
  strm_slDictionaryPos=0;
}

// read the dictionary from given offset in file (internal function)
void CTStream::ReadDictionary_intenal_t(SLONG slOffset)
{
  // remember where to continue loading
  SLONG slContinue = GetPos_t();

  // go to dictionary beginning
  SetPos_t(slOffset);

  // start dictionary processing
  strm_dmDictionaryMode = DM_PROCESSING;

  ExpectID_t("DICT");  // dictionary
  // read number of new filenames
  INDEX ctFileNamesOld = strm_afnmDictionary.Count();
  INDEX ctFileNamesNew;
  *this>>ctFileNamesNew;
  // if there are any new filenames
  if (ctFileNamesNew>0) {
    // create that much space
    strm_afnmDictionary.Push(ctFileNamesNew);
    // for each filename
    for(INDEX iFileName=ctFileNamesOld; iFileName<ctFileNamesOld+ctFileNamesNew; iFileName++) {
      // read it
      ReadFileName(strm_afnmDictionary[iFileName]);
    }
  }
  ExpectID_t("DEND");  // dictionary end

  // remember where end of dictionary is
  strm_slDictionaryPos = GetPos_t();

  // return to continuing position
  SetPos_t(slContinue);
}

// copy filename dictionary from another stream
void CTStream::CopyDictionary(CTStream &strmOther)
{
   strm_afnmDictionary = strmOther.strm_afnmDictionary;
   for (INDEX i=0; i<strm_afnmDictionary.Count(); i++) {
     strm_ntDictionary.Add(&strm_afnmDictionary[i]);
   }
}

SLONG CTStream::DictionaryReadBegin_t(void)
{
  ASSERT(strm_dmDictionaryMode == DM_NONE);
  ASSERT(strm_slDictionaryPos==0);
  strm_ntDictionary.SetAllocationParameters(100, 5, 5);

  SLONG slImportOffset = 0;
  // if there is imported dictionary
  if (PeekID_t()==CChunkID("DIMP")) {  // dictionary import
    // read dictionary importing data
    ExpectID_t("DIMP");  // dictionary import
    CTFileName fnmImportFrom;
    ReadFileName(fnmImportFrom);
    *this >> slImportOffset;

    // open that file
    CTFileStream strmOther;
    strmOther.Open_t(fnmImportFrom);
    // read the dictionary in that stream
    strmOther.ReadDictionary_intenal_t(slImportOffset);
    // copy the dictionary here
    CopyDictionary(strmOther);
  }

  // if the dictionary is not here
  if (PeekID_t()!=CChunkID("DPOS")) {  // dictionary position
    // do nothing
    return 0;
  }

  // read dictionary position
  ExpectID_t("DPOS"); // dictionary position
  SLONG slDictBeg;
  *this>>slDictBeg;

  // read the dictionary from that offset in file
  ReadDictionary_intenal_t(slDictBeg);

  // stop dictionary processing - go to dictionary using
  strm_dmDictionaryMode = DM_ENABLED;

  // return offset of dictionary for later cross-file importing
  if (slImportOffset!=0) {
    return slImportOffset;
  } else {
    return slDictBeg;
  }
}

void CTStream::DictionaryReadEnd_t(void)
{
  if (strm_dmDictionaryMode == DM_ENABLED) {
    ASSERT(strm_slDictionaryPos>0);
    // just skip the dictionary (it was already read)
    SetPos_t(strm_slDictionaryPos);
    strm_slDictionaryPos=0;
    strm_dmDictionaryMode = DM_NONE;
    strm_ntDictionary.Clear();

    // [Cecil] Free all preloaded instances
    FOREACHINDYNAMICCONTAINER(strm_cserPreloaded, CSerial, itser) {
      CSerial *pser = itser;
      CTString strExt = pser->GetName().FileExt();

      if (strExt == ".tex") {
        _pTextureStock->Release((CTextureData *)pser);
      } else if (strExt == ".mdl") {
        _pModelStock->Release((CModelData *)pser);
      }
    }

    strm_afnmDictionary.Clear();
    strm_cserPreloaded.Clear(); // [Cecil]
  }
}
void CTStream::DictionaryPreload_t(void)
{
  INDEX ctFileNames = strm_afnmDictionary.Count();
  // for each filename
  for(INDEX iFileName=0; iFileName<ctFileNames; iFileName++) {
    // preload it
    CTFileName &fnm = strm_afnmDictionary[iFileName];
    CTString strExt = fnm.FileExt();
    CallProgressHook_t(FLOAT(iFileName)/ctFileNames);
    try {
      if (strExt==".tex") {
        strm_cserPreloaded.Add(_pTextureStock->Obtain_t(fnm));
      } else if (strExt==".mdl") {
        strm_cserPreloaded.Add(_pModelStock->Obtain_t(fnm));
      }
    } catch (char *strError) {
      CPrintF(TRANS("Cannot preload %s: %s\n"), fnm.ConstData(), strError);
    }
  }
}

/////////////////////////////////////////////////////////////////////////////
// General construction/destruction

/* Default constructor. */
CTStream::CTStream(void) : strm_ntDictionary(*new CNameTable<CTFileName>)
{
  strm_strStreamDescription = "";
  strm_slDictionaryPos = 0;
  strm_dmDictionaryMode = DM_NONE;
}

/* Destructor. */
CTStream::~CTStream(void)
{
  strm_ntDictionary.Clear();
  strm_afnmDictionary.Clear();
  strm_cserPreloaded.Clear(); // [Cecil]

  delete &strm_ntDictionary;
}

/////////////////////////////////////////////////////////////////////////////
// File stream opening/closing methods

/*
 * Default constructor.
 */
CTFileStream::CTFileStream(void)
{
  fstrm_pFile = NULL;
  // mark that file is created for writing
  fstrm_bReadOnly = TRUE;
  fstrm_pZipHandle = NULL;
  fstrm_iZipLocation = 0;
  fstrm_pubZipBuffer = NULL;
}

/*
 * Destructor.
 */
CTFileStream::~CTFileStream(void)
{
  // close stream
  if (fstrm_pFile != NULL || fstrm_pZipHandle != NULL) {
    Close();
  }
}

/*
 * Open an existing file.
 */
// throws char *
void CTFileStream::Open_t(const CTFileName &fnFileName, CTStream::OpenMode om/*=OM_READ*/)
{
  // if current thread has not enabled stream handling
  if (!_bThreadCanHandleStreams) {
    // error
    ::ThrowF_t(TRANS("Cannot open file `%s', stream handling is not enabled for this thread"), fnFileName.ConstData());
  }

  // check parameters
  ASSERT(fnFileName.Length() > 0);
  // check that the file is not open
  ASSERT(fstrm_pFile == NULL && fstrm_pZipHandle == NULL);

  // expand the filename to full path
  CTFileName fnmFullFileName;
  INDEX iFile = ExpandFilePath((om == OM_READ)?EFP_READ:EFP_WRITE, fnFileName, fnmFullFileName);
  
  // if read only mode requested
  if( om == OM_READ) {
    // initially, no physical file
    fstrm_pFile = NULL;
    // if zip file
    if( iFile==EFP_MODZIP || iFile==EFP_BASEZIP) {
      // open from zip
      fstrm_pZipHandle = IZip::Open_t(fnmFullFileName);
      fstrm_slZipSize = IZip::GetEntry(fstrm_pZipHandle)->GetUncompressedSize();
      // load the file from the zip in the buffer
      fstrm_pubZipBuffer = new UBYTE[fstrm_slZipSize];
      IZip::ReadBlock_t(fstrm_pZipHandle, (UBYTE *)fstrm_pubZipBuffer, 0, fstrm_slZipSize);
    // if it is a physical file
    } else if (iFile==EFP_FILE) {
      // open file in read only mode
      fstrm_pFile = FileSystem::Open(fnmFullFileName, "rb");
    }
    fstrm_bReadOnly = TRUE;
  
  // if write mode requested
  } else if( om == OM_WRITE) {
    // open file for reading and writing
    fstrm_pFile = FileSystem::Open(fnmFullFileName, "rb+");
    fstrm_bReadOnly = FALSE;
  // if unknown mode
  } else {
    FatalError(TRANS("File stream opening requested with unknown open mode: %d\n"), om);
  }

  // if openning operation was not successfull
  if (fstrm_pFile == NULL && fstrm_pZipHandle == NULL) {
    // throw exception
    Throw_t(TRANS("Cannot open file `%s' (%s)"), fnmFullFileName.ConstData(), strerror(errno));
  }

  // if file opening was successfull, set stream description to file name
  strm_strStreamDescription = fnmFullFileName;
  // add this newly opened file into opened stream list
  _plhOpenedStreams->AddTail( strm_lnListNode);
}

/*
 * Create a new file or overwrite existing.
 */
void CTFileStream::Create_t(const CTFileName &fnFileName) // throws char *
{
  CTFileName fnFileNameAbsolute = fnFileName;
  fnFileNameAbsolute.NormalizePath();

  // if current thread has not enabled stream handling
  if (!_bThreadCanHandleStreams) {
    // error
    ::ThrowF_t(TRANS("Cannot create file `%s', stream handling is not enabled for this thread"), fnFileNameAbsolute.ConstData());
  }

  // [Cecil] Create necessary directories for the new file if they don't exist yet
  if (_fnmMod != "" && FileMatchesList(_afnmModWrite, fnFileNameAbsolute)) {
    CreateAllDirectories(_fnmMod + fnFileNameAbsolute); // Within the mod
  } else {
    CreateAllDirectories(fnFileNameAbsolute); // Within the game
  }

  CTFileName fnmFullFileName;
  INDEX iFile = ExpandFilePath(EFP_WRITE, fnFileNameAbsolute, fnmFullFileName);

  // check parameters
  ASSERT(fnFileNameAbsolute.Length() > 0);
  // check that the file is not open
  ASSERT(fstrm_pFile == NULL);

  // open file stream for writing (destroy file context if file existed before)
  fstrm_pFile = FileSystem::Open(fnmFullFileName, "wb+");
  // if not successfull
  if(fstrm_pFile == NULL)
  {
    // throw exception
    Throw_t(TRANS("Cannot create file `%s' (%s)"), fnmFullFileName.ConstData(), strerror(errno));
  }
  // if file creation was successfull, set stream description to file name
  strm_strStreamDescription = fnFileNameAbsolute;
  // mark that file is created for writing
  fstrm_bReadOnly = FALSE;
  // add this newly created file into opened stream list
  _plhOpenedStreams->AddTail( strm_lnListNode);
}

/*
 * Close an open file.
 */
void CTFileStream::Close(void)
{
  // if file is not open
  if (fstrm_pFile == NULL && fstrm_pZipHandle == NULL) {
    //ASSERT(FALSE);
    return;
  }

  // clear stream description
  strm_strStreamDescription = "";
  // remove file from list of curently opened streams
  strm_lnListNode.Remove();

  // if file on disk
  if (fstrm_pFile != NULL) {
    // close file
    fclose( fstrm_pFile);
    fstrm_pFile = NULL;
  // if file in zip
  } else if (fstrm_pZipHandle != NULL) {
    // close zip entry
    IZip::Close(fstrm_pZipHandle);
    fstrm_pZipHandle = NULL;

    delete[] fstrm_pubZipBuffer;

    _ulVirtuallyAllocatedSpace -= fstrm_slZipSize;
    //CPrintF("Freed virtual memory with size ^c00ff00%d KB^C (now %d KB)\n", (fstrm_slZipSize / 1000), (_ulVirtuallyAllocatedSpace / 1000));
  }

  // clear dictionary vars
  strm_dmDictionaryMode = DM_NONE;
  strm_ntDictionary.Clear();
  strm_afnmDictionary.Clear();
  strm_cserPreloaded.Clear(); // [Cecil]
  strm_slDictionaryPos=0;
}

/* Get CRC32 of stream */
ULONG CTFileStream::GetStreamCRC32_t(void)
{
  // if file on disk
  if (fstrm_pFile != NULL) {
    // use base class implementation (really calculates the CRC)
    return CTStream::GetStreamCRC32_t();
  // if file in zip
  } else if (fstrm_pZipHandle != NULL) {
    return IZip::GetEntry(fstrm_pZipHandle)->GetCRC();
  } else {
    ASSERT(FALSE);
    return 0;
  }
}

/* Read a block of data from stream. */
void CTFileStream::Read_t(void *pvBuffer, size_t slSize)
{
  if (fstrm_pZipHandle != NULL) {
    memcpy(pvBuffer, fstrm_pubZipBuffer + fstrm_iZipLocation, slSize);
    fstrm_iZipLocation += (INDEX)slSize;
    return;
  }

  fread(pvBuffer, slSize, 1, fstrm_pFile);
}

/* Write a block of data to stream. */
void CTFileStream::Write_t(const void *pvBuffer, size_t slSize)
{
  if (fstrm_bReadOnly || fstrm_pZipHandle != NULL) {
    throw "Stream is read-only!";
  }

  fwrite(pvBuffer, slSize, 1, fstrm_pFile);
}

/* Seek in stream. */
void CTFileStream::Seek_t(SLONG slOffset, enum SeekDir sd)
{
  if (fstrm_pZipHandle != NULL) {
    switch(sd) {
    case SD_BEG: fstrm_iZipLocation = slOffset; break;
    case SD_CUR: fstrm_iZipLocation += slOffset; break;
    case SD_END: fstrm_iZipLocation = GetSize_t() + slOffset; break;
    }
  } else {
    fseek(fstrm_pFile, slOffset, sd);
  }
}

/* Set absolute position in stream. */
void CTFileStream::SetPos_t(SLONG slPosition)
{
  Seek_t(slPosition, SD_BEG);
}

/* Get absolute position in stream. */
SLONG CTFileStream::GetPos_t(void)
{
  if (fstrm_pZipHandle != NULL) {
    return fstrm_iZipLocation;
  } else {
    return ftell(fstrm_pFile);
  }
}

/* Get size of stream */
SLONG CTFileStream::GetStreamSize(void)
{
  if (fstrm_pZipHandle != NULL) {
    return fstrm_slZipSize;

  } else {
    long lCurrentPos = ftell(fstrm_pFile);
    fseek(fstrm_pFile, 0, SD_END);
    long lRet = ftell(fstrm_pFile);
    fseek(fstrm_pFile, lCurrentPos, SD_BEG);
    return lRet;
  }
}

/* Check if file position points to the EOF */
BOOL CTFileStream::AtEOF(void)
{
  if (fstrm_pZipHandle != NULL) {
    return fstrm_iZipLocation >= fstrm_slZipSize;
  }

  // [Cecil] Rewritten to check for EOF immediately instead of relying on the last failed read
  //return feof(fstrm_pFile) != 0;

  // Remember current position before checking for EOF
  long iCurrentPos = ftell(fstrm_pFile);

  // Check EOF by trying to read past the end
  int iEOF = fgetc(fstrm_pFile);

  // Restore remembered position if not at EOF yet
  if (iEOF != EOF) {
    fseek(fstrm_pFile, iCurrentPos, SD_BEG);
    return FALSE; // Didn't hit the end
  }

  return TRUE; // Hit the end
}

// whether or not the given pointer is coming from this stream (mainly used for exception handling)
BOOL CTFileStream::PointerInStream(void* pPointer)
{
  // we're not using virtual allocation buffers so it's fine to return FALSE here.
  return FALSE;
}

/////////////////////////////////////////////////////////////////////////////
// Memory stream construction/destruction

/*
 * Create dynamically resizing stream for reading/writing.
 */
CTMemoryStream::CTMemoryStream(void)
{
  // if current thread has not enabled stream handling
  if (!_bThreadCanHandleStreams) {
    // error
    ::FatalError(TRANS("Can create memory stream, stream handling is not enabled for this thread"));
  }

  mstrm_ctLocked = 0;
  mstrm_bReadable = TRUE;
  mstrm_bWriteable = TRUE;
  mstrm_slLocation = 0;
  // set stream description
  strm_strStreamDescription = "dynamic memory stream";
  // add this newly created memory stream into opened stream list
  _plhOpenedStreams->AddTail( strm_lnListNode);
  // allocate amount of memory needed to hold maximum allowed file length (when saving)
  mstrm_pubBuffer = new UBYTE[_ulMaxLenghtOfSavingFile];
  mstrm_pubBufferEnd = mstrm_pubBuffer + _ulMaxLenghtOfSavingFile;
  mstrm_pubBufferMax = mstrm_pubBuffer;
}

/*
 * Create static stream from given buffer.
 */
CTMemoryStream::CTMemoryStream(void *pvBuffer, SLONG slSize,
                               CTStream::OpenMode om /*= CTStream::OM_READ*/)
{
  // if current thread has not enabled stream handling
  if (!_bThreadCanHandleStreams) {
    // error
    ::FatalError(TRANS("Can create memory stream, stream handling is not enabled for this thread"));
  }

  // allocate amount of memory needed to hold maximum allowed file length (when saving)
  mstrm_pubBuffer = new UBYTE[_ulMaxLenghtOfSavingFile];
  mstrm_pubBufferEnd = mstrm_pubBuffer + _ulMaxLenghtOfSavingFile;
  mstrm_pubBufferMax = mstrm_pubBuffer + slSize;
  // copy given block of memory into memory file
  memcpy( mstrm_pubBuffer, pvBuffer, slSize);

  mstrm_ctLocked = 0;
  mstrm_bReadable = TRUE;
  mstrm_slLocation = 0;
  // if stram is opened in read only mode
  if( om == OM_READ)
  {
    mstrm_bWriteable = FALSE;
  }
  // otherwise, write is enabled
  else
  {
    mstrm_bWriteable = TRUE;
  }
  // set stream description
  strm_strStreamDescription = "dynamic memory stream";
  // add this newly created memory stream into opened stream list
  _plhOpenedStreams->AddTail( strm_lnListNode);
}

/* Destructor. */
CTMemoryStream::~CTMemoryStream(void)
{
  ASSERT(mstrm_ctLocked==0);
  delete[] mstrm_pubBuffer;
  // remove memory stream from list of curently opened streams
  strm_lnListNode.Remove();
}

/////////////////////////////////////////////////////////////////////////////
// Memory stream buffer operations

/*
 * Lock the buffer contents and it's size.
 */
void CTMemoryStream::LockBuffer(void **ppvBuffer, SLONG *pslSize)
{
  mstrm_ctLocked++;
  ASSERT(mstrm_ctLocked>0);

  *ppvBuffer = mstrm_pubBuffer;
  *pslSize = GetSize_t();
}

/*
 * Unlock buffer.
 */
void CTMemoryStream::UnlockBuffer()
{
  mstrm_ctLocked--;
  ASSERT(mstrm_ctLocked>=0);
}

/////////////////////////////////////////////////////////////////////////////
// Memory stream overrides from CTStream

BOOL CTMemoryStream::IsReadable(void)
{
  return mstrm_bReadable && (mstrm_ctLocked==0);
}
BOOL CTMemoryStream::IsWriteable(void)
{
  return mstrm_bWriteable && (mstrm_ctLocked==0);
}
BOOL CTMemoryStream::IsSeekable(void)
{
  return TRUE;
}

/* Read a block of data from stream. */
void CTMemoryStream::Read_t(void *pvBuffer, size_t slSize)
{
  memcpy(pvBuffer, mstrm_pubBuffer + mstrm_slLocation, slSize);
  mstrm_slLocation += (SLONG)slSize;
}

/* Write a block of data to stream. */
void CTMemoryStream::Write_t(const void *pvBuffer, size_t slSize)
{
  memcpy(mstrm_pubBuffer + mstrm_slLocation, pvBuffer, slSize);
  mstrm_slLocation += (SLONG)slSize;

  if(mstrm_pubBuffer + mstrm_slLocation > mstrm_pubBufferMax) {
    mstrm_pubBufferMax = mstrm_pubBuffer + mstrm_slLocation;
  }
}

/* Seek in stream. */
void CTMemoryStream::Seek_t(SLONG slOffset, enum SeekDir sd)
{
  switch(sd) {
  case SD_BEG: mstrm_slLocation = slOffset; break;
  case SD_CUR: mstrm_slLocation += slOffset; break;
  case SD_END: mstrm_slLocation = GetStreamSize() + slOffset; break;
  }
}

/* Set absolute position in stream. */
void CTMemoryStream::SetPos_t(SLONG slPosition)
{
  mstrm_slLocation = slPosition;
}

/* Get absolute position in stream. */
SLONG CTMemoryStream::GetPos_t(void)
{
  return mstrm_slLocation;
}

/* Get size of stream. */
SLONG CTMemoryStream::GetSize_t(void)
{
  return GetStreamSize();
}

/* Get size of stream */
SLONG CTMemoryStream::GetStreamSize(void)
{
  return mstrm_pubBufferMax - mstrm_pubBuffer;
}

/* Get CRC32 of stream */
ULONG CTMemoryStream::GetStreamCRC32_t(void)
{
  return CTStream::GetStreamCRC32_t();
}

/* Check if file position points to the EOF */
BOOL CTMemoryStream::AtEOF(void)
{
  return mstrm_slLocation >= GetStreamSize();
}

// whether or not the given pointer is coming from this stream (mainly used for exception handling)
BOOL CTMemoryStream::PointerInStream(void* pPointer)
{
  return pPointer >= mstrm_pubBuffer && pPointer < mstrm_pubBufferEnd;
}

// Test if a file exists.
BOOL FileExists(const CTFileName &fnmFile)
{
  // if no file
  if (fnmFile=="") {
    // it doesn't exist
    return FALSE;
  }
  // try to
  try {
    // open the file for reading
    CTFileStream strmFile;
    strmFile.Open_t(fnmFile);
    // if successful, it means that it exists,
    return TRUE;
  // if failed, it means that it doesn't exist
  } catch (char *strError) {
    (void) strError;
    return FALSE;
  }
}

// Test if a file exists for writing. 
// (this is can be diferent than normal FileExists() if a mod uses basewriteexclude.lst
BOOL FileExistsForWriting(const CTFileName &fnmFile)
{
  // if no file
  if (fnmFile=="") {
    // it doesn't exist
    return FALSE;
  }
  // expand the filename to full path for writing
  CTFileName fnmFullFileName;
  INDEX iFile = ExpandFilePath(EFP_WRITE, fnmFile, fnmFullFileName);

  return FileSystem::Exists(fnmFullFileName.ConstData());
}

// Get file timestamp
SLONG GetFileTimeStamp_t(const CTFileName &fnm)
{
  // expand the filename to full path
  CTFileName fnmExpanded;
  INDEX iFile = ExpandFilePath(EFP_READ, fnm, fnmExpanded);
  if (iFile!=EFP_FILE) {
    return FALSE;
  }

  int file_handle;
  // try to open file for reading
  fnmExpanded.ReplaceChar('\\', '/'); // [Cecil] NOTE: For _open()
  file_handle = _open(fnmExpanded.ConstData(), _O_RDONLY | _O_BINARY);
  if(file_handle==-1) {
    ThrowF_t(TRANS("Cannot open file '%s' for reading"), fnm.ConstData());
    return -1;
  }
  struct stat statFileStatus;
  // get file status
  fstat( file_handle, &statFileStatus);
  _close( file_handle);
  ASSERT(statFileStatus.st_mtime<=time(NULL));
  return statFileStatus.st_mtime;
}

// Get CRC32 of a file
ULONG GetFileCRC32_t(const CTFileName &fnmFile) // throw char *
{
  // open the file
  CTFileStream fstrm;
  fstrm.Open_t(fnmFile);
  // return the checksum
  return fstrm.GetStreamCRC32_t();
}

// Test if a file is read only (also returns FALSE if file does not exist)
BOOL IsFileReadOnly(const CTFileName &fnm)
{
  // expand the filename to full path
  CTFileName fnmExpanded;
  INDEX iFile = ExpandFilePath(EFP_READ, fnm, fnmExpanded);
  if (iFile!=EFP_FILE) {
    return FALSE;
  }

  int file_handle;
  // try to open file for reading
  fnmExpanded.ReplaceChar('\\', '/'); // [Cecil] NOTE: For _open()
  file_handle = _open(fnmExpanded.ConstData(), _O_RDONLY | _O_BINARY);
  if(file_handle==-1) {
    return FALSE;
  }
  struct stat statFileStatus;
  // get file status
  fstat( file_handle, &statFileStatus);
  _close( file_handle);
  ASSERT(statFileStatus.st_mtime<=time(NULL));
  return !(statFileStatus.st_mode&_S_IWRITE);
}

// Delete a file
BOOL RemoveFile(const CTFileName &fnmFile)
{
  // expand the filename to full path
  CTFileName fnmExpanded;
  INDEX iFile = ExpandFilePath(EFP_WRITE, fnmFile, fnmExpanded);
  if (iFile==EFP_FILE) {
    int ires = remove(fnmExpanded.ConstData());
    return ires==0;
  } else {
    return FALSE;
  }
}

// check for some file extensions that can be substituted
static BOOL SubstExt_internal(CTFileName &fnmFullFileName)
{
  if (fnmFullFileName.FileExt() == ".mp3") {
    fnmFullFileName = fnmFullFileName.NoExt() + ".ogg";
    return TRUE;

  } else if (fnmFullFileName.FileExt() == ".ogg") {
    fnmFullFileName = fnmFullFileName.NoExt() + ".mp3";
    return TRUE;

  // [Cecil] ASCII and binary model configs are interchangeable
  } else if (fnmFullFileName.FileExt() == ".smc") {
    fnmFullFileName = fnmFullFileName.NoExt() + ".bmc";
    return TRUE;

  } else if (fnmFullFileName.FileExt() == ".bmc") {
    fnmFullFileName = fnmFullFileName.NoExt() + ".smc";
    return TRUE;
  }

  // [Cecil] TRUE -> FALSE
  return FALSE;
}

// [Cecil] Check if file exists at a specific path (relative or absolute)
static inline BOOL CheckFileAt(CTString strBaseDir, CTFileName fnmFile, CTFileName &fnmExpanded) {
  // Match backward slashes for consistency
  strBaseDir.ReplaceChar('/', '\\');
  fnmFile.ReplaceChar('/', '\\');

  if (fnmFile.HasPrefix(strBaseDir)) {
    fnmExpanded = fnmFile;
  } else {
    fnmExpanded = strBaseDir + fnmFile;
  }

  return FileSystem::Exists(fnmExpanded.ConstData());
};

static INDEX ExpandFilePath_read(ULONG ulType, const CTFileName &fnmFile, CTFileName &fnmExpanded)
{
  // Search for the file in archives
  const IZip::CEntry *pZipEntry = IZip::FindEntry(fnmFile);
  const BOOL bFoundInZip = (pZipEntry != NULL);

  // [Cecil] Already an absolute path
  if (fnmFile.IsAbsolute()) {
    fnmExpanded = fnmFile;

    if (bFoundInZip) {
      return (_fnmMod != "" && pZipEntry->IsMod()) ? EFP_MODZIP : EFP_BASEZIP;
    }

    return EFP_FILE;
  }

  // [Cecil] Check file at a specific directory and return if it exists
  #define RETURN_FILE_AT(_Dir) \
    { if (CheckFileAt(_Dir, fnmFile, fnmExpanded)) return EFP_FILE; }

  // If a mod is active
  if (_fnmMod != "") {
    // Try mod directory before archives
    if (!fil_bPreferZips) {
      RETURN_FILE_AT(_fnmApplicationPath + _fnmMod);
    }

    // If allowing archives, use the found file
    if (!(ulType & EFP_NOZIPS) && bFoundInZip && pZipEntry->IsMod()) {
      // [Cecil] Instead of returning relative path to the file inside any archive, return full path to the file
      // inside its archive, for example: "C:\\SeriousSam\\Mods\\MyMod\\Content.gro\\MyModels\\Model01.mdl"
      fnmExpanded = pZipEntry->GetArchive() + "\\" + fnmFile;
      return EFP_MODZIP;
    }

    // Try mod directory after archives
    if (fil_bPreferZips) {
      RETURN_FILE_AT(_fnmApplicationPath + _fnmMod);
    }
  }

  // Try game root directory before archives
  if (!fil_bPreferZips) {
    RETURN_FILE_AT(_fnmApplicationPath);
  }

  // If allowing archives, use the found file
  if (!(ulType & EFP_NOZIPS) && bFoundInZip) {
    // [Cecil] Instead of returning relative path to the file inside any archive, return full path to the file
    // inside its archive, for example: "C:\\SeriousSam\\MyAssets.gro\\MyModels\\Model01.mdl"
    fnmExpanded = pZipEntry->GetArchive() + "\\" + fnmFile;
    return EFP_BASEZIP;
  }

  // Try game root directory after archives
  if (fil_bPreferZips) {
    RETURN_FILE_AT(_fnmApplicationPath);
  }

  // [Cecil] Try searching other game directories after the main one
  const INDEX ctDirs = _aContentDirs.Count();

  for (INDEX iDir = ctDirs - 1; iDir >= 0; iDir--) {
    const ExtraContentDir_t &dir = _aContentDirs[iDir];

    // No game directory
    if (!dir.bGame || dir.fnmPath == "") continue;

    RETURN_FILE_AT(dir.fnmPath);
  }

  return EFP_NONE;
};

// Expand a file's filename to full path
INDEX ExpandFilePath(ULONG ulType, const CTFileName &fnmFile, CTFileName &fnmExpanded)
{
  CTFileName fnmFileAbsolute = fnmFile;
  fnmFileAbsolute.NormalizePath();

  // if writing
  if (ulType&EFP_WRITE) {
    // [Cecil] Already an absolute path
    if (fnmFileAbsolute.IsAbsolute()) {
      fnmExpanded = fnmFileAbsolute;
      return EFP_FILE;
    }

    // if should write to mod dir
    if (_fnmMod != "" && FileMatchesList(_afnmModWrite, fnmFileAbsolute)) {
      // do that
      fnmExpanded = _fnmApplicationPath + _fnmMod + fnmFileAbsolute;
      return EFP_FILE;
    // if should not write to mod dir
    } else {
      // write to base dir
      fnmExpanded = _fnmApplicationPath + fnmFileAbsolute;
      return EFP_FILE;
    }

  // if reading
  } else if (ulType&EFP_READ) {

    // check for expansions for reading
    INDEX iRes = ExpandFilePath_read(ulType, fnmFileAbsolute, fnmExpanded);
    // if not found
    if (iRes==EFP_NONE) {
      //check for some file extensions that can be substituted
      CTFileName fnmSubst = fnmFileAbsolute;
      if (SubstExt_internal(fnmSubst)) {
        iRes = ExpandFilePath_read(ulType, fnmSubst, fnmExpanded);
      }
    }

    fnmExpanded.NormalizePath();

    if (iRes!=EFP_NONE) {
      return iRes;
    }

  // in other cases
  } else  {
    ASSERT(FALSE);
    fnmExpanded = _fnmApplicationPath + fnmFileAbsolute;
    return EFP_FILE;
  }

  fnmExpanded = _fnmApplicationPath + fnmFileAbsolute;
  return EFP_NONE;
}
