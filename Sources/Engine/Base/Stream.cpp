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

#include "stdh.h"

#include <sys\types.h>
#include <sys\stat.h>
#include <fcntl.h>
#include <io.h>
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
#include <Engine/Templates/NameTable_CTFileName.h>
#include <Engine/Templates/StaticArray.cpp>
#include <Engine/Templates/DynamicStackArray.cpp>

#include <Engine/Templates/Stock_CTextureData.h>
#include <Engine/Templates/Stock_CModelData.h>

// default size of page used for stream IO operations (4Kb)
ULONG _ulPageSize = 0;
// maximum lenght of file that can be saved (default: 128Mb)
ULONG _ulMaxLenghtOfSavingFile = (1UL<<20)*128;
extern INDEX fil_bPreferZips = FALSE;

// set if current thread has currently enabled stream handling
static _declspec(thread) BOOL _bThreadCanHandleStreams = FALSE;
// list of currently opened streams
static _declspec(thread) CListHead *_plhOpenedStreams = NULL;

// global string with application path
CTFileName _fnmApplicationPath;
// global string with filename of the started application
CTFileName _fnmApplicationExe;
// global string with current MOD path
CTFileName _fnmMod;
// global string with current name (the parameter that is passed on cmdline)
CTString _strModName;
// global string with url to be shown to users that don't have the mod installed
// (should be set by game.dll)
CTString _strModURL;
// global string with current MOD extension (for adding to dlls)
CTString _strModExt;
// global string with CD path (for minimal installations)
CTFileName _fnmCDPath;

// include/exclude lists for base dir writing/browsing
CDynamicStackArray<CTFileName> _afnmBaseWriteInc;
CDynamicStackArray<CTFileName> _afnmBaseWriteExc;
CDynamicStackArray<CTFileName> _afnmBaseBrowseInc;
CDynamicStackArray<CTFileName> _afnmBaseBrowseExc;
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

extern BOOL FileMatchesList(CDynamicStackArray<CTFileName> &afnm, const CTFileName &fnm)
{
  for(INDEX i=0; i<afnm.Count(); i++) {
    if (fnm.Matches(afnm[i]) || fnm.HasPrefix(afnm[i])) {
      return TRUE;
    }
  }
  return FALSE;
}

static CTFileName _fnmApp;

