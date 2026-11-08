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
#include <Engine/Base/ErrorReporting.h>
#include <Engine/Sound/Wave.h>

/* ====================================================
 *
 *      CONVERSION FUNCTIONS
 *
 */

// check wave format
void PCMWaveInput::CheckWaveFormat_t(const SEAudioFormat &afCheck, const char *strErrorString)
{
  // check bits per sample
  if (afCheck.iBytesPerSample != 1 && afCheck.iBytesPerSample != 2) {
    ThrowF_t(TRANS("%s: Unknown Bits Per Sample value!"), strErrorString);
  }

  // check number of channels
  if (afCheck.iChannels != 1 && afCheck.iChannels != 2) {
    ThrowF_t(TRANS("%s: Invalid number of channels!"), strErrorString);
  }
}


// Get next data
inline ULONG PCMWaveInput::GetData_t(CTStream *pCstrInput)
{
  ASSERT(pwi_bInfoLoaded);

  // read data according to bits per sample value
  if (pwi_afWave.iBytesPerSample == 1) {
    // read UBYTE
    UBYTE ubData;
    *pCstrInput >> ubData;
    return ((ULONG)ubData) <<16;            // (shift) prepare data for shrink/expand operation

  } else {
    // read UWORD
    SWORD swData;
    *pCstrInput >> swData;
    return ((ULONG)(swData+0x8000)) <<8;   // (shift) prepare data for shrink/expand operation
  }
}


// Store data
inline void PCMWaveInput::StoreData(ULONG ulData)
{
  ASSERT(pwi_afDesired.iBytesPerSample == 2);
  *pwi_pswMemory++ = ((SWORD)(ulData>>8) -0x8000);  // (shift) restore data format
}


/*
 *  Copy data
 */
void PCMWaveInput::CopyData_t(CTStream *pCstrInput)
{
  // for all input data (mono and stereo)
  ULONG ulDataCount = GetDataLength() * pwi_afWave.iChannels;
  while (ulDataCount > 0) {
    StoreData(GetData_t(pCstrInput));   // read and store data from input (hidden BitsPerSample conversion!)
    ulDataCount--;                      // to next data
  }
}



/*
 *  Shrink data
 */
// Shrink data
void PCMWaveInput::ShrinkData_t(CTStream *pCstrInput)
{
  ASSERT(pwi_dRatio>1.0);

  // *** MONO ***
  if (pwi_afWave.iChannels == 1) {
    DOUBLE dInterData, dTempData, dRatio;
    ULONG ulDataCount;

    // data intermediate value
    dInterData = 0.0;
    // for all input data (mono)
    ulDataCount = GetDataLength();
    dRatio = pwi_dRatio;
    while (ulDataCount > 0) {
      // read part of data (<100%)
      if (dRatio<1.0) {
        dTempData = GetData_t(pCstrInput);
        dInterData += dTempData*dRatio;
        StoreData(ULONG(dInterData/pwi_dRatio));
        // new intermediate value
        dRatio = 1 - dRatio;
        dInterData = dTempData*dRatio;
        dRatio = pwi_dRatio - dRatio;

      // read complete data (100%)
      } else {
        dInterData += GetData_t(pCstrInput);
        dRatio -= 1.0;
      }
      ulDataCount--;        // to next data
    }
    StoreData(ULONG(dInterData/(pwi_dRatio-dRatio)));


  // *** STEREO ***
  } else if (pwi_afWave.iChannels == 2) {
    DOUBLE dLInterData, dRInterData, dLTempData, dRTempData, dRatio;
    ULONG ulDataCount;

    // data intermediate value
    dLInterData = 0.0;
    dRInterData = 0.0;
    // for all input data (mono)
    ulDataCount = GetDataLength();
    dRatio = pwi_dRatio;
    while (ulDataCount > 0) {
      // read part of data (<100%)
      if (dRatio<1.0) {
        dLTempData = GetData_t(pCstrInput);
        dRTempData = GetData_t(pCstrInput);
        dLInterData += dLTempData*dRatio;
        dRInterData += dRTempData*dRatio;
        StoreData(ULONG(dLInterData/pwi_dRatio));
        StoreData(ULONG(dRInterData/pwi_dRatio));
        // new intermediate value
        dRatio = 1 - dRatio;
        dLInterData = dLTempData*dRatio;
        dRInterData = dRTempData*dRatio;
        dRatio = pwi_dRatio - dRatio;

      // read complete data (100%)
      } else {
        dLInterData += GetData_t(pCstrInput);
        dRInterData += GetData_t(pCstrInput);
        dRatio -= 1.0;
      }
      ulDataCount--;        // to next data
    }
    StoreData(ULONG(dLInterData/(pwi_dRatio-dRatio)));
    StoreData(ULONG(dRInterData/(pwi_dRatio-dRatio)));
  }
}




