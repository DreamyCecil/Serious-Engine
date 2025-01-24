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

#ifndef SE_INCL_ALLOCATIONARRAY_CPP
#define SE_INCL_ALLOCATIONARRAY_CPP

#ifdef PRAGMA_ONCE
  #pragma once
#endif

#include <Engine/Templates/AllocationArray.h>
#include <Engine/Templates/StaticStackArray.cpp>
#include <Engine/Templates/StaticArray.cpp>

extern BOOL _bAllocationArrayParanoiaCheck;

// Default constructor
template<class Type>
inline CAllocationArray<Type>::CAllocationArray(void) :
  CStaticArray<Type>(), aa_aiFreeElements()
{
  aa_ctAllocationStep = 256;
};

// Copy constructor
template<class Type>
inline CAllocationArray<Type>::CAllocationArray(const CAllocationArray<Type> &aaOriginal)
{
  // Call assignment operator
  (*this) = aaOriginal;
};

// Destructor
template<class Type>
inline CAllocationArray<Type>::~CAllocationArray(void)
{
};

// Destroy all objects and reset the array to the initial (empty) state
template<class Type>
inline void CAllocationArray<Type>::Clear(void) {
  // Delete objects themselves
  CStaticArray<Type>::Clear();

  // Clear the array of free indices
  aa_aiFreeElements.Clear();
};

// Set how many elements to allocate when pool overflows
template<class Type>
inline void CAllocationArray<Type>::SetAllocationStep(INDEX ctStep) 
{
  ASSERT(ctStep > 0);
  aa_ctAllocationStep = ctStep;
};

// Create a given number of objects - do not use
template<class Type>
inline void CAllocationArray<Type>::New(INDEX iCount) {
  // Never call this!
  ASSERT(FALSE);
};

// Destroy all objects - do not use
template<class Type>
inline void CAllocationArray<Type>::Delete(void) {
  // Never call this!
  ASSERT(FALSE);
};

// Preallocate to fixate memory usage
template<class Type>
void CAllocationArray<Type>::PreAllocate(INDEX ctCount)
{
  // Must be empty when calling this
  ASSERT(aa_aiFreeElements.Count() == 0 && CStaticArray<Type>::Count() == 0);

  // Expand the array to that size
  this->Expand(ctCount);

  // Fill all elements as free
  INDEX *piNewFree = aa_aiFreeElements.Push(ctCount);

  // Fill them up
  for (INDEX iNew = 0; iNew < ctCount; iNew++) {
    piNewFree[iNew] = iNew;
  }
};

// Allocate a new object
template<class Type>
inline INDEX CAllocationArray<Type>::Allocate(void)
{
  // If there are no more free indices
  if (aa_aiFreeElements.Count() == 0) {
    // Remember old size
    INDEX ctOldSize = CStaticArray<Type>::Count();

    // Expand the array by the allocation step
    this->Expand(ctOldSize + aa_ctAllocationStep);

    // Create new free indices
    INDEX *piNewFree = aa_aiFreeElements.Push(aa_ctAllocationStep);

    // Fill them up
    for (INDEX iNew = 0; iNew < aa_ctAllocationStep; iNew++) {
      piNewFree[iNew] = ctOldSize + iNew;
    }
  }

  // Pop one free index from the top of stack, and use that one
  return aa_aiFreeElements.Pop();
};

// Free object with given index
template<class Type>
inline void CAllocationArray<Type>::Free(INDEX iToFree)
{
#ifndef NDEBUG
  // Must be within pool limits
  ASSERT(iToFree >= 0 && iToFree < CStaticArray<Type>::Count());

  // Must not be free
  if (_bAllocationArrayParanoiaCheck) {
    ASSERT(IsAllocated(iToFree));
  }
#endif

  // Push its index on top of the free stack
  aa_aiFreeElements.Push() = iToFree;
};

// Free all objects but keep pool space
template<class Type>
inline void CAllocationArray<Type>::FreeAll(void)
{
  // Clear the free array
  aa_aiFreeElements.PopAll();

  // Push as many free elements as there is pool space
  INDEX ctSize = CStaticArray<Type>::Count();
  INDEX *piNewFree = aa_aiFreeElements.Push(ctSize);

  // Fill them up
  for (INDEX iNew = 0; iNew < ctSize; iNew++) {
    piNewFree[iNew] = iNew;
  }
};

// Check if an index is allocated (slow!)
template<class Type>
inline BOOL CAllocationArray<Type>::IsAllocated(INDEX i)
{
  // Must be within pool limits
  ASSERT(i >= 0 && i < CStaticArray<Type>::Count());

  // For each free index
  INDEX ctFree = aa_aiFreeElements.Count();

  for (INDEX iFree = 0; iFree < ctFree; iFree++) {
    // If it is that one
    if (aa_aiFreeElements[iFree] == i) {
      // It is not allocated
      return FALSE;
    }
  }

  // If not found as free, it is allocated
  return TRUE;
};

// Random access operator
template<class Type>
inline Type &CAllocationArray<Type>::operator[](INDEX iObject)
{
#ifndef NDEBUG
  ASSERT(this != NULL);

  // Must be within pool limits
  ASSERT(iObject >= 0 && iObject < CStaticArray<Type>::Count());

  // Must not be free
  if (_bAllocationArrayParanoiaCheck) {
    ASSERT(IsAllocated(iObject));
  }
#endif

  return CStaticArray<Type>::operator[](iObject);
};

// Constant random access operator
template<class Type>
inline const Type &CAllocationArray<Type>::operator[](INDEX iObject) const
{
#ifndef NDEBUG
  ASSERT(this != NULL);

  // Must be within pool limits
  ASSERT(iObject >= 0 && iObject < CStaticArray<Type>::Count());

  // Must not be free
  if (_bAllocationArrayParanoiaCheck) {
    ASSERT(IsAllocated(iObject));
  }
#endif

  return CStaticArray<Type>::operator[](iObject);
};

// Get number of allocated objects in array
template<class Type>
INDEX CAllocationArray<Type>::Count(void) const
{
  ASSERT(this != NULL);

  // It is pool size without the count of free elements
  return CStaticArray<Type>::Count() - aa_aiFreeElements.Count();
};

// Get index of an object from its pointer
template<class Type>
INDEX CAllocationArray<Type>::Index(Type *ptObject)
{
  ASSERT(this != NULL);

  INDEX i = CStaticArray<Type>::Index(ptObject);
  ASSERT(IsAllocated(i));

  return i;
};

// Assignment operator
template<class Type>
CAllocationArray<Type> &CAllocationArray<Type>::operator=(const CAllocationArray<Type> &aaOriginal)
{
  ASSERT(this != NULL);

  CStaticArray<Type>::operator=(aaOriginal);

  aa_aiFreeElements = aaOriginal.aa_aiFreeElements;
  aa_ctAllocationStep = aaOriginal.aa_ctAllocationStep;
};

#endif // include-once check
