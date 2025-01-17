/* Copyright (c) 2002-2012 Croteam Ltd. 
   Copyright (c) 2023-2024 Dreamy Cecil
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

// [Cecil] ASM code translation:
// https://gitlab.com/TwilightWingsStudio/SSE/SeriousEngineE/-/blob/master/Core/Base/String.cpp

#include "StdH.h"

#include <Engine/Base/CTString.h>
#include <Engine/Base/Memory.h>
#include <Engine/Base/Stream.h>
#include <Engine/Base/Console.h>

// [Cecil] Invalid character index
const size_t CTString::npos = size_t(-1);

/*
 * Equality comparison.
 */
BOOL CTString::operator==(const CTString &strOther) const
{
  ASSERT(IsValid() && strOther.IsValid());

  return stricmp( str_String, strOther.str_String) == 0;
}
BOOL CTString::operator==(const char *strOther) const
{
  ASSERT(IsValid() && strOther!=NULL);

  return stricmp( str_String, strOther) == 0;
}
 BOOL operator==(const char *strThis, const CTString &strOther)
{
  ASSERT(strOther.IsValid() && strThis!=NULL);

  return strOther == strThis;
}

/*
 * Inequality comparison.
 */
BOOL CTString::operator!=(const CTString &strOther) const
{
  ASSERT(IsValid() && strOther.IsValid());

  return !( *this == strOther );
}
BOOL CTString::operator!=(const char *strOther) const
{
  ASSERT(IsValid() && strOther!=NULL);

  return !( *this == strOther );
}
 BOOL operator!=(const char *strThis, const CTString &strOther)
{
  ASSERT(strOther.IsValid() && strThis!=NULL);

  return !( strOther == strThis);
}


/*
 * String concatenation.
 */
CTString CTString::operator+(const CTString &strSecond) const
{
  ASSERT(IsValid() && strSecond.IsValid());

  return(CTString(*this)+=strSecond);
}
CTString operator+(const char *strFirst, const CTString &strSecond)
{
  ASSERT(strFirst!=NULL && strSecond.IsValid());

  return(CTString(strFirst)+=strSecond);
}
CTString &CTString::operator+=(const CTString &strSecond)
{
  ASSERT(IsValid() && strSecond.IsValid());

  GrowMemory((void **)&str_String, Length() + strSecond.Length() + 1);
  strcat(str_String, strSecond.str_String);
  return *this;
}

/*
 * Remove given prefix string from this string
 */
BOOL CTString::RemovePrefix( const CTString &strPrefix)
{
  INDEX lenPrefix = strPrefix.Length();
  INDEX lenDest = Length() - lenPrefix;

  if (strnicmp(str_String, strPrefix.str_String, lenPrefix) != 0)
    return FALSE;
  CTString strTemp = &str_String[lenPrefix];
  ShrinkMemory( (void **)&str_String, lenDest+1);
  strcpy( str_String, strTemp.str_String);
  return TRUE;
}
/* Check if has given prefix */
BOOL CTString::HasPrefix( const CTString &strPrefix) const
{
  INDEX lenPrefix = strPrefix.Length();
  if (strnicmp(str_String, strPrefix.str_String, lenPrefix) != 0)
    return FALSE;
  return TRUE;
}

/* Replace a substring in a string. */
BOOL CTString::ReplaceSubstr(const CTString &strSub, const CTString &strNewSub)
{
  size_t iPos = Find(strSub);

  if (iPos == npos) {
    return FALSE;
  }

  CTString strPart1, strPart2;
  Split((INDEX)iPos, strPart1, strPart2);
  strPart2.RemovePrefix(strSub);

  *this = strPart1+strNewSub+strPart2;

  return TRUE;
}

// [Cecil] Replace specific character in the entire string
void CTString::ReplaceChar(char chOld, char chNew) {
  char *str = str_String;

  while (*str != '\0') {
    if (*str == chOld) {
      *str = chNew;
    }
    ++str;
  }
};

