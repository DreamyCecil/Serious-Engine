/* Copyright (c) 2024 Dreamy Cecil
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

#ifndef SE_INCL_SOUND_INTERFACE_H
#define SE_INCL_SOUND_INTERFACE_H

// [Cecil] Needed here for EAX_ENVIRONMENT_* enum values
#include <Engine/Sound/eax_envtypes.h>

class ENGINE_API CAbstractSoundAPI {
  public:
    enum ESoundAPI {
      E_SND_INVALID = -1,

      // Windows-only APIs
    #if SE1_WIN
    #if SE1_SND_WAVEOUT
      E_SND_WAVEOUT,
    #endif // SE1_SND_WAVEOUT
    #if SE1_SND_DSOUND
      E_SND_DSOUND,
    #if SE1_SND_EAX
      E_SND_EAX,
    #endif // SE1_SND_EAX
    #endif // SE1_SND_DSOUND
    #endif // SE1_WIN

      // Cross-platform APIs
    #if SE1_SND_SDLAUDIO
      E_SND_SDL,
    #endif // SE1_SND_SDLAUDIO

    #if SE1_SND_OPENAL
      E_SND_OPENAL,
    #endif // SE1_SND_OPENAL

      // [Cecil] NOTE: This should always be at least 1 under any configuration
      E_SND_MAX,

      // Default API to use (last possible one)
      E_SND_DEFAULT = E_SND_MAX - 1,
    };

  public:
    // Amount of data that can be stored in a single audio buffer in a multi-buffer interface
    static const int ctBufferBlockSize;

    SLONG *m_pslMixerBuffer;     // buffer for mixing sounds (32-bit!)
    SLONG  m_slMixerBufferSize;  // mixer buffer size

    SWORD *m_pswDecodeBuffer;    // buffer for decoding encoded sounds (ogg, mpeg...)
    SLONG  m_slDecodeBufferSize; // decoder buffer size

  public:
    // Constructor
    CAbstractSoundAPI();

    // Destructor
    virtual ~CAbstractSoundAPI();

    // Calculate mixer buffer size
    SLONG CalculateMixerSize(void);

    // Calculate decoder buffer size (only after mixer size)
    SLONG CalculateDecoderSize(SLONG slMixerSize);

    // Allocate new buffer memory
    // Must always be called from interface's StartUp() method
    void AllocBuffers(BOOL bAlignToBlockSize);

    // Free buffer memory
    void FreeBuffers(void);

    // Report generic info about an interface
    void ReportGenericInfo(void);

    // Get API name from type
    static const CTString &GetApiName(ESoundAPI eAPI);

    // Create API from type
    static CAbstractSoundAPI *CreateAPI(ESoundAPI eAPI);

    // Sound interfaces should always return a distinct type
    virtual ESoundAPI GetType(void) {
      ASSERTALWAYS("Sound interface shouldn't return E_SND_INVALID as its type!");
      return E_SND_INVALID;
    };

  public:
    virtual BOOL StartUp(BOOL bReport) = 0;

    // Free memory by default
    virtual void ShutDown(void) {
      FreeBuffers();
    };

    // Determine how much space is available in the sound buffers for new samples
    virtual SLONG PrepareSoundBuffer(void) = 0;

    // Copy samples from the mixer buffer into the sound buffers
    virtual void CopyMixerBuffer(SLONG slMixedSize) = 0;

    // Assume that it will mute itself eventually by default
    virtual void Mute(BOOL &bSetSoundMuted) {};

    // Update API every frame
    virtual void Update(void) {};
};

#endif // include-once check