/* ====================================================
 *
 *      WAVE FUNCTIONS
 *
 */

/*
 *  Load Wave info
 */

void PCMWaveInput::LoadInfo_t(CTStream *pCstrInput, SEAudioFormat &afInfo)
{
  // if already loaded -> exception
  if (pwi_bInfoLoaded) {
    throw (TRANS("PCM Wave Input: Info already loaded."));
  }

  /* Read Riff */
  pCstrInput->ExpectID_t(CChunkID("RIFF"));     // ID "RIFF"
  (*pCstrInput) >> pwi_ulRiffLength;            // Ucitaj duljinu file-a

  /* Read Wave */
  pCstrInput->ExpectID_t(CChunkID("WAVE"));     // ID "WAVE"

  // [Cecil] Skip occasional junk
  if (pCstrInput->PeekID_t() == CChunkID("JUNK")) {
    pCstrInput->Seek_t(4, CTStream::SD_CUR);
    SLONG slJunkLength;
    (*pCstrInput) >> slJunkLength;
    pCstrInput->Seek_t(slJunkLength, CTStream::SD_CUR);
  }

  pCstrInput->ExpectID_t(CChunkID("fmt "));     // ID "fmt "
  // read Format Chunk length
  SLONG  slFmtLength;
  (*pCstrInput) >> slFmtLength;

  UWORD uwDummy;
  ULONG ulDummy;

  // read WAVE format
  (*pCstrInput) >> uwDummy;

  // [Cecil] Check format tag (WAVE_FORMAT_PCM in Windows)
  if (uwDummy != 1) {
    ThrowF_t(TRANS("%s: Invalid format tag, not a PCM Wave file!"), "PCM Wave Input (input)");
  }

  (*pCstrInput) >> uwDummy;
  pwi_afWave.iChannels = uwDummy;

  (*pCstrInput) >> ulDummy;
  pwi_afWave.iSampleRate = ulDummy;

  (*pCstrInput) >> ulDummy;
  (*pCstrInput) >> uwDummy;

  (*pCstrInput) >> uwDummy;
  pwi_afWave.iBytesPerSample = uwDummy / 8;

  // WARNING !!! - Only for PCM Wave - Skip extra information if exists
  if( slFmtLength > 16) {
    //WarningMessage("PCM Wave Input: Wave format Extra information skipped!");
    pCstrInput->Seek_t(slFmtLength - 16, CTStream::SD_CUR);
  }

  // WARNING - If exist Fact chunk skip it (purpose unknown)
  if( pCstrInput->GetID_t() == CChunkID("fact")) {
    //WarningMessage("PCM Wave Input: Fact Chunk skipped!");
    SLONG  slSkipLength;
    (*pCstrInput) >> slSkipLength;
    pCstrInput->Seek_t(slSkipLength, CTStream::SD_CUR);
  // seek back on Chunk ID
  } else {
    pCstrInput->Seek_t(-CID_LENGTH, CTStream::SD_CUR);
  }

  // [Cecil] Skip "LIST" chunk
  if (pCstrInput->PeekID_t() == CChunkID("LIST")) {
    pCstrInput->Seek_t(4, CTStream::SD_CUR);
    SLONG slJunkLength;
    (*pCstrInput) >> slJunkLength;
    pCstrInput->Seek_t(slJunkLength, CTStream::SD_CUR);
  }

  /* Read Data */
  pCstrInput->ExpectID_t(CChunkID("data"));      // ID "data"
  // read Data length (in bytes)
  (*pCstrInput) >> pwi_ulDataLength;

  /* Check PCM format */
  CheckWaveFormat_t(pwi_afWave, "PCM Wave Input (input)");

  // mark Info loaded
  pwi_bInfoLoaded = TRUE;

  // return Wave Format
  afInfo = pwi_afWave;
}


