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

#ifndef SE_INCL_STATICSTACKARRAY_H
#define SE_INCL_STATICSTACKARRAY_H

#ifdef PRAGMA_ONCE
  #pragma once
#endif

#include <Engine/Templates/StaticArray.h>

// Template class for stack-like array with static allocation of objects
template<class Type>
class CStaticStackArray : public CStaticArray<Type> {
  public:
    INDEX sa_UsedCount; // Number of used objects in array
    INDEX sa_ctAllocationStep; // How many elements to allocate when stack overflows

  public:
    // Default constructor
    inline CStaticStackArray(void);

    // Copy constructor
    inline CStaticStackArray(const CStaticStackArray<Type> &arOther);

    // Destructor
    inline ~CStaticStackArray(void);

    // Set how many elements to allocate when stack overflows
    inline void SetAllocationStep(INDEX ctStep);

    // Create a given number of objects
    inline void New(INDEX iCount);

    // Destroy all objects
    inline void Delete(void);

    // Destroy all objects and reset the stack to the initial (empty) state
    inline void Clear(void);

  public:
    // Add a new object on top of the stack
    inline Type &Push(void);

    // Add a number of objects on top of the stack
    inline Type *Push(INDEX ct);

    // Add a new object on top of the stack
    inline void Add(const Type &tObject);

    // Remove one object from the top of the stack and return it
    inline Type &Pop(void);

    // Remove objects from the stack with a higher than given index, but keep stack space
    inline void PopUntil(INDEX iNewTop);

    // Remove all objects from stack, but keep stack space
    inline void PopAll(void);

  public:
    // Random access operator
    inline Type &operator[](INDEX iObject);

    // Constant random access operator
    inline const Type &operator[](INDEX iObject) const;

    // Get number of elements in the stack
    INDEX Count(void) const;

    // Get index of an object from its pointer
    INDEX Index(Type *ptObject);

    // Copy all elements of another stack into this one
    void CopyArray(const CStaticStackArray<Type> &arOriginal);

    // Move all elements of another stack into this one
    void MoveArray(CStaticStackArray<Type> &arOther);

    // Assignment operator
    CStaticStackArray<Type> &operator=(const CStaticStackArray<Type> &arOriginal);
};

// [Cecil] Inline definition
#include <Engine/Templates/StaticStackArray.cpp>

#endif // include-once check
