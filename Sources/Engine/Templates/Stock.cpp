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

#include <Engine/Base/Stream.h>

#include <Engine/Templates/DynamicContainer.cpp>

// Default constructor
template<class Type>
CResourceStock<Type>::CResourceStock()
{
  st_ntObjects.SetAllocationParameters(50, 2, 2);
};

// Destructor
template<class Type>
CResourceStock<Type>::~CResourceStock()
{
  // Free all unused elements of the stock
  FreeUnused();
};

// Obtain an object from stock - loads if not loaded
template<class Type>
Type *CResourceStock<Type>::Obtain_internal_t(const CTFileName &fnmFileName)
{
  // Find stocked object with same name
  Type *pExisting = st_ntObjects.Find(fnmFileName);

  // If found
  if (pExisting != NULL) {
    // Mark that it is used once again
    pExisting->MarkUsed();

    // Return its pointer
    return pExisting;
  }

  // Create a new stock object, if not found
  Type *ptNew = new Type;
  ptNew->ser_FileName = fnmFileName;

  st_ctObjects.Add(ptNew);
  st_ntObjects.Add(ptNew);

  // Try to load it
  try {
    ptNew->Load_t(fnmFileName);

  // Failed to load
  } catch (char *) {
    st_ctObjects.Remove(ptNew);
    st_ntObjects.Remove(ptNew);

    delete ptNew;
    throw;
  }

  // Mark that it is used for the first time
  //ASSERT(!ptNew->IsUsed());
  ptNew->MarkUsed();

  // Return the new object
  return ptNew;
};

// Release an object when it's not needed any more
template<class Type>
void CResourceStock<Type>::Release_internal(Type *ptObject) {
  // Mark that it is used once less
  ptObject->MarkUnused();

  // If it is not used at all anymore and should be freed automatically
  if (!ptObject->IsUsed() && ptObject->IsAutoFreed()) {
    // Remove and delete it
    st_ctObjects.Remove(ptObject);
    st_ntObjects.Remove(ptObject);

    delete ptObject;
  }
};

// Free all unused elements from the stock
template<class Type>
void CResourceStock<Type>::FreeUnused_internal(void)
{
  BOOL bAnyRemoved;

  do {
    // Fill a container with objects that should be freed
    CDynamicContainer<Type> ctToFree;

    {FOREACHINDYNAMICCONTAINER(st_ctObjects, Type, itt) {
      if (!itt->IsUsed()) {
        ctToFree.Add(itt);
      }
    }}

    bAnyRemoved = ctToFree.Count() > 0;

    // Go through objects that should be freed
    {FOREACHINDYNAMICCONTAINER(ctToFree, Type, itt) {
      // Remove and delete it
      st_ctObjects.Remove(itt);
      st_ntObjects.Remove(itt);

      delete (&*itt);
    }}

  // Go for as long as there is something to remove
  } while (bAnyRemoved);
};

// Calculate amount of memory used by all objects in the stock
template<class Type>
SLONG CResourceStock<Type>::CalculateUsedMemory(void)
{
  SLONG slUsedTotal = 0;

  // Go through all stock objects
  FOREACHINDYNAMICCONTAINER(st_ctObjects, Type, itt) {
    SLONG slUsedByObject = itt->GetUsedMemory();

    // Invalid memory
    if (slUsedByObject < 0) {
      return -1;
    }

    // Add used memory to the total amount of memory
    slUsedTotal += slUsedByObject;
  }

  return slUsedTotal;
};

// Dump memory usage report into a file
template<class Type>
void CResourceStock<Type>::DumpMemoryUsage_t(CTStream &strm)
{
  CTString strLine;
  SLONG slUsedTotal = 0;

  // Go through all stock objects
  FOREACHINDYNAMICCONTAINER(st_ctObjects, Type, itt) {
    SLONG slUsedByObject = itt->GetUsedMemory();

    // Invalid memory
    if (slUsedByObject < 0) {
      strm.PutLine_t("Error!");
      return;
    }

    // Print out memory usage of this object
    strLine.PrintF("%7.1fk %s(%d) %s", slUsedByObject / 1024.0f,
      itt->GetName().ConstData(), itt->GetUsedCount(), itt->GetDescription().ConstData());

    strm.PutLine_t(strLine.ConstData());
  }
};

// Get number of total elements in stock
template<class Type>
INDEX CResourceStock<Type>::GetTotalCount(void) {
  return st_ctObjects.Count();
};

// Get number of used elements in stock
template<class Type>
INDEX CResourceStock<Type>::GetUsedCount(void)
{
  INDEX ctUsed = 0;

  // Go through all stock objects
  FOREACHINDYNAMICCONTAINER(st_ctObjects, Type, itt) {
    // Count used ones
    if (itt->IsUsed()) {
      ctUsed++;
    }
  }

  return ctUsed;
};
