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

#ifndef SE_INCL_CTSTRING_H
#define SE_INCL_CTSTRING_H
#ifdef PRAGMA_ONCE
  #pragma once
#endif

#include <Engine/Base/Types.h>
#include <Engine/Base/Assert.h>

#if SE1_EXF_VERIFY_VA_IN_PRINTF

// [Cecil] Special structure for verifying types of variadic arguments
struct VATypeVerifier
{
  static void Bail(void) {
    extern void CPutString(const char *);
    ASSERTALWAYS("Do not compile the engine with SE1_EXF_VERIFY_VA_IN_PRINTF enabled!!!");
    CPutString  ("Do not compile the engine with SE1_EXF_VERIFY_VA_IN_PRINTF enabled!!!\n");
    exit(EXIT_FAILURE);
  };

  #define EXF_VA_TYPEFUNC(_Type) inline VATypeVerifier(_Type) { Bail(); }

  // String printing
  EXF_VA_TYPEFUNC(const char *);
  EXF_VA_TYPEFUNC(char *);
  EXF_VA_TYPEFUNC(const UBYTE *);
  EXF_VA_TYPEFUNC(UBYTE *);
  // Number printing
  EXF_VA_TYPEFUNC(int);
  EXF_VA_TYPEFUNC(UINT);
  EXF_VA_TYPEFUNC(FLOAT);
  EXF_VA_TYPEFUNC(DOUBLE);
  EXF_VA_TYPEFUNC(UBYTE);
  EXF_VA_TYPEFUNC(SBYTE);
  EXF_VA_TYPEFUNC(UWORD);
  EXF_VA_TYPEFUNC(SWORD);
  EXF_VA_TYPEFUNC(ULONG);
  EXF_VA_TYPEFUNC(SLONG);
  EXF_VA_TYPEFUNC(UQUAD);
  EXF_VA_TYPEFUNC(SQUAD);
  // Miscellaneous printing
  EXF_VA_TYPEFUNC(const class CEntityPointer &);
  EXF_VA_TYPEFUNC(const CEntity *);
  EXF_VA_TYPEFUNC(const class CChunkID &);

  VATypeVerifier(const CTString &) = delete;
  VATypeVerifier(const VATypeVerifier &) = delete;
};

// Define dummy functions for verifying variadic arguments for printing (without return type)
#define EXF_VERIFY_VA_FUNC(_FuncName) \
  inline \
  void _FuncName(const char *str)                    { VATypeVerifier::Bail(); }; \
  template<class T> inline \
  void _FuncName(const char *str, T a)               { VATypeVerifier::Bail(); VATypeVerifier v(a); }; \
  template<class T, class ...Args> inline \
  void _FuncName(const char *str, T a, Args... args) { VATypeVerifier::Bail(); VATypeVerifier v(a); _FuncName(str, args...); };

// Define dummy functions for verifying variadic arguments for printing (with return type)
#define EXF_VERIFY_VA_FUNC_RETURN(_Return, _FuncName) \
  inline \
  _Return _FuncName(const char *str)                    { VATypeVerifier::Bail(); return _Return(); }; \
  template<class T> inline \
  _Return _FuncName(const char *str, T a)               { VATypeVerifier::Bail(); VATypeVerifier v(a); return _Return(); }; \
  template<class T, class ...Args> inline \
  _Return _FuncName(const char *str, T a, Args... args) { VATypeVerifier::Bail(); VATypeVerifier v(a); return _FuncName(str, args...); };

#endif // SE1_EXF_VERIFY_VA_IN_PRINTF

/*
 * Main string class
 */
class ENGINE_API CTString {
public:
  char *str_String;         // pointer to memory holding the character string

