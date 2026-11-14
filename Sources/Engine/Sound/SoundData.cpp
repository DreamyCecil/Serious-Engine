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

#include <Engine/Sound/SoundData.h>

#include <Engine/Base/Memory.h>
#include <Engine/Base/Stream.h>
#include <Engine/Base/ListIterator.inl>
#include <Engine/Sound/Wave.h>
#include <Engine/Sound/SoundDecoder.h>
#include <Engine/Sound/SoundLibrary.h>
#include <Engine/Sound/SoundObject.h>

#include <Engine/Templates/Stock_CSoundData.h>

/* ====================================================
 *
 *  Sound data awareness functions
 */

/*
 *  Pause all playing
 */
void CSoundData::PausePlayingObjects(void)
{
  // for all objects linked to data pause playing
  FOREACHINLIST(CSoundObject, so_Node, sd_ClhLinkList, itCsoPause) {
    // pause playing
    itCsoPause->Pause();
  }
}

/*
 *  For all objects resume play
 */
void CSoundData::ResumePlayingObjects(void)
{
  // for all objects resume play
  FOREACHINLIST(CSoundObject, so_Node, sd_ClhLinkList, itCsoResume) {
    // call play method again
    itCsoResume->Resume();
  }
}


/*
 *  Add object in sound aware list
 */
void CSoundData::AddObjectLink(CSoundObject &CsoAdd)
{
  // add object to list tail
  sd_ClhLinkList.AddTail(CsoAdd.so_Node);
}

/*
 *  Remove a object from aware list
 */
void CSoundData::RemoveObjectLink(CSoundObject &CsoRemove)
{
  // remove it from list
  CsoRemove.so_Node.Remove();
}


/* ====================================================
 *
 *  Class global methods
 */

// Constructor
CSoundData::CSoundData()
{
  sd_pswBuffer = NULL;
  sd_psdcStaticDecoder = NULL;
}

// Destructor
CSoundData::~CSoundData()
{
  Clear();
}


// Free Buffer (and all linked Objects)
void CSoundData::ClearBuffer(void)
{
  // if buffer exist
  if( sd_pswBuffer!=NULL) {
    // release it
    FreeMemory( sd_pswBuffer);
    sd_pswBuffer = NULL;
  }

  // [Cecil] Destroy the decoder
  if (sd_psdcStaticDecoder != NULL) {
    delete sd_psdcStaticDecoder;
    sd_psdcStaticDecoder = NULL;
  }
}


// Get Sound Length in seconds
SECOND CSoundData::GetSecondsLength(void)
{
  // [Cecil] Now valid for encoded audio as well
  return sd_dSecondsLength;
}


/* ====================================================
 *
 *  Input methods (load sound in SoundData class)
 */