/* Trim the string from left to contain at most given number of characters. */
INDEX CTString::TrimLeft( INDEX ctCharacters)
{
  // clamp negative values
  if( ctCharacters<0) ctCharacters = 0;
  // find how much characters to remove
  INDEX lenOriginal = Length();
  INDEX lenPrefix = lenOriginal-ctCharacters;
  // if nothing needs to be removed
  if( lenPrefix<=0) return 0;
  // crop
  memmove( str_String, &str_String[ lenPrefix], ctCharacters+1);
  ShrinkMemory( (void **)&str_String, ctCharacters+1);
  return lenPrefix;
}

/* Trim the string from right to contain at most given number of characters. */
INDEX CTString::TrimRight( INDEX ctCharacters)
{
  // clamp negative values
  if( ctCharacters<0) ctCharacters = 0;
  // find how much characters to remove
  INDEX lenOriginal = Length();
  INDEX lenPrefix = lenOriginal-ctCharacters;
  // if nothing needs to be removed
  if( lenPrefix<=0) return 0;
  // crop
  str_String[ctCharacters] = '\0';
  ShrinkMemory( (void**)&str_String, ctCharacters+1);
  return lenPrefix;
}


// return naked length of the string (ignoring all decorate codes)
INDEX CTString::LengthNaked(void) const
{
  return Undecorated().Length();
}

// strip decorations from the string
CTString CTString::Undecorated(void) const
{
  // make a copy of the string to hold the result - we will rewrite it without the codes
  CTString strResult = *this;

  // start at the beginning of both strings
  const char *pchSrc = str_String;
  char *pchDst = strResult.str_String;

  // while the source is not finished
  while(pchSrc[0]!=0) {
    // if the source char is not escape char
    if (pchSrc[0]!='^') {
      // copy it over
      *pchDst++ = *pchSrc++;
      // go to next char
      continue;
    }
    // check the next char
    switch(pchSrc[1]) {
      // if one of the control codes, skip corresponding number of characters
      case 'c':  pchSrc += 2+FindZero((UBYTE*)pchSrc+2,6);  break;
      case 'a':  pchSrc += 2+FindZero((UBYTE*)pchSrc+2,2);  break;
      case 'f':  pchSrc += 2+FindZero((UBYTE*)pchSrc+2,1);  break;
      case 'b':  case 'i':  case 'r':  case 'o':
      case 'C':  case 'A':  case 'F':  case 'B':  case 'I':  pchSrc+=2;  break;
      // if it is the escape char again, skip the first escape and copy the char
      case '^':  pchSrc++; *pchDst++ = *pchSrc++; break;
      // if it is something else
      default:
        // just copy over the control char
        *pchDst++ = *pchSrc++;
        break;
    }
  }
  *pchDst++ = 0;
  ASSERT(strResult.Length()<=Length());
  return strResult;
}

BOOL IsSpace(char c)
{
  return c==' ' || c=='\t' || c=='\n' || c=='\r';
}

/* Trim the string from from spaces from left. */
INDEX CTString::TrimSpacesLeft(void)
{
  // for each character in string
  const char *chr;
  for(chr = str_String; *chr!=0; chr++) {
    // if the character is not space 
    if (!IsSpace(*chr)) {
      // stop searching
      break;
    }
  }
  // trim to that character
  return TrimLeft(str_String + Length() - chr);
}

/* Trim the string from from spaces from right. */
INDEX CTString::TrimSpacesRight(void)
{
  // for each character in string reversed
  const char *chr;
  for (chr = str_String + Length() - 1; chr > str_String; chr--) {
    // if the character is not space 
    if (!IsSpace(*chr)) {
      // stop searching
      break;
    }
  }
  // trim to that character
  return TrimRight(chr-str_String+1);
}


// retain only first line of the string
void CTString::OnlyFirstLine(void)
{
  // get position of first line end
  size_t iNL = Find('\n');
  // if none
  if (iNL == CTString::npos) {
    // do nothing
    return;
  }
  // trim everything after that char
  TrimRight((INDEX)iNL);
}

