/* Copyright (c) 2025 Dreamy Cecil
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

#ifndef SE_INCL_SOUNDAPI_OPENAL_H
#define SE_INCL_SOUNDAPI_OPENAL_H

#include <Engine/Sound/SoundAPI.h>

#if SE1_SND_OPENAL

#include <AL/al.h>
#include <AL/alc.h>
#include <AL/efx-presets.h>

class CSoundAPI_OpenAL : public CAbstractSoundAPI {
  private:
    ALCdevice *m_pDevice;
    ALCcontext *m_pContext;

    ALuint m_iSource;
    CStaticArray<ALuint> m_aiBuffers; // Actual buffer storage
    CStaticStackArray<ALuint> m_aiFreeBuffers; // Buffers ready to be queued

    UBYTE *m_pubStreamBuffer; // Proxy buffer for moving data from the mixer and into the buffers

  #if SE1_OPENAL_EFX
    BOOL m_bUsingEFX;
    ALuint m_iEffectSlot;
    ALuint m_iEffect;
    BOOL m_bEAXReverb;

    INDEX m_iLastEnvType;
    FLOAT m_fLastEnvSize;
  #endif

  public:
    CSoundAPI_OpenAL() {
      m_pDevice = NULL;
      m_pContext = NULL;

      m_iSource = AL_NONE;
      m_pubStreamBuffer = NULL;

    #if SE1_OPENAL_EFX
      m_bUsingEFX = FALSE;
      m_iEffectSlot = AL_NONE;
      m_iEffect = AL_NONE;
      m_bEAXReverb = FALSE;
    #endif
    };

    virtual ESoundAPI GetType(void) {
      return E_SND_OPENAL;
    };

  public:
    virtual BOOL StartUp(BOOL bReport);
    virtual void ShutDown(void);

    virtual SLONG PrepareSoundBuffer(void);
    virtual void CopyMixerBuffer(SLONG slMixedSize);

  #if SE1_OPENAL_EFX
    virtual void Update(void);

  private:
    BOOL InitEFX(void);
    void SetEnvironment(INDEX iEnvType, FLOAT fEnvSize);
    void LoadEffect(const EFXEAXREVERBPROPERTIES *pReverb);
  #endif
};

#endif // SE1_SND_OPENAL

#endif // include-once check