void InitStreams(void)
{
  // obtain information about system
  SYSTEM_INFO siSystemInfo;
  GetSystemInfo( &siSystemInfo);
  // and remember page size
  _ulPageSize = siSystemInfo.dwPageSize*16;   // cca. 64kB on WinNT/Win95

  // keep a copy of path for setting purposes
  _fnmApp = _fnmApplicationPath;

  // if no mod defined yet
  if (_fnmMod=="") {
    // check for 'default mod' file
    LoadStringVar(CTString("DefaultMod.txt"), _fnmMod);
  }

  CPrintF(TRANS("Current mod: %s\n"), _fnmMod==""?TRANS("<none>"):(CTString&)_fnmMod);
  // if there is a mod active
  if (_fnmMod!="") {
    // load mod's include/exclude lists
    CPrintF(TRANS("Loading mod include/exclude lists...\n"));
    BOOL bOK = FALSE;
    bOK |= LoadFileList(_afnmBaseWriteInc , CTString("BaseWriteInclude.lst"));
    bOK |= LoadFileList(_afnmBaseWriteExc , CTString("BaseWriteExclude.lst"));
    bOK |= LoadFileList(_afnmBaseBrowseInc, CTString("BaseBrowseInclude.lst"));
    bOK |= LoadFileList(_afnmBaseBrowseExc, CTString("BaseBrowseExclude.lst"));

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
    }
  }
  // find eventual extension for the mod's dlls
  _strModExt = "";
  LoadStringVar(CTString("ModExt.txt"), _strModExt);


  CPrintF(TRANS("Loading group files...\n"));

  // for each group file in base directory
  struct _finddata_t c_file;
  long hFile;
  hFile = _findfirst(_fnmApplicationPath+"*.gro", &c_file);
  BOOL bOK = (hFile!=-1);
  while(bOK) {
    if (CTString(c_file.name).Matches("*.gro")) {
      // add it to active set
      UNZIPAddArchive(_fnmApplicationPath+c_file.name);
    }
    bOK = _findnext(hFile, &c_file)==0;
  }
  _findclose( hFile );

  // if there is a mod active
  if (_fnmMod!="") {
    // for each group file in mod directory
    struct _finddata_t c_file;
    long hFile;
    hFile = _findfirst(_fnmApplicationPath+_fnmMod+"*.gro", &c_file);
    BOOL bOK = (hFile!=-1);
    while(bOK) {
      if (CTString(c_file.name).Matches("*.gro")) {
        // add it to active set
        UNZIPAddArchive(_fnmApplicationPath+_fnmMod+c_file.name);
      }
      bOK = _findnext(hFile, &c_file)==0;
    }
    _findclose( hFile );
  }

  // if there is a CD path
  if (_fnmCDPath!="") {
    // for each group file on the CD
    struct _finddata_t c_file;
    long hFile;
    hFile = _findfirst(_fnmCDPath+"*.gro", &c_file);
    BOOL bOK = (hFile!=-1);
    while(bOK) {
      if (CTString(c_file.name).Matches("*.gro")) {
        // add it to active set
        UNZIPAddArchive(_fnmCDPath+c_file.name);
      }
      bOK = _findnext(hFile, &c_file)==0;
    }
    _findclose( hFile );

    // if there is a mod active
    if (_fnmMod!="") {
      // for each group file in mod directory
      struct _finddata_t c_file;
      long hFile;
      hFile = _findfirst(_fnmCDPath+_fnmMod+"*.gro", &c_file);
      BOOL bOK = (hFile!=-1);
      while(bOK) {
        if (CTString(c_file.name).Matches("*.gro")) {
          // add it to active set
          UNZIPAddArchive(_fnmCDPath+_fnmMod+c_file.name);
        }
        bOK = _findnext(hFile, &c_file)==0;
      }
      _findclose( hFile );
    }
  }

  // try to
  try {
    // read the zip directories
    UNZIPReadDirectoriesReverse_t();
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


void UseApplicationPath(void) 
{
  _fnmApplicationPath = _fnmApp;
}

void IgnoreApplicationPath(void)
{
  _fnmApplicationPath = CTString("");
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
    if (pIllegalAdress >= itStream->strm_pubBufferBegin && pIllegalAdress < itStream->strm_pubBufferEnd)
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
  FatalError( GetWindowsError( GetLastError()) );
}

/*
 * Throw an exception of formatted string.
 */
void CTStream::Throw_t(char *strFormat, ...)  // throws char *
{
  const SLONG slBufferSize = 256;
  char strFormatBuffer[slBufferSize];
  char strBuffer[slBufferSize];
  // add the stream description to the format string
  _snprintf(strFormatBuffer, slBufferSize, "%s (%s)", strFormat, strm_strStreamDescription);
  // format the message in buffer
  va_list arg;
  va_start(arg, strFormat); // variable arguments start after this argument
  _vsnprintf(strBuffer, slBufferSize, strFormatBuffer, arg);
  throw strBuffer;
}

/////////////////////////////////////////////////////////////////////////////
// Binary access methods

// [Cecil] 1.07 compatibility
void CTStream::Seek_t(SLONG slOffset, SeekDir sd) {
  UpdateMaxPos();
  ASSERT(IsSeekable());

  if (sd == SD_BEG) {
    strm_pubCurrentPos = strm_pubBufferBegin + slOffset;

  } else if (sd == SD_END) {
    strm_pubCurrentPos = strm_pubEOF + slOffset;

  } else if (sd == SD_CUR) {
    strm_pubCurrentPos += slOffset;

  } else {
    FatalError(TRANS("Stream seeking requested with unknown seek direction: %d\n"), sd);
  }

  if (strm_pubCurrentPos < strm_pubBufferBegin || strm_pubCurrentPos > strm_pubBufferEnd) {
    Throw_t(TRANS("Error while seeking"));
  }
};


// [Cecil] 1.07 compatibility
void CTStream::SetPos_t(SLONG slPosition) {
  Seek_t(slPosition, CTStream::SD_BEG);
};

// [Cecil] 1.07 compatibility
SLONG CTStream::GetPos_t(void) {
  ASSERT(IsSeekable());
  return strm_pubCurrentPos - strm_pubBufferBegin;
};

// [Cecil] 1.07 compatibility
BOOL CTStream::AtEOF(void) {
  ASSERT(IsSeekable());
  ASSERT(!IsWriteable());
  return (strm_pubCurrentPos >= strm_pubEOF);
};

// [Cecil] 1.07 compatibility
SLONG CTStream::GetStreamSize(void) {
  if (!IsWriteable()) {
    return strm_pubEOF - strm_pubBufferBegin;
  } else {
    UpdateMaxPos();
    return strm_pubMaxPos - strm_pubBufferBegin;
  }
};

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
    ThrowF_t(TRANS("EOF reached, file %s"), strm_strStreamDescription);
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
  INDEX iStringLength = strlen(strBuffer);
  // put line into stream
  for (INDEX iLetter = 0; iLetter < iStringLength; iLetter++) {
    *strm_pubCurrentPos++ = *strBuffer++;
  }
  // write "\r\n" into stream
  *strm_pubCurrentPos++ = '\r';
  *strm_pubCurrentPos++ = '\n';
}

