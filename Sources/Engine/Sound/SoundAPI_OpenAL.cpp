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

#include "StdH.h"

#include <Engine/Sound/SoundAPI_OpenAL.h>
#include <Engine/Sound/SoundLibrary.h>

#if SE1_SND_OPENAL

#include <AL/alext.h>
#include <AL/efx.h>

#pragma comment(lib, "OpenAL32.lib")

#if SE1_OPENAL_EFX

// EFX function pointers
static LPALGENEFFECTS alGenEffects = NULL;
static LPALDELETEEFFECTS alDeleteEffects = NULL;
static LPALEFFECTI alEffecti = NULL;
static LPALEFFECTF alEffectf = NULL;
static LPALEFFECTFV alEffectfv = NULL;
static LPALGENAUXILIARYEFFECTSLOTS alGenAuxiliaryEffectSlots = NULL;
static LPALDELETEAUXILIARYEFFECTSLOTS alDeleteAuxiliaryEffectSlots = NULL;
static LPALAUXILIARYEFFECTSLOTI alAuxiliaryEffectSloti = NULL;

#endif // SE1_OPENAL_EFX

// Error checking
static BOOL OAL_CheckError(const char *strErrorMsg = "Error") {
  ALenum err = alGetError();

  if (err != AL_NO_ERROR) {
    CTString strError(0, "[OpenAL] %s: %s\n", strErrorMsg, alGetString(err));
    CPutString(strError.ConstData());
    ASSERTALWAYS("OpenAL Error!");

    // Error occurred
    return TRUE;
  }

  // No error
  return FALSE;
};

// Parse a list of strings returned by OpenAL
static void OAL_ParseList(CStaticStackArray<CTString> &aStrings, const ALCchar *aList) {
  if (aList == NULL) return;

  const ALCchar *strEntry = aList;

  // As long as it's not an empty entry
  while (strEntry[0] != '\0') {
    // Add it and move to the next one past the null terminator
    aStrings.Add(strEntry);
    strEntry += strlen(strEntry) + 1;
  }
};

BOOL CSoundAPI_OpenAL::StartUp(BOOL bReport) {
  const WAVEFORMATEX &wfe = _pSound->sl_SwfeFormat;

  // Create OpenAL device
  m_pDevice = alcOpenDevice(NULL);

  if (m_pDevice == NULL) {
    OAL_CheckError(TRANS("Cannot create OpenAL device"));
    return FALSE;
  }

  // Specify context attributes
  const ALCint aAttributes[] = {
    ALC_FREQUENCY, (ALCint)wfe.nSamplesPerSec,
    0
  };

  // Create and set OpenAL context
  m_pContext = alcCreateContext(m_pDevice, aAttributes);

  if (m_pContext == NULL) {
    OAL_CheckError(TRANS("Cannot create OpenAL context"));
    alcCloseDevice(m_pDevice);
    return FALSE;
  }

  if (!alcMakeContextCurrent(m_pContext)) {
    OAL_CheckError(TRANS("Cannot set OpenAL context"));
    alcCloseDevice(m_pDevice);
    return FALSE;
  }

  // Setup the listener
  const ALfloat aOrientation[] = {
    0.0f, 0.0f, 1.0f, // Front vector
    0.0f, 1.0f, 0.0f, // Up vector
  };

  alListener3f(AL_POSITION, 0, 0, 0);
  alListener3f(AL_VELOCITY, 0, 0, 0);
  alListenerfv(AL_ORIENTATION, aOrientation);
  OAL_CheckError();

  // Setup the audio source
  alGenSources(1, &m_iSource);
  OAL_CheckError(TRANS("Cannot generate a source"));

  // Set default parameters
  alSourcef(m_iSource, AL_PITCH, 1);
  alSourcef(m_iSource, AL_GAIN, 1);
  alSource3f(m_iSource, AL_POSITION, 0, 0, 0);
  alSource3f(m_iSource, AL_VELOCITY, 0, 0, 0);
  alSourcei(m_iSource, AL_LOOPING, AL_FALSE);
  OAL_CheckError();

#if SE1_OPENAL_EFX
  // Check for EFX support and load extension functions
  m_bUsingEFX = InitEFX();

  // Reset reverb parameters
  m_iLastEnvType = -999;
  m_fLastEnvSize = -999;

  if (bReport) {
    CPrintF(TRANS("  EFX: %s\n"), m_bUsingEFX ? TRANS("Enabled") : TRANS("Disabled"));
  }
#endif

  // Allocate mixer buffers
  AllocBuffers(TRUE);

  // Determine number of OpenAL buffers
  const INDEX ctBuffers = m_slMixerBufferSize / ctBufferBlockSize;

  if (bReport) {
    CPrintF(TRANS("  output buffers: %d x %d bytes\n"), ctBuffers, ctBufferBlockSize);
    ReportGenericInfo();
  }

  // Setup the audio buffers
  m_aiBuffers.New(ctBuffers);
  alGenBuffers(m_aiBuffers.Count(), m_aiBuffers.sa_Array);
  OAL_CheckError(TRANS("Cannot generate buffers"));

  // All buffers can be queued from the beginning
  FOREACHINSTATICARRAY(m_aiBuffers, ALuint, it) {
    m_aiFreeBuffers.Add(*it);
  }

  // Allocate proxy buffer
  m_pubStreamBuffer = (UBYTE *)AllocMemory(ctBufferBlockSize);

  return TRUE;
};