  static const size_t npos; // [Cecil] Invalid character index

public:
  /* Default constructor. */
  inline CTString(void);
  /* Copy constructor. */
  inline CTString(const CTString &strOriginal);
  // [Cecil] Constructor from character string with optional offset and amount of characters
  inline CTString(const char *strCharString, size_t iFrom = 0, size_t ct = npos);

#if !SE1_EXF_VERIFY_VA_IN_PRINTF
  /* Constructor with formatting. */
  inline CTString(INDEX iDummy, const char *strFormat, ...) SE1_FORMAT_FUNC(3, 4);

#else
  // [Cecil] See 'SE1_EXF_VERIFY_VA_IN_PRINTF' definition; custom definition because of the custom constructor
  inline
  CTString(INDEX iDummy, const char *str)                    { VATypeVerifier::Bail(); };
  template<class T> inline
  CTString(INDEX iDummy, const char *str, T a)               { VATypeVerifier::Bail(); VATypeVerifier v(a); };
  template<class T, class ...Args> inline
  CTString(INDEX iDummy, const char *str, T a, Args... args) { VATypeVerifier::Bail(); VATypeVerifier v(a); PrintF(str, args...); };
#endif

  /* Destructor. */
  inline ~CTString();
  /* Clear the object. */
  inline void Clear(void);

  /* Conversion into character string. */
  inline operator const char*() const {
    ASSERT(IsValid());
    return str_String;
  };

  /* Assignment. */
  inline CTString &operator=(const char *strCharString);
  inline CTString &operator=(const CTString &strOther);

  /* Check if string data is valid. */
  BOOL IsValid(void) const;

  // return length of the string
  inline INDEX Length(void) const { return (INDEX)strlen(str_String); };
  INDEX LengthNaked(void) const;

  // strip decorations from the string
  CTString Undecorated(void) const;

  /* Replace a substring in a string. */
  BOOL ReplaceSubstr(const CTString &strSub, const CTString &strNewSub);

  // [Cecil] Replace specific character in the entire string
  void ReplaceChar(char chOld, char chNew);

  /* Check if has given prefix */
  BOOL HasPrefix( const CTString &strPrefix) const;
  /* Remove given prefix string from this string */
  BOOL RemovePrefix( const CTString &strPrefix);
  /* Trim the string to contain at most given number of characters. */
  INDEX TrimLeft(  INDEX ctCharacters);
  INDEX TrimRight( INDEX ctCharacters);
  /* Trim the string from spaces. */
  INDEX TrimSpacesLeft(void);
  INDEX TrimSpacesRight(void);
  /* Calcuate hashing value for the string. */
  ULONG GetHash(void) const;
  // retain only first line of the string
  void OnlyFirstLine(void);

  /* Equality comparison. */
  BOOL operator==(const CTString &strOther) const;
  BOOL operator==(const char *strOther) const;
  ENGINE_API friend BOOL operator==(const char *strThis, const CTString &strOther);
  /* Inequality comparison. */
  BOOL operator!=(const CTString &strOther) const;
  BOOL operator!=(const char *strOther) const;
  ENGINE_API friend BOOL operator!=(const char *strThis, const CTString &strOther);
  // wild card comparison (other string may contain wildcards)
  BOOL Matches(const CTString &strOther) const;
  BOOL Matches(const char *strOther) const;

  /* String concatenation. */
  CTString operator+(const CTString &strSecond) const;
  CTString &operator+=(const CTString &strSecond);
  ENGINE_API friend CTString operator+(const char *strFirst, const CTString &strSecond);

  // split string in two strings at specified position (char AT splitting position goes to str2)
  void Split( INDEX iPos, CTString &str1, CTString &str2) const;
  void InsertChar( INDEX iPos, char cChr); // insert char at position
  void DeleteChar( INDEX iPos); // delete char at position
  
  /* Throw exception */
  void Throw_t(void);

  /* Read from stream. */
  ENGINE_API friend CTStream &operator>>(CTStream &strmStream, CTString &strString);
  // [Cecil] Read filename flag
  void ReadFromText_t(CTStream &strmStream, const CTString &strKeyword, BOOL bFileName = FALSE);
  /* Write to stream. */
  ENGINE_API friend CTStream &operator<<(CTStream &strmStream, const CTString &strString);