void CTStream::PutString_t(const char *strString) // throw char *
{
  // check parameters
  ASSERT(strString!=NULL);
  // check that the stream is writteable
  ASSERT(IsWriteable());
  // get string length
  INDEX iStringLength = strlen(strString);
  // put line into stream
  for( INDEX iLetter=0; iLetter<iStringLength; iLetter++)
  {
    if (*strString=='\n') {
      // write "\r\n" into stream
      *strm_pubCurrentPos++ = '\r';
      *strm_pubCurrentPos++ = '\n';
      strString++;
    } else {
      *strm_pubCurrentPos++ = *strString++;
    }
  }
}

void CTStream::FPrintF_t(const char *strFormat, ...) // throw char *
{
  const SLONG slBufferSize = 2048;
  char strBuffer[slBufferSize];
  // format the message in buffer
  va_list arg;
  va_start(arg, strFormat); // variable arguments start after this argument
  _vsnprintf(strBuffer, slBufferSize, strFormat, arg);
  // print the buffer
  PutString_t(strBuffer);
}

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
  for(INDEX iKeywordChar=0; iKeywordChar<(INDEX)strlen(strKeyword); iKeywordChar++) {
    SBYTE chKeywordChar;
    (*this)>>chKeywordChar;
    if (chKeywordChar!=strKeyword[iKeywordChar]) {
      ThrowF_t(TRANS("Expected keyword %s not found"), strKeyword);
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
    *this<<fnmImportFrom<<slImportOffset;
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
    *this<<strm_afnmDictionary[iFileName];
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
      *this>>strm_afnmDictionary[iFileName];
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
    *this>>fnmImportFrom>>slImportOffset;

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

    // for each filename
    INDEX ctFileNames = strm_afnmDictionary.Count();
    for(INDEX iFileName=0; iFileName<ctFileNames; iFileName++) {
      CTFileName &fnm = strm_afnmDictionary[iFileName];
      // if not preloaded
      if (fnm.fnm_pserPreloaded==NULL) {
        // skip
        continue;
      }
      // free preloaded instance
      CTString strExt = fnm.FileExt();
      if (strExt==".tex") {
        _pTextureStock->Release((CTextureData*)fnm.fnm_pserPreloaded);
      } else if (strExt==".mdl") {
        _pModelStock->Release((CModelData*)fnm.fnm_pserPreloaded);
      }
    }

    strm_afnmDictionary.Clear();
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
        fnm.fnm_pserPreloaded = _pTextureStock->Obtain_t(fnm);
      } else if (strExt==".mdl") {
        fnm.fnm_pserPreloaded = _pModelStock->Obtain_t(fnm);
      }
    } catch (char *strError) {
      CPrintF( TRANS("Cannot preload %s: %s\n"), (CTString&)fnm, strError);
    }
  }
}

