#ifndef SE_INCL_CTSTRING_INL
#define SE_INCL_CTSTRING_INL
#ifdef PRAGMA_ONCE
  #pragma once
#endif

#include <Engine/Base/Memory.h>

/*
 * Default constructor.
 */
ENGINE_API CTString::CTString(void)
{
  str_String = StringDuplicate("");
}

/*
 * Copy constructor.
 */
ENGINE_API CTString::CTString(const CTString &strOriginal)
{
  ASSERT(strOriginal.IsValid());

  // make string duplicate
  str_String = StringDuplicate(strOriginal.str_String);
}

// [Cecil] Constructor from character string with optional offset and amount of characters
ENGINE_API CTString::CTString(const char *strCharString, size_t iFrom, size_t ct)
{
  ASSERT(strCharString!=NULL);

  // make string duplicate
  str_String = StringDuplicate(strCharString + iFrom);

  if (ct < (size_t)Length()) {
    str_String[ct] = '\0';
  }
}

#if !SE1_EXF_VERIFY_VA_IN_PRINTF

/* Constructor with formatting. */
ENGINE_API CTString::CTString(INDEX iDummy, const char *strFormat, ...)
{
  str_String = StringDuplicate("");
  va_list arg;
  va_start(arg, strFormat);
  VPrintF(strFormat, arg);
}

#endif // SE1_EXF_VERIFY_VA_IN_PRINTF

/*
 * Destructor.
 */
ENGINE_API CTString::~CTString()
{
  // check that it is valid
  ASSERT(IsValid());
  // free memory
  FreeMemory(str_String);
}

/*
 * Clear the object.
 */
ENGINE_API void CTString::Clear(void)
{
  operator=("");
}

/*
 * Assignment.
 */
ENGINE_API CTString &CTString::operator=(const char *strCharString)
{
  ASSERT(IsValid());
  ASSERT(strCharString!=NULL);

  /* The other string must be copied _before_ this memory is freed, since it could be same
     pointer!
   */
  // make a copy of character string
  char *strCopy = StringDuplicate(strCharString);
  // empty this string
  FreeMemory(str_String);
  // assign it the copy of the character string
  str_String = strCopy;

  return *this;
}
ENGINE_API CTString &CTString::operator=(const CTString &strOther)
{
  ASSERT(IsValid());
  ASSERT(strOther.IsValid());

  /* The other string must be copied _before_ this memory is freed, since it could be same
     pointer!
   */
  // make a copy of character string
  char *strCopy = StringDuplicate(strOther.str_String);
  // empty this string
  FreeMemory(str_String);
  // assign it the copy of the character string
  str_String = strCopy;

  return *this;
}

// [Cecil] Assign a new string consisting of a specific character
ENGINE_API void CTString::Fill(size_t ct, char ch) {
  ASSERT(IsValid());

  // Recreate the string array
  FreeMemory(str_String);
  str_String = (char *)AllocMemory(ct + 1);

  // Set every character and add a null terminator
  memset(str_String, ch, ct);
  str_String[ct] = '\0';
};

#endif  /* include-once check. */

