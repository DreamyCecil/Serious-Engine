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

#ifndef SE_INCL_DYNAMICCONTAINER_H
#define SE_INCL_DYNAMICCONTAINER_H

#ifdef PRAGMA_ONCE
  #pragma once
#endif

#include <Engine/Templates/StaticStackArray.h>

// Template class for a container that holds references to objects of some class
template<class Type>
class CDynamicContainer : public CStaticStackArray<Type *> {
  public:
  #if CHECKARRAYLOCKING
    INDEX da_LockCt; // Lock counter for getting indices
  #endif

  public:
    // Default constructor
    CDynamicContainer(void);

    // Copy constructor
    CDynamicContainer(const CDynamicContainer<Type> &coOriginal);

    // Destructor
    ~CDynamicContainer(void);

    // Remove all objects and reset the container to the initial (empty) state
    void Clear(void);

  public:
    // Add a given object to the container
    void Add(Type *ptNewObject);

    // Insert a given object in the container at a specified position
    void Insert(Type *ptNewObject, const INDEX iPos = 0);

    // Remove a given object from the container
    void Remove(Type *ptOldObject);

    // Check if a given object is in the container
    BOOL IsMember(Type *ptOldObject);

    // Get pointer to an object from its index
    Type *Pointer(INDEX iObject);

    // Get constant pointer to an object from its index
    const Type *Pointer(INDEX iObject) const;

    // Random access operator
    inline Type &operator[](INDEX iObject) {
      return *Pointer(iObject);
    };

    // Constant random access operator
    inline const Type &operator[](INDEX iObject) const {
      return *Pointer(iObject);
    };

    // Get first object in container (there must be at least one when calling this)
    Type &GetFirst(void);

  public:
    // Lock for getting indices
    void Lock(void);

    // Unlock after getting indices
    void Unlock(void);

    // Get index of an object from its pointer
    INDEX Index(Type *ptObject);

    // Get index of an object from its pointer without locking
    INDEX GetIndex(Type *ptMember);

    // Assignment operator
    CDynamicContainer<Type> &operator=(const CDynamicContainer<Type> &coOriginal);

    // Move all elements of another container into this one
    void MoveContainer(CDynamicContainer<Type> &coOther);
};

// [Cecil] Inline definition
#include <Engine/Templates/DynamicContainer.cpp>

#endif // include-once check
