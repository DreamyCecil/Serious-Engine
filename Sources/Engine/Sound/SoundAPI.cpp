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

#include <Engine/Sound/SoundAPI.h>

// Available sound interfaces
#include <Engine/Sound/SoundAPI_DSound.h>
#include <Engine/Sound/SoundAPI_OpenAL.h>
#include <Engine/Sound/SoundAPI_SDL.h>
#include <Engine/Sound/SoundAPI_WaveOut.h>

extern FLOAT snd_tmMixAhead;

// Amount of data that can be stored in a single audio buffer in a multi-buffer interface
const size_t CAbstractSoundAPI::ctBufferBlockSize = 1024;

CAbstractSoundAPI::CAbstractSoundAPI() {
  m_pslMixerBuffer = NULL;
  m_ctMixerBufferSize = 0;

  m_pswDecodeBuffer = NULL;
  m_ctDecodeBufferSize = 0;
};

CAbstractSoundAPI::~CAbstractSoundAPI() {
  ShutDown();
};

// Calculate mixer buffer size
size_t CAbstractSoundAPI::CalculateMixerSize(void) {
  const SEAudioFormat &af = _pSound->sl_afFormat;
  return size_t(ceil(snd_tmMixAhead * af.iSampleRate) * af.GetBytesPerFrame());
};

// Calculate decoder buffer size (only after mixer size)
size_t CAbstractSoundAPI::CalculateDecoderSize(size_t ctMixerSize) {
  // [Cecil] FIXME: If the sound decoder decodes audio with the sample rate higher than
  // CSoundLibrary::sl_ulMaxSampleRate, it may write data outside the buffer boundaries!

  // Decoder buffer always works at maximum sample rate
  const SEAudioFormat &af = _pSound->sl_afFormat;
  return ctMixerSize * ((CSoundLibrary::sl_ulMaxSampleRate + af.iSampleRate - 1) / af.iSampleRate);
};

// Allocate new buffer memory
void CAbstractSoundAPI::AllocBuffers(BOOL bAlignToBlockSize) {
  ASSERT(m_pslMixerBuffer == NULL);
  ASSERT(m_pswDecodeBuffer == NULL);

  // Determine initial buffer sizes
  m_ctMixerBufferSize = CalculateMixerSize();

  // Align mixer size to be the next multiple of the block size
  if (bAlignToBlockSize) {
    m_ctMixerBufferSize += ctBufferBlockSize - (m_ctMixerBufferSize % ctBufferBlockSize);
  }

  m_ctDecodeBufferSize = CalculateDecoderSize(m_ctMixerBufferSize);

  // Twice as much because of 32-bit buffer
  m_pslMixerBuffer = (SLONG *)AllocMemory(m_ctMixerBufferSize * 2);

  // [Cecil] FIXME: Don't just use 4, do it based on the audio setup
  // Right now it's 4 because: 16-bit samples * 2 channels (stereo) = 32 bits = 4 bytes per sample

  // +4 because of linear interpolation of last samples
  m_pswDecodeBuffer = (SWORD *)AllocMemory(m_ctDecodeBufferSize + 4);
};

// Free buffer memory
void CAbstractSoundAPI::FreeBuffers(void) {
  if (m_pslMixerBuffer != NULL) FreeMemory(m_pslMixerBuffer);
  m_pslMixerBuffer = NULL;
  m_ctMixerBufferSize = 0;

  if (m_pswDecodeBuffer != NULL) FreeMemory(m_pswDecodeBuffer);
  m_pswDecodeBuffer = NULL;
  m_ctDecodeBufferSize = 0;
};

// Report generic info about an interface
void CAbstractSoundAPI::ReportGenericInfo(void) {
  const SEAudioFormat &af = _pSound->sl_afFormat;
  CPrintF(TRANS("Sound interface info:\n"));

  CPrintF(TRANS("  %u Hz, %u bit, %u channels, mix-ahead: %gs\n"),
    af.iSampleRate, af.iBytesPerSample * 8, af.iChannels, snd_tmMixAhead);

  CPrintF(TRANS("  mixer buffer size:  %u bytes\n"), (ULONG)m_ctMixerBufferSize);
  CPrintF(TRANS("  decode buffer size: %u bytes\n"), (ULONG)m_ctDecodeBufferSize);
};

// Get API name from type
const CTString &CAbstractSoundAPI::GetApiName(CAbstractSoundAPI::ESoundAPI eAPI)
{
  // Invalid API
  if (eAPI < 0 || eAPI >= E_SND_MAX) {
    static const CTString strNone = "none";
    return strNone;
  }

  static const CTString astrApiNames[E_SND_MAX] = {
  #if SE1_WIN
  #if SE1_SND_WAVEOUT
    "WaveOut",
  #endif // SE1_SND_WAVEOUT
  #if SE1_SND_DSOUND
    "DirectSound",
  #if SE1_SND_EAX
    "EAX",
  #endif // SE1_SND_EAX
  #endif // SE1_SND_DSOUND
  #endif // SE1_WIN

  #if SE1_SND_SDLAUDIO
    "SDL Audio",
  #endif // SE1_SND_SDLAUDIO

  #if SE1_SND_OPENAL
    "OpenAL",
  #endif // SE1_SND_OPENAL
  };

  return astrApiNames[eAPI];
};

// Create API from type
CAbstractSoundAPI *CAbstractSoundAPI::CreateAPI(CAbstractSoundAPI::ESoundAPI eAPI) {
  switch (eAPI) {
  #if SE1_WIN
  #if SE1_SND_WAVEOUT
    case E_SND_WAVEOUT: return new CSoundAPI_WaveOut;
  #endif // SE1_SND_WAVEOUT
  #if SE1_SND_DSOUND
    case E_SND_DSOUND:  return new CSoundAPI_DSound;
  #if SE1_SND_EAX
    case E_SND_EAX:     return new CSoundAPI_DSound;
  #endif // SE1_SND_EAX
  #endif // SE1_SND_DSOUND
  #endif // SE1_WIN

  #if SE1_SND_SDLAUDIO
    case E_SND_SDL:     return new CSoundAPI_SDL;
  #endif // SE1_SND_SDLAUDIO

  #if SE1_SND_OPENAL
    case E_SND_OPENAL:  return new CSoundAPI_OpenAL;
  #endif // SE1_SND_OPENAL

    // Invalid API
    default: {
      ASSERT(NULL);
      return NULL;
    }
  }
};