// [Cecil] Resize the character array to fit a specific amount of characters
void CTString::Resize(size_t ct) {
  // Empty string
  if (ct == 0) {
    *this = "";
    return;
  }

  ASSERT(IsValid());

  // Create a new string full of null terminators
  char *strNew = (char *)AllocMemory(ct + 1);
  memset(strNew, '\0', ct + 1);

  // Copy characters from the old string
  strncpy(strNew, str_String, Min(ct, Length()));

  // Reassign to the new string
  FreeMemory(str_String);
  str_String = strNew;
};

// [Cecil] Convert all characters to lowercase
CTString CTString::ToLower(void) const {
  CTString strCopy(*this);
  INDEX i = Length();

  while (--i >= 0) {
    strCopy.Data()[i] = tolower(strCopy[i]);
  }

  return strCopy;
};

// [Cecil] Convert all characters to uppercase
CTString CTString::ToUpper(void) const {
  CTString strCopy(*this);
  INDEX i = Length();

  while (--i >= 0) {
    strCopy.Data()[i] = toupper(strCopy[i]);
  }

  return strCopy;
};

// [Cecil] Inline implementation of bit rotation
static __forceinline ULONG rotl_inline(ULONG ul, int bits) {
  return (ul << bits) | (ul >> (-bits & 31));
};

/* Calculate hashing value for the string. */
ULONG CTString::GetHash(void) const
{
  ULONG ulKey = 0;
  INDEX len = Length();

  for(INDEX i=0; i<len; i++) {
    ulKey = rotl_inline(ulKey, 4) + toupper(str_String[i]);
  }
  return ulKey;
}

/*
 * Throw exception
 */
void CTString::Throw_t(void)
{
  throw(str_String);
}


/*
 * Read from stream.
 */
CTStream &operator>>(CTStream &strmStream, CTString &strString)
{
  ASSERT(strString.IsValid());

  // read length
  INDEX iLength;
  strmStream>>iLength;
  ASSERT(iLength>=0);
  // allocate that much memory
  FreeMemory(strString.str_String);
  strString.str_String = (char *) AllocMemory(iLength+1);  // take end-marker in account
  // if the string is not empty
  if (iLength>0) {
    // read string
    strmStream.Read_t( strString.str_String, iLength);  // without end-marker
  }
  // set end-marker
  strString.str_String[iLength] = 0;

  return strmStream;
}

// [Cecil] Read filename flag
void CTString::ReadFromText_t(CTStream &strmStream, const CTString &strKeyword, BOOL bFileName)
{
  ASSERT(IsValid());

  // keyword must be present
  strmStream.ExpectKeyword_t(strKeyword);

  // [Cecil] Expect dependency keyword before the filename
  if (bFileName) {
    char strTag[] = "_FNM "; strTag[0] = 'T'; // must create tag at run-time!
    strmStream.ExpectKeyword_t(strTag);
  }

  // read the string from the file
  strmStream.GetLine_t(*this);
}

/*
 * Write to stream.
 */
CTStream &operator<<(CTStream &strmStream, const CTString &strString)
{
  ASSERT(strString.IsValid());

  // calculate size
  INDEX iStringLen = strString.Length();
  // write size
  strmStream<<iStringLen;
  // if the string is not empty
  if (iStringLen>0) {
    // write string
    strmStream.Write_t(strString.str_String, iStringLen); // without end-marker
  }

  return strmStream;
}


#ifndef NDEBUG
/*
 * Check if string data is valid.
 */
BOOL CTString::IsValid(void) const
{
  ASSERT(str_String != NULL);
  return TRUE;
}
#endif // NDEBUG


