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

#ifndef SE_INCL_WAVE_H
#define SE_INCL_WAVE_H
#ifdef PRAGMA_ONCE
  #pragma once
#endif

/*
 *  PCM Wave Input
 */
class PCMWaveInput {
private:
  // Wave data
  SEAudioFormat pwi_afWave;
  SEAudioFormat pwi_afDesired;
  ULONG  pwi_ulRiffLength, pwi_ulDataLength;
  BOOL   pwi_bInfoLoaded,  pwi_bDataLoaded; // Status
  SWORD *pwi_pswMemory; // Memory

  /* Conversion */
  DOUBLE pwi_dRatio;
  // get and store data
  inline ULONG GetData_t( CTStream *pCstrInput);
  inline void  StoreData( ULONG ulData);
  void CopyData_t(   CTStream *pCstrInput);
  void ShrinkData_t( CTStream *pCstrInput);

public:
  // Check wave format
  static void CheckWaveFormat_t(const SEAudioFormat &afCheck, const char *strErrorString);

  /* Constructor */
  inline PCMWaveInput(void) { pwi_bInfoLoaded = FALSE; pwi_bDataLoaded = FALSE; };
  /* Load Wave info */
  void LoadInfo_t(CTStream *pCstrInput, SEAudioFormat &afInfo);
  /* Load and convert Wave data */
  void LoadData_t(CTStream *pCstrInput, SWORD *pswMemory, const SEAudioFormat &afDesired);

  /* Length in bytes / blocks / seconds */
  ULONG  GetByteLength(void);
  ULONG  GetDataLength(void);
  ULONG  GetDataLength(const SEAudioFormat &afDesired);
  DOUBLE GetSecondsLength(void);

  /* Buffer length in bytes */
  ULONG DetermineBufferSize(void);
  ULONG DetermineBufferSize(const SEAudioFormat &afDesired);
};



#endif  /* include-once check. */

