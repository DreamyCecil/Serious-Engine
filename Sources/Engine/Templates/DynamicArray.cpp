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

#ifndef SE_INCL_DYNAMICARRAY_CPP
#define SE_INCL_DYNAMICARRAY_CPP

#ifdef PRAGMA_ONCE
  #pragma once
#endif

#include <Engine/Base/Memory.h>
#include <Engine/Base/ListIterator.inl>

#include <Engine/Templates/DynamicArray.h>

// Default constructor
template<class Type>
CDynamicArray<Type>::CDynamicArray(void) {
#if CHECKARRAYLOCKING
  // Not locked
  da_LockCt = 0;
#endif
  // Set to an empty array of pointers
  da_Pointers = NULL;
  da_Count = 0;
};

// Copy constructor
template<class Type>
CDynamicArray<Type>::CDynamicArray(const CDynamicArray<Type> &daOriginal)
{
#if CHECKARRAYLOCKING
  // Not locked
  da_LockCt = 0;
#endif
  // Set to an empty array of pointers
  da_Pointers = NULL;
  da_Count = 0;

  // Call assignment operator
  (*this) = daOriginal;
};

// Destructor
template<class Type>
CDynamicArray<Type>::~CDynamicArray(void) {
  Clear();
};

// Destroy all objects and reset the array to the initial (empty) state
template<class Type>
void CDynamicArray<Type>::Clear(void) {
  ASSERT(this != NULL);

  // If any pointers are allocated
  if (da_Count != 0) {
    // Destroy each object manually because array deletion does not call object destructors
    for (INDEX iPointer = 0; iPointer < da_Count; iPointer++) {
      ::Clear(*da_Pointers[iPointer]);
    }

    // Free the pointers
    FreeMemory(da_Pointers);

    // Mark as freed
    da_Pointers = NULL;
    da_Count = 0;

  // Otherwise make sure that the pointers are not allocated
  } else {
    ASSERT(da_Pointers == NULL);
  }

  // For all memory blocks
  FORDELETELIST(CDABlockInfo, bi_ListNode, da_BlocksList, itBlock) {
    // Free memory used by the block
    delete[] (Type *)itBlock->bi_Memory;

    // Free memory used by the block info
    delete &itBlock.Current();
  }
};

// Allocate a new memory block
template<class Type>
Type *CDynamicArray<Type>::AllocBlock(INDEX iCount) {
  ASSERT(this != NULL && iCount > 0);

  Type *ptBlock;
  CDABlockInfo *pbi;

  // Allocate memory and call constructors for all members
  ptBlock = new Type[iCount + 1]; // (+1 for cache-prefetch opt)

  // Allocate the block info and add it to the list
  pbi = new CDABlockInfo;
  da_BlocksList.AddTail(pbi->bi_ListNode);

  // Remember block memory
  pbi->bi_Memory = ptBlock;

  return ptBlock;
};

// Grow pointer array by a given number of objects
template<class Type>
void CDynamicArray<Type>::GrowPointers(INDEX iCount) {
  ASSERT(this != NULL && iCount > 0);

  // If not yet allocated
  if (da_Count == 0) {
    // Make sure that the pointers are not allocated
    ASSERT(da_Pointers == NULL);

    // Allocate new memory
    da_Count = iCount;
    da_Pointers = (Type **)AllocMemory(da_Count * sizeof(Type *));

  // Otherwise grow to a new size
  } else {
    da_Count += iCount;
    GrowMemory((void **)&da_Pointers, da_Count * sizeof(Type *));
  }
};

// Shrink pointer array by a given number of objects
template<class Type>
void CDynamicArray<Type>::ShrinkPointers(INDEX iCount) {
  ASSERT(this != NULL && iCount > 0);

  // Make sure that the pointers are allocated
  ASSERT(da_Pointers != NULL);

  // Decrement by some amount
  da_Count -= iCount;

  // Make sure that it hasn't gone below zero
  ASSERT(da_Count >= 0);

  // If all pointers are freed
  if (da_Count == 0) {
    // Free the array
    FreeMemory(da_Pointers);
    da_Pointers = NULL;

  // If some remain
  } else {
    // Shrink to a new size
    ShrinkMemory((void **)&da_Pointers, da_Count * sizeof(Type *));
  }
};

