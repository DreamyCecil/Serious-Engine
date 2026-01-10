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

#ifndef SE_INCL_NODE_H
#define SE_INCL_NODE_H

#ifdef PRAGMA_ONCE
  #pragma once
#endif

class CNode {
  private:
    // List of child nodes (both pointers reference the same node if there's only one)
    CNode *m_pHead;
    CNode *m_pTail;

    CNode *m_pParent; // Node that owns this child node in a list (may be NULL)
    CNode *m_pPrev; // Previous neighboring node or NULL if it's the head of the parent
    CNode *m_pNext; // Next neighboring node or NULL if it's the tail of the parent

  private:
    // Setup the very first node in a list
    __forceinline void SetFirstNode(CNode *pFirst) {
      m_pHead = m_pTail = pFirst;

      // Relink the node to this list
      pFirst->Expunge();
      pFirst->m_pParent = this;
    };

    // Unlink a node from its previous neighbor
    __forceinline void UnlinkPrev(void) {
      if (m_pPrev != NULL) m_pPrev->m_pNext = NULL;
      m_pPrev = NULL;
    };

    // Unlink a node from its next neighbor
    __forceinline void UnlinkNext(void) {
      if (m_pNext != NULL) m_pNext->m_pPrev = NULL;
      m_pNext = NULL;
    };

  public:
    CNode() : m_pHead(NULL), m_pTail(NULL), m_pParent(NULL), m_pPrev(NULL), m_pNext(NULL)
    {
    };

    // Remove this node from any chain upon destruction
    virtual ~CNode() {
      Expunge();
    };

    // Get the first child node
    __forceinline CNode *GetHead(void) const { return m_pHead; };

    // Get the last child node
    __forceinline CNode *GetTail(void) const { return m_pTail; };

    // Get the parent that owns this node
    __forceinline CNode *GetParent(void) const { return m_pParent; };

    // Get the previous node in the chain adjacent to this one
    __forceinline CNode *GetPrev(void) const { return m_pPrev; };

    // Get the next node in the chain adjacent to this one
    __forceinline CNode *GetNext(void) const { return m_pNext; };

    // Check whether the node has any children
    __forceinline bool HasNodes(void) const {
      // Both pointers should either be empty or point to something
      ASSERT((m_pHead == NULL && m_pTail == NULL) || (m_pHead != NULL && m_pTail != NULL));
      return m_pHead != NULL;
    };

    // Count all child nodes
    inline size_t GetNodeCount(void) const {
      size_t ct = 0;
      CNode *pCheck = m_pHead;

      while (pCheck) {
        pCheck = pCheck->m_pNext;
        ++ct;
      }

      return ct;
    };

    // Get an n-th node from the beginning of child nodes
    inline CNode *GetNode(size_t n) const {
      for (CNode *pCheck = m_pHead; pCheck != NULL; pCheck = pCheck->m_pNext) {
        if (n == 0) return pCheck;
        --n;
      }

      return NULL;
    };

    // Check if a certain node exists among the children
    inline bool IsChild(CNode *pNode) const {
      for (CNode *pCheck = m_pHead; pCheck != NULL; pCheck = pCheck->m_pNext) {
        if (pCheck == pNode) return true;
      }

      return false;
    };

  public:
    // Add a new child at the beginning of the list
    inline void AddHead(CNode *pNode) {
      // Insert at the beginning if there is already a list
      if (m_pHead != NULL) {
        pNode->InsertBefore(m_pHead);
      } else {
        SetFirstNode(pNode);
      }
    };

    // Add a new child at the end of the list
    inline void AddTail(CNode *pNode) {
      // Insert at the end if there is already a list
      if (m_pTail != NULL) {
        pNode->InsertAfter(m_pTail);
      } else {
        SetFirstNode(pNode);
      }
    };

