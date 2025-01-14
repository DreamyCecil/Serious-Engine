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

#include "StdH.h"

#include <Engine/Base/Stream.h>
#include <Engine/Entities/EntityClass.h>
#include <Engine/Entities/EntityProperties.h>
#include <Engine/Base/ErrorReporting.h>
#include <Engine/Entities/Precaching.h>
#include <Engine/Base/Translation.h>
#include <Engine/Base/CRCTable.h>

#include <Engine/Templates/Stock_CAnimData.h>
#include <Engine/Templates/Stock_CTextureData.h>
#include <Engine/Templates/Stock_CModelData.h>
#include <Engine/Templates/Stock_CSoundData.h>
#include <Engine/Templates/Stock_CEntityClass.h>

#include <Engine/Templates/Stock_CEntityClass.h>

/////////////////////////////////////////////////////////////////////
// CEntityClass

/*
 * Default constructor.
 */
CEntityClass::CEntityClass(void)
{
  ec_fnmClassDLL.Clear();
  ec_pdecDLLClass = NULL;
}
/*
 * Constructor for a fixed class.
 */
CEntityClass::CEntityClass(class CDLLEntityClass *pdecDLLClass)
{
  ec_pdecDLLClass = pdecDLLClass;
  ec_fnmClassDLL.Clear();
}

/*
 * Destructor.
 */
CEntityClass::~CEntityClass(void)
{
  Clear();
}

/////////////////////////////////////////////////////////////////////
// Reference counting functions
void CEntityClass::AddReference(void) {
  MarkUsed();
};
void CEntityClass::RemReference(void) {
  _pEntityClassStock->Release(this);
};

/*
 * Clear the object.
 */
void CEntityClass::Clear(void)
{
  // if the DLL is loaded
  if (ec_mdClassDLL.IsLoaded()) {
    // [Cecil] Free the class if it's been loaded
    if (ec_pdecDLLClass != NULL) {
      ec_pdecDLLClass->dec_OnEndClass();

      // Release all components needed by the DLL
      ec_pdecDLLClass->ReleaseComponents();
    }

    // [Cecil] The library should never be released from memory because declared
    // shell symbols must remain avaliable, since they cannot be undeclared
    ec_mdClassDLL = NULL;
  }

  ec_pdecDLLClass = NULL;
  ec_fnmClassDLL.Clear();
}

/* Check that all properties have been properly declared. */
void CEntityClass::CheckClassProperties(void)
{
// do nothing in release version
#ifndef NDEBUG
  // for all classes in hierarchy of this entity
  {for(CDLLEntityClass *pdecDLLClass1 = ec_pdecDLLClass;
      pdecDLLClass1!=NULL;
      pdecDLLClass1 = pdecDLLClass1->dec_pdecBase) {
    // for all properties
    for(INDEX iProperty1=0; iProperty1<pdecDLLClass1->dec_ctProperties; iProperty1++) {
      CEntityProperty &epProperty1 = pdecDLLClass1->dec_aepProperties[iProperty1];

      // for all classes in hierarchy of this entity
      for(CDLLEntityClass *pdecDLLClass2 = ec_pdecDLLClass;
          pdecDLLClass2!=NULL;
          pdecDLLClass2 = pdecDLLClass2->dec_pdecBase) {
        // for all properties
        for(INDEX iProperty2=0; iProperty2<pdecDLLClass2->dec_ctProperties; iProperty2++) {
          CEntityProperty &epProperty2 = pdecDLLClass2->dec_aepProperties[iProperty2];
          // the two properties must not have same id unless they are same property
          ASSERTMSG(&epProperty1==&epProperty2 || epProperty1.ep_ulID!=epProperty2.ep_ulID,
            "No two properties may have same id!");
        }
      }
    }
  }}

  // for all classes in hierarchy of this entity
  {for(CDLLEntityClass *pdecDLLClass1 = ec_pdecDLLClass;
      pdecDLLClass1!=NULL;
      pdecDLLClass1 = pdecDLLClass1->dec_pdecBase) {
    // for all components
    for(INDEX iComponent1=0; iComponent1<pdecDLLClass1->dec_ctComponents; iComponent1++) {
      CEntityComponent &ecComponent1 = pdecDLLClass1->dec_aecComponents[iComponent1];

      // for all classes in hierarchy of this entity
      for(CDLLEntityClass *pdecDLLClass2 = ec_pdecDLLClass;
          pdecDLLClass2!=NULL;
          pdecDLLClass2 = pdecDLLClass2->dec_pdecBase) {
        // for all components
        for(INDEX iComponent2=0; iComponent2<pdecDLLClass2->dec_ctComponents; iComponent2++) {
          CEntityComponent &ecComponent2 = pdecDLLClass2->dec_aecComponents[iComponent2];
          // the two components must not have same id unless they are same component
          ASSERTMSG(&ecComponent1==&ecComponent2 || ecComponent1.ec_slID!=ecComponent2.ec_slID,
            "No two components may have same id!");
        }
      }
    }
  }}
#endif
}

