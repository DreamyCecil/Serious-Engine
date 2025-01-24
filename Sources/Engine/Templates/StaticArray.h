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

#ifndef SE_INCL_STATICARRAY_H
#define SE_INCL_STATICARRAY_H

#ifdef PRAGMA_ONCE
  #pragma once
#endif

// Template class for array with static allocation of objects
template<class Type>
class CStaticArray {
  public:
    INDEX sa_Count; // Number of objects in the array
    Type *sa_Array; // Object array

  public:
    // Default constructor
    inline CStaticArray(void);

    // Copy constructor
    inline CStaticArray(const CStaticArray<Type> &arOther);

    // Destructor
    inline ~CStaticArray(void);

    // Destroy all objects and reset the array to the initial (empty) state
    inline void Clear(void);

    // Create a given number of objects
    inline void New(INDEX iCount);

    // Expand array size but keep old objects
    inline void Expand(INDEX iNewCount);

    // Destroy all objects
    inline void Delete(void);

  public:
    // Random access operator
    inline Type &operator[](INDEX iObject);

    // Constant random access operator
    inline const Type &operator[](INDEX iObject) const;

    // Get number of objects in the array
    INDEX Count(void) const;

    // Get index of an object from its pointer
    INDEX Index(Type *ptObject);

    // Copy all elements of another array into this one
    void CopyArray(const CStaticArray<Type> &arOriginal);

    // Move all elements of another array into this one
    void MoveArray(CStaticArray<Type> &arOther);

    // Assignment operator
    CStaticArray<Type> &operator=(const CStaticArray<Type> &arOriginal);
};

// [Cecil] Inline definition
#include <Engine/Templates/StaticArray.cpp>

#endif // include-once check
