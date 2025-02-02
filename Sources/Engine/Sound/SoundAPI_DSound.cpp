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

#include <Engine/Sound/SoundAPI_DSound.h>
#include <Engine/Sound/SoundLibrary.h>

#if SE1_WIN && SE1_SND_DSOUND

#include <Engine/Sound/EAX.h>

#pragma comment(lib, "winmm.lib")
#pragma comment(lib, "dxguid.lib")

#define MINPAN (1.0f)
#define MAXPAN (9.0f)

extern INDEX snd_iInterface;
extern FLOAT snd_fEAXPanning;

BOOL CSoundAPI_DSound::Fail(const char *strError) {
  CPrintF(strError);
  ShutDown();
  snd_iInterface = E_SND_DSOUND; // If EAX failed, try DirectSound
  return FALSE;
};

BOOL CSoundAPI_DSound::InitSecondary(LPDIRECTSOUNDBUFFER &pBuffer, SLONG slSize) {
  WAVEFORMATEX &wfe = _pSound->sl_SwfeFormat;

  // Eventually adjust for EAX
  DWORD dwFlag3D = NONE;

#if SE1_SND_EAX
  if (snd_iInterface == E_SND_EAX) {
    dwFlag3D = DSBCAPS_CTRL3D;
    wfe.nChannels = 1;  // mono output
    wfe.nBlockAlign /= 2;
    wfe.nAvgBytesPerSec /= 2;
    slSize /= 2;
  }
#endif

  DSBUFFERDESC dsBuffer;
  memset(&dsBuffer, 0, sizeof(dsBuffer));

  dsBuffer.dwSize = sizeof(dsBuffer);
  dsBuffer.dwFlags = DSBCAPS_GETCURRENTPOSITION2 | dwFlag3D;
  dsBuffer.dwBufferBytes = slSize;
  dsBuffer.lpwfxFormat = &wfe;

  HRESULT hResult = m_pDS->CreateSoundBuffer(&dsBuffer, &pBuffer, NULL);

#if SE1_SND_EAX
  if (snd_iInterface == E_SND_EAX) {
    // Revert back to original wave format (stereo)
    wfe.nChannels = 2;
    wfe.nBlockAlign *= 2;
    wfe.nAvgBytesPerSec *= 2;
  }
#endif

  if (hResult != DS_OK) {
    return Fail(TRANS("  ! DirectSound error: Cannot create secondary buffer.\n"));
  }

  return TRUE;
};

BOOL CSoundAPI_DSound::LockBuffer(LPDIRECTSOUNDBUFFER pBuffer, SLONG slSize, LPVOID &lpData, DWORD &dwSize) {
  INDEX ctRetries = 1000; // Too many?

#if SE1_SND_EAX
  // Buffer is mono in case of EAX
  if (m_bUsingEAX) slSize /= 2;
#endif

  FOREVER {
    HRESULT hResult = pBuffer->Lock(0, slSize, &lpData, &dwSize, NULL, NULL, 0);
    if (hResult == DS_OK && slSize == dwSize) return TRUE;

    if (hResult != DSERR_BUFFERLOST) return Fail(TRANS("  ! DirectSound error: Cannot lock sound buffer.\n"));
    if (ctRetries-- == 0) return Fail(TRANS("  ! DirectSound error: Couldn't restore sound buffer.\n"));

    pBuffer->Restore();
  }

  // Shouldn't be here
  ASSERT(FALSE);
  return FALSE;
};