/*
 * Construct a new member of the class.
 */
CEntity *CEntityClass::New(void)
{
  // the DLL must be loaded
  ASSERT(ec_pdecDLLClass!= NULL);
  // ask the DLL class to call the 'operator new' in the scope where the class is declared
  CEntity *penNew = ec_pdecDLLClass->dec_New();
  // remember this class as class of the entity
  AddReference();
  penNew->en_pecClass = this;
  // set all properties to default
  penNew->SetDefaultProperties();

  // return it
  return penNew;
}

/*
 * Obtain all components from component table.
 */
void CDLLEntityClass::ObtainComponents_t(void)
{
  // for each component
  for (INDEX iComponent = 0; iComponent < dec_ctComponents; iComponent++) {
    // if not precaching all
    if( gam_iPrecachePolicy<PRECACHE_ALL) {
      // if component is not class
      CEntityComponent &ec = dec_aecComponents[iComponent];
      if (ec.ec_ectType!=ECT_CLASS) {
        // skip it
        continue;
      }
    }

    // try to
    try {
      // obtain the component
      dec_aecComponents[iComponent].Obtain_t();
    // if failed
    } catch (char *) {
      // if in paranoia mode
      if( gam_iPrecachePolicy==PRECACHE_PARANOIA) {
        // fail
        throw;
      // if not in paranoia mode
      } else {
        // ignore all errors
        NOTHING;
      }
    }
  }
}

/*
 * Release all components from component table.
 */
void CDLLEntityClass::ReleaseComponents(void)
{
  // for each component
  for (INDEX iComponent = 0; iComponent < dec_ctComponents; iComponent++) {
    // release the component
    dec_aecComponents[iComponent].Release();
  }
}

// overrides from CSerial /////////////////////////////////////////////////////

/*
 * Read from stream.
 */
void CEntityClass::Read_t( CTStream *istr) // throw char *
{
  // read the dll filename and class name from the stream
  CTFileName fnmDLL;
  fnmDLL.ReadFromText_t(*istr, "Package: ", TRUE);
  CTString strClassName;
  strClassName.ReadFromText_t(*istr, "Class: ");

  // create name of dll
  #ifndef NDEBUG
    fnmDLL = _fnmApplicationExe.FileDir()+fnmDLL.FileName()+_strModExt+"D"+fnmDLL.FileExt();
  #else
    fnmDLL = _fnmApplicationExe.FileDir()+fnmDLL.FileName()+_strModExt+fnmDLL.FileExt();
  #endif

  // load the DLL
  CTFileName fnmExpanded;
  ExpandFilePath(EFP_READ, fnmDLL, fnmExpanded);

  ec_mdClassDLL.LoadOrThrow_t(fnmExpanded.ConstData());
  ec_fnmClassDLL = fnmDLL;

  // [Cecil] Try to find the class in the global registry, assuming it's been added
  EntityClassRegistry_t::const_iterator itInRegistry = _pEntityClassRegistry->find(strClassName);

  // if class structure is not found
  if (itInRegistry == _pEntityClassRegistry->end()) {
    // free the library
    BOOL bSuccess = ec_mdClassDLL.Free();
    ASSERT(bSuccess);
    ec_fnmClassDLL.Clear();
    // report error
    ThrowF_t(TRANS("Class '%s' not found in entity class package file '%s'"), strClassName.ConstData(), fnmDLL.ConstData());
  }

  // [Cecil] Get the pointer to the DLL class from the registry
  ec_pdecDLLClass = itInRegistry->second;

  // obtain all components needed by the DLL
  {
    CTmpPrecachingNow tpn;
    ec_pdecDLLClass->ObtainComponents_t();
  }

  // attach the DLL
  ec_pdecDLLClass->dec_OnInitClass();

  // check that the class properties have been properly declared
  CheckClassProperties();
}

