/* Copyright (c) 2022-2025 Dreamy Cecil
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

#ifndef SE_INCL_LINKEDNODE_H
#define SE_INCL_LINKEDNODE_H

#ifdef PRAGMA_ONCE
  #pragma once
#endif

#include <Engine/Base/Lists.h>
#include <Engine/Base/ListIterator.inl>

// Classed derived from this node can have other nodes as children or be added to other parent nodes
class CLinkedNode : protected CListNode {
  protected:
    CListHead m_lhChildren; // Nodes attached to this one
    CLinkedNode *m_pParent; // Parent node class

  public:
    // Constructor
    CLinkedNode() : m_pParent(NULL)
    {
    };

    // Get list of child nodes
    inline const CListHead &GetChildren(void) const {
      return m_lhChildren;
    };

    // Get parent class from the list node
    inline CLinkedNode *GetParent(void) const {
      return m_pParent;
    };

    // Add new child node
    void AddChild(CLinkedNode *pChild) {
      // No child or already parented
      if (pChild == NULL || pChild->GetParent() == this) {
        return;
      }

      pChild->SetParent(this);
    };

    // Set new parent of this node
    void SetParent(CLinkedNode *pParent) {
      // Unparent from existing parent
      if (IsLinked()) {
        Remove();
      }

      // No new parent
      if (pParent == NULL) {
        m_pParent = NULL;
        return;
      }

      // Go through parents of a new parent
      for (CLinkedNode *pCheck = pParent; pCheck != NULL; pCheck = pCheck->GetParent())
      {
        // Abort if encountered this node, which shouldn't be a parent
        if (pCheck == this) {
          ASSERT(FALSE);
          return;
        }
      }

      // Add this node as a child to the new parent
      m_pParent = pParent;
      m_pParent->m_lhChildren.AddTail(*this);
    };
};

// Iteration through nodes
// Example: FOREACHNODE( pParentPtr, CChildType, itChild )
#define FOREACHNODE(ParentNodePtr, ChildNodeClass, Iterator) \
  for (CListIter<ChildNodeClass, sizeof(void *)> Iterator((ParentNodePtr)->GetChildren()); !Iterator.IsPastEnd(); Iterator.MoveToNext())

#endif