void CSoundAPI_DSound::PlayBuffers(void) {
  DWORD dw;
  BOOL bInitiatePlay = FALSE;

  ASSERT(m_pDSSecondary != NULL && m_pDSPrimary != NULL);

#if SE1_SND_EAX
  if (m_bUsingEAX && m_pDSSecondary2->GetStatus(&dw) == DS_OK && !(dw & DSBSTATUS_PLAYING)) bInitiatePlay = TRUE;
#endif

  if (m_pDSSecondary->GetStatus(&dw) == DS_OK && !(dw & DSBSTATUS_PLAYING)) bInitiatePlay = TRUE;
  if (m_pDSPrimary->GetStatus(&dw)   == DS_OK && !(dw & DSBSTATUS_PLAYING)) bInitiatePlay = TRUE;

  // Done if all buffers are already playing
  if (!bInitiatePlay) return;

  // Stop buffers (in case some buffers are playing
  m_pDSPrimary->Stop();
  m_pDSSecondary->Stop();

#if SE1_SND_EAX
  if (m_bUsingEAX) m_pDSSecondary2->Stop();
#endif

  // Check sound buffer lock and clear sound buffer(s)
  LPVOID lpData;
  DWORD	dwSize;

  if (!LockBuffer(m_pDSSecondary, m_slMixerBufferSize, lpData, dwSize)) return;
  memset(lpData, 0, dwSize);
  m_pDSSecondary->Unlock(lpData, dwSize, NULL, 0);

#if SE1_SND_EAX
  if (m_bUsingEAX)
  {
    if (!LockBuffer(m_pDSSecondary2, m_slMixerBufferSize, lpData, dwSize)) return;
    memset(lpData, 0, dwSize);
    m_pDSSecondary2->Unlock(lpData, dwSize, NULL, 0);

    // Start playing EAX additional buffer
    m_pDSSecondary2->Play(0, 0, DSBPLAY_LOOPING);
  }
#endif

  // Start playing standard DirectSound buffers
  m_pDSPrimary->Play(0, 0, DSBPLAY_LOOPING);
  m_pDSSecondary->Play(0, 0, DSBPLAY_LOOPING);

  m_iWriteOffset = 0;

#if SE1_SND_EAX
  m_iWriteOffsetEAX = 0;

  // Adjust starting offsets for EAX
  if (m_bUsingEAX) {
    DWORD dwCursor1, dwCursor2;
    SLONG slMinDelta = MAX_SLONG;

    // Should be enough to screw interrupts
    for (INDEX i = 0; i < 10; i++) {
      m_pDSSecondary->GetCurrentPosition(&dwCursor1, NULL);
      m_pDSSecondary2->GetCurrentPosition(&dwCursor2, NULL);

      SLONG slDelta1 = dwCursor2 - dwCursor1;
      m_pDSSecondary2->GetCurrentPosition(&dwCursor2, NULL);
      m_pDSSecondary->GetCurrentPosition(&dwCursor1, NULL);

      SLONG slDelta2 = dwCursor2 - dwCursor1;
      SLONG slDelta = (slDelta1 + slDelta2) / 2;

      if (slDelta < slMinDelta) slMinDelta = slDelta;
      //CPrintF("D1=%5d,  D2=%5d,  AD=%5d,  MD=%5d\n", slDelta1, slDelta2, slDelta, slMinDelta);
    }

    // 2 because of stereo offsets
    if (slMinDelta < 0) m_iWriteOffset  = -slMinDelta * 2;
    if (slMinDelta > 0) m_iWriteOffsetEAX = +slMinDelta * 2;

    // Round to 4 bytes
    m_iWriteOffset += m_iWriteOffset & 3;
    m_iWriteOffsetEAX += m_iWriteOffsetEAX & 3;

    // Assure that first writing offsets are inside buffers
    if (m_iWriteOffset >= m_slMixerBufferSize) m_iWriteOffset -= m_slMixerBufferSize;
    if (m_iWriteOffsetEAX >= m_slMixerBufferSize) m_iWriteOffsetEAX -= m_slMixerBufferSize;

    ASSERT(m_iWriteOffset >= 0 && m_iWriteOffset < m_slMixerBufferSize);
    ASSERT(m_iWriteOffsetEAX >= 0 && m_iWriteOffsetEAX < m_slMixerBufferSize);
  }
#endif // SE1_SND_EAX
};