// Create a given number of new objects
template<class Type>
Type *CDynamicArray<Type>::New(INDEX iCount) {
  ASSERT(this != NULL && iCount >= 0);

  // No new members are needed
  if (iCount == 0) {
    return NULL;
  }

  Type *ptBlock;
  INDEX iOldCount = da_Count;

  // Grow the pointer table
  GrowPointers(iCount);

  // Allocate the memory block
  ptBlock = AllocBlock(iCount);

  // Set pointers
  for (INDEX iNewMember = 0; iNewMember < iCount; iNewMember++) {
    da_Pointers[iOldCount + iNewMember] = ptBlock + iNewMember;
  }

  return ptBlock;
};

// Delete a given object
template<class Type>
void CDynamicArray<Type>::Delete(Type *ptObject) {
  ASSERT(this != NULL);

#if CHECKARRAYLOCKING
  // Check if unlocked
  ASSERT(da_LockCt == 0);
#endif

  // Clear the object
  ::Clear(*ptObject);

  INDEX iMember = GetIndex(ptObject);

  // [Cecil] Not found
  if (iMember == -1) {
    ASSERT(FALSE);
    return;
  }

  // Move last pointer here
  da_Pointers[iMember] = da_Pointers[da_Count - 1];

  // Shrink pointers by one without freeing previous memory
  ShrinkPointers(1);
};

// Get pointer to an object from its index
template<class Type>
Type *CDynamicArray<Type>::Pointer(INDEX iObject) {
  ASSERT(this != NULL);

  // Check if the index is currently valid
  ASSERT(iObject >= 0 && iObject < da_Count);

#if CHECKARRAYLOCKING
  // Check if locked
  ASSERT(da_LockCt > 0);
#endif

  return da_Pointers[iObject];
};

// Get constant pointer to an object from its index
template<class Type>
const Type *CDynamicArray<Type>::Pointer(INDEX iObject) const {
  ASSERT(this != NULL);

  // Check if the index is currently valid
  ASSERT(iObject >= 0 && iObject < da_Count);

#if CHECKARRAYLOCKING
  // Check if locked
  ASSERT(da_LockCt > 0);
#endif

  return da_Pointers[iObject];
};

// Random access operator
template<class Type>
Type &CDynamicArray<Type>::operator[](INDEX iObject) {
  return *Pointer(iObject); 
};

// Constant random access operator
template<class Type>
const Type &CDynamicArray<Type>::operator[](INDEX iObject) const {
  return *Pointer(iObject); 
};

// Lock for getting indices
template<class Type>
void CDynamicArray<Type>::Lock(void) {
  ASSERT(this != NULL);

#if CHECKARRAYLOCKING
  ASSERT(da_LockCt >= 0);

  // Increment lock counter
  da_LockCt++;
#endif
};

// Unlock after getting indices
template<class Type>
void CDynamicArray<Type>::Unlock(void) {
  ASSERT(this != NULL);

#if CHECKARRAYLOCKING
  // Decrement lock counter
  da_LockCt--;

  ASSERT(da_LockCt >= 0);
#endif
};

// Get index of an object from its pointer
template<class Type>
INDEX CDynamicArray<Type>::Index(Type *ptMember) {
  ASSERT(this != NULL);

#if CHECKARRAYLOCKING
  // Check if locked
  ASSERT(da_LockCt > 0);
#endif

  return GetIndex(ptMember);
};

// Get index of an object from its pointer without locking
template<class Type>
INDEX CDynamicArray<Type>::GetIndex(Type *ptMember) {
  ASSERT(this != NULL);

  // Check all members (slow!)
  for (INDEX iMember = 0; iMember < da_Count; iMember++) {
    if (da_Pointers[iMember] == ptMember) {
      return iMember;
    }
  }

  ASSERTALWAYS("CDynamicArray<>::Index(): Not a member of this array!");
  return -1; // [Cecil] Invalid index
};

// Get number of elements in the array
template<class Type>
INDEX CDynamicArray<Type>::Count(void) const {
  ASSERT(this != NULL);
  return da_Count;
};