/* Load an entire text file into a string. */
void CTString::ReadUntilEOF_t(CTStream &strmFile)  // throw char *
{
  // get the file size
  SLONG slFileSize = strmFile.GetStreamSize()-strmFile.GetPos_t();

  // allocate that much memory
  FreeMemory(str_String);
  str_String = (char *) AllocMemory(slFileSize+1);  // take end-marker in account
  // read the entire file there
  if (slFileSize>0) {
    strmFile.Read_t( str_String, slFileSize);
  }
  // add end marker
  str_String[slFileSize] = 0;

  // rewrite entire string
  char *pchRead=str_String;
  char *pchWrite=str_String;
  while(*pchRead!=0) {
    // skip the '\r' characters
    if (*pchRead!='\r') {
      *pchWrite++ = *pchRead++;
    } else {
      pchRead++;
    }
  }
  *pchWrite = 0;
}


void CTString::Load_t(const CTString &fnmFile)  // throw char *
{
  ASSERT(IsValid());

  // open the file for reading
  CTFileStream strmFile;
  strmFile.Open_t(fnmFile);

  // read string until end of file
  ReadUntilEOF_t(strmFile);
}


void CTString::LoadKeepCRLF_t(const CTString &fnmFile)  // throw char *
{
  ASSERT(IsValid());

  // open the file for reading
  CTFileStream strmFile;
  strmFile.Open_t(fnmFile);
  // get the file size
  SLONG slFileSize = strmFile.GetStreamSize();

  // allocate that much memory
  FreeMemory(str_String);
  str_String = (char *) AllocMemory(slFileSize+1);  // take end-marker in account
  // read the entire file there
  if (slFileSize>0) {
    strmFile.Read_t( str_String, slFileSize);
  }
  // add end marker
  str_String[slFileSize] = 0;
}

/* Save an entire string into a text file. */
void CTString::Save_t(const CTString &fnmFile)  // throw char *
{
  // open the file for writing
  CTFileStream strmFile;
  strmFile.Create_t(fnmFile);
  // save the string to the file
  strmFile.PutString_t(str_String);
}
void CTString::SaveKeepCRLF_t(const CTString &fnmFile)  // throw char *
{
  // open the file for writing
  CTFileStream strmFile;
  strmFile.Create_t(fnmFile);
  // save the string to the file
  if (Length() > 0) {
    strmFile.Write_t(str_String, Length());
  }
}

#if !SE1_EXF_VERIFY_VA_IN_PRINTF

// Print formatted to a string
INDEX CTString::PrintF(const char *strFormat, ...)
{
  va_list arg;
  va_start(arg, strFormat);
  return VPrintF(strFormat, arg);
}

#endif // SE1_EXF_VERIFY_VA_IN_PRINTF

INDEX CTString::VPrintF(const char *strFormat, va_list arg)
{
  // [Cecil] 4 times more
  static const ULONG ulAddSize = 1024;

  // [Cecil] Local variables instead of static
  INDEX ctBufferSize = ulAddSize;
  char *pchBuffer = (char *)AllocMemory(ulAddSize);

  INDEX iLen;

  FOREVER {
    // print to the buffer
    iLen = _vsnprintf(pchBuffer, ctBufferSize, strFormat, arg);
    // if printed ok
    if (iLen!=-1) {
      // stop
      break;
    }
    // increase the buffer size
    ctBufferSize += ulAddSize;
    GrowMemory((void **)&pchBuffer, ctBufferSize);
  }

  (*this) = pchBuffer;

  // [Cecil] Free local buffer memory
  FreeMemory(pchBuffer);

  return iLen;
}

// [Cecil] Prioritize old compiler
#if SE1_OLD_COMPILER || SE1_USE_ASM

static void *psscanf = &sscanf;

// Scan formatted from a string
__declspec(naked) INDEX CTString::ScanF(const char *strFormat, ...) const
{
  __asm {
    push    eax
    mov     eax,dword ptr [esp+8]
    mov     eax,dword ptr [eax]
    mov     dword ptr [esp+8], eax
    pop     eax
    jmp     dword ptr [psscanf]
  }
}

#else

