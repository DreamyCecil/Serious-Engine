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

#ifndef SE_INCL_STATICARRAY_CPP
#define SE_INCL_STATICARRAY_CPP

#ifdef PRAGMA_ONCE
  #pragma once
#endif

#define FOREACHINSTATICARRAY(array, type, iter) \
  for (CStaticArrayIterator<type> iter(array); !iter.IsPastEnd(); iter.MoveToNext())

#include <Engine/Base/Console.h>
#include <Engine/Templates/StaticArray.h>

// Default constructor
template<class Type>
inline CStaticArray<Type>::CStaticArray(void) {
  sa_Count = 0;
  sa_Array = NULL;
};

// Copy constructor
template<class Type>
inline CStaticArray<Type>::CStaticArray(const CStaticArray<Type> &arOther) {
  CopyArray(arOther);
};

// Destructor
template<class Type>
inline CStaticArray<Type>::~CStaticArray(void) {
  Clear();
};

// Destroy all objects and reset the array to the initial (empty) state
template<class Type>
inline void CStaticArray<Type>::Clear(void) {
  // Destroy allocated objects
  if (sa_Count != 0) {
    Delete();
  }
};

// Create a given number of objects
template<class Type>
inline void CStaticArray<Type>::New(INDEX iCount) {
  ASSERT(this != NULL && iCount >= 0);

  // No new elements
  if (iCount == 0) {
    return;
  }

  ASSERT(sa_Count == 0 && sa_Array == NULL);

  sa_Count = iCount;
  sa_Array = new Type[iCount + 1]; // (+1 for cache-prefetch opt)
};

// Expand array size but keep old objects
template<class Type>
inline void CStaticArray<Type>::Expand(INDEX iNewCount)
{
  ASSERT(this != NULL && iNewCount > sa_Count);

  // Not yet allocated
  if (sa_Count == 0) {
    New(iNewCount);
    return;
  }

  // Already allocated
  ASSERT(sa_Count != 0 && sa_Array != NULL);

  // Allocate a new array with more space
  Type *ptNewArray = new Type[iNewCount + 1]; // (+1 for cache-prefetch opt)

  // Copy old objects
  for (INDEX iOld = 0; iOld < sa_Count; iOld++) {
    ptNewArray[iOld] = sa_Array[iOld];
  }

  // Free the old array
  delete[] sa_Array;

  // Remember the new array
  sa_Count = iNewCount;
  sa_Array = ptNewArray;
};

// Destroy all objects
template<class Type>
inline void CStaticArray<Type>::Delete(void) {
  ASSERT(this != NULL);
  ASSERT(sa_Count != 0 && sa_Array != NULL);

  delete[] sa_Array;
  sa_Count = 0;
  sa_Array = NULL;
};

// Random access operator
template<class Type>
inline Type &CStaticArray<Type>::operator[](INDEX i) {
  ASSERT(this != NULL);
  ASSERT(i >= 0 && i < sa_Count); // Check bounds

  return sa_Array[i];
};

// Constant random access operator
template<class Type>
inline const Type &CStaticArray<Type>::operator[](INDEX i) const {
  ASSERT(this != NULL);
  ASSERT(i >= 0 && i < sa_Count); // Check bounds

  return sa_Array[i];
};

// Get number of elements in the array
template<class Type>
INDEX CStaticArray<Type>::Count(void) const {
  ASSERT(this != NULL);
  return sa_Count;
};

// Get index of an object from its pointer
template<class Type>
INDEX CStaticArray<Type>::Index(Type *ptMember) {
  ASSERT(this != NULL);
  ASSERT(sa_Count > 0);
  ASSERT(UINT_PTR(ptMember) >= UINT_PTR(sa_Array)); // Error if behind the array pointer

  // Find the element
  for (INDEX i = 0; i < sa_Count; i++)
  {
    if (ptMember == &sa_Array[i]) {
      return i;
    }
  }

  ASSERTALWAYS("CStaticArray<>::Index(): Not a member of this array!");
  return 0;
};

// Copy all elements of another array into this one
template<class Type>
void CStaticArray<Type>::CopyArray(const CStaticArray<Type> &arOriginal)
{
  ASSERT(this != NULL);
  ASSERT(&arOriginal != NULL);
  ASSERT(this != &arOriginal);

  // Clear previous contents
  Clear();

  // Get amount of elements in the original array
  INDEX ctOriginal = arOriginal.Count();

  // The other array has no elements
  if (ctOriginal == 0) {
    return;
  }

  // Create that many elements
  New(ctOriginal);

  // Copy the elements
  for (INDEX iNew = 0; iNew < ctOriginal; iNew++) {
    sa_Array[iNew] = arOriginal[iNew];
  }
};

// Move all elements of another array into this one
template<class Type>
void CStaticArray<Type>::MoveArray(CStaticArray<Type> &arOther)
{
  ASSERT(this != NULL);
  ASSERT(&arOther != NULL);
  ASSERT(this != &arOther);

  // Clear previous contents
  Clear();

  // The other array has no elements
  if (arOther.Count() == 0) {
    return;
  }

  // Move data from the other array into this one and clear the other one
  sa_Count = arOther.sa_Count;
  sa_Array = arOther.sa_Array;
  arOther.sa_Count = 0;
  arOther.sa_Array = NULL;
};

// Assignment operator
template<class Type>
inline CStaticArray<Type> &CStaticArray<Type>::operator=(const CStaticArray<Type> &arOriginal) {
  CopyArray(arOriginal);
  return *this;
};

// CStaticArrayIterator

// Template class for iterating the static array
template<class Type>
class CStaticArrayIterator {
  private:
    INDEX sai_Index; // Index of current element
    CStaticArray<Type> &sai_Array; // Reference to the array

  public:
    // Constructor for given array
    inline CStaticArrayIterator(CStaticArray<Type> &sa);

    // Destructor
    inline ~CStaticArrayIterator(void);

    // Move to next object
    inline void MoveToNext(void);

    // Check if finished
    inline BOOL IsPastEnd(void);

    // Get current element
    Type &Current(void)    { return  sai_Array[sai_Index]; };
    Type &operator*(void)  { return  sai_Array[sai_Index]; };
    operator Type *(void)  { return &sai_Array[sai_Index]; };
    Type *operator->(void) { return &sai_Array[sai_Index]; };

    // [Cecil] Get current index
    inline INDEX GetIndex(void) {
      return sai_Index;
    };
};

// Constructor for given array
template<class Type>
inline CStaticArrayIterator<Type>::CStaticArrayIterator(CStaticArray<Type> &sa) :
  sai_Array(sa)
{
  sai_Index = 0;
};

// Destructor
template<class Type>
inline CStaticArrayIterator<Type>::~CStaticArrayIterator(void)
{
  sai_Index = -1;
};

// Move to next object
template<class Type>
inline void CStaticArrayIterator<Type>::MoveToNext(void)
{
  ASSERT(this != NULL);
  sai_Index++;
};

// Check if finished
template<class Type>
inline BOOL CStaticArrayIterator<Type>::IsPastEnd(void)
{
  ASSERT(this != NULL);
  return sai_Index >= sai_Array.sa_Count;
};

#endif // include-once check
