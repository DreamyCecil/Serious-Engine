/* Copyright (c) 2002-2012 Croteam Ltd.
   Copyright (c) 2024 Dreamy Cecil
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

#include <Engine/Sound/SoundAPI_SDL.h>
#include <Engine/Sound/SoundLibrary.h>

#if SE1_PREFER_SDL || SE1_SND_SDLAUDIO

static void AudioCallback(void *pUserData, Uint8 *pubStream, int iLength) {
  CSoundAPI_SDL *pAPI = (CSoundAPI_SDL *)pUserData;

  ASSERT(pAPI->m_pBackBuffer != NULL);
  ASSERT(pAPI->m_slBackBufferPos    >= 0 && pAPI->m_slBackBufferPos    <  pAPI->m_slBackBufferAlloc);
  ASSERT(pAPI->m_slBackBufferRemain >= 0 && pAPI->m_slBackBufferRemain <= pAPI->m_slBackBufferAlloc);

  // How many bytes can actually be copied
  SLONG slToCopy = (iLength < pAPI->m_slBackBufferRemain) ? iLength : (SLONG)pAPI->m_slBackBufferRemain;

  // Cap at the end of the ring buffer
  const SLONG slBytesLeft = pAPI->m_slBackBufferAlloc - pAPI->m_slBackBufferPos;

  if (slToCopy >= slBytesLeft) {
    slToCopy = slBytesLeft;
  }

  if (slToCopy > 0) {
    // Move first block to SDL stream
    Uint8 *pSrc = pAPI->m_pBackBuffer + pAPI->m_slBackBufferPos;
    memcpy(pubStream, pSrc, slToCopy);

    pAPI->m_slBackBufferRemain -= slToCopy;
    ASSERT(pAPI->m_slBackBufferRemain >= 0);

    iLength -= slToCopy;
    ASSERT(iLength >= 0);

    pubStream += slToCopy;
    pAPI->m_slBackBufferPos += slToCopy;
  }

  // See if we need to rotate to start of ring buffer
  ASSERT(pAPI->m_slBackBufferPos <= pAPI->m_slBackBufferAlloc);

  if (pAPI->m_slBackBufferPos == pAPI->m_slBackBufferAlloc) {
    pAPI->m_slBackBufferPos = 0;

    // Might need to feed SDL more data now
    if (iLength > 0) {
      slToCopy = (iLength < pAPI->m_slBackBufferRemain) ? iLength : (SLONG)pAPI->m_slBackBufferRemain;

      if (slToCopy > 0) {
        // Move second block to SDL stream
        memcpy(pubStream, pAPI->m_pBackBuffer, slToCopy);

        pAPI->m_slBackBufferPos += slToCopy;
        ASSERT(pAPI->m_slBackBufferPos < pAPI->m_slBackBufferAlloc);

        pAPI->m_slBackBufferRemain -= slToCopy;
        ASSERT(pAPI->m_slBackBufferRemain >= 0);

        iLength -= slToCopy;
        ASSERT(iLength >= 0);

        pubStream += slToCopy;
      }
    }
  }

  // Fill the rest of the data with silence, if there's still some left
  if (iLength > 0) {
    ASSERT(pAPI->m_slBackBufferRemain == 0);
    memset(pubStream, pAPI->m_ubSilence, iLength);
  }
};

// [Cecil] FIXME: Get rid of this SDL3 wrapper for an SDL2 callback
void SDL3to2_Callback(void *pUserData, SDL_AudioStream *pStream, int ctAdditional, int ctTotal) {
  if (ctAdditional <= 0) return;

  Uint8 *pData = SDL_stack_alloc(Uint8, ctAdditional);

  if (pData != NULL) {
    AudioCallback(pUserData, pData, ctAdditional);
    SDL_PutAudioStreamData(pStream, pData, ctAdditional);
    SDL_stack_free(pData);
  }
};

BOOL CSoundAPI_SDL::StartUp(BOOL bReport) {
  const WAVEFORMATEX &wfe = _pSound->sl_SwfeFormat;
  Sint16 iBPS = wfe.wBitsPerSample;
  SDL_AudioFormat formatSet;

  if (iBPS <= 8) {
    formatSet = SDL_AUDIO_U8;

  } else if (iBPS <= 16) {
    formatSet = SDL_AUDIO_S16LE;

  } else if (iBPS <= 32) {
    formatSet = SDL_AUDIO_S32LE;

  } else {
    CPrintF(TRANS("Unsupported bits-per-sample: %d\n"), iBPS);
    return FALSE;
  }

  // [Cecil] NOTE: Sample count is needed for buffer size calculation below
  const SLONG iSpecFreq = wfe.nSamplesPerSec;
  SLONG iSpecSamples;

  if (iSpecFreq <= 11025) {
    iSpecSamples = 512;

  } else if (iSpecFreq <= 22050) {
    iSpecSamples = 1024;

  } else if (iSpecFreq <= 44100) {
    iSpecSamples = 2048;

  } else {
    iSpecSamples = 4096;
  }

  const SDL_AudioSpec spec = { formatSet, wfe.nChannels, iSpecFreq };
  m_pAudioStream = SDL_OpenAudioDeviceStream(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, &spec, &SDL3to2_Callback, this);

  if (m_pAudioStream == NULL) {
    CPrintF(TRANS("SDL_OpenAudioDeviceStream() error: %s\n"), SDL_GetError());
    return FALSE;
  }

  // Allocate mixer buffers
  AllocBuffers(TRUE);

  // Manually calculate buffer size as a replacement for SDL_AudioSpec::size from SDL2
  const SLONG iSpecSize = SDL_AUDIO_FRAMESIZE(spec) * iSpecSamples;

  m_ubSilence = SDL_GetSilenceValueForFormat(formatSet);
  m_slBackBufferAlloc = (iSpecSize * 4);
  m_pBackBuffer = (Uint8 *)AllocMemory(m_slBackBufferAlloc);
  m_slBackBufferRemain = 0;
  m_slBackBufferPos = 0;

  // Report success
  if (bReport) {
    CPrintF(TRANS("  opened device: %s\n"), SDL_GetAudioDeviceName(SDL_GetAudioStreamDevice(m_pAudioStream)));
    CPrintF(TRANS("  audio driver: %s\n"), SDL_GetCurrentAudioDriver());
    CPrintF(TRANS("  output buffers: %d x %d bytes\n"), 2, iSpecSize);
    ReportGenericInfo();
  }

  // Audio callback can now safely fill the audio stream with silence until there is actual audio data to mix
  SDL_ResumeAudioDevice(SDL_GetAudioStreamDevice(m_pAudioStream));
  return TRUE;
};

void CSoundAPI_SDL::ShutDown(void) {
  SDL_PauseAudioDevice(SDL_GetAudioStreamDevice(m_pAudioStream));

  if (m_pBackBuffer != NULL) {
    FreeMemory(m_pBackBuffer);
    m_pBackBuffer = NULL;
  }

  SDL_DestroyAudioStream(m_pAudioStream);
  m_pAudioStream = NULL;

  CAbstractSoundAPI::ShutDown();
};

void CSoundAPI_SDL::CopyMixerBuffer(SLONG slMixedSize) {
  ASSERT(m_slBackBufferAlloc - m_slBackBufferRemain >= slMixedSize);

  SLONG slFillPos = m_slBackBufferPos + m_slBackBufferRemain;

  if (slFillPos > m_slBackBufferAlloc) {
    slFillPos -= m_slBackBufferAlloc;
  }

  SLONG slCopyBytes = slMixedSize;

  if (slCopyBytes + slFillPos > m_slBackBufferAlloc) {
    slCopyBytes = m_slBackBufferAlloc - slFillPos;
  }

  Uint8 *pBufferOffset = m_pBackBuffer + slFillPos;
  CopyMixerBuffer_stereo(0, pBufferOffset, slCopyBytes);

  slMixedSize -= slCopyBytes;
  m_slBackBufferRemain += slCopyBytes;

  // Rotate to start of ring buffer
  if (slMixedSize > 0) {
    CopyMixerBuffer_stereo(slCopyBytes, m_pBackBuffer, slMixedSize);
    m_slBackBufferRemain += slMixedSize;
  }

  ASSERT(m_slBackBufferRemain <= m_slBackBufferAlloc);

  // PrepareSoundBuffer() needs to be called beforehand
  SDL_UnlockAudioStream(m_pAudioStream);
};

SLONG CSoundAPI_SDL::PrepareSoundBuffer(void) {
  SDL_LockAudioStream(m_pAudioStream);

  ASSERT(m_slBackBufferRemain >= 0);
  ASSERT(m_slBackBufferRemain <= m_slBackBufferAlloc);

  SLONG slDataToMix = (m_slBackBufferAlloc - m_slBackBufferRemain);

  if (slDataToMix <= 0) {
    // It shouldn't end up in CopyMixerBuffer() with 0 data, so it needs to be done here
    SDL_UnlockAudioStream(m_pAudioStream);
  }

  return slDataToMix;
};

void CSoundAPI_SDL::Mute(BOOL &bSetSoundMuted) {
  SDL_LockAudioStream(m_pAudioStream);
  bSetSoundMuted = TRUE;

  // Ditch pending audio data
  m_slBackBufferRemain = 0;
  m_slBackBufferPos = 0;

  SDL_UnlockAudioStream(m_pAudioStream);
};

#endif // SE1_PREFER_SDL || SE1_SND_SDLAUDIO
