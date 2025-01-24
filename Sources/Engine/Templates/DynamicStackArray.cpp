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

#ifndef SE_INCL_DYNAMICSTACKARRAY_CPP
#define SE_INCL_DYNAMICSTACKARRAY_CPP

#ifdef PRAGMA_ONCE
  #pragma once
#endif

#include <Engine/Templates/DynamicStackArray.h>
#include <Engine/Templates/DynamicArray.cpp>

// Default constructor
template<class Type>
inline CDynamicStackArray<Type>::CDynamicStackArray(void) : CDynamicArray<Type>() {
  da_ctUsed = 0;
  da_ctAllocationStep = 256;

  // Lock the array upon construction
  CDynamicArray<Type>::Lock();
};

// Copy constructor
template<class Type>
inline CDynamicStackArray<Type>::CDynamicStackArray(const CDynamicStackArray<Type> &arOriginal) {
  da_ctUsed = 0;
  da_ctAllocationStep = 256;

  // Copy array contents
  (*this) = arOriginal;

  // Lock the array upon construction
  CDynamicArray<Type>::Lock();
};

// Destructor
template<class Type>
inline CDynamicStackArray<Type>::~CDynamicStackArray(void) {
  // Unlock the array upon destruction
  CDynamicArray<Type>::Unlock();
};

// Set how many elements to allocate when stack overflows
template<class Type>
inline void CDynamicStackArray<Type>::SetAllocationStep(INDEX ctStep) 
{
  ASSERT(ctStep > 0);
  da_ctAllocationStep = ctStep;
};

// Destroy all objects and reset the stack to the initial (empty) state
template<class Type>
inline void CDynamicStackArray<Type>::Clear(void) {
  CDynamicArray<Type>::Clear();
  da_ctUsed = 0; 
};

// Add new object on top of the stack
template<class Type>
inline Type &CDynamicStackArray<Type>::Push(void) {
  // If there's no space for the new element
  if (CDynamicArray<Type>::Count() - da_ctUsed < 1) {
    // Allocate a new block
    CDynamicArray<Type>::New(da_ctAllocationStep);
  }

  // Increase used amount
  da_ctUsed++;
  ASSERT(da_ctUsed <= CDynamicArray<Type>::Count());

  // Get new element
  return CDynamicArray<Type>::operator[](da_ctUsed - 1);
};

// Add a number of new objects on top of the stack
template<class Type>
inline Type *CDynamicStackArray<Type>::Push(INDEX ct) {
  // If there's no space for new elements
  while (CDynamicArray<Type>::Count() - da_ctUsed < ct) {
    // Allocate new blocks
    CDynamicArray<Type>::New(da_ctAllocationStep);
  }

  // Increase used amount
  da_ctUsed += ct;
  ASSERT(da_ctUsed <= CDynamicArray<Type>::Count());

  // Get new elements
  return &CDynamicArray<Type>::operator[](da_ctUsed - ct);
};

// Remove all objects from stack, but keep stack space
template<class Type>
inline void CDynamicStackArray<Type>::PopAll(void) {
  // If there is only one block allocated
  if (this->da_BlocksList.IsEmpty() || &this->da_BlocksList.Head() == &this->da_BlocksList.Tail()) {
    // Just clear the counter
    da_ctUsed = 0;
    return;
  }

  // Remember how much was allocated
  INDEX ctUsedBefore = CDynamicArray<Type>::Count();

  // Free all memory
  CDynamicArray<Type>::Clear();

  // Allocate one big block
  CDynamicArray<Type>::New(ctUsedBefore);
  da_ctUsed = 0;
};

// Remove last object from the stack
template<class Type>
void CDynamicStackArray<Type>::Pop(void) {
  // [Cecil] Decrease used amount and clear the last object
  da_ctUsed--;

  Type *ptLast = &CDynamicArray<Type>::operator[](da_ctUsed);
  ::Clear(ptLast);
};

// Delete a given object
template<class Type>
void CDynamicStackArray<Type>::Delete(Type *ptObject) {
  // [Cecil] Inline code of CDynamicArray::Delete()
  ASSERT(this != NULL);

#if CHECKARRAYLOCKING
  // Check if unlocked
  ASSERT(da_LockCt == 0);
#endif

  // Clear the object
  ::Clear(*ptObject);

  INDEX iMember = this->GetIndex(ptObject);

  // [Cecil] Not found
  if (iMember == -1) {
    ASSERT(FALSE);
    return;
  }

  // [Cecil] Decrease used amount instead of shrinking pointers
  da_ctUsed--;

  // [Cecil] Move last used pointer here
  this->da_Pointers[iMember] = this->da_Pointers[da_ctUsed];
};

// Random access operator
template<class Type>
inline Type &CDynamicStackArray<Type>::operator[](INDEX i) {
  ASSERT(this != NULL);
  ASSERT(i < da_ctUsed); // Check bounds

  return CDynamicArray<Type>::operator[](i);
};

// Constant random access operator
template<class Type>
inline const Type &CDynamicStackArray<Type>::operator[](INDEX i) const {
  ASSERT(this != NULL);
  ASSERT(i < da_ctUsed); // Check bounds

  return CDynamicArray<Type>::operator[](i);
};

// Get number of used elements in the stack
template<class Type>
INDEX CDynamicStackArray<Type>::Count(void) const {
  ASSERT(this != NULL);
  return da_ctUsed;
};

// Get index of a member from its pointer
template<class Type>
INDEX CDynamicStackArray<Type>::Index(Type *ptMember) {
  ASSERT(this != NULL);

  INDEX i = CDynamicArray<Type>::Index(ptMember);
  ASSERTMSG(i < da_ctUsed, "CDynamicStackArray<>::Index(): Not a member of this array!");

  return i;
};

// Get array of pointers to elements (used for sorting elements by sorting pointers)
template<class Type>
Type **CDynamicStackArray<Type>::GetArrayOfPointers(void)
{
  return this->da_Pointers;
};

// Assignment operator
template<class Type>
CDynamicStackArray<Type> &CDynamicStackArray<Type>::operator=(const CDynamicStackArray<Type> &arOriginal)
{
  ASSERT(this != NULL);
  ASSERT(&arOriginal != NULL);
  ASSERT(this != &arOriginal);

  // Copy stack arrays
  CDynamicArray<Type>::operator=(arOriginal);

  // Copy used count
  da_ctUsed = arOriginal.da_ctUsed;

  return *this;
};

#endif // include-once check
