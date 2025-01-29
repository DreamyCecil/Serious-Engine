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

#ifndef SE_INCL_SOUNDAPI_WAVEOUT_H
#define SE_INCL_SOUNDAPI_WAVEOUT_H

#include <Engine/Sound/SoundAPI.h>

#if SE1_WIN && SE1_SND_WAVEOUT

class CSoundAPI_WaveOut : public CAbstractSoundAPI {
  public:
    HWAVEOUT m_hwoWaveOut; // WaveOut handle
    CStaticStackArray<HWAVEOUT> m_ahwoExtra; // Preventively taken channels

    UBYTE *m_pubBuffersMemory; // Memory allocated for the sound buffer(s) output
    CStaticArray<WAVEHDR> m_awhWOBuffers; // WaveOut buffers

  public:
    // Constructor
    CSoundAPI_WaveOut() : CAbstractSoundAPI() {
      m_hwoWaveOut = NULL;
      m_pubBuffersMemory = NULL;
    };

    virtual ESoundAPI GetType(void) {
      return E_SND_WAVEOUT;
    };

  public:
    virtual BOOL StartUp(BOOL bReport);
    virtual void ShutDown(void);

    virtual void CopyMixerBuffer(SLONG slMixedSize);
    virtual SLONG PrepareSoundBuffer(void);
};

#endif // SE1_WIN && SE1_SND_WAVEOUT

#endif // include-once check