void CSoundAPI_OpenAL::ShutDown(void) {
#if SE1_OPENAL_EFX
  if (m_bUsingEFX) {
    if (m_iEffect != AL_NONE) {
      alDeleteEffects(1, &m_iEffect);
      m_iEffect = AL_NONE;
    }

    if (m_iEffectSlot != AL_NONE) {
      alDeleteAuxiliaryEffectSlots(1, &m_iEffectSlot);
      m_iEffectSlot = AL_NONE;
    }

    m_bEAXReverb = FALSE;
  }
#endif

  if (m_pubStreamBuffer != NULL) {
    FreeMemory(m_pubStreamBuffer);
    m_pubStreamBuffer = NULL;
  }

  const INDEX ctBuffers = m_aiBuffers.Count();

  if (ctBuffers != 0) {
    alDeleteBuffers(ctBuffers, m_aiBuffers.sa_Array);
    m_aiBuffers.Delete();
  }

  if (m_iSource != AL_NONE) {
    alSourceStop(m_iSource);
    alDeleteSources(1, &m_iSource);
    m_iSource = AL_NONE;
  }

  if (m_pContext != NULL) {
    alcMakeContextCurrent(NULL);
    alcDestroyContext(m_pContext);
    m_pContext = NULL;
  }

  if (m_pDevice != NULL) {
    alcCloseDevice(m_pDevice);
    m_pDevice = NULL;
  }

  CAbstractSoundAPI::ShutDown();
};

SLONG CSoundAPI_OpenAL::PrepareSoundBuffer(void) {
  // Unqueue all processed buffers that are ready to receive new data
  ALint iProcessed;
  alGetSourcei(m_iSource, AL_BUFFERS_PROCESSED, &iProcessed);
  OAL_CheckError();

  while (iProcessed--) {
    ALuint &iBuffer = m_aiFreeBuffers.Push();
    alSourceUnqueueBuffers(m_iSource, 1, &iBuffer);
    OAL_CheckError();
  }

  // Mix as much data as current free buffers can handle
  const INDEX slDataToMix = m_aiFreeBuffers.Count() * ctBufferBlockSize;

  ASSERT(slDataToMix <= m_slMixerBufferSize);
  return slDataToMix;
};

void CSoundAPI_OpenAL::CopyMixerBuffer(SLONG slMixedSize) {
  // Must have at least one free buffer to play sound through
  ASSERT(m_aiFreeBuffers.Count() != 0);

  SLONG slOffset = 0;

  while (m_aiFreeBuffers.Count() != 0) {
    ALuint iBuffer = m_aiFreeBuffers.Pop();

    // Copy mixer data into the proxy buffer
    CopyMixerBuffer_stereo(slOffset, m_pubStreamBuffer, ctBufferBlockSize);
    slOffset += ctBufferBlockSize;

    // Queue this buffer for playing
    alBufferData(iBuffer, AL_FORMAT_STEREO16, m_pubStreamBuffer, ctBufferBlockSize, _pSound->sl_SwfeFormat.nSamplesPerSec);
    OAL_CheckError();

    alSourceQueueBuffers(m_iSource, 1, &iBuffer);
    OAL_CheckError();
  }

  // Play queued buffers immediately
  ALint iSourceState;
  alGetSourcei(m_iSource, AL_SOURCE_STATE, &iSourceState);

  if (iSourceState != AL_PLAYING) {
    alSourcePlay(m_iSource);
  }
};