// [Cecil] Workaround for supporting x64 compilers before VS2013
#if SE1_INCOMPLETE_CPP11
  static __forceinline int vsscanf(const char *str, const char *format, va_list ap)
  {
    const int maxArgs = 20;
    void *a[maxArgs];

    for (int i = 0; i < maxArgs; i++) {
      a[i] = va_arg(ap, void *);
    }

    return sscanf(str, format, a[0], a[1], a[2], a[3], a[4], a[5], a[6], a[7], a[8], a[9],
                  a[10], a[11], a[12], a[13], a[14], a[15], a[16], a[17], a[18], a[19]);
  }
#endif

// Scan formatted from a string
INDEX CTString::ScanF(const char *strFormat, ...) const
{
  va_list arg;
  va_start(arg, strFormat);

  INDEX iResult = vsscanf(str_String, strFormat, arg);
  va_end(arg);

  return iResult;
}

#endif


  // split string in two strings at specified position (char AT splitting position goes to str2)
void CTString::Split( INDEX iPos, CTString &str1, CTString &str2) const
{
  str1 = str_String;
  str2 = str_String;
  str1.TrimRight(iPos);
  str2.TrimLeft(str2.Length() - iPos);
}


// insert one character in string at specified pos
void CTString::InsertChar( INDEX iPos, char cChr)
{
  // clamp position
  INDEX ctChars = Length();
  if( iPos>ctChars) iPos=ctChars;
  else if( iPos<0)  iPos=0;
  // grow memory used by string
  GrowMemory( (void**)&str_String, ctChars+2);
  // copy part of string to make room for char to insert
  memmove( &str_String[iPos+1], &str_String[iPos], ctChars+1-iPos);
  str_String[iPos] = cChr;
}


// delete one character from string at specified pos
void CTString::DeleteChar( INDEX iPos)
{
  // clamp position
  INDEX ctChars = Length();
  if (ctChars==0) {
    return;
  }
  if( iPos>ctChars) iPos=ctChars;
  else if( iPos<0)  iPos=0;
  // copy part of string
  memmove( &str_String[iPos], &str_String[iPos+1], ctChars-iPos+1);
  // shrink memory used by string over deleted char
  ShrinkMemory( (void**)&str_String, ctChars);
}



// wild card comparison
BOOL CTString::Matches(const CTString &strOther) const
{
  return Matches(strOther.str_String);
}


BOOL CTString::Matches(const char *strOther) const
{
// pattern matching code from sourceforge.net codesnippet archive
// adjusted a bit to match in/out parameters
#define        MAX_CALLS        200
  int        calls=0, wild=0, q=0;
  const char *mask=strOther, *name=str_String;
  const char *m=mask, *n=name, *ma=mask, *na=name;
  
  for(;;) {
    if (++calls > MAX_CALLS) {
      return FALSE;
    }
    if (*m == '*') {
      while (*m == '*') ++m;
      wild = 1;
      ma = m;
      na = n;
    }
    
    if (!*m) {
      if (!*n) {
        return TRUE;
      }
      
      for (--m; (m > mask) && (*m == '?'); --m) ;
      
      if ((*m == '*') && (m > mask) &&
        (m[-1] != '\\')) {
        return TRUE;
      }
      if (!wild) {
        return FALSE;
      }
      m = ma;
    } else if (!*n) {
      while(*m == '*') ++m;
      if (*m != 0) {
        return FALSE;
      } else {
        return TRUE;
      }
    }
    if ((*m == '\\') && ((m[1] == '*') || (m[1] == '?'))) {
      ++m;
      q = 1;
    } else {
      q = 0;
    }
    
    if ((tolower(static_cast<UBYTE>(*m)) != tolower(static_cast<UBYTE>(*n))) && ((*m != '?') || q)) {
      if (!wild) {
        return FALSE;
      }
      m = ma;
      n = ++na;
    } else {
      if (*m) ++m;
        if (*n) ++n;
    }
  }
}