  /* Load an entire text file into a string. */
  void Load_t(const CTString &fnmFile);  // throw char *
  void LoadKeepCRLF_t(const CTString &fnmFile);  // throw char *
  void ReadUntilEOF_t(CTStream &strmStream);  // throw char *
  /* Save an entire string into a text file. */
  void Save_t(const CTString &fnmFile);  // throw char *
  void SaveKeepCRLF_t(const CTString &fnmFile);  // throw char *

#if !SE1_EXF_VERIFY_VA_IN_PRINTF
  // Print formatted to a string
  INDEX PrintF(const char *strFormat, ...) SE1_FORMAT_FUNC(2, 3);
#else
  EXF_VERIFY_VA_FUNC_RETURN(INDEX, PrintF); // [Cecil] See 'SE1_EXF_VERIFY_VA_IN_PRINTF' definition
#endif

  INDEX VPrintF(const char *strFormat, va_list arg);
  // Scan formatted from a string
  INDEX ScanF(const char *strFormat, ...) const;

  // variable management functions
  void LoadVar(const CTString &fnmFile);
  void SaveVar(const CTString &fnmFile);

  // [Cecil] Split a string using a character delimiter
  template<typename TypeContainer>
  void CharSplit(const char chDelimiter, TypeContainer &aStrings) const {
    size_t iLast = 0;
    size_t iPos = Find(chDelimiter);

    while (iPos != npos) {
      aStrings.push_back(Substr(iLast, iPos - iLast));

      // Skip delimiter
      iPos += 1;

      // Remember position after the delimiter
      iLast = iPos;

      // Get position of the next one
      iPos = Find(chDelimiter, iPos);
    }

    // Last token
    aStrings.push_back(Substr(iLast));
  };

  // [Cecil] Split a string using a string delimiter
  template<typename TypeContainer>
  void StringSplit(const CTString &strDelimiter, TypeContainer &aStrings) const {
    const size_t iSize = strDelimiter.Length();

    size_t iLast = 0;
    size_t iPos = Find(strDelimiter);

    while (iPos != npos) {
      aStrings.push_back(Substr(iLast, iPos - iLast));

      // Skip delimiter
      iPos += iSize;

      // Remember position after the delimiter
      iLast = iPos;

      // Get position of the next one
      iPos = Find(strDelimiter, iPos);
    }

    // Last token
    aStrings.push_back(Substr(iLast));
  };

// [Cecil] Extra methods
public:

  // Retrieve string data
  inline char *Data(void) {
    return str_String;
  };

  // Retrieve constant string data
  inline const char *ConstData(void) const {
    return str_String;
  };

  // Assign a new string consisting of a specific character
  inline void Fill(size_t ct, char ch);

  // Convert all characters to lowercase
  CTString ToLower(void) const;

  // Convert all characters to uppercase
  CTString ToUpper(void) const;

  // Erase a portion of characters from the middle of the string
  CTString &Erase(size_t iFrom, size_t ct = npos);

  // Extract substring from the string
  __forceinline CTString Substr(size_t iFrom, size_t ct = npos) const {
    return CTString(str_String, iFrom, ct);
  };

// [Cecil] New methods for searching
public:

  // Find substring within the string
  const char *GetSubstr(const char *strSub, size_t iFrom = 0) const;
  inline const char *GetSubstr(const CTString &strSub, size_t iFrom = 0) const {
    return GetSubstr(strSub.ConstData(), iFrom);
  };

  // Find character within the string
  char *GetChar(char ch, size_t iFrom = 0) const;

  // Find substring position in a string
  size_t Find(const char *strSub, size_t iFrom = 0) const;
  inline size_t Find(const CTString &strSub, size_t iFrom = 0) const {
    return Find(strSub.ConstData(), iFrom);
  };

  // Find character position in a string
  size_t Find(char ch, size_t iFrom = 0) const;

  // Deprecated method left for compatibility
  INDEX FindSubstr(const CTString &strSub) {
    return (INDEX)Find(strSub.str_String);
  };

