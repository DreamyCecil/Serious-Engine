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

#include <Engine/Sound/SoundAPI_WaveOut.h>
#include <Engine/Sound/SoundLibrary.h>

#if SE1_WIN && SE1_SND_WAVEOUT

extern INDEX snd_iDevice;
extern INDEX snd_iMaxOpenRetries;
extern INDEX snd_iMaxExtraChannels;
extern FLOAT snd_tmOpenFailDelay;

BOOL CSoundAPI_WaveOut::StartUp(BOOL bReport) {
  static INDEX _ctChannelsOpened;
  _ctChannelsOpened = 0;

  const WAVEFORMATEX &wfe = _pSound->sl_SwfeFormat;

  // Set maximum total number of retries for device opening
  INDEX ctMaxRetries = snd_iMaxOpenRetries;
  MMRESULT res;

  FOREVER {
    // Try to open wave device
    HWAVEOUT hwo;
    res = waveOutOpen(&hwo, (snd_iDevice < 0) ? WAVE_MAPPER : snd_iDevice, &wfe, NULL, NULL, NONE);

    if (res == MMSYSERR_NOERROR) {
      _ctChannelsOpened++;

      // Remember as used WaveOut on the first one
      if (_ctChannelsOpened == 1) {
        m_hwoWaveOut = hwo;

      // Remember extra channels
      } else {
        m_ahwoExtra.Push() = hwo;
      }

      // No more tries if no extra channels should be taken
      if (_ctChannelsOpened >= snd_iMaxExtraChannels + 1) {
        break;
      }

    // If cannot open
    } else {
      // Decrement retry counter
      ctMaxRetries--;

      // No more retries
      if (ctMaxRetries < 0) {
        break;
      }

      // Wait a bit (probably sound-scheme is playing)
      _pTimer->Suspend(ULONG(snd_tmOpenFailDelay * 1000.0f));
    }
  }

  // If couldn't set format
  if (_ctChannelsOpened == 0 && res != MMSYSERR_NOERROR) {
    // Report error
    CTString strError;

    switch (res) {
      case MMSYSERR_ALLOCATED:   strError = TRANS("Device already in use.");     break;
      case MMSYSERR_BADDEVICEID: strError = TRANS("Bad device number.");         break;
      case MMSYSERR_NODRIVER:    strError = TRANS("No driver installed.");       break;
      case MMSYSERR_NOMEM:       strError = TRANS("Memory allocation problem."); break;
      case WAVERR_BADFORMAT:     strError = TRANS("Unsupported data format.");   break;
      case WAVERR_SYNC:          strError = TRANS("Wrong flag?");                break;
      default: strError.PrintF("%d", res);
    };

    CPrintF(TRANS("  ! WaveOut error: %s\n"), strError.ConstData());
    return FALSE;
  }

  // Allocate mixer buffers
  AllocBuffers(TRUE);

  // Get WaveOut capabilities
  WAVEOUTCAPS woc;
  memset(&woc, 0, sizeof(woc));

  res = waveOutGetDevCaps((UINT_PTR)m_hwoWaveOut, &woc, sizeof(woc));

  // Determine number of WaveOut buffers
  const INDEX ctWOBuffers = m_slMixerBufferSize / WAVEOUTBLOCKSIZE;

  // Report success
  if (bReport) {
    CTString strDevice = TRANS("default");
    if (snd_iDevice >= 0) strDevice.PrintF("%d", snd_iDevice);

    CPrintF(TRANS("  opened device (%s): %s\n"), strDevice.ConstData(), woc.szPname);
    CPrintF(TRANS("  output buffers: %d x %d bytes\n"), ctWOBuffers, WAVEOUTBLOCKSIZE);
    CPrintF(TRANS("  extra sound channels taken: %d\n"), _ctChannelsOpened-1);
    ReportGenericInfo();
  }

  // Initialize WaveOut sound buffers
  m_pubBuffersMemory = (UBYTE *)AllocMemory(m_slMixerBufferSize);
  memset(m_pubBuffersMemory, 0, m_slMixerBufferSize);

  m_awhWOBuffers.New(ctWOBuffers);

  for (INDEX iBuffer = 0; iBuffer < m_awhWOBuffers.Count(); iBuffer++)
  {
    WAVEHDR &wh = m_awhWOBuffers[iBuffer];
    wh.lpData = (char *)(m_pubBuffersMemory + iBuffer * WAVEOUTBLOCKSIZE);
    wh.dwBufferLength = WAVEOUTBLOCKSIZE;
    wh.dwFlags = 0;
  }

  return TRUE;
};