/////////////////////////////////////////////////////////////////////////////
// General construction/destruction

/* Default constructor. */
CTStream::CTStream(void) : strm_ntDictionary(*new CNameTable_CTFileName)
{
  // [Cecil] 1.07 compatibility
  strm_pubBufferBegin = NULL;
  strm_pubBufferEnd = NULL;
  strm_pubCurrentPos = NULL;
  strm_pubMaxPos = NULL;
  strm_pubEOF = NULL;

  strm_strStreamDescription = "";
  strm_slDictionaryPos = 0;
  strm_dmDictionaryMode = DM_NONE;
}

// [Cecil] 1.07 compatibility: Free allocated memory
void CTStream::FreeBuffer(void) {
  if (strm_pubBufferBegin != NULL) {
    free(strm_pubBufferBegin);
    strm_pubBufferBegin = NULL;
  }
};

/* Destructor. */
CTStream::~CTStream(void)
{
  // [Cecil] 1.07 compatibility: Free allocated memory
  FreeBuffer(); 

  strm_ntDictionary.Clear();
  strm_afnmDictionary.Clear();

  delete &strm_ntDictionary;
}

// [Cecil] 1.07 compatibility: Allocate memory for the stream
void CTStream::AllocateVirtualMemory(ULONG ulBytesToAllocate)
{
  strm_pubBufferBegin = (UBYTE *)calloc(ulBytesToAllocate, 1);
  strm_pubBufferEnd = strm_pubBufferBegin + ulBytesToAllocate;
  // Reset location pointers
  strm_pubCurrentPos = strm_pubBufferBegin;
  strm_pubMaxPos = strm_pubBufferBegin;
};

// [Cecil] 1.07 compatibility: None of these functions are needed
void CTStream::CommitPage(INDEX iPage) {};
void CTStream::DecommitPage(INDEX iPage) {};
void CTStream::ProtectPageFromWritting(INDEX iPage) {};
void CTFileStream::WritePageToFile(INDEX iPageToWrite) {};
void CTFileStream::FileCommitPage( INDEX iPageToCommit) {};
void CTFileStream::FileDecommitPage( INDEX iPageToDecommit) {};
void CTFileStream::HandleAccess(INDEX iAccessedPage, BOOL bReadAttempted) {};
void CTMemoryStream::HandleAccess(INDEX iAccessedPage, BOOL bReadAttempted) {};

/////////////////////////////////////////////////////////////////////////////
// File stream opening/closing methods

/*
 * Default constructor.
 */
CTFileStream::CTFileStream(void)
{
  fstrm_pFile = NULL;
  fstrm_iLastAccessedPage = -1; // [Cecil] 1.07 compatibility
  // mark that file is created for writing
  fstrm_bReadOnly = TRUE;
  fstrm_iZipHandle = -1;
}

/*
 * Destructor.
 */