    // Insert this node in some chain before another node
    inline void InsertBefore(CNode *pNode) {
      // Same node, no need to move it anywhere
      if (pNode == this) return;

      // Remove from the current list and borrow the new parent
      Expunge();
      m_pParent = pNode->m_pParent;

      // Relink the parent to this new node, if it's at the beginning
      if (m_pParent != NULL && m_pParent->m_pHead == pNode) {
        m_pParent->m_pHead = this;
      }

      // Remember the node that goes before this one (may be NULL)
      CNode *pBefore = pNode->m_pPrev;
      pNode->UnlinkPrev();

      // Link the other node after this one
      pNode->m_pPrev = this;
      m_pNext = pNode;

      // Link the node that goes before
      if (pBefore != NULL) {
        m_pPrev = pBefore;
        pBefore->m_pNext = this;
      }
    };

    // Insert this node in some chain after another node
    inline void InsertAfter(CNode *pNode) {
      // Same node, no need to move it anywhere
      if (pNode == this) return;

      // Remove from the current list and borrow the new parent
      Expunge();
      m_pParent = pNode->m_pParent;

      // Relink the parent to this new node, if it's at the end
      if (m_pParent != NULL && m_pParent->m_pTail == pNode) {
        m_pParent->m_pTail = this;
      }

      // Remember the node that goes after this one (may be NULL)
      CNode *pAfter = pNode->m_pNext;
      pNode->UnlinkNext();

      // Link the other node before this one
      pNode->m_pNext = this;
      m_pPrev = pNode;

      // Link the node that goes after
      if (pAfter != NULL) {
        m_pNext = pAfter;
        pAfter->m_pPrev = this;
      }
    };

    // Remove this node from whichever chain it's currently in
    inline void Expunge(void) {
      // Link neighboring nodes together
      if (m_pPrev) m_pPrev->m_pNext = m_pNext;
      if (m_pNext) m_pNext->m_pPrev = m_pPrev;

      // Relink list head and tail
      if (m_pParent != NULL) {
        if (m_pParent->m_pHead == this) {
          m_pParent->m_pHead = m_pNext;
        }

        if (m_pParent->m_pTail == this) {
          m_pParent->m_pTail = m_pPrev;
        }
      }

      // Reset the links
      m_pParent = NULL;
      m_pPrev = m_pNext = NULL;
    };
};

// Helper class for iteration through node's children
// Allows for safe removal of processed nodes from the list in the middle of the loop by caching the next node in advance
template<class Type>
class CNodeIterator {
  private:
    CNode *m_pCurrent; // Current node to process
    CNode *m_pNext; // Next node to process
    bool m_bReverse; // Whether to move backward instead of forward with each next node

  public:
    // Constructor from the first node
    CNodeIterator(CNode *pNode, bool bReverse = false) :
      m_pCurrent(pNode),
      m_pNext(pNode ? (bReverse ? pNode->GetPrev() : pNode->GetNext()) : NULL),
      m_bReverse(bReverse)
    {
    };

    // Advance to the next node
    inline void Advance(void) {
      m_pCurrent = m_pNext;

      if (m_pCurrent) {
        m_pNext = m_bReverse ? m_pCurrent->GetPrev() : m_pCurrent->GetNext();
      }
    };

    // Check if there are no more nodes to process
    __forceinline bool IsPastEnd(void) {
      return m_pCurrent == NULL;
    };

    // Check if this is the final node in the loop
    __forceinline bool IsLast(void) {
      return m_pNext == NULL;
    };

    // Get the current node
    Type &Current(void)    { return *static_cast<Type *>(m_pCurrent); };
    Type &operator*(void)  { return *static_cast<Type *>(m_pCurrent); };
    operator Type *(void)  { return  static_cast<Type *>(m_pCurrent); };
    Type *operator->(void) { return  static_cast<Type *>(m_pCurrent); };
};

// Iteration through nodes
// Example: FOREACHNODE( pParentPtr, CChildType, itChild )
#define FOREACHNODE(ParentNodePtr, ChildNodeClass, Iterator) \
  for (CNodeIterator<ChildNodeClass> Iterator((ParentNodePtr)->GetHead(), false); !Iterator.IsPastEnd(); Iterator.Advance())

#endif
