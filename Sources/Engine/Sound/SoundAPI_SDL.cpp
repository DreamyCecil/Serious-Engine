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

extern INDEX snd_iDevice;
extern FLOAT snd_tmMixAhead;

static void AudioCallback(void *pUserData, Uint8 *pubStream, int iLength) {
  CSoundAPI_SDL *pAPI = (CSoundAPI_SDL *)pUserData;

  ASSERT(pAPI->m_pBackBuffer != NULL);
  ASSERT(pAPI->m_slBackBufferPos    >= 0 && pAPI->m_slBackBufferPos    <  pAPI->m_slBackBufferAlloc);
  ASSERT(pAPI->m_slBackBufferRemain >= 0 && pAPI->m_slBackBufferRemain <= pAPI->m_slBackBufferAlloc);

  // How many bytes can actually be copied
  SLONG slToCopy = (iLength < pAPI->m_slBackBufferRemain) ? iLength : pAPI->m_slBackBufferRemain;

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
      slToCopy = (iLength < pAPI->m_slBackBufferRemain) ? iLength : pAPI->m_slBackBufferRemain;

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

BOOL CSoundAPI_SDL::StartUp(BOOL bReport) {
  snd_iDevice = 0;

  if (bReport) CPrintF(TRANS("SDL audio initialization ...\n"));

  SDL_AudioSpec desired, obtained;
  SDL_zero(desired);
  SDL_zero(obtained);

  const WAVEFORMATEX &wfe = _pSound->sl_SwfeFormat;
  Sint16 iBPS = wfe.wBitsPerSample;

  if (iBPS <= 8) {
    desired.format = AUDIO_U8;

  } else if (iBPS <= 16) {
    desired.format = AUDIO_S16LSB;

  } else if (iBPS <= 32) {
    desired.format = AUDIO_S32LSB;

  } else {
    CPrintF(TRANS("Unsupported bits-per-sample: %d\n"), iBPS);
    return FALSE;
  }

  desired.freq = wfe.nSamplesPerSec;

  // [Cecil] TODO: Check if this is fine or a console variable should be used instead
  if (desired.freq <= 11025) {
    desired.samples = 512;

  } else if (desired.freq <= 22050) {
    desired.samples = 1024;

  } else if (desired.freq <= 44100) {
    desired.samples = 2048;

  } else {
    desired.samples = 4096;
  }

  desired.channels = wfe.nChannels;
  desired.userdata = this;
  desired.callback = AudioCallback;

  // [Cecil] NOTE: Use SDL_GetAudioDeviceName() here to select a specific audio device and report its name below
  const char *strDevice = NULL;
  const char *strDeviceName = (strDevice == NULL) ? "[system default playback device]" : strDevice;
  m_iAudioDevice = SDL_OpenAudioDevice(strDevice, 0, &desired, &obtained, 0);

  if (m_iAudioDevice == 0) {
    CPrintF(TRANS("SDL_OpenAudioDevice() error: %s\n"), SDL_GetError());
    return FALSE;
  }

  m_ubSilence = obtained.silence;
  m_slBackBufferAlloc = (obtained.size * 4);
  m_pBackBuffer = (Uint8 *)AllocMemory(m_slBackBufferAlloc);
  m_slBackBufferRemain = 0;
  m_slBackBufferPos = 0;

  // Report success
  if (bReport) {
    CPrintF(TRANS("  opened device: %s\n"), strDeviceName);
    CPrintF(TRANS("  %dHz, %dbit, %s\n"), wfe.nSamplesPerSec, wfe.wBitsPerSample, SDL_GetCurrentAudioDriver());
  }

  // Determine whole mixer buffer size
  m_slMixerBufferSize = CalculateMixerSize(wfe);

  // Align size to be next multiply of WAVEOUTBLOCKSIZE
  m_slMixerBufferSize += WAVEOUTBLOCKSIZE - (m_slMixerBufferSize % WAVEOUTBLOCKSIZE);
  m_slDecodeBufferSize = CalculateDecoderSize(wfe);

  if (bReport) {
    CPrintF(TRANS("  parameters: %d Hz, %d bit, stereo, mix-ahead: %gs\n"), wfe.nSamplesPerSec, wfe.wBitsPerSample, snd_tmMixAhead);
    CPrintF(TRANS("  output buffers: %d x %d bytes\n"), 2, obtained.size);
    CPrintF(TRANS("  mpx decode: %d bytes\n"), m_slDecodeBufferSize);
  }

  // Initialize mixing and decoding buffers
  AllocBuffers();

  // Audio callback can now safely fill the audio stream with silence until there is actual audio data to mix
  SDL_PauseAudioDevice(m_iAudioDevice, 0);
  return TRUE;
};

void CSoundAPI_SDL::ShutDown(void) {
  SDL_PauseAudioDevice(m_iAudioDevice, 1);

  if (m_pBackBuffer != NULL) {
    FreeMemory(m_pBackBuffer);
    m_pBackBuffer = NULL;
  }

  SDL_CloseAudioDevice(m_iAudioDevice);
  m_iAudioDevice = 0;

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
  SDL_UnlockAudioDevice(m_iAudioDevice);
};

SLONG CSoundAPI_SDL::PrepareSoundBuffer(void) {
  SDL_LockAudioDevice(m_iAudioDevice);

  ASSERT(m_slBackBufferRemain >= 0);
  ASSERT(m_slBackBufferRemain <= m_slBackBufferAlloc);

  SLONG slDataToMix = (m_slBackBufferAlloc - m_slBackBufferRemain);

  if (slDataToMix <= 0) {
    // It shouldn't end up in CopyMixerBuffer() with 0 data, so it needs to be done here
    SDL_UnlockAudioDevice(m_iAudioDevice);
  }

  return slDataToMix;
};

void CSoundAPI_SDL::Mute(BOOL &bSetSoundMuted) {
  SDL_LockAudioDevice(m_iAudioDevice);
  bSetSoundMuted = TRUE;

  // Ditch pending audio data
  m_slBackBufferRemain = 0;
  m_slBackBufferPos = 0;

  SDL_UnlockAudioDevice(m_iAudioDevice);
};

#endif // SE1_PREFER_SDL || SE1_SND_SDLAUDIO