/*
 * Write to stream.
 */
void CEntityClass::Write_t( CTStream *ostr) // throw char *
{
  ASSERTALWAYS("Do not write CEntityClass objects!");
}
// get amount of memory used by this object
SLONG CEntityClass::GetUsedMemory(void)
{
  // we don't know exact memory used, but we want to enumerate them
  return 0;
}
// check if this kind of objects is auto-freed
BOOL CEntityClass::IsAutoFreed(void) 
{ 
  return FALSE;
};
// gather the CRC of the file
void CEntityClass::AddToCRCTable(void)
{
  const CTFileName &fnm = GetName();
  // if already added
  if (CRCT_IsFileAdded(fnm)) {
    // do nothing
    return;
  }

  // add the file itself
  CRCT_AddFile_t(fnm);
  // add its DLL
  CRCT_AddFile_t(ec_fnmClassDLL);
}

/* Get pointer to entity property from its name. */
class CEntityProperty *CEntityClass::PropertyForName(const CTString &strPropertyName) {
  return ec_pdecDLLClass->PropertyForName(strPropertyName);
};
/* Get pointer to entity property from its packed identifier. */
class CEntityProperty *CEntityClass::PropertyForTypeAndID(
  ULONG ulType, ULONG ulID) {
  return ec_pdecDLLClass->PropertyForTypeAndID((CEntityProperty::PropertyType)ulType, ulID);
};

/* Get event handler for given state and event code. */
CEntity::pEventHandler CEntityClass::HandlerForStateAndEvent(SLONG slState, SLONG slEvent) {
  return ec_pdecDLLClass->HandlerForStateAndEvent(slState, slEvent);
}

// convert value of an enum to its name
const char *CEntityPropertyEnumType::NameForValue(INDEX iValue)
{
  for(INDEX i=0; i<epet_ctValues; i++) {
    if (epet_aepevValues[i].epev_iValue==iValue) {
      return epet_aepevValues[i].epev_strName;
    }
  }
  return "";
}

/*
 * Get pointer to entity property from its name.
 */
class CEntityProperty *CDLLEntityClass::PropertyForName(const CTString &strPropertyName)
{
  // for each property
  for (INDEX iProperty=0; iProperty<dec_ctProperties; iProperty++) {
    // if it has that name
    if (dec_aepProperties[iProperty].ep_strName==strPropertyName) {
      // return it
      return &dec_aepProperties[iProperty];
    }
  }
  // if base class exists
  if (dec_pdecBase!=NULL) {
    // look in the base class
    return dec_pdecBase->PropertyForName(strPropertyName);
  // otherwise
  } else {
    // none found
    return NULL;
  }
}

/*
 * Get pointer to entity property from its packed identifier.
 */
class CEntityProperty *CDLLEntityClass::PropertyForTypeAndID(
  CEntityProperty::PropertyType eptType, ULONG ulID)
{
  // for each property
  for (INDEX iProperty=0; iProperty<dec_ctProperties; iProperty++) {
    // if it has that same identifier
    if (dec_aepProperties[iProperty].ep_ulID==ulID) {

      // if it also has same type
      if (dec_aepProperties[iProperty].ep_eptType==eptType) {
        // return it
        return &dec_aepProperties[iProperty];

      // if it has different type
      } else {
        // return that it was not found, this makes the whole thing much safer
        return NULL;
      }
    }
  }
  // if base class exists
  if (dec_pdecBase!=NULL) {
    // look in the base class
    return dec_pdecBase->PropertyForTypeAndID(eptType, ulID);
  // otherwise
  } else {
    // none found
    return NULL;
  }
};

// [Cecil] Get pointer to component from its identifier, ignoring the type (IDs must be unique anyway)
CEntityComponent *CDLLEntityClass::ComponentForID(SLONG slID)
{
  for (INDEX iComponent = 0; iComponent < dec_ctComponents; iComponent++) {
    // Obtain and return the component with the same identifier
    if (dec_aecComponents[iComponent].ec_slID == slID)
    {
      dec_aecComponents[iComponent].ObtainWithCheck();
      return &dec_aecComponents[iComponent];
    }
  }

  // Try looking for it in the base class
  if (dec_pdecBase != NULL) {
    return dec_pdecBase->ComponentForID(slID);
  }

  // None found
  return NULL;
};