// variable management functions
void CTString::LoadVar(const CTString &fnmFile)
{
  try {
    CTString str;
    str.Load_t(fnmFile);
    *this = str;
  } catch (char *strError) {
    CPrintF(TRANS("Cannot load variable from '%s':\n%s\n"), fnmFile.ConstData(), strError);
  }
}

void CTString::SaveVar(const CTString &fnmFile)
{
  try {
    Save_t(fnmFile);
  } catch (char *strError) {
    CPrintF(TRANS("Cannot save variable to '%s':\n%s\n"), fnmFile.ConstData(), strError);
  }
}


// general variable functions
void LoadStringVar(const CTFileName &fnmVar, CTString &strVar)
{
  strVar.LoadVar(fnmVar);
}

void SaveStringVar(const CTFileName &fnmVar, CTString &strVar)
{
  strVar.SaveVar(fnmVar);
}


void LoadIntVar(const CTFileName &fnmVar, INDEX &iVar)
{
  CTString strVar;
  strVar.LoadVar(fnmVar);
  if (strVar!="") {
    CTString strHex = strVar;
    if (strHex.RemovePrefix("0x")) {
      strHex.ScanF("%x", &iVar);
    } else {
      strVar.ScanF("%d", &iVar);
    }
  }
}

void SaveIntVar(const CTFileName &fnmVar, INDEX &iVar)
{
  CTString strVar;
  strVar.PrintF("%d", iVar);
  strVar.SaveVar(fnmVar);
}



// remove special codes from string
CTString RemoveSpecialCodes( const CTString &str)
{
  CTString strRet=str;
  char *pcSrc = strRet.Data();
  char *pcDst = strRet.Data();
  // copy char inside string skipping special codes
  while( *pcSrc != 0)
  {
    if( *pcSrc != '^')
    { // advance to next char
      *pcDst = *pcSrc;
      pcSrc++;
      pcDst++;
    }
    else
    { // skip some characters 
      pcSrc++;
      switch( *pcSrc) {
      case 'c':  pcSrc+=FindZero((UBYTE*)pcSrc,7);  continue;
      case 'a':  pcSrc+=FindZero((UBYTE*)pcSrc,3);  continue;
      case 'f':  pcSrc+=FindZero((UBYTE*)pcSrc,2);  continue;
      case 'b':  case 'i':  case 'r':  case 'o':
      case 'C':  case 'A':  case 'F':  case 'B':  case 'I':  pcSrc+=1;  continue;
      // if we get here this means that ^ or an unrecognized special code was specified
      default: continue;
      }
    }
  }
  // terminate string
  *pcDst = 0;
  return strRet;
}

// [Cecil] Erase a portion of characters from the middle of the string
CTString &CTString::Erase(size_t iFrom, size_t ct) {
  // Nothing to erase
  if (ct == 0) return *this;

  // Erase past the length
  const size_t iLen = Length();
  if (iFrom >= iLen) return *this;

  // Simple cutoff if past the end
  if (ct > iLen - iFrom) {
    str_String[iFrom] = '\0';
    return *this;
  }

  // Move the rest of the string to the current position, including null terminator
  char *pchStart = str_String + iFrom;
  size_t ctRest = (iLen - ct - iFrom) + 1;

  memmove(pchStart, pchStart + ct, ctRest);
  return *this;
};

// [Cecil] Find substring in a string
const char *CTString::GetSubstr(const char *strSub, size_t iFrom) const {
  ASSERT(iFrom <= Length());

  // Inline implementation of strstr()
  const size_t ct = strlen(strSub);
  if (ct == 0) return NULL; // Empty substring

  const char *pch = str_String + iFrom;

  while (*pch != '\0') {
    // Case-insensitive check
    if (!strnicmp(pch, strSub, ct)) {
      return pch;
    }
    ++pch;
  }

  return NULL;
};

// [Cecil] Find character in a string
char *CTString::GetChar(char ch, size_t iFrom) const {
  ASSERT(iFrom <= Length());

  // Inline implementation of strchr()
  char *pch = str_String + iFrom;

  while (*pch != '\0') {
    if (*pch == ch) {
      return pch;
    }
    ++pch;
  }

  return NULL;
};