BOOL CSoundAPI_DSound::StartUp(BOOL bReport) {
  ASSERT(m_hDSoundLib == NULL && m_pDS == NULL);
  ASSERT(m_pDSSecondary == NULL && m_pDSPrimary == NULL);

  m_hDSoundLib = OS::LoadLib("dsound.dll");

  if (m_hDSoundLib == NULL) {
    CPrintF(TRANS("  ! DirectSound error: Cannot load 'DSOUND.DLL'.\n"));
    return FALSE;
  }

  // Get main procedure address
  typedef HRESULT (WINAPI *CCreateFunc)(GUID FAR *, LPDIRECTSOUND FAR *, IUnknown FAR *);
  CCreateFunc pDirectSoundCreate = (CCreateFunc)OS::GetLibSymbol(m_hDSoundLib, "DirectSoundCreate");

  if (pDirectSoundCreate == NULL) return Fail(TRANS("  ! DirectSound error: Cannot get procedure address.\n"));

  // Initialize DirectSound
  HRESULT	hResult = pDirectSoundCreate(NULL, &m_pDS, NULL);
  if (hResult != DS_OK) return Fail(TRANS("  ! DirectSound error: Cannot create object.\n"));

  // Get capabilities
  DSCAPS dsCaps;
  dsCaps.dwSize = sizeof(dsCaps);

  hResult = m_pDS->GetCaps(&dsCaps);
  if (hResult != DS_OK) return Fail(TRANS("  ! DirectSound error: Cannot determine capabilites.\n"));

  // Fail if in emulation mode
  if (dsCaps.dwFlags & DSCAPS_EMULDRIVER) {
    CPrintF(TRANS("  ! DirectSound error: No driver installed.\n"));
    ShutDown();
    return FALSE;
  }

  // Set cooperative level to priority
  extern OS::Window _hwndCurrent;
  m_wndCurrent = _hwndCurrent;

  // [Cecil] Always use native window handle
  hResult = m_pDS->SetCooperativeLevel(m_wndCurrent.GetNativeHandle(), DSSCL_PRIORITY);

  if (hResult != DS_OK) return Fail(TRANS("  ! DirectSound error: Cannot set cooperative level.\n"));

  // Prepare 3D flag if EAX
  DWORD dwFlag3D = NONE;

#if SE1_SND_EAX
  if (snd_iInterface == E_SND_EAX) dwFlag3D = DSBCAPS_CTRL3D;
#endif

  // Create primary sound buffer (must have one)
  DSBUFFERDESC dsBuffer;
  memset(&dsBuffer, 0, sizeof(dsBuffer));

  dsBuffer.dwSize = sizeof(dsBuffer);
  dsBuffer.dwFlags = DSBCAPS_PRIMARYBUFFER | dwFlag3D;
  dsBuffer.dwBufferBytes = 0;
  dsBuffer.lpwfxFormat = NULL;

  hResult = m_pDS->CreateSoundBuffer(&dsBuffer, &m_pDSPrimary, NULL);
  if(hResult != DS_OK) return Fail(TRANS("  ! DirectSound error: Cannot create primary sound buffer.\n"));

  // Set primary buffer format
  const WAVEFORMATEX &wfe = _pSound->sl_SwfeFormat;

  hResult = m_pDSPrimary->SetFormat(&wfe);
  if (hResult != DS_OK) return Fail(TRANS("  ! DirectSound error: Cannot set primary sound buffer format.\n"));

  // Allocate mixer buffers
  AllocBuffers(FALSE);

  // Start up secondary sound buffers
  if (!InitSecondary(m_pDSSecondary, m_slMixerBufferSize)) return FALSE;

#if SE1_SND_EAX
  // Set some additionals for EAX
  if (snd_iInterface == E_SND_EAX) {
    // 2nd secondary buffer
    if (!InitSecondary(m_pDSSecondary2, m_slMixerBufferSize)) return FALSE;

    // Set 3D for all buffers
    HRESULT hr1, hr2, hr3, hr4;
    hr1 = m_pDSPrimary->QueryInterface(IID_IDirectSound3DListener, (LPVOID *)&m_pDSListener);
    hr2 = m_pDSSecondary->QueryInterface(IID_IDirectSound3DBuffer, (LPVOID *)&m_pDSSourceLeft);
    hr3 = m_pDSSecondary2->QueryInterface(IID_IDirectSound3DBuffer, (LPVOID *)&m_pDSSourceRight);

    if (hr1 != DS_OK || hr2 != DS_OK || hr3 != DS_OK) {
      return Fail(TRANS("  ! DirectSound3D error: Cannot set 3D sound buffer.\n"));
    }

    hr1 = m_pDSListener->SetPosition(0, 0, 0, DS3D_DEFERRED);
    hr2 = m_pDSListener->SetOrientation(0, 0, 1, 0, 1, 0, DS3D_DEFERRED);
    hr3 = m_pDSListener->SetRolloffFactor(1, DS3D_DEFERRED);

    if (hr1 != DS_OK || hr2 != DS_OK || hr3 != DS_OK) {
      return Fail(TRANS("  ! DirectSound3D error: Cannot set 3D parameters for listener.\n"));
    }

    hr1 = m_pDSSourceLeft->SetMinDistance(MINPAN, DS3D_DEFERRED);
    hr2 = m_pDSSourceLeft->SetMaxDistance(MAXPAN, DS3D_DEFERRED);
    hr3 = m_pDSSourceRight->SetMinDistance(MINPAN, DS3D_DEFERRED);
    hr4 = m_pDSSourceRight->SetMaxDistance(MAXPAN, DS3D_DEFERRED);

    if (hr1 != DS_OK || hr2 != DS_OK || hr3 != DS_OK || hr4 != DS_OK) {
      return Fail(TRANS("  ! DirectSound3D error: Cannot set 3D parameters for sound source.\n"));
    }

    // Apply
    hResult = m_pDSListener->CommitDeferredSettings();
    if (hResult != DS_OK) return Fail(TRANS("  ! DirectSound3D error: Cannot apply 3D parameters.\n"));

    // Reset EAX parameters
    m_iLastEnvType = -999;
    m_fLastEnvSize = -999;
    m_fLastPanning = -999;

    // Query property interface to EAX
    hResult = m_pDSSourceLeft->QueryInterface(IID_IKsPropertySet, (LPVOID *)&m_pKSProperty);
    if (hResult != DS_OK) return Fail(TRANS("  ! EAX error: Cannot set property interface.\n"));

    // Query support
    ULONG ulSupport = 0;

    hResult = m_pKSProperty->QuerySupport(DSPROPSETID_EAX_ListenerProperties, DSPROPERTY_EAXLISTENER_ENVIRONMENT, &ulSupport);
    if (hResult != DS_OK || !(ulSupport & KSPROPERTY_SUPPORT_SET)) return Fail(TRANS("  ! EAX error: Cannot query property support.\n"));

    hResult = m_pKSProperty->QuerySupport(DSPROPSETID_EAX_ListenerProperties, DSPROPERTY_EAXLISTENER_ENVIRONMENTSIZE, &ulSupport);
    if (hResult != DS_OK || !(ulSupport & KSPROPERTY_SUPPORT_SET)) return Fail(TRANS("  ! EAX error: Cannot query property support.\n"));

    // Made it
    m_bUsingEAX = TRUE;
  }

  m_iWriteOffsetEAX = 0;

  if (bReport) {
    CPrintF(TRANS("  EAX: %s\n"), m_bUsingEAX ? TRANS("Enabled") : TRANS("Disabled"));
  }
#endif // SE1_SND_EAX

  // Mark that DirectSound is operative
  m_iWriteOffset = 0;

  if (bReport) {
    ReportGenericInfo();
  }

  return TRUE;
};

