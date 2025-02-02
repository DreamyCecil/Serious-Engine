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

// Write some audio data into a stream
void CSoundAPI_SDL::WriteAudioData(UBYTE *pubStream, SLONG slStreamSize) {
  ASSERT(m_pBackBuffer != NULL);
  ASSERT(m_slBackBufferPos    >= 0 && m_slBackBufferPos    <  m_slBackBufferAlloc);
  ASSERT(m_slBackBufferRemain >= 0 && m_slBackBufferRemain <= m_slBackBufferAlloc);

  // Stream length left
  SLONG iLength = slStreamSize;

  // How many bytes can actually be copied
  SLONG slToCopy = (iLength < m_slBackBufferRemain) ? iLength : (SLONG)m_slBackBufferRemain;

  // Cap at the end of the ring buffer
  const SLONG slBytesLeft = m_slBackBufferAlloc - m_slBackBufferPos;

  if (slToCopy >= slBytesLeft) {
    slToCopy = slBytesLeft;
  }

  if (slToCopy > 0) {
    // Move the first block to the stream
    Uint8 *pSrc = m_pBackBuffer + m_slBackBufferPos;
    memcpy(pubStream, pSrc, slToCopy);

    m_slBackBufferRemain -= slToCopy;
    ASSERT(m_slBackBufferRemain >= 0);

    iLength -= slToCopy;
    ASSERT(iLength >= 0);

    pubStream += slToCopy;
    m_slBackBufferPos += slToCopy;
  }

  // See if we need to rotate to the start of the ring buffer
  ASSERT(m_slBackBufferPos <= m_slBackBufferAlloc);

  if (m_slBackBufferPos == m_slBackBufferAlloc) {
    m_slBackBufferPos = 0;

    // Feed more data to the stream
    if (iLength > 0) {
      slToCopy = (iLength < m_slBackBufferRemain) ? iLength : (SLONG)m_slBackBufferRemain;

      if (slToCopy > 0) {
        // Move the second block to the stream
        memcpy(pubStream, m_pBackBuffer, slToCopy);

        m_slBackBufferPos += slToCopy;
        ASSERT(m_slBackBufferPos < m_slBackBufferAlloc);

        m_slBackBufferRemain -= slToCopy;
        ASSERT(m_slBackBufferRemain >= 0);

        iLength -= slToCopy;
        ASSERT(iLength >= 0);

        pubStream += slToCopy;
      }
    }
  }

  // Fill the rest of the data with silence, if there's still some left
  if (iLength > 0) {
    ASSERT(m_slBackBufferRemain == 0);
    memset(pubStream, m_ubSilence, iLength);
  }
};

static void AudioCallback(void *pUserData, SDL_AudioStream *pStream, int ctAdditional, int ctTotal) {
  if (ctAdditional <= 0) return;

  // Allocate some memory for the audio stream
  UBYTE *pubStream = SDL_stack_alloc(UBYTE, ctAdditional);
  if (pubStream == NULL) return;

  // Write prepared audio data from the interface into it
  CSoundAPI_SDL *pAPI = (CSoundAPI_SDL *)pUserData;
  pAPI->WriteAudioData(pubStream, ctAdditional);

  // And then queue it
  SDL_PutAudioStreamData(pStream, pubStream, ctAdditional);
  SDL_stack_free(pubStream);
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

  const SDL_AudioSpec spec = { formatSet, wfe.nChannels, wfe.nSamplesPerSec };
  m_pAudioStream = SDL_OpenAudioDeviceStream(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, &spec, &AudioCallback, this);

  if (m_pAudioStream == NULL) {
    CPrintF(TRANS("SDL_OpenAudioDeviceStream() error: %s\n"), SDL_GetError());
    return FALSE;
  }

  // Allocate mixer buffers
  AllocBuffers(TRUE);

  m_ubSilence = SDL_GetSilenceValueForFormat(formatSet);
  m_slBackBufferAlloc = m_slMixerBufferSize;
  m_pBackBuffer = (Uint8 *)AllocMemory(m_slBackBufferAlloc);
  m_slBackBufferRemain = 0;
  m_slBackBufferPos = 0;

  // Report success
  if (bReport) {
    CPrintF(TRANS("  opened device: %s\n"), SDL_GetAudioDeviceName(SDL_GetAudioStreamDevice(m_pAudioStream)));
    CPrintF(TRANS("  audio driver: %s\n"), SDL_GetCurrentAudioDriver());
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

  // Rotate to the start of the ring buffer
  if (slMixedSize > 0) {
    CopyMixerBuffer_stereo(slCopyBytes, m_pBackBuffer, slMixedSize);
    m_slBackBufferRemain += slMixedSize;
  }

  ASSERT(m_slBackBufferRemain <= m_slBackBufferAlloc);

  // PrepareSoundBuffer() was called prior to this
  SDL_UnlockAudioStream(m_pAudioStream);
};

SLONG CSoundAPI_SDL::PrepareSoundBuffer(void) {
  SDL_LockAudioStream(m_pAudioStream);

  ASSERT(m_slBackBufferRemain >= 0);
  ASSERT(m_slBackBufferRemain <= m_slBackBufferAlloc);

  const SLONG slDataToMix = (m_slBackBufferAlloc - m_slBackBufferRemain);

  if (slDataToMix <= 0) {
    // It won't end up in CopyMixerBuffer() with 0 data, so unlock it again
    SDL_UnlockAudioStream(m_pAudioStream);
    return 0;
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