#if SE1_OPENAL_EFX

void CSoundAPI_OpenAL::Update(void) {
  if (!m_bUsingEFX) return;

  // Determine number of listeners and get a listener
  INDEX ctListeners = 0;
  CSoundListener *pListener = NULL;

  {FOREACHINLIST(CSoundListener, sli_lnInActiveListeners, _pSound->sl_lhActiveListeners, itsli) {
    pListener = itsli;
    ctListeners++;
  }}

  // Change environment properties for one listener (EAX isn't supported in split-screen)
  if (ctListeners == 1) {
    SetEnvironment(pListener->sli_iEnvironmentType, pListener->sli_fEnvironmentSize);

  // Reset environment properties for no listeners
  } else if (ctListeners < 1) {
    SetEnvironment(-1, -1);
  }
};

// Helper method for retrieving OpenAL functions
template<class FuncPtr> inline
BOOL GetOALFunction(ALCdevice *pDevice, FuncPtr &pFunc, const char *strFunc) {
  pFunc = (FuncPtr)alcGetProcAddress(pDevice, strFunc);

  if (pFunc == NULL) {
    CPrintF(TRANS("Cannot find '%s' function in the OpenAL module!\n"), strFunc);
    return FALSE;
  }

  return TRUE;
};

BOOL CSoundAPI_OpenAL::InitEFX(void) {
  // No EFX extension
  if (!alcIsExtensionPresent(m_pDevice, "ALC_EXT_EFX")) return FALSE;

  // Macro for retrieving required function by variable
  // If at least one function fails, EFX is disabled
  #define GET_OAL_FUNC(_Func) if (!GetOALFunction(m_pDevice, _Func, #_Func)) return FALSE

  // Retrieve required functions
  GET_OAL_FUNC(alGenEffects);
  GET_OAL_FUNC(alDeleteEffects);
  GET_OAL_FUNC(alEffecti);
  GET_OAL_FUNC(alEffectf);
  GET_OAL_FUNC(alEffectfv);
  GET_OAL_FUNC(alGenAuxiliaryEffectSlots);
  GET_OAL_FUNC(alDeleteAuxiliaryEffectSlots);
  GET_OAL_FUNC(alAuxiliaryEffectSloti);

  #undef GET_OAL_FUNC

  // Create reverb effect
  alGenEffects(1, &m_iEffect);
  alEffecti(m_iEffect, AL_EFFECT_TYPE, AL_EFFECT_EAXREVERB);

  // Mark EAX reverb if no error
  m_bEAXReverb = !OAL_CheckError(TRANS("Cannot set EAX reverb"));

  // Otherwise fall back to standard reverb
  if (!m_bEAXReverb) {
    alEffecti(m_iEffect, AL_EFFECT_TYPE, AL_EFFECT_REVERB);

    if (OAL_CheckError(TRANS("Cannot set standard reverb"))) {
      // No reverb effect available
      alDeleteEffects(1, &m_iEffect);
      return FALSE;
    }
  }

  // Create slot for the effect
  alGenAuxiliaryEffectSlots(1, &m_iEffectSlot);
  OAL_CheckError();
  alAuxiliaryEffectSloti(m_iEffectSlot, AL_EFFECTSLOT_EFFECT, m_iEffect);
  OAL_CheckError();

  // Attach effect slot to the source
  alSource3i(m_iSource, AL_AUXILIARY_SEND_FILTER, m_iEffectSlot, 0, AL_FILTER_NULL);
  OAL_CheckError();
  return TRUE;
};