CTFileStream::~CTFileStream(void)
{
  // close stream
  if (fstrm_pFile != NULL || fstrm_iZipHandle!=-1) {
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
    ::ThrowF_t(TRANS("Cannot open file `%s', stream handling is not enabled for this thread"),
      (CTString&)fnFileName);
  }

  // check parameters
  ASSERT(strlen(fnFileName)>0);
  // check that the file is not open
  ASSERT(fstrm_pFile==NULL && fstrm_iZipHandle==-1);

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
      fstrm_iZipHandle = UNZIPOpen_t(fnmFullFileName);

      // [Cecil] 1.07 compatibility: Allocate buffer for the file and read all of its bytes into it
      SLONG slFileSize = UNZIPGetSize(fstrm_iZipHandle);
      AllocateVirtualMemory((slFileSize / _ulPageSize + 2) * _ulPageSize);
      strm_pubEOF = strm_pubBufferBegin + slFileSize;

      UNZIPReadBlock_t(fstrm_iZipHandle, strm_pubBufferBegin, 0, slFileSize);

    // if it is a physical file
    } else if (iFile==EFP_FILE) {
      // open file in read only mode
      fstrm_pFile = fopen(fnmFullFileName, "rb");

      // [Cecil] 1.07 compatibility: Allocate buffer for the file and read all of its bytes into it
      fseek(fstrm_pFile, 0, SEEK_END);
      SLONG slFileSize = ftell( fstrm_pFile);
      fseek(fstrm_pFile, 0, SEEK_SET);

      AllocateVirtualMemory((slFileSize / _ulPageSize + 2) * _ulPageSize);
      strm_pubEOF = strm_pubBufferBegin + slFileSize;

      fread(strm_pubBufferBegin, slFileSize, 1, fstrm_pFile);
    }
    fstrm_bReadOnly = TRUE;
  
  // if write mode requested
  } else if( om == OM_WRITE) {
    // open file for reading and writing
    fstrm_pFile = fopen(fnmFullFileName, "rb+");
    fstrm_bReadOnly = FALSE;

    // [Cecil] 1.07 compatibility: Allocate enough memory for the contents of the file
    AllocateVirtualMemory(_ulMaxLenghtOfSavingFile + _ulPageSize);

  // if unknown mode
  } else {
    FatalError(TRANS("File stream opening requested with unknown open mode: %d\n"), om);
  }

  // if openning operation was not successfull
  if(fstrm_pFile == NULL && fstrm_iZipHandle==-1) {
    // throw exception
    Throw_t(TRANS("Cannot open file `%s' (%s)"), (CTString&)fnmFullFileName,
      strerror(errno));
  }

  // if file opening was successfull, set stream description to file name
  strm_strStreamDescription = fnFileName; // [Cecil] 1.07 compatibility: Relative filename
  // add this newly opened file into opened stream list
  _plhOpenedStreams->AddTail( strm_lnListNode);
}

/*
 * Create a new file or overwrite existing.
 */
void CTFileStream::Create_t(const CTFileName &fnFileName,
                            enum CTStream::CreateMode cm) // throws char *
{
  (void)cm; // OBSOLETE!

  // if current thread has not enabled stream handling
  if (!_bThreadCanHandleStreams) {
    // error
    ::ThrowF_t(TRANS("Cannot create file `%s', stream handling is not enabled for this thread"),
      (CTString&)fnFileName);
  }

  CTFileName fnmFullFileName;
  INDEX iFile = ExpandFilePath(EFP_WRITE, fnFileName, fnmFullFileName);

  // check parameters
  ASSERT(strlen(fnFileName)>0);
  // check that the file is not open
  ASSERT(fstrm_pFile == NULL);
  // open file stream for writing (destroy file context if file existed before)
  fstrm_pFile = fopen(fnmFullFileName, "wb+");
  // if not successfull
  if(fstrm_pFile == NULL)
  {
    // throw exception
    Throw_t(TRANS("Cannot create file `%s' (%s)"), (CTString&)fnmFullFileName,
      strerror(errno));
  }

  // [Cecil] 1.07 compatibility: Allocate enough memory for the contents of the file
  AllocateVirtualMemory(_ulMaxLenghtOfSavingFile + _ulPageSize);

  // if file creation was successfull, set stream description to file name
  strm_strStreamDescription = fnFileName; // [Cecil] 1.07 compatibility: Relative filename
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
  if (fstrm_pFile==NULL && fstrm_iZipHandle==-1) {
    ASSERT(FALSE);
    return;
  }

