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

#ifndef SE_INCL_STATICSTACKARRAY_CPP
#define SE_INCL_STATICSTACKARRAY_CPP

#ifdef PRAGMA_ONCE
  #pragma once
#endif

#include <Engine/Templates/StaticStackArray.h>
#include <Engine/Templates/StaticArray.cpp>

// Default constructor
template<class Type>
inline CStaticStackArray<Type>::CStaticStackArray(void) : CStaticArray<Type>() {
  sa_UsedCount = 0;
  sa_ctAllocationStep = 256;
};

// Copy constructor
template<class Type>
inline CStaticStackArray<Type>::CStaticStackArray(const CStaticStackArray<Type> &arOther) {
  CopyArray(arOther);
};

// Destructor
template<class Type>
inline CStaticStackArray<Type>::~CStaticStackArray(void)
{
};

// Destroy all objects and reset the stack to the initial (empty) state
template<class Type>
inline void CStaticStackArray<Type>::Clear(void) {
  if (CStaticArray<Type>::Count() != 0) {
    Delete();
  }
};

// Set how many elements to allocate when stack overflows
template<class Type>
inline void CStaticStackArray<Type>::SetAllocationStep(INDEX ctStep) 
{
  ASSERT(ctStep > 0);
  sa_ctAllocationStep = ctStep;
};

// Create a given number of objects
template<class Type>
inline void CStaticStackArray<Type>::New(INDEX iCount) {
  CStaticArray<Type>::New(iCount);
  sa_UsedCount = 0;
};

// Destroy all objects
template<class Type>
inline void CStaticStackArray<Type>::Delete(void) {
  CStaticArray<Type>::Delete();
  sa_UsedCount = 0;
};

// Add a new object on top of the stack
template<class Type>
inline Type &CStaticStackArray<Type>::Push(void) {
  sa_UsedCount++;

  if (sa_UsedCount > CStaticArray<Type>::Count()) {
    this->Expand(CStaticArray<Type>::Count() + sa_ctAllocationStep);
  }

  ASSERT(sa_UsedCount <= CStaticArray<Type>::Count());
  return CStaticArray<Type>::operator[](sa_UsedCount - 1);
};

// Add a number of objects on top of the stack
template<class Type>
inline Type *CStaticStackArray<Type>::Push(INDEX ct) {
  sa_UsedCount += ct;

  if (sa_UsedCount > CStaticArray<Type>::Count()) {
    ASSERT(ct > 0);

    const INDEX ctAllocSteps = (ct - 1) / sa_ctAllocationStep + 1;
    this->Expand(CStaticArray<Type>::Count() + sa_ctAllocationStep * ctAllocSteps);
  }

  ASSERT(sa_UsedCount <= CStaticArray<Type>::Count());
  return &CStaticArray<Type>::operator[](sa_UsedCount - ct);
};

// Add a new object on top of the stack
template<class Type>
inline void CStaticStackArray<Type>::Add(const Type &tObject) {
  sa_UsedCount++;

  if (sa_UsedCount > CStaticArray<Type>::Count()) {
    this->Expand(CStaticArray<Type>::Count() + sa_ctAllocationStep);
  }

  ASSERT(sa_UsedCount <= CStaticArray<Type>::Count());
  CStaticArray<Type>::operator[](sa_UsedCount - 1) = tObject;
};

// Remove one object from the top of the stack and return it
template<class Type>
inline Type &CStaticStackArray<Type>::Pop(void)
{
  ASSERT(sa_UsedCount > 0);
  sa_UsedCount--;

  return CStaticArray<Type>::operator[](sa_UsedCount);
};

// Remove objects from the stack with a higher than given index, but keep stack space
template<class Type>
inline void CStaticStackArray<Type>::PopUntil(INDEX iNewTop)
{
  ASSERT(iNewTop < sa_UsedCount);
  sa_UsedCount = iNewTop + 1;
};

// Remove all objects from stack, but keep stack space
template<class Type>
inline void CStaticStackArray<Type>::PopAll(void) {
  sa_UsedCount = 0;
};

// Random access operator
template<class Type>
inline Type &CStaticStackArray<Type>::operator[](INDEX i) {
  ASSERT(this != NULL);
  ASSERT(i < sa_UsedCount); // Check bounds

  return CStaticArray<Type>::operator[](i);
};

// Constant random access operator
template<class Type>
inline const Type &CStaticStackArray<Type>::operator[](INDEX i) const {
  ASSERT(this != NULL);
  ASSERT(i < sa_UsedCount); // Check bounds

  return CStaticArray<Type>::operator[](i);
};

// Get number of elements in the stack
template<class Type>
INDEX CStaticStackArray<Type>::Count(void) const {
  ASSERT(this != NULL);
  return sa_UsedCount;
};

// Get index of an object from its pointer
template<class Type>
INDEX CStaticStackArray<Type>::Index(Type *ptMember) {
  ASSERT(this != NULL);

  INDEX i = CStaticArray<Type>::Index(ptMember);
  ASSERTMSG(i < sa_UsedCount, "CStaticStackArray<>::Index(): Not a member of this array!");

  return i;
};

// Copy all elements of another stack into this one
template<class Type>
void CStaticStackArray<Type>::CopyArray(const CStaticStackArray<Type> &arOriginal)
{
  ASSERT(this != NULL);
  ASSERT(&arOriginal != NULL);
  ASSERT(this != &arOriginal);

  // Copy stack array and used count
  CStaticArray<Type>::CopyArray(arOriginal);
  sa_UsedCount = arOriginal.sa_UsedCount;
};

// Move all elements of another array into this one
template<class Type>
void CStaticStackArray<Type>::MoveArray(CStaticStackArray<Type> &arOther)
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
  CStaticArray<Type>::MoveArray(arOther);
  sa_UsedCount = arOther.sa_UsedCount;
  sa_ctAllocationStep = arOther.sa_ctAllocationStep;
  arOther.sa_UsedCount = 0;
};

// Assignment operator
template<class Type>
inline CStaticStackArray<Type> &CStaticStackArray<Type>::operator=(const CStaticStackArray<Type> &arOriginal) {
  CopyArray(arOriginal);
  return *this;
};

#endif // include-once check
