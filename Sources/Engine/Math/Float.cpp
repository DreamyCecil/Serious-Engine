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

#include <Engine/Math/Float.h>

#if SE1_WIN

#if SE1_X64
  // Simplified logic with overwriting
  static inline ULONG FPUControl(ULONG newcw, ULONG mask)
  {
    static ULONG fpcw = _PC_64;

    if (mask != 0) {
      fpcw = newcw;
    }

    return fpcw;
  };

#elif SE1_X86
  // Use proper _control87()
  #define FPUControl _control87

#else
  #error Define FPUControl() for CPU architectures other than x86 and x64
#endif

#elif SE1_X64 || SE1_X86
  #define MCW_PC  0x0300
  #define _MCW_PC MCW_PC
  #define _PC_24  0x0000
  #define _PC_53  0x0200
  #define _PC_64  0x0300

  // Windows' _control87() reimplementation
  static inline ULONG FPUControl(WORD newcw, WORD mask)
  {
    static WORD fpw = _PC_64;

    if (mask != 0) {
      fpw &= ~mask;
      fpw |= (newcw & mask);
    }

    return fpw;
  };

#else
  #error Define FPUControl() for CPU architectures other than x86 and x64
#endif

/* Get current precision setting of FPU. */
enum FPUPrecisionType GetFPUPrecision(void)
{
  // get control flags from FPU
  ULONG fpcw = FPUControl(0, 0);

  // extract the precision from the flags
  switch(fpcw&_MCW_PC) {
  case _PC_24:
    return FPT_24BIT;
    break;
  case _PC_53:
    return FPT_53BIT;
    break;
  case _PC_64:
    return FPT_64BIT;
    break;
  default:
    ASSERT(FALSE);
    return FPT_24BIT;
  };
}

/* Set current precision setting of FPU. */
void SetFPUPrecision(enum FPUPrecisionType fptNew)
{
  ULONG fpcw;
  // create FPU flags from the precision
  switch(fptNew) {
  case FPT_24BIT:
    fpcw=_PC_24;
    break;
  case FPT_53BIT:
    fpcw=_PC_53;
    break;
  case FPT_64BIT:
    fpcw=_PC_64;
    break;
  default:
    ASSERT(FALSE);
    fpcw=_PC_24;
  };
  // set the FPU precission
  FPUControl(fpcw, MCW_PC);
}

/////////////////////////////////////////////////////////////////////
// CSetFPUPrecision
/*
 * Constructor with automatic setting of FPU precision.
 */
CSetFPUPrecision::CSetFPUPrecision(enum FPUPrecisionType fptNew)
{
  // remember old precision
  sfp_fptOldPrecision = GetFPUPrecision();
  // set new precision if needed
  sfp_fptNewPrecision = fptNew;
  if (sfp_fptNewPrecision!=sfp_fptOldPrecision) {
    SetFPUPrecision(fptNew);
  }
}

/*
 * Destructor with automatic restoring of FPU precision.
 */
CSetFPUPrecision::~CSetFPUPrecision(void)
{
  // check consistency
  ASSERT(GetFPUPrecision()==sfp_fptNewPrecision);
  // restore old precision if needed
  if (sfp_fptNewPrecision!=sfp_fptOldPrecision) {
    SetFPUPrecision(sfp_fptOldPrecision);
  }
}

BOOL IsValidFloat(float f)
{
  return _finite(f) && (*(ULONG*)&f)!=0xcdcdcdcdUL;
/*  int iClass = _fpclass(f);
  return
    iClass==_FPCLASS_NN ||
    iClass==_FPCLASS_ND ||
    iClass==_FPCLASS_NZ ||
    iClass==_FPCLASS_PZ ||
    iClass==_FPCLASS_PD ||
    iClass==_FPCLASS_PN;
    */
}

BOOL IsValidDouble(double f)
{
#if SE1_WIN
  return _finite(f) && (*(UQUAD *)&f) != 0xcdcdcdcdcdcdcdcdI64;
#else
  return _finite(f) && (*(UQUAD *)&f) != 0xcdcdcdcdcdcdcdcdll;
#endif
/*  int iClass = _fpclass(f);
  return
    iClass==_FPCLASS_NN ||
    iClass==_FPCLASS_ND ||
    iClass==_FPCLASS_NZ ||
    iClass==_FPCLASS_PZ ||
    iClass==_FPCLASS_PD ||
    iClass==_FPCLASS_PN;
    */
}