void CSoundAPI_DSound::ShutDown(void) {
  m_wndCurrent = NULL;

  #define DSOUND_FREE(_Buf)          if (_Buf != NULL) {               _Buf->Release(); _Buf = NULL; }
  #define DSOUND_STOP_AND_FREE(_Buf) if (_Buf != NULL) { _Buf->Stop(); _Buf->Release(); _Buf = NULL; }

#if SE1_SND_EAX
  m_bUsingEAX = FALSE;

  // Free EAX buffers
  DSOUND_FREE(m_pDSSourceRight);
  DSOUND_FREE(m_pDSSourceLeft);
  DSOUND_FREE(m_pDSListener);
  DSOUND_STOP_AND_FREE(m_pDSSecondary2);
  DSOUND_FREE(m_pKSProperty);
#endif

  // Free DirectSound buffers
  DSOUND_STOP_AND_FREE(m_pDSSecondary);
  DSOUND_STOP_AND_FREE(m_pDSPrimary);

  // Free DirectSound object
  if (m_pDS != NULL)
  {
    // Reset cooperative level
    if (m_wndCurrent != NULL) {
      // [Cecil] Always use native window handle
      m_pDS->SetCooperativeLevel(m_wndCurrent.GetNativeHandle(), DSSCL_NORMAL);
    }

    m_pDS->Release();
    m_pDS = NULL;
  }

  // Free library
  if (m_hDSoundLib != NULL) {
    OS::FreeLib(m_hDSoundLib);
    m_hDSoundLib = NULL;
  }

  CAbstractSoundAPI::ShutDown();
};