  // Find last substring position in a string
  size_t RFind(const char *strSub, size_t iFrom = npos) const;
  inline size_t RFind(const CTString &strSub, size_t iFrom = npos) const {
    return RFind(strSub.ConstData(), iFrom);
  };

  // Find last character position in a string
  size_t RFind(char ch, size_t iFrom = npos) const;

  // Find first occurrence of any of the characters
  size_t FindFirstOf(const char *str, size_t iFrom = 0) const;

  // Find first absence of any of the characters
  size_t FindFirstNotOf(const char *str, size_t iFrom = 0) const;

  // Find last occurrence of any of the characters
  size_t FindLastOf(const char *str, size_t iFrom = npos) const;

  // Find last absence of any of the characters
  size_t FindLastNotOf(const char *str, size_t iFrom = npos) const;

// [Cecil] New path and filename methods
public:

  // Check if there's a path separator character at some position
  bool PathSeparatorAt(size_t i) const;

  // Remove directory from the filename
  CTString NoDir(void) const;

  // Remove extension from the filename
  CTString NoExt(void) const;

  // Get name of the file
  CTString FileName(void) const;

  // Get path to the file
  CTString FileDir(void) const;

  // Get file extension with the period
  CTString FileExt(void) const;

  // Go up the path until a certain directory
  size_t GoUpUntilDir(CTString strDirName) const;

  // Normalize the path taking "backward" and "current" directories into consideration
  // E.g. "abc/sub1/../sub2/./qwe" -> "abc/sub2/qwe"
  void NormalizePath(void);

  // Get length of the root name, if there's any
  size_t RootNameLength() const;

  // Check if the path starts with a root name (e.g. "C:" or "//abc")
  inline bool HasRootName() const {
    return RootNameLength() != 0;
  };

  // Check if the path starts with a root directory (e.g. "C:/" or "//abc/")
  inline bool HasRootDirectory() const {
    const size_t ctRoot = RootNameLength();
    return (size_t)Length() > ctRoot && PathSeparatorAt(ctRoot);
  };

  // Check if it's a full path
  inline bool IsAbsolute() const {
  #if !SE1_WIN
    return HasRootDirectory();
  #else
    return HasRootName() && HasRootDirectory();
  #endif
  };

  // Check if it's a relative path
  inline bool IsRelative() const {
    return !IsAbsolute();
  };

  // Convert from a relative path to an absolute path and add missing backslashes
  // Returns TRUE if the path becomes absolute as a result
  BOOL SetFullDirectory(void);

// [Cecil] Migrated methods from CTFileName
public:

  // Remove application path from a file name
  BOOL RemoveApplicationPath_t(void);

  // Filename is its own name (used for storing in nametable)
  inline const CTString &GetName(void) const {
    return *this;
  };
};

// [Cecil] Alias for compatibility
typedef CTString CTFileName;

// [Cecil] Migrated macros for defining a literal filename in code (EFNM = EXE filename)
#define CTFILENAME(string) CTString("EFNM" string, 4)
#define DECLARE_CTFILENAME(name, string) CTString name("EFNM" string, 4)

// general variable functions
ENGINE_API void LoadStringVar( const CTFileName &fnmVar, CTString &strVar);
ENGINE_API void SaveStringVar( const CTFileName &fnmVar, CTString &strVar);
ENGINE_API void LoadIntVar(    const CTFileName &fnmVar, INDEX &iVar);
ENGINE_API void SaveIntVar(    const CTFileName &fnmVar, INDEX &iVar);

ENGINE_API CTString RemoveSpecialCodes( const CTString &str);


#include <Engine/Base/CTString.inl>

// [Cecil] Hashing function for compatibility with STL
namespace std {
  template<class Key> struct hash;

  template<> struct hash<CTString> {
    inline size_t operator()(const CTString &str) const SE1_NOEXCEPT {
      return str.GetHash();
    };
  };
};

#endif  /* include-once check. */