// Read sound in memory
void CSoundData::Read_t(CTStream *inFile)  // throw char *
{
  // synchronize access to sounds
  CTSingleLock slSounds(&_pSound->sl_csSound, TRUE);

  ASSERT( sd_pswBuffer==NULL);
  sd_ulFlags = NONE;

  // get filename
  const CTString &fnm = inFile->GetFilePath(); // [Cecil] Relative file path
  const CTString strExt = fnm.FileExt();

  // [Cecil] If it's an encoded file of any supported format
  if (strExt == ".ogg" || strExt == ".mp3" || strExt == ".flac") {
    // [Cecil] Initialize decoder with a separate method to load from an existing stream
    sd_psdcStaticDecoder = new CSoundDecoder;
    // [Cecil] FIXME: If this is loaded from a physical file on disk, it will simply read it into memory instead of streaming it
    sd_psdcStaticDecoder->Initialize(fnm, inFile);

    // [Cecil] If sound decoder fails to initialize, it doesn't throw exceptions to allow the game to function without sound.
    // Failure to initialize and open also means that it wasn't loaded into memory, which results in this dynamically created
    // class being freed from memory and this sound data getting marked with SDF_STREAMING to let subsequent instances of
    // CSoundObject instances to try loading audio individually instead of crashing by reusing a decoder with no data.
    BOOL bGiveUpOnStaticDecoder = TRUE;

    if (sd_psdcStaticDecoder->IsOpen()) {
      sd_psdcStaticDecoder->GetFormat(sd_afFormat);

      // [Cecil] Remember length of the decoded sound
      sd_dSecondsLength = sd_psdcStaticDecoder->GetSoundLength();

      // [Cecil] If audio is loaded into memory, make CSoundObject reuse the static decoder
      // from this sound data instead of creating a personal decoder to stream audio from disk
      if (sd_psdcStaticDecoder->IsInMemory()) {
        // Mark as encoded audio and preserve this static decoder
        sd_ulFlags = SDF_ENCODED;
        bGiveUpOnStaticDecoder = FALSE;
      }
    }

    // [Cecil] Let CSoundObject instances stream audio using their own decoders
    if (bGiveUpOnStaticDecoder) {
      delete sd_psdcStaticDecoder;
      sd_psdcStaticDecoder = NULL;

      // Mark as streaming encoded audio
      sd_ulFlags = SDF_ENCODED | SDF_STREAMING;
    }

    // add to sound aware list
    _pSound->AddSoundAware(*this);
    return;
  }

  // [Cecil] Cached error message to avoid chained ThrowF_t() calls within catch blocks
  CTString strWavError;

  // if this is wave file
  try {
    // load wave info
    PCMWaveInput CpwiLoad;
    CpwiLoad.LoadInfo_t(inFile, sd_afFormat);
    // store sample length in seconds and average byte rate
    sd_dSecondsLength = CpwiLoad.GetSecondsLength();

    // if sound library is in lower format convert sound to library format
    if (_pSound->sl_afFormat.iSampleRate < sd_afFormat.iSampleRate) {
      sd_afFormat.iSampleRate = _pSound->sl_afFormat.iSampleRate;
    }
    // same goes for bits per sample (must be 16)
    sd_afFormat.iBytesPerSample = 2;

    // if library is active create buffer and load sound data
    if (_pSound->IsActive()) {
      // create Buffer
      sd_slBufferSampleSize = CpwiLoad.GetDataLength(sd_afFormat);
      SLONG slBufferSize = CpwiLoad.DetermineBufferSize(sd_afFormat);
      sd_pswBuffer = (SWORD*)AllocMemory( slBufferSize+8);
      // load data into buffer
      CpwiLoad.LoadData_t(inFile, sd_pswBuffer, sd_afFormat);
      // copy first sample to the last one (this is needed for linear interpolation)
      (ULONG&)(((UBYTE*)sd_pswBuffer)[slBufferSize]) = *(ULONG*)sd_pswBuffer;
    }

    // add to sound aware list
    _pSound->AddSoundAware(*this);
    return;

  // [Cecil] Avoid messing with the thrown message buffer by copying the error before another ThrowF_t()
  } catch (char *strError) {
    strWavError = strError;
  }

  // [Cecil] Report more detailed error
  ThrowF_t(TRANS("Cannot open audio '%s': %s"), fnm.ConstData(), strWavError.ConstData());
}


// Sound can't be written to file
void CSoundData::Write_t( CTStream *outFile)
{
  ASSERTALWAYS("Cannot write sounds!");
  throw TRANS("Cannot write sounds!");
}

/* Get the description of this object. */
CTString CSoundData::GetDescription(void)
{
  CTString str;
  str.PrintF("%dkHz %dbit %s %.2lfs",
    sd_afFormat.iSampleRate / 1000,
    sd_afFormat.iBytesPerSample * 8,
    sd_afFormat.iChannels == 1 ? "mono" : "stereo",
    sd_dSecondsLength);
  return str;
}


/* ====================================================
 *
 *  Class CLEAR method
 */

// Free memory allocated for sound and Release DXBuffer
void CSoundData::Clear(void)
{
  // synchronize access to sounds
  CTSingleLock slSounds(&_pSound->sl_csSound, TRUE);

  // clear BASE class
  CSerial::Clear();

  // free DXBuffer
  ClearBuffer();

  // if added as sound aware, remove it from sound aware list
  if(IsHooked()) {
    _pSound->RemoveSoundAware(*this);
  }
}


// check if this kind of objects is auto-freed
BOOL CSoundData::IsAutoFreed(void)
{
  return FALSE;
}


// get amount of memory used by this object
SLONG CSoundData::GetUsedMemory(void)
{
  SLONG slUsed = sizeof(*this);
  if( sd_pswBuffer!=NULL) {
    ASSERT(sd_afFormat.iChannels == 1 || sd_afFormat.iChannels == 2);
    slUsed += sd_slBufferSampleSize * sd_afFormat.GetBytesPerFrame();
  }

  // [Cecil] Decoder memory (total bytes of fully decoded audio)
  if (sd_psdcStaticDecoder != NULL) {
    slUsed += sd_psdcStaticDecoder->GetLengthInFrames() * sd_afFormat.GetBytesPerFrame();
  }

  return slUsed;
}


/* ====================================================
 *
 *  Reference counting functions
 */

// Add one reference
void CSoundData::AddReference(void)
{
  MarkUsed();
}


// Remove one reference
void CSoundData::RemReference(void)
{
  _pSoundStock->Release(this);
}