void CSoundAPI_DSound::CopyMixerBuffer(SLONG slMixedSize) {
  LPVOID lpData;
  DWORD dwSize;
  SLONG slPart1Size, slPart2Size;

#if SE1_SND_EAX
  // If EAX is in use
  if (m_bUsingEAX) {
    // Lock left buffer and copy first part of the 1st mono block
    if (!LockBuffer(m_pDSSecondary, m_slMixerBufferSize, lpData, dwSize)) return;

    slPart1Size = Min(m_slMixerBufferSize - m_iWriteOffset, slMixedSize);
    CopyMixerBuffer_mono(0, ((UBYTE *)lpData) + m_iWriteOffset / 2, slPart1Size);

    // Copy second part of the 1st mono block
    slPart2Size = slMixedSize - slPart1Size;
    CopyMixerBuffer_mono(slPart1Size, lpData, slPart2Size);

    m_iWriteOffset += slMixedSize;
    if (m_iWriteOffset >= m_slMixerBufferSize) m_iWriteOffset -= m_slMixerBufferSize;

    ASSERT(m_iWriteOffset >= 0 && m_iWriteOffset < m_slMixerBufferSize);
    m_pDSSecondary->Unlock(lpData, dwSize, NULL, 0);

    // Lock right buffer and copy first part of the 2nd mono block
    if (!LockBuffer(m_pDSSecondary2, m_slMixerBufferSize, lpData, dwSize)) return;

    slPart1Size = Min(m_slMixerBufferSize - m_iWriteOffsetEAX, slMixedSize);
    CopyMixerBuffer_mono(2, ((UBYTE *)lpData) + m_iWriteOffsetEAX / 2, slPart1Size);

    // Copy second part of the 2nd mono block
    slPart2Size = slMixedSize - slPart1Size;
    CopyMixerBuffer_mono(slPart1Size + 2, lpData, slPart2Size);

    m_iWriteOffsetEAX += slMixedSize;
    if (m_iWriteOffsetEAX >= m_slMixerBufferSize) m_iWriteOffsetEAX -= m_slMixerBufferSize;

    ASSERT(m_iWriteOffsetEAX >= 0 && m_iWriteOffsetEAX < m_slMixerBufferSize);
    m_pDSSecondary2->Unlock(lpData, dwSize, NULL, 0);
    return;
  }
#endif // SE1_SND_EAX

  // Only standard DirectSound (no EAX)
  {
    // Lock stereo buffer and copy first part of block
    if (!LockBuffer(m_pDSSecondary, m_slMixerBufferSize, lpData, dwSize)) return;

    slPart1Size = Min(m_slMixerBufferSize - m_iWriteOffset, slMixedSize);
    CopyMixerBuffer_stereo(0, ((UBYTE *)lpData) + m_iWriteOffset, slPart1Size);

    // Copy second part of block
    slPart2Size = slMixedSize - slPart1Size;
    CopyMixerBuffer_stereo(slPart1Size, lpData, slPart2Size);

    m_iWriteOffset += slMixedSize;
    if (m_iWriteOffset >= m_slMixerBufferSize) m_iWriteOffset -= m_slMixerBufferSize;

    ASSERT(m_iWriteOffset >= 0 && m_iWriteOffset < m_slMixerBufferSize);
    m_pDSSecondary->Unlock(lpData, dwSize, NULL, 0);
  }
};

