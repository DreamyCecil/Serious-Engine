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
#include <Engine/Sound/SoundAPI_SDL.h>
#include <Engine/Sound/SoundAPI_WaveOut.h>

extern FLOAT snd_tmMixAhead;

CAbstractSoundAPI::CAbstractSoundAPI() {
  m_pslMixerBuffer = NULL;
  m_slMixerBufferSize = 0;

  m_pswDecodeBuffer = NULL;
  m_slDecodeBufferSize = 0;
};

CAbstractSoundAPI::~CAbstractSoundAPI() {
  ShutDown();
};

// Calculate mixer buffer size
SLONG CAbstractSoundAPI::CalculateMixerSize(void) {
  const WAVEFORMATEX &wfe = _pSound->sl_SwfeFormat;
  return SLONG(ceil(snd_tmMixAhead * wfe.nSamplesPerSec) * wfe.wBitsPerSample / 8 * wfe.nChannels);
};

// Calculate decoder buffer size (only after mixer size)
SLONG CAbstractSoundAPI::CalculateDecoderSize(void) {
  // Decoder buffer always works at 44khz
  const WAVEFORMATEX &wfe = _pSound->sl_SwfeFormat;
  return m_slMixerBufferSize * ((44100 + wfe.nSamplesPerSec - 1) / wfe.nSamplesPerSec);
};

// Allocate new buffer memory
void CAbstractSoundAPI::AllocBuffers(BOOL bAlignToBlockSize) {
  ASSERT(m_pslMixerBuffer == NULL);
  ASSERT(m_pswDecodeBuffer == NULL);

  // Determine initial buffer sizes
  m_slMixerBufferSize = CalculateMixerSize();
  m_slDecodeBufferSize = CalculateDecoderSize();

  // Align mixer size to be the next multiple of WAVEOUTBLOCKSIZE
  if (bAlignToBlockSize) {
    m_slMixerBufferSize += WAVEOUTBLOCKSIZE - (m_slMixerBufferSize % WAVEOUTBLOCKSIZE);
  }

  // Twice as much because of 32-bit buffer
  m_pslMixerBuffer = (SLONG *)AllocMemory(m_slMixerBufferSize * 2);

  // +4 because of linear interpolation of last samples
  m_pswDecodeBuffer = (SWORD *)AllocMemory(m_slDecodeBufferSize + 4);
};

// Free buffer memory
void CAbstractSoundAPI::FreeBuffers(void) {
  if (m_pslMixerBuffer != NULL) FreeMemory(m_pslMixerBuffer);
  m_pslMixerBuffer = NULL;
  m_slMixerBufferSize = 0;

  if (m_pswDecodeBuffer != NULL) FreeMemory(m_pswDecodeBuffer);
  m_pswDecodeBuffer = NULL;
  m_slDecodeBufferSize = 0;
};

// Report generic info about an interface
void CAbstractSoundAPI::ReportGenericInfo(void) {
  const WAVEFORMATEX &wfe = _pSound->sl_SwfeFormat;
  CPrintF(TRANS("Sound interface info:\n"));

  CPrintF(TRANS("  %d Hz, %d bit, %d channels, mix-ahead: %gs\n"),
    wfe.nSamplesPerSec, wfe.wBitsPerSample, wfe.nChannels, snd_tmMixAhead);

  CPrintF(TRANS("  mixer buffer size:  %d bytes\n"), m_slMixerBufferSize);
  CPrintF(TRANS("  decode buffer size: %d bytes\n"), m_slDecodeBufferSize);
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

    // Invalid API
    default: {
      ASSERT(NULL);
      return NULL;
    }
  }
};