/*
 *  Load and convert Wave data
 */
void PCMWaveInput::LoadData_t(CTStream *pCstrInput, SWORD *pswMemory, const SEAudioFormat &afDesired)
{
  // if info not loaded -> exception
  if (!pwi_bInfoLoaded) {
    throw (TRANS("PCM Wave Input: Info not loaded."));
  }
  // if already loaded -> exception
  if (pwi_bDataLoaded) {
    throw (TRANS("PCM Wave Input: Data already loaded"));
  }

  // set memory pointer
  pwi_pswMemory = pswMemory;

  // store and check desired sound format
  CheckWaveFormat_t(afDesired, "PCM Wave Input (desired)");
  pwi_afDesired = afDesired;

  // calculate expand/shrink ratio (number of channels remain the same)
  pwi_dRatio = (DOUBLE)pwi_afDesired.iSampleRate / (DOUBLE)pwi_afWave.iSampleRate;

  // determine converion type from input and desired sound frequency, and convert sound
  if (pwi_dRatio < 1) {
    pwi_dRatio = 1/pwi_dRatio;
    ShrinkData_t(pCstrInput);
  } else if (pwi_dRatio > 1) {
    ASSERTALWAYS("Can't expand wave data");
    memset(pwi_pswMemory, 0, DetermineBufferSize(pwi_afDesired));
  // copy data
  } else {
    ASSERT(pwi_dRatio==1.0f);
    CopyData_t(pCstrInput);
  }

  // data is loaded (and maybe converted)
  pwi_bDataLoaded = TRUE;
}



/*
 *  Length in bytes
 */
ULONG PCMWaveInput::GetByteLength(void)
{
  ASSERT(pwi_bInfoLoaded);
  return pwi_ulDataLength;
}

/*
 *  Length in blocks
 */
ULONG PCMWaveInput::GetDataLength(void)
{
  ASSERT(pwi_bInfoLoaded);
  return GetByteLength() / pwi_afWave.GetBytesPerFrame();
}


ULONG PCMWaveInput::GetDataLength(const SEAudioFormat &afDesired)
{
  ASSERT(pwi_bInfoLoaded);
  // return buffer size
  return DetermineBufferSize(afDesired) / afDesired.GetBytesPerFrame();
}

/*
 *  Length in seconds
 */
DOUBLE PCMWaveInput::GetSecondsLength(void)
{
  ASSERT(pwi_bInfoLoaded);
  return (DOUBLE)GetDataLength() / (DOUBLE)pwi_afWave.iSampleRate;
}


/*
 *  Buffer length in bytes
 */
ULONG PCMWaveInput::DetermineBufferSize(void)
{
  return DetermineBufferSize(pwi_afWave);
}


ULONG PCMWaveInput::DetermineBufferSize(const SEAudioFormat &afDesired)
{
  ASSERT(pwi_bInfoLoaded);

  // calculate ratio between formats
  DOUBLE dRatio = (DOUBLE)afDesired.iSampleRate     / (DOUBLE)pwi_afWave.iSampleRate
                * (DOUBLE)afDesired.iBytesPerSample / (DOUBLE)pwi_afWave.iBytesPerSample;

  // return buffer size (must calculate with data length to avoid miss align data, for example:
  // 16 bit sound with 2 channels must be aligned to 4 bytes boundary and a multiply with
  // random ratio can as result give any possible number
  return (ULONG)ceil(dRatio * GetDataLength()) * pwi_afWave.GetBytesPerFrame();
}