SLONG CSoundAPI_DSound::PrepareSoundBuffer(void)
{
  // Determine writable block size (difference between write and play pointers)
  HRESULT hr1, hr2;
  DWORD dwCurrentCursor, dwCurrentCursor2;
  SLONG slDataToMix;

  ASSERT(m_pDSSecondary != NULL && m_pDSPrimary != NULL);

#if SE1_SND_EAX
  // If EAX is in use
  if (m_bUsingEAX) {
    hr1 = m_pDSSecondary->GetCurrentPosition(&dwCurrentCursor, NULL);
    hr2 = m_pDSSecondary2->GetCurrentPosition(&dwCurrentCursor2, NULL);

    if (hr1 != DS_OK || hr2 != DS_OK) {
      return Fail(TRANS("  ! DirectSound error: Cannot obtain sound buffer write position.\n"));
    }

    // Stereo mixer
    dwCurrentCursor *= 2;
    dwCurrentCursor2 *= 2;

    // Store pointers and wrapped block sizes
    SLONG slDataToMix1 = dwCurrentCursor - m_iWriteOffset;
    if (slDataToMix1 < 0) slDataToMix1 += m_slMixerBufferSize;

    ASSERT(slDataToMix1 >= 0 && slDataToMix1 <= m_slMixerBufferSize);
    slDataToMix1 = Min(slDataToMix1, m_slMixerBufferSize);

    SLONG slDataToMix2 = dwCurrentCursor2 - m_iWriteOffsetEAX;
    if (slDataToMix2 < 0) slDataToMix2 += m_slMixerBufferSize;

    ASSERT(slDataToMix2 >= 0 && slDataToMix2 <= m_slMixerBufferSize);
    slDataToMix = Min(slDataToMix1, slDataToMix2);

    return slDataToMix;
  }
#endif // SE1_SND_EAX

  // Standard DirectSound (no EAX)
  {
    hr1 = m_pDSSecondary->GetCurrentPosition(&dwCurrentCursor, NULL);

    if (hr1 != DS_OK) {
      return Fail(TRANS("  ! DirectSound error: Cannot obtain sound buffer write position.\n"));
    }

    // Store pointer and wrapped block size
    slDataToMix = dwCurrentCursor - m_iWriteOffset;
    if (slDataToMix < 0) slDataToMix += m_slMixerBufferSize;

    ASSERT(slDataToMix >= 0 && slDataToMix <= m_slMixerBufferSize);
    slDataToMix = Min(slDataToMix, m_slMixerBufferSize);
  }

  //CPrintF("LP/LW: %5d / %5d,   RP/RW: %5d / %5d,    MIX: %5d\n", dwCurrentCursor, _iWriteOffset, dwCurrentCursor2, _iWriteOffset2, slDataToMix); // grgr
  return slDataToMix;
};

void CSoundAPI_DSound::Mute(BOOL &bSetSoundMuted) {
  // Synchronize access to sounds
  CTSingleLock slSounds(&_pSound->sl_csSound, TRUE);

  // Suppress future mixing and erase sound buffer
  bSetSoundMuted = TRUE;

  LPVOID lpData;
  DWORD dwSize;

  // Flush one secondary buffer
  if (!LockBuffer(m_pDSSecondary, m_slMixerBufferSize, lpData, dwSize)) return;

  memset(lpData, 0, dwSize);
  m_pDSSecondary->Unlock(lpData, dwSize, NULL, 0);

#if SE1_SND_EAX
  // Flush right buffer as well for EAX
  if (m_bUsingEAX) {
    if (!LockBuffer(m_pDSSecondary2, m_slMixerBufferSize, lpData, dwSize)) return;

    memset(lpData, 0, dwSize);
    m_pDSSecondary2->Unlock(lpData, dwSize, NULL, 0);
  }
#endif
};

#if SE1_SND_EAX

