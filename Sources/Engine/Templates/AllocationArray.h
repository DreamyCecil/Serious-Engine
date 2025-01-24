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

#ifndef SE_INCL_ALLOCATIONARRAY_H
#define SE_INCL_ALLOCATIONARRAY_H

#ifdef PRAGMA_ONCE
  #pragma once
#endif

#include <Engine/Templates/StaticStackArray.h>

// Template class for stack-like array with static allocation of objects
template<class Type>
class CAllocationArray : public CStaticArray<Type> {
  public:
    CStaticStackArray<INDEX> aa_aiFreeElements; // array of indices of free elements
    INDEX aa_ctAllocationStep;  // how many elements to allocate when pool overflows

  public:
    // Default constructor
    inline CAllocationArray(void);

    // Copy constructor
    inline CAllocationArray(const CAllocationArray<Type> &aaOriginal);

    // Destructor
    inline ~CAllocationArray(void);

    // Destroy all objects and reset the array to the initial (empty) state
    inline void Clear(void);

    // Set how many elements to allocate when pool overflows
    inline void SetAllocationStep(INDEX ctStep);

  public:
    // Create a given number of objects - do not use
    inline void New(INDEX iCount);

    // Destroy all objects - do not use
    inline void Delete(void);

    // Preallocate to fixate memory usage
    void PreAllocate(INDEX ctCount);

    // Allocate a new object
    inline INDEX Allocate(void);

    // Free object with given index
    inline void Free(INDEX iToFree);

    // Free all objects but keep pool space
    inline void FreeAll(void);

    // Check if an index is allocated (slow!)
    inline BOOL IsAllocated(INDEX i);

  public:
    // Random access operator
    inline Type &operator[](INDEX iObject);

    // Constant random access operator
    inline const Type &operator[](INDEX iObject) const;

    // Get number of allocated objects in array
    INDEX Count(void) const;

    // Get index of an object from its pointer
    INDEX Index(Type *ptObject);

    // Assignment operator
    CAllocationArray<Type> &operator=(const CAllocationArray<Type> &aaOriginal);
};

// [Cecil] Inline definition
#include <Engine/Templates/AllocationArray.cpp>

#endif // include-once check