// [Cecil] Find substring position in a string
size_t CTString::Find(const char *strSub, size_t iFrom) const {
  const char *str = GetSubstr(strSub, iFrom);
  if (str == NULL) return npos;

  return str - str_String;
};

// [Cecil] Find character position in a string
size_t CTString::Find(char ch, size_t iFrom) const {
  const char *pch = GetChar(ch, iFrom);
  if (pch == NULL) return npos;

  return pch - str_String;
};

// [Cecil] Find last substring position in a string
size_t CTString::RFind(const char *strSub, size_t iFrom) const {
  const size_t ct = strlen(strSub);
  if (ct == 0) return npos; // Empty substring

  const size_t iLen = Length();
  if (iLen < ct) return npos; // Substring wouldn't fit

  iFrom = ClampUp(iFrom, iLen - ct);

  do {
    // Case-insensitive check
    if (!strnicmp(str_String + iFrom, strSub, ct)) {
      return iFrom;
    }
  } while (iFrom-- != 0);

  return npos;
};

// [Cecil] Find last character position in a string
size_t CTString::RFind(char ch, size_t iFrom) const {
  const size_t iLen = Length();
  if (iLen == 0) return npos;

  iFrom = ClampUp(iFrom, iLen - 1);

  do {
    if (str_String[iFrom] == ch) {
      return iFrom;
    }
  } while (iFrom-- != 0);

  return npos;
};

// [Cecil] Find first character equal to one of the characters
size_t CTString::FindFirstOf(const char *str, size_t iFrom) const {
  // No characters in either string
  if (*str == '\0') return npos;

  const size_t iLen = Length();
  if (iLen == 0) return npos;

  char *pch = str_String + ClampUp(iFrom, iLen);

  while (*pch != '\0') {
    // Check every character
    const char *pchCheck = str;

    while (*pchCheck != '\0') {
      if (*pch == *pchCheck) {
        return pch - str_String;
      }
      ++pchCheck;
    }
    ++pch;
  }

  return npos;
};

// [Cecil] Find first character equal to none of the characters
size_t CTString::FindFirstNotOf(const char *str, size_t iFrom) const {
  // No characters in either string
  if (*str == '\0') return npos;

  const size_t iLen = Length();
  if (iLen == 0) return npos;

  char *pch = str_String + ClampUp(iFrom, iLen);

  while (*pch != '\0') {
    // Check every character
    BOOL bFound = FALSE;
    const char *pchCheck = str;

    while (*pchCheck != '\0') {
      if (*pch == *pchCheck) {
        bFound = TRUE;
        break;
      }
      ++pchCheck;
    }

    // None found
    if (!bFound) return pch - str_String;

    ++pch;
  }

  return npos;
};

// [Cecil] Find last character equal to one of the characters
size_t CTString::FindLastOf(const char *str, size_t iFrom) const {
  const size_t iLen = Length();
  if (iLen == 0) return npos;

  iFrom = ClampUp(iFrom, iLen - 1);

  do {
    // Check every character
    const char *pchCheck = str;

    while (*pchCheck != '\0') {
      if (str_String[iFrom] == *pchCheck) {
        return iFrom;
      }
      ++pchCheck;
    }
  } while (iFrom-- != 0);

  return npos;
};

// [Cecil] Find last character equal to none of the characters
size_t CTString::FindLastNotOf(const char *str, size_t iFrom) const {
  const size_t iLen = Length();
  if (iLen == 0) return npos;

  iFrom = ClampUp(iFrom, iLen - 1);

  do {
    BOOL bFound = FALSE;

    // Check every character
    const char *pchCheck = str;

    while (*pchCheck != '\0') {
      if (str_String[iFrom] == *pchCheck) {
        bFound = TRUE;
        break;
      }

      ++pchCheck;
    }

    // None found
    if (!bFound) return iFrom;

  } while (iFrom-- != 0);

  return npos;
};
