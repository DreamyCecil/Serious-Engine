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
  sd_psdcDecoder = NULL;
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
  if (sd_psdcDecoder != NULL) {
    delete sd_psdcDecoder;
    sd_psdcDecoder = NULL;
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

  // [Cecil] Get full path to the audio and its extension
  const CTString &fnm = inFile->GetDescription();
  const CTString strExt = fnm.FileExt();

  // [Cecil] Completely removed the old PCM Wave decoder and replaced it with miniaudio-powered decoder of WAV, OGG, MP3 & FLAC
  // This flag determines which formats should always be preloaded into memory for immediate access (e.g. sound effects as WAV)
  // [Cecil] FIXME: Forcing preloading of MP3 because of an issue described in a comment in CSoundDecoder::SeekToFrame()
  const bool bPreloadFormat = (strExt == ".wav" || strExt == ".mp3");

  // [Cecil] Initialize decoder with a separate method to load from an existing stream
  CSoundDecoder *psdc = new CSoundDecoder;
  psdc->Initialize(inFile, false); // Will only load it into memory if it can't be streamed

  // [Cecil] If decoder fails to initialize, it doesn't throw exceptions to allow the game to function without sound
  if (!psdc->IsOpen()) {
    const CTString strError = psdc->GetError();
    delete psdc;

    // [Cecil] TODO: May wanna always throw to make sure the game can always open required sound effects and music on levels
    // [Cecil] If audio has to be preloaded, throw an error
    if (bPreloadFormat) {
      ThrowF_t(TRANS("Cannot open audio '%s': %s"), fnm.ConstData(), strError.ConstData());
    } else {
      CPrintF(TRANS("Cannot open audio '%s': %s\n"), fnm.ConstData(), strError.ConstData());
    }

    // Sound mixer won't attempt to play encoded sounds if there's no decoder
    sd_ulFlags = SDF_ENCODED;

    // Add to sound aware list
    _pSound->AddSoundAware(*this);
    return;
  }

  // [Cecil] Retrieve its audio format and length of the encoded audio
  psdc->GetFormat(sd_afFormat);
  sd_dSecondsLength = psdc->GetSoundLength();

  // [Cecil] If audio needs to be preloaded or it's short enough, decode all of it into memory immediately
  if (bPreloadFormat || sd_dSecondsLength < 30.0) {
    // Create buffer for holding all decoded frames
    sd_slBufferSampleSize = psdc->GetLengthInFrames();
    const ULONG ulFrameSize = sd_afFormat.GetBytesPerFrame();
    const size_t ctBufferSize = sd_slBufferSampleSize * ulFrameSize;
    sd_pswBuffer = (SWORD *)AllocMemory(ctBufferSize + ulFrameSize * 2); // Two extra frames for interpolation

    // Decode all audio frames and copy the first frame to the end for interpolation
    psdc->DecodeAll(sd_pswBuffer);
    memcpy(((UBYTE *)sd_pswBuffer) + ctBufferSize, sd_pswBuffer, ulFrameSize);

    delete psdc; // Not needed anymore

  // [Cecil] Otherwise remember the decoder to let CSoundObject instances use it
  } else {
    sd_ulFlags = SDF_ENCODED;
    sd_psdcDecoder = psdc;
  }

  // Add to sound aware list
  _pSound->AddSoundAware(*this);
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
  if (sd_psdcDecoder != NULL) {
    slUsed += sd_psdcDecoder->GetLengthInFrames() * sd_afFormat.GetBytesPerFrame();
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
