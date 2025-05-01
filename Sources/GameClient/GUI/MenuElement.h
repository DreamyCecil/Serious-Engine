/* Copyright (c) 2025 Dreamy Cecil
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

#ifndef CECIL_INCL_MENUELEMENT_H
#define CECIL_INCL_MENUELEMENT_H

#ifdef PRAGMA_ONCE
  #pragma once
#endif

#include <Engine/Base/LinkedNode.h>

// Abstract base for any menu element (an entire menu or one of its gadgets)
class CAbstractMenuElement : public CLinkedNode {
  protected:
    // Dummy string value for returning from specific methods
    static const CTString me_strDummyString;

  public:
    // Destructor for derived elements
    virtual ~CAbstractMenuElement() {};

    // Get parent menu element that owns this element
    inline CAbstractMenuElement *GetParentElement(void) const {
      return (CAbstractMenuElement *)GetParent();
    };

    // Check if this element is an entire menu
    virtual bool IsMenu(void) const = 0;
};

#endif