// Assignment operator
template<class Type>
CDynamicArray<Type> &CDynamicArray<Type>::operator=(const CDynamicArray<Type> &arOriginal)
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
    return *this;
  }

  // Create that many elements
  Type *atNew = New(ctOriginal);

  // [Cecil] FIXME: Can't remember why I designed these arrays the way that required addition of "operator[] const" and "Pointer() const"
  CDynamicArray<Type> &arNonConst = const_cast<CDynamicArray<Type> &>(arOriginal);

  // Copy the elements
  arNonConst.Lock();

  for (INDEX iNew = 0; iNew < ctOriginal; iNew++) {
    atNew[iNew] = arNonConst[iNew];
  }

  arNonConst.Unlock();

  return *this;
};

// Move all elements of another array into this one
template<class Type>
void CDynamicArray<Type>::MoveArray(CDynamicArray<Type> &arOther)
{
  ASSERT(this != NULL && &arOther != NULL);

#if CHECKARRAYLOCKING
  // Check if unlocked
  ASSERT(da_LockCt == 0 && arOther.da_LockCt == 0);
#endif

  // The other array has no elements
  if (arOther.da_Count == 0) {
    return;
  }

  // Remember number of elements
  INDEX iOldCount = da_Count;

  // Grow pointer array to add the pointers to elements of other array
  GrowPointers(arOther.da_Count);

  // For each pointer in the other array
  for (INDEX iOtherPointer = 0; iOtherPointer < arOther.da_Count; iOtherPointer++) {
    // Copy it to the end of this array
    da_Pointers[iOldCount + iOtherPointer] = arOther.da_Pointers[iOtherPointer];
  }

  // Remove array of pointers in the other array
  arOther.ShrinkPointers(arOther.da_Count);

  // Move list of allocated blocks from the other array to the end of this one
  da_BlocksList.MoveList(arOther.da_BlocksList);
};

// CDynamicArrayIterator

// Template class for iterating a dynamic array
template<class Type>
class CDynamicArrayIterator {
  private:
    INDEX dai_Index; // Index of the current element
    CDynamicArray<Type> &dai_Array; // Reference to the array

  public:
    // Constructor for given array
    inline CDynamicArrayIterator(CDynamicArray<Type> &da);

    // Destructor
    inline ~CDynamicArrayIterator(void);

    // Move to the next object
    inline void MoveToNext(void);

    // Check if finished
    inline BOOL IsPastEnd(void);

    // Get current element
    Type &Current(void)    { return *dai_Array.Pointer(dai_Index); };
    Type &operator*(void)  { return *dai_Array.Pointer(dai_Index); };
    operator Type *(void)  { return  dai_Array.Pointer(dai_Index); };
    Type *operator->(void) { return  dai_Array.Pointer(dai_Index); };

    // [Cecil] Get current index
    inline INDEX GetIndex(void) {
      return dai_Index;
    };
};

// Constructor for given array
template<class Type>
inline CDynamicArrayIterator<Type>::CDynamicArrayIterator(CDynamicArray<Type> &da) :
  dai_Array(da)
{
  // Lock indices
  dai_Array.Lock();
  dai_Index = 0;
};

// Destructor
template<class Type>
inline CDynamicArrayIterator<Type>::~CDynamicArrayIterator(void) {
  // Unlock indices
  dai_Array.Unlock();
  dai_Index = -1;
};

// Move to the next object
template<class Type>
inline void CDynamicArrayIterator<Type>::MoveToNext(void) {
  ASSERT(this != NULL);
  dai_Index++;
};

// Check if finished
template<class Type>
inline BOOL CDynamicArrayIterator<Type>::IsPastEnd(void) {
  ASSERT(this != NULL);
  return dai_Index >= dai_Array.Count();
};

// Iterate the whole dynamic array
// NOTE: The iterator defined by this macro must be destroyed before adding/removing
// elements in the array. To do so, embed the for loop in additional curly braces.
#define FOREACHINDYNAMICARRAY(array, type, iter) \
  for (CDynamicArrayIterator<type> iter(array); !iter.IsPastEnd(); iter.MoveToNext())

#endif // include-once check