void CSoundAPI_OpenAL::SetEnvironment(INDEX iEnvType, FLOAT fEnvSize) {
  if (!m_bUsingEFX) return;

  // Already set
  if (m_iLastEnvType == iEnvType && m_fLastEnvSize == fEnvSize) return;

  // Remember new values
  m_iLastEnvType = iEnvType;
  m_fLastEnvSize = fEnvSize;

  // Set default environment, if invalid
  if (iEnvType < 0 || iEnvType >= EAX_ENVIRONMENT_COUNT) {
    // Equal to the first environment type defined in WorldBase called "Normal"
    iEnvType = EAX_ENVIRONMENT_PADDEDCELL;
    fEnvSize = 1.4f;
  }

  // Macro for matching EAX environment types to EFX presets
  #define COPY_EAX_PRESET(_Name) \
    case EAX_ENVIRONMENT_##_Name: { \
      EFXEAXREVERBPROPERTIES reverb = EFX_REVERB_PRESET_##_Name; \
      LoadEffect(&reverb); \
    } break;

  switch (iEnvType) {
    COPY_EAX_PRESET(GENERIC)
    COPY_EAX_PRESET(PADDEDCELL)
    COPY_EAX_PRESET(ROOM)
    COPY_EAX_PRESET(BATHROOM)
    COPY_EAX_PRESET(LIVINGROOM)
    COPY_EAX_PRESET(STONEROOM)
    COPY_EAX_PRESET(AUDITORIUM)
    COPY_EAX_PRESET(CONCERTHALL)
    COPY_EAX_PRESET(CAVE)
    COPY_EAX_PRESET(ARENA)
    COPY_EAX_PRESET(HANGAR)
    COPY_EAX_PRESET(CARPETEDHALLWAY)
    COPY_EAX_PRESET(HALLWAY)
    COPY_EAX_PRESET(STONECORRIDOR)
    COPY_EAX_PRESET(ALLEY)
    COPY_EAX_PRESET(FOREST)
    COPY_EAX_PRESET(CITY)
    COPY_EAX_PRESET(MOUNTAINS)
    COPY_EAX_PRESET(QUARRY)
    COPY_EAX_PRESET(PLAIN)
    COPY_EAX_PRESET(PARKINGLOT)
    COPY_EAX_PRESET(SEWERPIPE)
    COPY_EAX_PRESET(UNDERWATER)
    COPY_EAX_PRESET(DRUGGED)
    COPY_EAX_PRESET(DIZZY)
    COPY_EAX_PRESET(PSYCHOTIC)

    default:
      ASSERTALWAYS("Invalid EAX environment type!");
      return;
  }

  #undef COPY_EAX_PRESET

  // [Cecil] NOTE: In EAX, the environment size seems to be between 1 and 100. Based on the formula below (from the internal reverb code of OpenAL Soft),
  // the maximum environment size may only be ~2.5 because (2.5 ^ 3) / 16 == ~1, which is also the upper limit of the AL_EAXREVERB_DENSITY property.
  // However, this "EAX environment size -> OpenAL EAX reverb density" formula seems to produce the same sound as DirectSound EAX through DSOAL.

  // Convert EAX environment size to OpenAL reverb density
  const FLOAT fDensity = Min((fEnvSize * fEnvSize * fEnvSize) / 16.0f, AL_EAXREVERB_MAX_DENSITY);

  if (m_bEAXReverb) {
    alEffectf(m_iEffect, AL_EAXREVERB_DENSITY, fDensity);
  } else {
    alEffectf(m_iEffect, AL_REVERB_DENSITY, fDensity);
  }
  OAL_CheckError(TRANS("Cannot set reverb density based on EAX environment size"));

  // Update the effect in the slot
  alAuxiliaryEffectSloti(m_iEffectSlot, AL_EFFECTSLOT_EFFECT, m_iEffect);
  OAL_CheckError();
};

