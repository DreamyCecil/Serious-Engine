/* Copyright (c) 2002-2012 Croteam Ltd.
   Copyright (c) 2025 Dreamy Cecil
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

#include <Engine/Templates/StaticArray.cpp>

#define COMPARENAMES(a, b) (bCaseSensitive ? (strcmp((a).ConstData(), (b).ConstData()) == 0) : (a == b))

// Default constructor
template<class Type, bool bCaseSensitive>
CNameTable<Type, bCaseSensitive>::CNameTable(void)
{
  nt_ctCompartments = 0;
  nt_ctSlotsPerComp = 0;
  nt_ctSlotsPerCompStep = 0;
};

// Destructor
template<class Type, bool bCaseSensitive>
CNameTable<Type, bCaseSensitive>::~CNameTable(void)
{
};

// Remove all slots and reset the name table to the initial (empty) state
template<class Type, bool bCaseSensitive>
void CNameTable<Type, bCaseSensitive>::Clear(void)
{
  nt_ctCompartments = 0;
  nt_ctSlotsPerComp = 0;
  nt_ctSlotsPerCompStep = 0;
  nt_antsSlots.Clear();
};

// Set allocation parameters
template<class Type, bool bCaseSensitive>
void CNameTable<Type, bCaseSensitive>::SetAllocationParameters(INDEX ctCompartments, INDEX ctSlotsPerComp, INDEX ctSlotsPerCompStep)
{
  ASSERT(nt_ctCompartments == 0 && nt_ctSlotsPerComp == 0 && nt_ctSlotsPerCompStep == 0);
  ASSERT(   ctCompartments >  0 &&    ctSlotsPerComp >  0 &&    ctSlotsPerCompStep >  0);

  nt_ctCompartments = ctCompartments;
  nt_ctSlotsPerComp = ctSlotsPerComp;
  nt_ctSlotsPerCompStep = ctSlotsPerCompStep;

  nt_antsSlots.New(nt_ctCompartments * nt_ctSlotsPerComp);
};

// Get pointer to the slot from its key and value
template<class Type, bool bCaseSensitive>
CNameTableSlot<Type> *CNameTable<Type, bCaseSensitive>::FindSlot(ULONG ulKey, const CTString &strName)
{
  ASSERT(nt_ctCompartments > 0 && nt_ctSlotsPerComp > 0);

  // Find compartment number
  INDEX iComp = ulKey % nt_ctCompartments;

  // For each slot in the compartment
  INDEX iSlot = iComp * nt_ctSlotsPerComp;

  for (INDEX iSlotInComp = 0; iSlotInComp < nt_ctSlotsPerComp; iSlotInComp++, iSlot++) {
    CNameTableSlot<Type> *pnts = &nt_antsSlots[iSlot];

    // Skip if empty
    if (pnts->nts_ptElement == NULL) {
      continue;
    }

    // The same key
    if (pnts->nts_ulKey == ulKey) {
      // The same name
      if (COMPARENAMES(pnts->nts_ptElement->GetName(), strName)) {
        return pnts;
      }
    }
  }

  // Not found
  return NULL;
};

// Get index of an object in the name table
template<class Type, bool bCaseSensitive>
INDEX CNameTable<Type, bCaseSensitive>::FindSlotIndex(ULONG ulKey, const CTString &strName)
{
  ASSERT(nt_ctCompartments > 0 && nt_ctSlotsPerComp > 0);

  // Find compartment number
  INDEX iComp = ulKey % nt_ctCompartments;

  // For each slot in the compartment
  INDEX iSlot = iComp * nt_ctSlotsPerComp;

  for (INDEX iSlotInComp = 0; iSlotInComp < nt_ctSlotsPerComp; iSlotInComp++, iSlot++) {
    CNameTableSlot<Type> *pnts = &nt_antsSlots[iSlot];

    // Skip if empty
    if (pnts->nts_ptElement == NULL) {
      continue;
    }

    // The same key
    if (pnts->nts_ulKey == ulKey) {
      // The same name
      if (COMPARENAMES(pnts->nts_ptElement->GetName(), strName)) {
        return iSlot;
      }
    }
  }

  // Not found
  return -1;
};

// Get name from the name table by its index
template<class Type, bool bCaseSensitive>
const CTString CNameTable<Type, bCaseSensitive>::GetNameFromIndex(INDEX iIndex)
{
  ASSERT(nt_ctCompartments > 0 && nt_ctSlotsPerComp > 0);
  ASSERT(iIndex >= 0 && iIndex < nt_ctCompartments * nt_ctSlotsPerComp);

  return nt_antsSlots[iIndex].nts_ptElement->GetName();
};

// Find object by its name
template<class Type, bool bCaseSensitive>
Type *CNameTable<Type, bCaseSensitive>::Find(const CTString &strName)
{
  ASSERT(nt_ctCompartments > 0 && nt_ctSlotsPerComp > 0);

  CNameTableSlot<Type> *pnts = FindSlot(strName.GetHash(), strName);
  return (pnts == NULL) ? NULL : pnts->nts_ptElement;
};

// Get index of an object by its name
template<class Type, bool bCaseSensitive>
INDEX CNameTable<Type, bCaseSensitive>::FindIndex(const CTString &strName)
{
  ASSERT(nt_ctCompartments > 0 && nt_ctSlotsPerComp > 0);

  return FindSlotIndex(strName.GetHash(), strName);
};

// Expand the name table to the next step
template<class Type, bool bCaseSensitive>
void CNameTable<Type, bCaseSensitive>::Expand(void)
{
  ASSERT(nt_ctCompartments > 0 && nt_ctSlotsPerComp > 0);

  // The compartment has overflown
  ASSERT(nt_ctSlotsPerCompStep > 0);

  // Move the array of slots
  CStaticArray< CNameTableSlot<Type> > antsSlotsOld;
  antsSlotsOld.MoveArray(nt_antsSlots);

  // Allocate a new bigger array
  INDEX ctOldSlotsPerComp = nt_ctSlotsPerComp;
  nt_ctSlotsPerComp += nt_ctSlotsPerCompStep;

  nt_antsSlots.New(nt_ctSlotsPerComp * nt_ctCompartments);

  // For each compartment
  for (INDEX iComp = 0; iComp < nt_ctCompartments; iComp++) {
    // For each old slot in compartment
    for (INDEX iSlotInComp = 0; iSlotInComp < ctOldSlotsPerComp; iSlotInComp++) {
      CNameTableSlot<Type> &ntsOld = antsSlotsOld[iSlotInComp + iComp * ctOldSlotsPerComp];
      CNameTableSlot<Type> &ntsNew = nt_antsSlots[iSlotInComp + iComp * nt_ctSlotsPerComp];

      // If it is used
      if (ntsOld.nts_ptElement != NULL) {
        // Copy it to the new array
        ntsNew.nts_ptElement = ntsOld.nts_ptElement;
        ntsNew.nts_ulKey = ntsOld.nts_ulKey;
      }
    }
  }
};

// Flag to check for recursive expanding
static BOOL _bExpanding = FALSE;

// Add a new object
template<class Type, bool bCaseSensitive>
void CNameTable<Type, bCaseSensitive>::Add(Type *ptNew)
{
  ASSERT(nt_ctCompartments > 0 && nt_ctSlotsPerComp > 0);

  ULONG ulKey = ptNew->GetName().GetHash();

  // Find compartment number
  INDEX iComp = ulKey % nt_ctCompartments;

  // For each slot in the compartment
  INDEX iSlot = iComp * nt_ctSlotsPerComp;

  for (INDEX iSlotInComp = 0; iSlotInComp < nt_ctSlotsPerComp; iSlotInComp++, iSlot++) {
    CNameTableSlot<Type> *pnts = &nt_antsSlots[iSlot];

    // If it is empty
    if (pnts->nts_ptElement == NULL) {
      // Put it here
      pnts->nts_ulKey = ulKey;
      pnts->nts_ptElement = ptNew;
      return;
    }

    // Must not already exist
    //ASSERT(pnts->nts_ptElement->GetName() != ptNew->GetName());
  }

  // The compartment has overflown
  ASSERT(!_bExpanding);
  _bExpanding = TRUE;

  // Expand the hash table
  Expand();

  // Add the new element
  Add(ptNew);

  _bExpanding = FALSE;
};

// Remove an object
template<class Type, bool bCaseSensitive>
void CNameTable<Type, bCaseSensitive>::Remove(Type *ptOld)
{
  ASSERT(nt_ctCompartments > 0 && nt_ctSlotsPerComp > 0);

  // Find its slot
  const CTString &strName = ptOld->GetName();
  CNameTableSlot<Type> *pnts = FindSlot(strName.GetHash(), strName);

  if (pnts != NULL) {
    // Mark slot as unused
    ASSERT(pnts->nts_ptElement == ptOld);
    pnts->nts_ptElement = NULL;
  }
};

// Remove all objects but keep slots
template<class Type, bool bCaseSensitive>
void CNameTable<Type, bCaseSensitive>::Reset(void)
{
  for (INDEX iSlot = 0; iSlot < nt_antsSlots.Count(); iSlot++) {
    nt_antsSlots[iSlot].Clear();
  }
};
