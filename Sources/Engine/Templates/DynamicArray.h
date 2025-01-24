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

#ifndef SE_INCL_DYNAMICARRAY_H
#define SE_INCL_DYNAMICARRAY_H

#ifdef PRAGMA_ONCE
  #pragma once
#endif

#include <Engine/Base/Lists.h>

// One block in a dynamic array
class CDABlockInfo {
  public:
    CListNode bi_ListNode;
    void *bi_Memory; // Block memory
};

// Template class for array with dynamic allocation of objects
template<class Type>
class CDynamicArray {
  public:
    CListHead da_BlocksList; // List of allocated memory blocks
    Type **da_Pointers; // Array of object pointers
    INDEX da_Count; // Number of objects

  #if CHECKARRAYLOCKING
    INDEX da_LockCt; // Lock counter for getting indices
  #endif

  public:
    // Default constructor
    CDynamicArray(void);

    // Copy constructor
    CDynamicArray(const CDynamicArray<Type> &daOriginal);

    // Destructor
    ~CDynamicArray(void);

    // Destroy all objects and reset the array to the initial (empty) state
    void Clear(void);

    // Allocate a new memory block
    Type *AllocBlock(INDEX iCount);

    // Grow pointer array by a given number of objects
    void GrowPointers(INDEX iCount);

    // Shrink pointer array by a given number of objects
    void ShrinkPointers(INDEX iCount);

  public:
    // Create a given number of new objects
    Type *New(INDEX iCount = 1);

    // Destroy a given object
    void Delete(Type *ptObject);

    // Get pointer to an object from its index
    Type *Pointer(INDEX iObject);

    // Get constant pointer to an object from its index
    const Type *Pointer(INDEX iObject) const;

    // Random access operator
    inline Type &operator[](INDEX iObject);

    // Constant random access operator
    inline const Type &operator[](INDEX iObject) const;

  public:
    // Lock for getting indices
    void Lock(void);

    // Unlock after getting indices
    void Unlock(void);

    // Get index of an object from its pointer
    INDEX Index(Type *ptObject);

    // Get index of an object from its pointer without locking
    INDEX GetIndex(Type *ptMember);

    // Get number of objects in the array
    INDEX Count(void) const;

    // Assignment operator
    CDynamicArray<Type> &operator=(const CDynamicArray<Type> &arOriginal);

    // Move all elements of another array into this one
    void MoveArray(CDynamicArray<Type> &arOther);
};

// [Cecil] Inline definition
#include <Engine/Templates/DynamicArray.cpp>

#endif // include-once check
