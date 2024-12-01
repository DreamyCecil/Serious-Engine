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

#ifndef SE_INCL_ENTITYPOINTER_H
#define SE_INCL_ENTITYPOINTER_H
#ifdef PRAGMA_ONCE
  #pragma once
#endif

// [Cecil] Moved inline definitions of all methods here, so it depends on the definiton of the entity class now
#include <Engine/Entities/Entity.h>

/*
 * Smart pointer to entity objects, does the book-keeping for reference counting.
 */
class CEntityPointer {
public:
  CEntity *ep_pen;  // the pointer itself

public:
  // Constructors
  inline CEntityPointer(void) : ep_pen(NULL) {};

  inline CEntityPointer(const CEntityPointer &penOther) : ep_pen(penOther.ep_pen) {
    if (ep_pen != NULL) ep_pen->AddReference();
  };

  inline CEntityPointer(CEntity *pen) : ep_pen(pen) {
    if (ep_pen != NULL) ep_pen->AddReference();
  };

  // Destructor
  inline ~CEntityPointer(void) {
    if (ep_pen != NULL) ep_pen->RemReference();
  };

  // Assignment operators
  inline const CEntityPointer &operator=(CEntity *pen) {
    // Have to add the new entity and only then remove the old one
    if (pen != NULL) pen->AddReference();
    if (ep_pen != NULL) ep_pen->RemReference();
    ep_pen = pen;
    return *this;
  };

  inline const CEntityPointer &operator=(const CEntityPointer &penOther) {
    return operator=(penOther.ep_pen);
  };

  // Casting operators
  inline CEntity *operator->(void) const { return ep_pen; };
  inline operator CEntity *(void) const { return ep_pen; };
  inline CEntity &operator*(void) const { return *ep_pen; };
};

#endif  /* include-once check. */