// Load given reverb properties into the effect object
void CSoundAPI_OpenAL::LoadEffect(const EFXEAXREVERBPROPERTIES *pReverb) {
  if (m_bEAXReverb) {
    // EAX reverb properties
    alEffectf (m_iEffect, AL_EAXREVERB_DENSITY,               pReverb->flDensity);
    alEffectf (m_iEffect, AL_EAXREVERB_DIFFUSION,             pReverb->flDiffusion);
    alEffectf (m_iEffect, AL_EAXREVERB_GAIN,                  pReverb->flGain);
    alEffectf (m_iEffect, AL_EAXREVERB_GAINHF,                pReverb->flGainHF);
    alEffectf (m_iEffect, AL_EAXREVERB_GAINLF,                pReverb->flGainLF);
    alEffectf (m_iEffect, AL_EAXREVERB_DECAY_TIME,            pReverb->flDecayTime);
    alEffectf (m_iEffect, AL_EAXREVERB_DECAY_HFRATIO,         pReverb->flDecayHFRatio);
    alEffectf (m_iEffect, AL_EAXREVERB_DECAY_LFRATIO,         pReverb->flDecayLFRatio);
    alEffectf (m_iEffect, AL_EAXREVERB_REFLECTIONS_GAIN,      pReverb->flReflectionsGain);
    alEffectf (m_iEffect, AL_EAXREVERB_REFLECTIONS_DELAY,     pReverb->flReflectionsDelay);
    alEffectfv(m_iEffect, AL_EAXREVERB_REFLECTIONS_PAN,       pReverb->flReflectionsPan);
    alEffectf (m_iEffect, AL_EAXREVERB_LATE_REVERB_GAIN,      pReverb->flLateReverbGain);
    alEffectf (m_iEffect, AL_EAXREVERB_LATE_REVERB_DELAY,     pReverb->flLateReverbDelay);
    alEffectfv(m_iEffect, AL_EAXREVERB_LATE_REVERB_PAN,       pReverb->flLateReverbPan);
    alEffectf (m_iEffect, AL_EAXREVERB_ECHO_TIME,             pReverb->flEchoTime);
    alEffectf (m_iEffect, AL_EAXREVERB_ECHO_DEPTH,            pReverb->flEchoDepth);
    alEffectf (m_iEffect, AL_EAXREVERB_MODULATION_TIME,       pReverb->flModulationTime);
    alEffectf (m_iEffect, AL_EAXREVERB_MODULATION_DEPTH,      pReverb->flModulationDepth);
    alEffectf (m_iEffect, AL_EAXREVERB_AIR_ABSORPTION_GAINHF, pReverb->flAirAbsorptionGainHF);
    alEffectf (m_iEffect, AL_EAXREVERB_HFREFERENCE,           pReverb->flHFReference);
    alEffectf (m_iEffect, AL_EAXREVERB_LFREFERENCE,           pReverb->flLFReference);
    alEffectf (m_iEffect, AL_EAXREVERB_ROOM_ROLLOFF_FACTOR,   pReverb->flRoomRolloffFactor);
    alEffecti (m_iEffect, AL_EAXREVERB_DECAY_HFLIMIT,         pReverb->iDecayHFLimit);

  } else {
    // Standard reverb properties
    alEffectf(m_iEffect, AL_REVERB_DENSITY,               pReverb->flDensity);
    alEffectf(m_iEffect, AL_REVERB_DIFFUSION,             pReverb->flDiffusion);
    alEffectf(m_iEffect, AL_REVERB_GAIN,                  pReverb->flGain);
    alEffectf(m_iEffect, AL_REVERB_GAINHF,                pReverb->flGainHF);
    alEffectf(m_iEffect, AL_REVERB_DECAY_TIME,            pReverb->flDecayTime);
    alEffectf(m_iEffect, AL_REVERB_DECAY_HFRATIO,         pReverb->flDecayHFRatio);
    alEffectf(m_iEffect, AL_REVERB_REFLECTIONS_GAIN,      pReverb->flReflectionsGain);
    alEffectf(m_iEffect, AL_REVERB_REFLECTIONS_DELAY,     pReverb->flReflectionsDelay);
    alEffectf(m_iEffect, AL_REVERB_LATE_REVERB_GAIN,      pReverb->flLateReverbGain);
    alEffectf(m_iEffect, AL_REVERB_LATE_REVERB_DELAY,     pReverb->flLateReverbDelay);
    alEffectf(m_iEffect, AL_REVERB_AIR_ABSORPTION_GAINHF, pReverb->flAirAbsorptionGainHF);
    alEffectf(m_iEffect, AL_REVERB_ROOM_ROLLOFF_FACTOR,   pReverb->flRoomRolloffFactor);
    alEffecti(m_iEffect, AL_REVERB_DECAY_HFLIMIT,         pReverb->iDecayHFLimit);
  }

  // Catch any potential error from setting effect properties
  OAL_CheckError();
};

#endif // SE1_OPENAL_EFX

#endif // SE1_SND_OPENAL
