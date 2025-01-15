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

#include "StdH.h"

#include <Engine/Ska/ModelConfig.h>
#include <Engine/Templates/Stock_CModelConfig.h>

CModelConfig::CModelConfig() : mc_pModelInstance(NULL)
{
};

CModelConfig::~CModelConfig() {
  if (mc_pModelInstance == NULL) return;

  // Make sure that the released model instance isn't used by anything anymore
  if (mc_pModelInstance->mi_pInStock != NULL) {
    ASSERT(mc_pModelInstance->mi_pInStock->ser_ctUsed == 0);
  }

  // Delete stock's model instance
  mc_pModelInstance->mi_pInStock = NULL;
  DeleteModelInstance(mc_pModelInstance);
  mc_pModelInstance = NULL;
};

// Get the description of this object
CTString CModelConfig::GetDescription(void) {
  if (mc_pModelInstance == NULL) return "no model loaded";

  CTString strSkeletons = "";

  if (mc_pModelInstance->mi_psklSkeleton != NULL) {
    strSkeletons.PrintF("%d skeletons, ", mc_pModelInstance->mi_psklSkeleton->skl_aSkeletonLODs.Count());
  }

  CTString str;
  str.PrintF("%d meshes, %s%d anims, %d colboxes, %d children", 
    mc_pModelInstance->mi_aMeshInst.Count(), strSkeletons.ConstData(), mc_pModelInstance->mi_aAnimSet.Count(),
    mc_pModelInstance->mi_cbAABox.Count(), mc_pModelInstance->mi_cmiChildren.Count());

  return str;
};

// Clear the object
void CModelConfig::Clear(void) {
  ASSERTALWAYS("CModelConfig cannot be cleared!");
};

// Write to stream
void CModelConfig::Write_t(CTStream *ostrFile) {
  ASSERTALWAYS("CModelConfig cannot be writen!");
};

// Read from stream
void CModelConfig::Read_t(CTStream *istrFile) {
  // Load model instance from the model config
  ASSERT(mc_pModelInstance == NULL);
  mc_pModelInstance = LoadModelInstance_t(istrFile->GetDescription());

  // Make model instance remember its config in the stock
  ASSERT(mc_pModelInstance != NULL);
  ASSERT(mc_pModelInstance->mi_pInStock == NULL);
  mc_pModelInstance->mi_pInStock = this;
};

// Check if this kind of objects is auto-freed
BOOL CModelConfig::IsAutoFreed(void) {
  return TRUE;
};

// Get amount of memory used by this object
SLONG CModelConfig::GetUsedMemory(void) {
  if (mc_pModelInstance != NULL) {
    return sizeof(*this) + mc_pModelInstance->GetUsedMemory();
  }

  return sizeof(*this);
};

// Load model from the stock into some model instance
void ObtainModelInstance_t(CModelInstance *pmi, const CTString &fnSmcFile) {
  ASSERT(pmi != NULL);

  // Obtain model instance from the stock
  CModelConfig *pmc = _pModelConfigStock->Obtain_t(fnSmcFile);
  ASSERT(pmc != NULL && pmc->mc_pModelInstance != NULL);

  // Copy obtained model instance
  pmi->Copy(*pmc->mc_pModelInstance); // Results in +1 reference
  _pModelConfigStock->Release(pmi->mi_pInStock); // Remove that reference
};

// Obtain model instance from the stock
CModelInstance *ObtainModelInstance_t(const CTString &fnSmcFile) {
  CModelInstance *pmi = CreateModelInstance("");
  ObtainModelInstance_t(pmi, fnSmcFile);
  return pmi;
};
