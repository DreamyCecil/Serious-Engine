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

#if SE1_SND_SDLAUDIO

extern INDEX snd_iDevice;

// Write some audio data into a stream
void CSoundAPI_SDL::WriteAudioData(UBYTE *pubStream, size_t ctStreamSize) {
  ASSERT(m_pBackBuffer != NULL);
  ASSERT(m_ctBackBufferPos    <  m_ctBackBufferAlloc);
  ASSERT(m_ctBackBufferRemain <= m_ctBackBufferAlloc);

  // Stream length left
  size_t iLength = ctStreamSize;

  // How many bytes can actually be copied
  size_t ctToCopy = (iLength < (size_t)m_ctBackBufferRemain) ? iLength : (size_t)m_ctBackBufferRemain;

  // Cap at the end of the ring buffer
  const size_t ctBytesLeft = m_ctBackBufferAlloc - m_ctBackBufferPos;

  if (ctToCopy >= ctBytesLeft) {
    ctToCopy = ctBytesLeft;
  }

  if (ctToCopy != 0) {
    // Move the first block to the stream
    Uint8 *pSrc = m_pBackBuffer + m_ctBackBufferPos;
    memcpy(pubStream, pSrc, ctToCopy);

    ASSERT(m_ctBackBufferRemain >= ctToCopy);
    m_ctBackBufferRemain -= ctToCopy;

    ASSERT(iLength >= ctToCopy);
    iLength -= ctToCopy;

    pubStream += ctToCopy;
    m_ctBackBufferPos += ctToCopy;
  }

  // See if we need to rotate to the start of the ring buffer
  ASSERT(m_ctBackBufferPos <= m_ctBackBufferAlloc);

  if (m_ctBackBufferPos == m_ctBackBufferAlloc) {
    m_ctBackBufferPos = 0;

    // Feed more data to the stream
    if (iLength > 0) {
      ctToCopy = (iLength < (size_t)m_ctBackBufferRemain) ? iLength : (size_t)m_ctBackBufferRemain;

      if (ctToCopy != 0) {
        // Move the second block to the stream
        memcpy(pubStream, m_pBackBuffer, ctToCopy);

        m_ctBackBufferPos += ctToCopy;
        ASSERT(m_ctBackBufferPos < m_ctBackBufferAlloc);

        ASSERT(m_ctBackBufferRemain >= ctToCopy);
        m_ctBackBufferRemain -= ctToCopy;

        ASSERT(iLength >= ctToCopy);
        iLength -= ctToCopy;

        pubStream += ctToCopy;
      }
    }
  }

  // Fill the rest of the data with silence, if there's still some left
  if (iLength != 0) {
    ASSERT(m_ctBackBufferRemain == 0);
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
  const SEAudioFormat &af = _pSound->sl_afFormat;
  SDL_AudioFormat formatSet;

  if (af.iBytesPerSample <= 1) {
    formatSet = SDL_AUDIO_U8;

  } else if (af.iBytesPerSample <= 2) {
    formatSet = SDL_AUDIO_S16LE;

  } else if (af.iBytesPerSample <= 4) {
    formatSet = SDL_AUDIO_S32LE;

  } else {
    CPrintF(TRANS("Unsupported bits-per-sample: %u\n"), af.iBytesPerSample * 8);
    return FALSE;
  }

  // Determine the audio device to open
  SDL_AudioDeviceID iDeviceToOpen = SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK;
  CTString strDevice = TRANS("default");

  if (snd_iDevice >= 0) {
    int ctDevices;
    SDL_AudioDeviceID *aDevices = SDL_GetAudioPlaybackDevices(&ctDevices);

    if (aDevices != NULL) {
      int iDevice = ClampUp(snd_iDevice, ctDevices - 1);

      iDeviceToOpen = aDevices[iDevice];
      strDevice.PrintF("%d", iDevice);

      SDL_free(aDevices);
    }
  }

  const SDL_AudioSpec spec = { formatSet, (int)af.iChannels, (int)af.iSampleRate };
  m_pAudioStream = SDL_OpenAudioDeviceStream(iDeviceToOpen, &spec, &AudioCallback, this);

  if (m_pAudioStream == NULL) {
    CPrintF(TRANS("SDL_OpenAudioDeviceStream() error: %s\n"), SDL_GetError());
    return FALSE;
  }

  // Allocate mixer buffers
  AllocBuffers(TRUE);

  m_ubSilence = SDL_GetSilenceValueForFormat(formatSet);
  m_ctBackBufferAlloc = m_ctMixerBufferSize;
  m_pBackBuffer = (Uint8 *)AllocMemory(m_ctBackBufferAlloc);
  m_ctBackBufferRemain = 0;
  m_ctBackBufferPos = 0;

  // Report success
  if (bReport) {
    CPrintF(TRANS("  opened device (%s): %s\n"), strDevice.ConstData(), SDL_GetAudioDeviceName(SDL_GetAudioStreamDevice(m_pAudioStream)));
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

void CSoundAPI_SDL::CopyMixerBuffer(size_t ctMixedSize) {
  ASSERT(m_ctBackBufferAlloc - m_ctBackBufferRemain >= ctMixedSize);

  size_t ctFillPos = m_ctBackBufferPos + m_ctBackBufferRemain;

  if (ctFillPos > m_ctBackBufferAlloc) {
    ctFillPos -= m_ctBackBufferAlloc;
  }

  size_t ctCopyBytes = ctMixedSize;

  if (ctCopyBytes + ctFillPos > m_ctBackBufferAlloc) {
    ctCopyBytes = m_ctBackBufferAlloc - ctFillPos;
  }

  Uint8 *pBufferOffset = m_pBackBuffer + ctFillPos;
  CopyMixerBuffer_stereo(0, pBufferOffset, ctCopyBytes);

  ctMixedSize -= ctCopyBytes;
  m_ctBackBufferRemain += ctCopyBytes;

  // Rotate to the start of the ring buffer
  if (ctMixedSize != 0) {
    CopyMixerBuffer_stereo(ctCopyBytes, m_pBackBuffer, ctMixedSize);
    m_ctBackBufferRemain += ctMixedSize;
  }

  ASSERT(m_ctBackBufferRemain <= m_ctBackBufferAlloc);

  // PrepareSoundBuffer() was called prior to this
  SDL_UnlockAudioStream(m_pAudioStream);
};

size_t CSoundAPI_SDL::PrepareSoundBuffer(void) {
  SDL_LockAudioStream(m_pAudioStream);

  ASSERT(m_ctBackBufferRemain <= m_ctBackBufferAlloc);

  const size_t ctDataToMix = (m_ctBackBufferAlloc - m_ctBackBufferRemain);

  if (ctDataToMix == 0) {
    // It won't end up in CopyMixerBuffer() with 0 data, so unlock it again
    SDL_UnlockAudioStream(m_pAudioStream);
    return 0;
  }

  return ctDataToMix;
};

void CSoundAPI_SDL::Mute(BOOL &bSetSoundMuted) {
  SDL_LockAudioStream(m_pAudioStream);
  bSetSoundMuted = TRUE;

  // Ditch pending audio data
  m_ctBackBufferRemain = 0;
  m_ctBackBufferPos = 0;

  SDL_UnlockAudioStream(m_pAudioStream);
};

#endif // SE1_SND_SDLAUDIO