void CSoundAPI_WaveOut::ShutDown(void) {
  MMRESULT res;
  INDEX ct;

  // Shut down WaveOut player buffers, if needed
  if (m_hwoWaveOut != NULL) {
    // Reset WaveOut play buffers (stop playing)
    res = waveOutReset(m_hwoWaveOut);
    ASSERT(res == MMSYSERR_NOERROR);

    // Clear buffers
    CStaticArray<WAVEHDR> &aBuffers = m_awhWOBuffers;
    ct = aBuffers.Count();

    for (INDEX iBuffer = 0; iBuffer < ct; iBuffer++) {
      res = waveOutUnprepareHeader(m_hwoWaveOut, &aBuffers[iBuffer], sizeof(aBuffers[iBuffer]));
      ASSERT(res == MMSYSERR_NOERROR);
    }

    aBuffers.Clear();

    // Close WaveOut device
    res = waveOutClose(m_hwoWaveOut);
    ASSERT(res == MMSYSERR_NOERROR);
    m_hwoWaveOut = NULL;
  }

  // Close device of each extra channel
  ct = m_ahwoExtra.Count();

  for (INDEX iChannel = 0; iChannel < ct; iChannel++) {
    res = waveOutClose(m_ahwoExtra[iChannel]);
    ASSERT(res == MMSYSERR_NOERROR);
  }

  // Free extra channel handles
  m_ahwoExtra.PopAll();

  // Free memory
  if (m_pubBuffersMemory != NULL) {
    FreeMemory(m_pubBuffersMemory);
    m_pubBuffersMemory = NULL;
  }

  CAbstractSoundAPI::ShutDown();
};

void CSoundAPI_WaveOut::CopyMixerBuffer(SLONG slMixedSize) {
  MMRESULT res;
  SLONG slOffset = 0;
  const INDEX ctBuffers = m_awhWOBuffers.Count();

  for (INDEX iBuffer = 0; iBuffer < ctBuffers; iBuffer++) {
    WAVEHDR &wh = m_awhWOBuffers[iBuffer];

    // Skip prepared buffers
    if (wh.dwFlags & WHDR_PREPARED) continue;

    // Copy part of a mixer buffer to wave buffer
    CopyMixerBuffer_stereo(slOffset, wh.lpData, WAVEOUTBLOCKSIZE);
    slOffset += WAVEOUTBLOCKSIZE;

    // Write wave buffer (ready for playing)
    res = waveOutPrepareHeader(m_hwoWaveOut, &wh, sizeof(wh));
    ASSERT(res == MMSYSERR_NOERROR);

    res = waveOutWrite(m_hwoWaveOut, &wh, sizeof(wh));
    ASSERT(res == MMSYSERR_NOERROR);
  }
};

SLONG CSoundAPI_WaveOut::PrepareSoundBuffer(void) {
  // Scan WaveOut buffers to find all that are ready to receive sound data (i.e. not playing)
  SLONG slDataToMix = 0;
  const INDEX ctBuffers = m_awhWOBuffers.Count();

  for (INDEX iBuffer = 0; iBuffer < ctBuffers; iBuffer++) {
    WAVEHDR &wh = m_awhWOBuffers[iBuffer];

    // Unprepare buffer, if done playing
    if (wh.dwFlags & WHDR_DONE) {
      MMRESULT res = waveOutUnprepareHeader(m_hwoWaveOut, &wh, sizeof(wh));
      ASSERT(res == MMSYSERR_NOERROR);
    }

    // Increase mix-in data size, if unprepared
    if (!(wh.dwFlags & WHDR_PREPARED)) {
      slDataToMix += WAVEOUTBLOCKSIZE;
    }
  }

  // Done
  ASSERT(slDataToMix <= m_slMixerBufferSize);
  return slDataToMix;
};

#endif // SE1_WIN && SE1_SND_WAVEOUT