// Set listener enviroment properties for EAX
BOOL CSoundAPI_DSound::SetEnvironment(INDEX iEnvNo, FLOAT fEnvSize)
{
  if (!m_bUsingEAX) return FALSE;

  // Trim values
  if (iEnvNo < 0 || iEnvNo > 25) {
    iEnvNo = 1;
  }

  // [Cecil] 99 -> 100 to allow "Big canyon" sector type (EAX_ENVIRONMENT_MOUNTAINS) to use its size of 100 meters
  if (fEnvSize < 1 || fEnvSize > 100) {
    fEnvSize = 8;
  }

  HRESULT hResult;

  hResult = m_pKSProperty->Set(DSPROPSETID_EAX_ListenerProperties, DSPROPERTY_EAXLISTENER_ENVIRONMENT, NULL, 0, &iEnvNo, sizeof(DWORD));
  if (hResult != DS_OK) return Fail(TRANS("  ! EAX error: Cannot set environment.\n"));

  hResult = m_pKSProperty->Set(DSPROPSETID_EAX_ListenerProperties, DSPROPERTY_EAXLISTENER_ENVIRONMENTSIZE, NULL, 0, &fEnvSize, sizeof(FLOAT));
  if (hResult != DS_OK) return Fail(TRANS("  ! EAX error: Cannot set environment size.\n"));

  return TRUE;
};

#endif // SE1_SND_EAX

void CSoundAPI_DSound::Update(void) {
  // Make sure that the buffers are playing
  PlayBuffers();

#if SE1_SND_EAX
  // Determine number of listeners and get listener
  INDEX ctListeners = 0;
  CSoundListener *sli;

  {FOREACHINLIST(CSoundListener, sli_lnInActiveListeners, _pSound->sl_lhActiveListeners, itsli) {
    sli = itsli;
    ctListeners++;
  }}

  // If there's only one listener, environment properties have been changed (in split-screen EAX is not supported)
  if (ctListeners == 1 && (m_iLastEnvType != sli->sli_iEnvironmentType || m_fLastEnvSize != sli->sli_fEnvironmentSize)) {
    // Keep new properties and eventually update environment (EAX)
    m_iLastEnvType = sli->sli_iEnvironmentType;
    m_fLastEnvSize = sli->sli_fEnvironmentSize;
    SetEnvironment(m_iLastEnvType, m_fLastEnvSize);
  }

  // If there are no listeners - reset environment properties
  if (ctListeners < 1 && (m_iLastEnvType != 1 || m_fLastEnvSize != 1.4f)) {
    // Keep new properties and update environment
    m_iLastEnvType = 1;
    m_fLastEnvSize = 1.4f;
    SetEnvironment(m_iLastEnvType, m_fLastEnvSize);
  }

  // Adjust panning if needed
  snd_fEAXPanning = Clamp(snd_fEAXPanning, -1.0f, +1.0f);

  // Not using EAX or panning is the same
  if (!m_bUsingEAX || m_fLastPanning == snd_fEAXPanning) return;

  // Determine new panning
  m_fLastPanning = snd_fEAXPanning;

  FLOAT fPanL = -1.0f;
  FLOAT fPanR = +1.0f;

  if (snd_fEAXPanning < 0) fPanR = MINPAN + Abs(snd_fEAXPanning) * MAXPAN; // Pan left
  if (snd_fEAXPanning > 0) fPanL = MINPAN + Abs(snd_fEAXPanning) * MAXPAN; // Pan right

  // Set and apply
  HRESULT hr1 = m_pDSSourceLeft->SetPosition( fPanL, 0, 0, DS3D_DEFERRED);
  HRESULT hr2 = m_pDSSourceRight->SetPosition(fPanR, 0, 0, DS3D_DEFERRED);
  HRESULT hr3 = m_pDSListener->CommitDeferredSettings();

  if (hr1 != DS_OK || hr2 != DS_OK || hr3 != DS_OK) {
    Fail(TRANS("  ! DirectSound3D error: Cannot set 3D position.\n"));
  }
#endif // SE1_SND_EAX
};

#endif // SE1_WIN && SE1_SND_DSOUND