  // clear stream description
  strm_strStreamDescription = "";
  // remove file from list of curently opened streams
  strm_lnListNode.Remove();

  // if file on disk
  if (fstrm_pFile != NULL) {
    // [Cecil] 1.07 compatibility: Flush written data back into the file
    if (!fstrm_bReadOnly) {
      fseek(fstrm_pFile, 0, SEEK_SET);
      fwrite(strm_pubBufferBegin, GetStreamSize(), 1, fstrm_pFile);
      fflush(fstrm_pFile);
    }

    // close file
    fclose( fstrm_pFile);
    fstrm_pFile = NULL;
  // if file in zip
  } else if (fstrm_iZipHandle>=0) {
    // close zip entry
    UNZIPClose(fstrm_iZipHandle);
    fstrm_iZipHandle = -1;
  }

  // [Cecil] 1.07 compatibility: Free allocated memory
  FreeBuffer();

  // clear dictionary vars
  strm_dmDictionaryMode = DM_NONE;
  strm_ntDictionary.Clear();
  strm_afnmDictionary.Clear();
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
  } else if (fstrm_iZipHandle >=0) {
    return UNZIPGetCRC(fstrm_iZipHandle);
  } else {
    ASSERT(FALSE);
    return 0;
  }
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

  // allocate amount of memory needed to hold maximum allowed file length (when saving)
  AllocateVirtualMemory(_ulMaxLenghtOfSavingFile);
  mstrm_ctLocked = 0;
  mstrm_bReadable = TRUE;
  mstrm_bWriteable = TRUE;
  // set stream description
  strm_strStreamDescription = "dynamic memory stream";
  // add this newly created memory stream into opened stream list
  _plhOpenedStreams->AddTail( strm_lnListNode);
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
  AllocateVirtualMemory(_ulMaxLenghtOfSavingFile);
  // copy given block of memory into memory file
  memcpy(strm_pubCurrentPos, pvBuffer, slSize);

  mstrm_ctLocked = 0;
  mstrm_bReadable = TRUE;
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
  UpdateMaxPos();

  *ppvBuffer = strm_pubBufferBegin;
  *pslSize = strm_pubMaxPos - strm_pubBufferBegin;
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

  // check if it exists
  FILE *f = fopen(fnmFullFileName, "rb");
  if (f!=NULL) { 
    fclose(f);
    return TRUE;
  } else {
    return FALSE;
  }
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
  file_handle = _open( fnmExpanded, _O_RDONLY | _O_BINARY);
  if(file_handle==-1) {
    ThrowF_t(TRANS("Cannot open file '%s' for reading"), CTString(fnm));
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
  file_handle = _open( fnmExpanded, _O_RDONLY | _O_BINARY);
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
    int ires = remove(fnmExpanded);
    return ires==0;
  } else {
    return FALSE;
  }
}


static BOOL IsFileReadable_internal(CTFileName &fnmFullFileName)
{
  FILE *pFile = fopen(fnmFullFileName, "rb");
  if (pFile!=NULL) {
    fclose(pFile);
    return TRUE;
  } else {
    return FALSE;
  }
}

// check for some file extensions that can be substituted
static BOOL SubstExt_internal(CTFileName &fnmFullFileName)
{
  if (fnmFullFileName.FileExt()==".mp3") {
    fnmFullFileName = fnmFullFileName.NoExt()+".ogg";
    return TRUE;
  } else if (fnmFullFileName.FileExt()==".ogg") {
    fnmFullFileName = fnmFullFileName.NoExt()+".mp3";
    return TRUE;
  } else {
    return TRUE;
  }
}