// Get pointer to component from the component
CEntityComponent *CDLLEntityClass::ComponentForPointer(void *pv)
{
  for (INDEX iComponent = 0; iComponent < dec_ctComponents; iComponent++) {
    // Obtain and return the component with the same pointer
    if (dec_aecComponents[iComponent].ec_pvPointer == pv)
    {
      dec_aecComponents[iComponent].ObtainWithCheck();
      return &dec_aecComponents[iComponent];
    }
  }

  // Try looking for it in the base class
  if (dec_pdecBase != NULL) {
    return dec_pdecBase->ComponentForPointer(pv);
  }

  // None found
  return NULL;
};

// precache given component
void CDLLEntityClass::PrecacheModel(SLONG slID)
{
  CTmpPrecachingNow tpn;

  CEntityComponent *pecModel = ComponentForID(slID);
  ASSERT(pecModel != NULL && pecModel->ec_ectType == ECT_MODEL);
  pecModel->ObtainWithCheck();
}

void CDLLEntityClass::PrecacheTexture(SLONG slID)
{
  CTmpPrecachingNow tpn;

  CEntityComponent *pecTexture = ComponentForID(slID);
  ASSERT(pecTexture != NULL && pecTexture->ec_ectType == ECT_TEXTURE);
  pecTexture->ObtainWithCheck();
}

void CDLLEntityClass::PrecacheSound(SLONG slID)
{
  CTmpPrecachingNow tpn;

  CEntityComponent *pecSound = ComponentForID(slID);
  ASSERT(pecSound != NULL && pecSound->ec_ectType == ECT_SOUND);
  pecSound->ObtainWithCheck();
}

void CDLLEntityClass::PrecacheClass(SLONG slID, INDEX iUser /* = -1 */)
{
  CTmpPrecachingNow tpn;

  CEntityComponent *pecClass = ComponentForID(slID);
  ASSERT(pecClass != NULL && pecClass->ec_ectType == ECT_CLASS);
  pecClass->ObtainWithCheck();
  pecClass->ec_pecEntityClass->ec_pdecDLLClass->dec_OnPrecache(
    pecClass->ec_pecEntityClass->ec_pdecDLLClass, iUser);
}

/*
 * Get event handler given state and event code.
 */
CEntity::pEventHandler CDLLEntityClass::HandlerForStateAndEvent(SLONG slState, SLONG slEvent)
{
  // we ignore the event code here
  (void) slEvent;

  // for each handler
  for (INDEX iHandler=0; iHandler<dec_ctHandlers; iHandler++) {
    // if it has that same state
    if (dec_aeheHandlers[iHandler].ehe_slState==slState) {
      // return it
      return dec_aeheHandlers[iHandler].ehe_pEventHandler;
    }
  }
  // if base class exists
  if (dec_pdecBase!=NULL) {
    // look in the base class
    return dec_pdecBase->HandlerForStateAndEvent(slState, slEvent);
  // otherwise
  } else {
    // none found
    return NULL;
  }
}

/* Get event handler name for given state. */
const char *CDLLEntityClass::HandlerNameForState(SLONG slState)
{
  // for each handler
  for (INDEX iHandler=0; iHandler<dec_ctHandlers; iHandler++) {
    // if it has that same state
    if (dec_aeheHandlers[iHandler].ehe_slState==slState) {
      // return its name
      return dec_aeheHandlers[iHandler].ehe_strName;
    }
  }
  // if base class exists
  if (dec_pdecBase!=NULL) {
    // look in the base class
    return dec_pdecBase->HandlerNameForState(slState);
  // otherwise
  } else {
    // none found
    return "no handler!?";
  }
}

/* Get derived class override for given state. */
SLONG CDLLEntityClass::GetOverridenState(SLONG slState)
{
  // for each handler
  for (INDEX iHandler=0; iHandler<dec_ctHandlers; iHandler++) {
    // if it has that same base state
    if (dec_aeheHandlers[iHandler].ehe_slBaseState>=0 &&
        dec_aeheHandlers[iHandler].ehe_slBaseState==slState) {
      // return overriden state with possible recursive overriding
      return GetOverridenState(dec_aeheHandlers[iHandler].ehe_slState);
    }
  }
  // if base class exists
  if (dec_pdecBase!=NULL) {
    // look in the base class
    return dec_pdecBase->GetOverridenState(slState);
  // otherwise
  } else {
    // none found
    return slState;
  }
}
