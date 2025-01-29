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

#ifndef SE_INCL_SOUNDAPI_DSOUND_H
#define SE_INCL_SOUNDAPI_DSOUND_H

#include <Engine/Sound/SoundAPI.h>

#if SE1_WIN && SE1_SND_DSOUND

#if !SE1_OLD_COMPILER
  #include <initguid.h>
#endif

#include <Engine/Sound/DSound.h>

class CSoundAPI_DSound : public CAbstractSoundAPI {
  public:
    HINSTANCE m_hDSoundLib;
    OS::Window m_wndCurrent;

    LPDIRECTSOUND           m_pDS;           // DirectSound handle
    LPDIRECTSOUNDBUFFER     m_pDSPrimary;
    LPDIRECTSOUNDBUFFER     m_pDSSecondary;  // 2D usage

  #if SE1_SND_EAX
    BOOL m_bUsingEAX;

    LPKSPROPERTYSET         m_pKSProperty;   // EAX properties
    LPDIRECTSOUNDBUFFER     m_pDSSecondary2;
    LPDIRECTSOUND3DLISTENER m_pDSListener;   // 3D EAX
    LPDIRECTSOUND3DBUFFER   m_pDSSourceLeft;
    LPDIRECTSOUND3DBUFFER   m_pDSSourceRight;
  #endif

    INDEX m_iWriteOffset;
    INDEX m_iWriteOffset2;
    INDEX m_iLastEnvType;
    FLOAT m_fLastEnvSize;
    FLOAT m_fLastPanning;

  public:
    // Constructor
    CSoundAPI_DSound() : CAbstractSoundAPI() {
      m_hDSoundLib = NULL;
      m_wndCurrent = NULL;

      m_pDS            = NULL;
      m_pDSPrimary     = NULL;
      m_pDSSecondary   = NULL;

    #if SE1_SND_EAX
      m_bUsingEAX = FALSE;

      m_pKSProperty    = NULL;
      m_pDSSecondary2  = NULL;
      m_pDSListener    = NULL;
      m_pDSSourceLeft  = NULL;
      m_pDSSourceRight = NULL;
    #endif

      m_iWriteOffset  = 0;
      m_iWriteOffset2 = 0;
      m_iLastEnvType = 1234;
      m_fLastEnvSize = 1234;
      m_fLastPanning = 1234;
    };

    virtual ESoundAPI GetType(void) {
      return E_SND_DSOUND;
    };

  public:
    BOOL Fail(const char *strError);
    BOOL InitSecondary(LPDIRECTSOUNDBUFFER &pBuffer, SLONG slSize);
    BOOL LockBuffer(LPDIRECTSOUNDBUFFER pBuffer, SLONG slSize, LPVOID &lpData, DWORD &dwSize);
    void PlayBuffers(void);

    // Set listener enviroment properties for EAX
    BOOL SetEnvironment(INDEX iEnvNo, FLOAT fEnvSize);

  public:
    virtual BOOL StartUp(BOOL bReport);
    virtual void ShutDown(void);

    virtual void CopyMixerBuffer(SLONG slMixedSize);
    virtual SLONG PrepareSoundBuffer(void);

    virtual void Mute(BOOL &bSetSoundMuted);
    virtual void UpdateEAX(void);
};

#endif // SE1_WIN && SE1_SND_DSOUND

#endif // include-once check