static INDEX ExpandFilePath_read(ULONG ulType, const CTFileName &fnmFile, CTFileName &fnmExpanded)
{
  // search for the file in zips
  INDEX iFileInZip = UNZIPGetFileIndex(fnmFile);

  // if a mod is active
  if (_fnmMod!="") {

    // first try in the mod's dir
    if (!fil_bPreferZips) {
      fnmExpanded = _fnmApplicationPath+_fnmMod+fnmFile;
      if (IsFileReadable_internal(fnmExpanded)) {
        return EFP_FILE;
      }
    }

    // if not disallowing zips
    if (!(ulType&EFP_NOZIPS)) {
      // if exists in mod's zip
      if (iFileInZip>=0 && UNZIPIsFileAtIndexMod(iFileInZip)) {
        // use that one
        fnmExpanded = fnmFile;
        return EFP_MODZIP;
      }
    }

    // try in the mod's dir after
    if (fil_bPreferZips) {
      fnmExpanded = _fnmApplicationPath+_fnmMod+fnmFile;
      if (IsFileReadable_internal(fnmExpanded)) {
        return EFP_FILE;
      }
    }
  }

  // try in the app's base dir
  if (!fil_bPreferZips) {
    CTFileName fnmAppPath = _fnmApplicationPath;
    fnmAppPath.SetAbsolutePath();

    if(fnmFile.HasPrefix(fnmAppPath)) {
      fnmExpanded = fnmFile;
    } else {
      fnmExpanded = _fnmApplicationPath+fnmFile;
    }

    if (IsFileReadable_internal(fnmExpanded)) {
      return EFP_FILE;
    }
  }

  // if not disallowing zips
  if (!(ulType&EFP_NOZIPS)) {
    // if exists in any zip
    if (iFileInZip>=0) {
      // use that one
      fnmExpanded = fnmFile;
      return EFP_BASEZIP;
    }
  }

  // try in the app's base dir
  if (fil_bPreferZips) {
    fnmExpanded = _fnmApplicationPath+fnmFile;
    if (IsFileReadable_internal(fnmExpanded)) {
      return EFP_FILE;
    }
  }

  // finally, try in the CD path
  if (_fnmCDPath!="") {

    // if a mod is active
    if (_fnmMod!="") {
      // first try in the mod's dir
      fnmExpanded = _fnmCDPath+_fnmMod+fnmFile;
      if (IsFileReadable_internal(fnmExpanded)) {
        return EFP_FILE;
      }
    }

    fnmExpanded = _fnmCDPath+fnmFile;
    if (IsFileReadable_internal(fnmExpanded)) {
      return EFP_FILE;
    }
  }
  return EFP_NONE;
}

// Expand a file's filename to full path
INDEX ExpandFilePath(ULONG ulType, const CTFileName &fnmFile, CTFileName &fnmExpanded)
{
  CTFileName fnmFileAbsolute = fnmFile;
  fnmFileAbsolute.SetAbsolutePath();

  // if writing
  if (ulType&EFP_WRITE) {
    // if should write to mod dir
    if (_fnmMod!="" && (!FileMatchesList(_afnmBaseWriteInc, fnmFileAbsolute) || FileMatchesList(_afnmBaseWriteExc, fnmFileAbsolute))) {
      // do that
      fnmExpanded = _fnmApplicationPath+_fnmMod+fnmFileAbsolute;
      fnmExpanded.SetAbsolutePath();
      return EFP_FILE;
    // if should not write to mod dir
    } else {
      // write to base dir
      fnmExpanded = _fnmApplicationPath+fnmFileAbsolute;
      fnmExpanded.SetAbsolutePath();
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

    fnmExpanded.SetAbsolutePath();

    if (iRes!=EFP_NONE) {
      return iRes;
    }

  // in other cases
  } else  {
    ASSERT(FALSE);
    fnmExpanded = _fnmApplicationPath+fnmFileAbsolute;
    fnmExpanded.SetAbsolutePath();
    return EFP_FILE;
  }

  fnmExpanded = _fnmApplicationPath+fnmFileAbsolute;
  fnmExpanded.SetAbsolutePath();
  return EFP_NONE;
}
