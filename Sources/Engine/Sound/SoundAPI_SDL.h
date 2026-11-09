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

#ifndef SE_INCL_SOUNDAPI_SDL_H
#define SE_INCL_SOUNDAPI_SDL_H

#include <Engine/Sound/SoundAPI.h>

// [Cecil] FIXME: You probably don't need SDL features that much if you're using VS2010 but this still sucks
#if SE1_INCOMPLETE_CPP11
  #define SE1_ATOMIC(_Type) _Type
#else
  #include <atomic>
  #define SE1_ATOMIC(_Type) std::atomic< _Type >
#endif

#if SE1_SND_SDLAUDIO

class CSoundAPI_SDL : public CAbstractSoundAPI {
  public:
    Uint8 m_ubSilence;
    size_t m_ctBackBufferAlloc;
    SE1_ATOMIC(Uint8 *) m_pBackBuffer;
    SE1_ATOMIC(size_t) m_ctBackBufferPos;
    SE1_ATOMIC(size_t) m_ctBackBufferRemain;
    SDL_AudioStream *m_pAudioStream;

  public:
    // Constructor
    CSoundAPI_SDL() : CAbstractSoundAPI() {
      m_ubSilence = 0;
      m_ctBackBufferAlloc = 0;
      m_pBackBuffer = NULL;
      m_ctBackBufferPos = 0;
      m_ctBackBufferRemain = 0;
      m_pAudioStream = NULL;
    };

    virtual ESoundAPI GetType(void) {
      return E_SND_SDL;
    };

  public:
    virtual BOOL StartUp(BOOL bReport);
    virtual void ShutDown(void);

    void WriteAudioData(UBYTE *pubStream, size_t ctStreamSize);

    virtual void CopyMixerBuffer(size_t ctMixedSize);
    virtual size_t PrepareSoundBuffer(void);
    virtual void Mute(BOOL &bSetSoundMuted);
};

#endif // SE1_SND_SDLAUDIO

#endif // include-once check
