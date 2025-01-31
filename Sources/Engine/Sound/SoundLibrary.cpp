/* Copyright (c) 2002-2012 Croteam Ltd. 
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

#include <Engine/Sound/SoundLibrary.h>
#include <Engine/Base/Translation.h>

#include <Engine/Base/Shell.h>
#include <Engine/Base/Memory.h>
#include <Engine/Base/ErrorReporting.h>
#include <Engine/Base/ListIterator.inl>
#include <Engine/Base/Console.h>
#include <Engine/Base/Console_internal.h>
#include <Engine/Base/Statistics_internal.h>
#include <Engine/Base/IFeel.h>

#include <Engine/Sound/SoundProfile.h>
#include <Engine/Sound/SoundListener.h>
#include <Engine/Sound/SoundData.h>
#include <Engine/Sound/SoundObject.h>
#include <Engine/Sound/SoundDecoder.h>
#include <Engine/Network/Network.h>

#include <Engine/Templates/StaticArray.cpp>
#include <Engine/Templates/StaticStackArray.cpp>

// [Cecil]
#include <Engine/Sound/SoundAPI_DSound.h>

// pointer to global sound library object
CSoundLibrary *_pSound = NULL;


// console variables
FLOAT snd_tmMixAhead  = 0.2f; // mix-ahead in seconds
FLOAT snd_fSoundVolume = 1.0f;   // master volume for sound playing [0..1]
FLOAT snd_fMusicVolume = 1.0f;   // master volume for music playing [0..1]
// NOTES: 
// - these 3d sound parameters have been set carefully, take extreme if changing !
// - ears distance of 20cm causes phase shift of up to 0.6ms which is very noticable
//   and is more than enough, too large values cause too much distorsions in other effects
// - pan strength needs not to be very strong, since lrfilter has panning-like influence also
// - if down filter is too large, it makes too much influence even on small elevation changes
//   and messes the situation completely
FLOAT snd_fDelaySoundSpeed   = 1E10;   // sound speed used for delay [m/s]
FLOAT snd_fDopplerSoundSpeed = 330.0f; // sound speed used for doppler [m/s]
FLOAT snd_fEarsDistance = 0.2f;   // distance between listener's ears
FLOAT snd_fPanStrength  = 0.1f;   // panning modifier (0=none, 1= full)
FLOAT snd_fLRFilter = 3.0f;   // filter for left-right
FLOAT snd_fBFilter  = 5.0f;   // filter for back
FLOAT snd_fUFilter  = 1.0f;   // filter for up
FLOAT snd_fDFilter  = 3.0f;   // filter for down

ENGINE_API INDEX snd_iFormat = 3;
INDEX snd_bMono = FALSE;
INDEX snd_iDevice = -1;
INDEX snd_iInterface = CAbstractSoundAPI::E_SND_DEFAULT; // [Cecil] Default API type depending on platform
INDEX snd_iMaxOpenRetries = 3;
INDEX snd_iMaxExtraChannels = 32;
FLOAT snd_tmOpenFailDelay = 0.5f;
FLOAT snd_fEAXPanning = 0.0f;

static FLOAT snd_fNormalizer = 0.9f;
static FLOAT _fLastNormalizeValue = 1;

static BOOL _bMutedForMixing = FALSE;

/**
 * ----------------------------
 *    Sound Library functions
 * ----------------------------
**/

/*
 *  Construct uninitialized sound library.
 */
CSoundLibrary::CSoundLibrary(void)
{
  // [Cecil] Should have at least one sound API
  ASSERT(CAbstractSoundAPI::E_SND_MAX > 0);

  // [Cecil] No sound interface by default
  sl_pInterface = NULL;

  sl_csSound.cs_eIndex = EThreadMutexType::E_MTX_SOUND;

  // access to the list of handlers must be locked
  CTSingleLock slHooks(&_pTimer->tm_csHooks, TRUE);
  // synchronize access to sounds
  CTSingleLock slSounds(&sl_csSound, TRUE);

  // clear sound format
  memset(&sl_SwfeFormat, 0, sizeof(sl_SwfeFormat));
  sl_EsfFormat = SF_NONE;
};

/*
 *  Destruct (and clean up).
 */
CSoundLibrary::~CSoundLibrary(void)
{
  // access to the list of handlers must be locked
  CTSingleLock slHooks(&_pTimer->tm_csHooks, TRUE);
  // synchronize access to sounds
  CTSingleLock slSounds(&sl_csSound, TRUE);

  // clear sound enviroment
  Clear();

  // clear any installed sound decoders
  CSoundDecoder::EndPlugins();

  // [Cecil]
  DestroyInterface();
};

// [Cecil] Destroy current sound interface
void CSoundLibrary::DestroyInterface(void) {
  if (sl_pInterface != NULL) {
    delete sl_pInterface;
    sl_pInterface = NULL;
  }
};

// post sound console variables' functions

static FLOAT _tmLastMixAhead = 1234;
static INDEX _iLastFormat = 1234;
static INDEX _iLastDevice = 1234;
static INDEX _iLastAPI = 1234;

static void SndPostFunc(void *pArgs)
{
  // clamp variables
  snd_tmMixAhead = Clamp(snd_tmMixAhead, 0.1f, 0.9f);
  snd_iFormat    = Clamp(snd_iFormat, (INDEX)CSoundLibrary::SF_NONE, (INDEX)CSoundLibrary::SF_44100_16);
  snd_iDevice    = Clamp(snd_iDevice, -1L, 15L);
  snd_iInterface = Clamp(snd_iInterface, 0L, CAbstractSoundAPI::E_SND_MAX - 1); // [Cecil] Until max interfaces
  // if any variable has been changed
  if( _tmLastMixAhead!=snd_tmMixAhead || _iLastFormat!=snd_iFormat
   || _iLastDevice!=snd_iDevice || _iLastAPI!=snd_iInterface) {
    // reinit sound format
    _pSound->SetFormat( (enum CSoundLibrary::SoundFormat)snd_iFormat, TRUE);
  }
};

/*
 *  Set wave format from library format
 */
void CSoundLibrary::SetWaveFormat(CSoundLibrary::SoundFormat EsfFormat)
{
  // change Library Wave Format
  WAVEFORMATEX &wfe = sl_SwfeFormat;
  memset(&wfe, 0, sizeof(wfe));

  wfe.wFormatTag = WAVE_FORMAT_PCM;
  wfe.nChannels = 2;
  wfe.wBitsPerSample = 16;

  switch( EsfFormat) {
    case CSoundLibrary::SF_11025_16: wfe.nSamplesPerSec = 11025;  break;
    case CSoundLibrary::SF_22050_16: wfe.nSamplesPerSec = 22050;  break;
    case CSoundLibrary::SF_44100_16: wfe.nSamplesPerSec = 44100;  break;
    case CSoundLibrary::SF_NONE: ASSERTALWAYS( "Can't set to NONE format"); break;
    default:                     ASSERTALWAYS( "Unknown Sound format");     break;
  }

  wfe.nBlockAlign = (wfe.wBitsPerSample / 8) * wfe.nChannels;
  wfe.nAvgBytesPerSec = wfe.nSamplesPerSec * wfe.nBlockAlign;
};

/*
 *  Set library format from wave format
 */
void CSoundLibrary::SetLibraryFormat(void)
{
  // if library format is none return
  if (sl_EsfFormat == CSoundLibrary::SF_NONE) return;

  // else check wave format to determine library format
  ULONG ulFormat = sl_SwfeFormat.nSamplesPerSec;
  // find format
  switch( ulFormat) {
    case 11025: sl_EsfFormat = CSoundLibrary::SF_11025_16; break;
    case 22050: sl_EsfFormat = CSoundLibrary::SF_22050_16; break;
    case 44100: sl_EsfFormat = CSoundLibrary::SF_44100_16; break;
    // unknown format
    default:
      ASSERTALWAYS( "Unknown sound format");
      FatalError( TRANS("Unknown sound format"));
      sl_EsfFormat = CSoundLibrary::SF_ILLEGAL;
  }
};

/*
 *  set sound format
 */
void CSoundLibrary::SetFormat_internal(CSoundLibrary::SoundFormat EsfNew, BOOL bReport)
{
  // access to the list of handlers must be locked
  CTSingleLock slHooks(&_pTimer->tm_csHooks, TRUE);
  // synchronize access to sounds
  CTSingleLock slSounds(&sl_csSound, TRUE);

  // remember library format
  sl_EsfFormat = EsfNew;
  // release library
  ClearLibrary();

  // if none skip initialization
  _fLastNormalizeValue = 1;
  if( bReport) CPrintF(TRANS("Setting sound format ...\n"));
  if (sl_EsfFormat == CSoundLibrary::SF_NONE) {
    if( bReport) CPrintF(TRANS("  (no sound)\n"));
    return;
  }

  // set wave format from library format
  SetWaveFormat(EsfNew);
  snd_iDevice    = Clamp(snd_iDevice, -1L, (INDEX)(sl_ctWaveDevices-1));
  snd_tmMixAhead = Clamp(snd_tmMixAhead, 0.1f, 0.9f);
  snd_iInterface = Clamp(snd_iInterface, 0L, CAbstractSoundAPI::E_SND_MAX - 1); // [Cecil] Until max interfaces

  BOOL bSoundOK = FALSE;

  // [Cecil] Try creating interfaces until one succeeds
  for (INDEX iCheck = snd_iInterface; iCheck >= 0; iCheck--) {
    CAbstractSoundAPI::ESoundAPI eAPI = (CAbstractSoundAPI::ESoundAPI)iCheck;

    // Create a new interface
    DestroyInterface();
    sl_pInterface = CAbstractSoundAPI::CreateAPI(eAPI);

    // Set and start it up
    snd_iInterface = eAPI;
    if (bReport) CPrintF(TRANS("%s initialization...\n"), CAbstractSoundAPI::GetApiName(eAPI).ConstData());

    bSoundOK = sl_pInterface->StartUp(bReport);

    // Setting succeeded
    if (bSoundOK) break;
  }

  // if didn't make it by now
  if (bReport) CPrintF("\n");

  if (!bSoundOK) {
    // revert to none in case sound init was unsuccessful
    sl_EsfFormat = CSoundLibrary::SF_NONE;

    // [Cecil] All failed
    snd_iInterface = 0;
    return;
  }

  // set library format from wave format
  SetLibraryFormat();

  // add timer handler
  _pTimer->AddHandler(&sl_thTimerHandler);
};

/*
 *  Initialization
 */
void CSoundLibrary::Init(void)
{
  // access to the list of handlers must be locked
  CTSingleLock slHooks(&_pTimer->tm_csHooks, TRUE);
  // synchronize access to sounds
  CTSingleLock slSounds(&sl_csSound, TRUE);

  _pShell->DeclareSymbol( "void SndPostFunc(INDEX);", &SndPostFunc);

  _pShell->DeclareSymbol( "           user INDEX snd_bMono;", &snd_bMono);
  _pShell->DeclareSymbol( "persistent user FLOAT snd_fEarsDistance;",      &snd_fEarsDistance);
  _pShell->DeclareSymbol( "persistent user FLOAT snd_fDelaySoundSpeed;",   &snd_fDelaySoundSpeed);
  _pShell->DeclareSymbol( "persistent user FLOAT snd_fDopplerSoundSpeed;", &snd_fDopplerSoundSpeed);
  _pShell->DeclareSymbol( "persistent user FLOAT snd_fPanStrength;", &snd_fPanStrength);
  _pShell->DeclareSymbol( "persistent user FLOAT snd_fLRFilter;",    &snd_fLRFilter);
  _pShell->DeclareSymbol( "persistent user FLOAT snd_fBFilter;",     &snd_fBFilter);
  _pShell->DeclareSymbol( "persistent user FLOAT snd_fUFilter;",     &snd_fUFilter);
  _pShell->DeclareSymbol( "persistent user FLOAT snd_fDFilter;",     &snd_fDFilter);
  _pShell->DeclareSymbol( "persistent user FLOAT snd_fSoundVolume;", &snd_fSoundVolume);
  _pShell->DeclareSymbol( "persistent user FLOAT snd_fMusicVolume;", &snd_fMusicVolume);
  _pShell->DeclareSymbol( "persistent user FLOAT snd_fNormalizer;",  &snd_fNormalizer);
  _pShell->DeclareSymbol( "persistent user FLOAT snd_tmMixAhead post:SndPostFunc;", &snd_tmMixAhead);
  _pShell->DeclareSymbol( "persistent user INDEX snd_iInterface post:SndPostFunc;", &snd_iInterface);
  _pShell->DeclareSymbol( "persistent user INDEX snd_iDevice post:SndPostFunc;", &snd_iDevice);
  _pShell->DeclareSymbol( "persistent user INDEX snd_iFormat post:SndPostFunc;", &snd_iFormat);
  _pShell->DeclareSymbol( "persistent user INDEX snd_iMaxExtraChannels;", &snd_iMaxExtraChannels);
  _pShell->DeclareSymbol( "persistent user INDEX snd_iMaxOpenRetries;",   &snd_iMaxOpenRetries);
  _pShell->DeclareSymbol( "persistent user FLOAT snd_tmOpenFailDelay;",   &snd_tmOpenFailDelay);
  _pShell->DeclareSymbol( "persistent user FLOAT snd_fEAXPanning;", &snd_fEAXPanning);

  // [Cecil] Ignore sounds on a dedicated server
  if (_SE1Setup.IsAppServer()) {
    CPrintF(TRANS("Skipping initialization of sound for dedicated servers...\n"));
    return;
  }

  // print header
  CPrintF(TRANS("Initializing sound...\n"));

  // initialize sound library and set no-sound format
  SetFormat(SF_NONE);

  // initialize any installed sound decoders
  CSoundDecoder::InitPlugins();

#if SE1_PREFER_SDL
  // [Cecil] SDL: List available audio devices
  sl_ctWaveDevices = (INDEX)SDL_GetNumAudioDevices(0);
  CPrintF(TRANS("  Detected devices: %d\n"), sl_ctWaveDevices);

  for (INDEX iDevice = 0; iDevice < sl_ctWaveDevices; iDevice++) {
    CPrintF(TRANS("    device %d: %s\n"), iDevice, SDL_GetAudioDeviceName(iDevice, 0));
  }

#else
  // get number of devices
  const INDEX ctDevices = (INDEX)waveOutGetNumDevs();
  CPrintF(TRANS("  Detected devices: %d\n"), ctDevices);
  sl_ctWaveDevices = ctDevices;
  
  // for each device
  for(INDEX iDevice=0; iDevice<ctDevices; iDevice++) {
    // get description
    WAVEOUTCAPS woc;
    memset( &woc, 0, sizeof(woc));
    MMRESULT res = waveOutGetDevCaps(iDevice, &woc, sizeof(woc));
    CPrintF(TRANS("    device %d: %s\n"), 
      iDevice, woc.szPname);
    CPrintF(TRANS("      ver: %d, id: %d.%d\n"), 
      woc.vDriverVersion, woc.wMid, woc.wPid);
    CPrintF(TRANS("      form: 0x%08x, ch: %d, support: 0x%08x\n"), 
      woc.dwFormats, woc.wChannels, woc.dwSupport);
  }
#endif // SE1_PREFER_SDL

  CPrintF("\n");
};

/*
 *  Clear Sound Library
 */
void CSoundLibrary::Clear(void) {
  // [Cecil] Ignore sounds on a dedicated server
  if (_SE1Setup.IsAppServer()) return;

  // access to the list of handlers must be locked
  CTSingleLock slHooks(&_pTimer->tm_csHooks, TRUE);
  // synchronize access to sounds
  CTSingleLock slSounds(&sl_csSound, TRUE);

  // clear all sounds and datas buffers
  {FOREACHINLIST(CSoundData, sd_Node, sl_ClhAwareList, itCsdStop) {
    FOREACHINLIST(CSoundObject, so_Node, (itCsdStop->sd_ClhLinkList), itCsoStop) {
      itCsoStop->Stop();
    }
    itCsdStop->ClearBuffer();
  }}

  // clear wave out data
  ClearLibrary();
  _fLastNormalizeValue = 1;
};

/* Clear Library WaveOut */
void CSoundLibrary::ClearLibrary(void)
{
  // [Cecil] Ignore sounds on a dedicated server
  if (_SE1Setup.IsAppServer()) return;

  // access to the list of handlers must be locked
  CTSingleLock slHooks(&_pTimer->tm_csHooks, TRUE);

  // synchronize access to sounds
  CTSingleLock slSounds(&sl_csSound, TRUE);

  // remove timer handler if added
  if (sl_thTimerHandler.th_Node.IsLinked()) {
    _pTimer->RemHandler(&sl_thTimerHandler);
  }

  // shut down direct sound buffers (if needed)
  if (sl_pInterface != NULL) {
    sl_pInterface->ShutDown();
  }
};

// mute all sounds (erase playing buffer(s) and supress mixer)
void CSoundLibrary::Mute(void)
{
  // stop all IFeel effects
  IFeel_StopEffect(NULL);

  // [Cecil] Ignore sounds on a dedicated server or when there's no sound interface
  if (_SE1Setup.IsAppServer() || _pSound == NULL || _pSound->sl_pInterface == NULL) return;

  _pSound->sl_pInterface->Mute(_bMutedForMixing);
};

/*
 * set sound format
 */
CSoundLibrary::SoundFormat CSoundLibrary::SetFormat( CSoundLibrary::SoundFormat EsfNew, BOOL bReport/*=FALSE*/)
{
  // [Cecil] Ignore sounds on a dedicated server
  if (_SE1Setup.IsAppServer()) {
    sl_EsfFormat = SF_NONE;
    return SF_NONE;
  }

  // access to the list of handlers must be locked
  CTSingleLock slHooks(&_pTimer->tm_csHooks, TRUE);
  // synchronize access to sounds
  CTSingleLock slSounds(&sl_csSound, TRUE);

  // pause playing all sounds
  {FOREACHINLIST( CSoundData, sd_Node, sl_ClhAwareList, itCsdStop) {
    itCsdStop->PausePlayingObjects();
  }}

  // change format and keep console variable states
  SetFormat_internal(EsfNew, bReport);
  _tmLastMixAhead = snd_tmMixAhead;
  _iLastFormat = snd_iFormat;
  _iLastDevice = snd_iDevice;
  _iLastAPI = snd_iInterface;

  // continue playing all sounds
  CListHead lhToReload;
  lhToReload.MoveList(sl_ClhAwareList);
  {FORDELETELIST( CSoundData, sd_Node, lhToReload, itCsdContinue) {
    CSoundData &sd = *itCsdContinue;
    if( !(sd.sd_ulFlags&SDF_ENCODED)) {
      sd.Reload();
    } else {
      sd.sd_Node.Remove();
      sl_ClhAwareList.AddTail(sd.sd_Node);
    }
    sd.ResumePlayingObjects();
  }}

  // done
  return sl_EsfFormat;
};

/* Update all 3d effects and copy internal data. */
void CSoundLibrary::UpdateSounds(void)
{
  // [Cecil] Ignore sounds on a dedicated server or when there's no sound interface
  if (_SE1Setup.IsAppServer() || _pSound == NULL || _pSound->sl_pInterface == NULL) return;

#if SE1_WIN && SE1_SND_DSOUND
  // see if we have valid handle for direct sound and eventually reinit sound
  if (_pSound->sl_pInterface->GetType() == CAbstractSoundAPI::E_SND_DSOUND) {
    // [Cecil] FIXME: Don't like including the interface just for accessing one of its fields
    CSoundAPI_DSound &apiDSound = (CSoundAPI_DSound &)*_pSound->sl_pInterface;
    extern OS::Window _hwndCurrent;

    if (apiDSound.m_wndCurrent != _hwndCurrent) {
      apiDSound.m_wndCurrent = _hwndCurrent;
      _pSound->SetFormat(_pSound->sl_EsfFormat);
    }
  }
#endif // SE1_WIN && SE1_SND_DSOUND

  _bMutedForMixing = FALSE; // enable mixer
  _sfStats.StartTimer(CStatForm::STI_SOUNDUPDATE);
  _pfSoundProfile.StartTimer(CSoundProfile::PTI_UPDATESOUNDS);

  // synchronize access to sounds
  CTSingleLock slSounds( &_pSound->sl_csSound, TRUE);

  _pSound->sl_pInterface->UpdateEAX();

  // for each sound
  {FOREACHINLIST( CSoundData, sd_Node, _pSound->sl_ClhAwareList, itCsdSoundData) {
    FORDELETELIST( CSoundObject, so_Node, itCsdSoundData->sd_ClhLinkList, itCsoSoundObject) {
      _sfStats.IncrementCounter(CStatForm::SCI_SOUNDSACTIVE);
      itCsoSoundObject->Update3DEffects();
    }
  }}

  // for each sound
  {FOREACHINLIST( CSoundData, sd_Node, _pSound->sl_ClhAwareList, itCsdSoundData) {
    FORDELETELIST( CSoundObject, so_Node, itCsdSoundData->sd_ClhLinkList, itCsoSoundObject) {
      CSoundObject &so = *itCsoSoundObject;
      // if sound is playing
      if( so.so_slFlags&SOF_PLAY) {
        // copy parameters
        so.so_sp = so.so_spNew;
        // prepare sound if not prepared already
        if ( !(so.so_slFlags&SOF_PREPARE)) {
          so.PrepareSound();
          so.so_slFlags |= SOF_PREPARE;
        }
      // if it is not playing
      } else {
        // remove it from list
        so.so_Node.Remove();
      }
    }
  }}

  // remove all listeners
  {FORDELETELIST( CSoundListener, sli_lnInActiveListeners, _pSound->sl_lhActiveListeners, itsli) {
    itsli->sli_lnInActiveListeners.Remove();
  }}

  _pfSoundProfile.StopTimer(CSoundProfile::PTI_UPDATESOUNDS);
  _sfStats.StopTimer(CStatForm::STI_SOUNDUPDATE);
};

/*
 * This is called every TickQuantum seconds.
 */
void CSoundTimerHandler::HandleTimer(void)
{
  /* memory leak checking routines
  ASSERT( _CrtCheckMemory());
  ASSERT( _CrtIsMemoryBlock( (void*)_pSound->sl_pInterface->sl_pswDecodeBuffer,
                             (ULONG)_pSound->sl_pInterface->sl_slDecodeBufferSize, NULL, NULL, NULL));
  ASSERT( _CrtIsValidPointer( (void*)_pSound->sl_pInterface->sl_pswDecodeBuffer,
                              (ULONG)_pSound->sl_pInterface->sl_slDecodeBufferSize, TRUE)); */
  // mix all needed sounds
  _pSound->MixSounds();
};
  
/* Update Mixer */
void CSoundLibrary::MixSounds(void)
{
  // [Cecil] Ignore sounds on a dedicated server or when there's no sound interface
  if (_SE1Setup.IsAppServer() || _pSound == NULL || _pSound->sl_pInterface == NULL) return;

  // synchronize access to sounds
  CTSingleLock slSounds( &_pSound->sl_csSound, TRUE);

  // do nothing if no sound
  if (_pSound->sl_EsfFormat == SF_NONE || _bMutedForMixing) return;

  _sfStats.StartTimer(CStatForm::STI_SOUNDMIXING);
  _pfSoundProfile.IncrementAveragingCounter();
  _pfSoundProfile.StartTimer(CSoundProfile::PTI_MIXSOUNDS);

  // seek available buffer(s) for next crop of samples
  SLONG slDataToMix = _pSound->sl_pInterface->PrepareSoundBuffer();

  // skip mixing if all sound buffers are still busy playing
  ASSERT( slDataToMix>=0);
  if( slDataToMix<=0) {
    _pfSoundProfile.StopTimer(CSoundProfile::PTI_MIXSOUNDS);
    _sfStats.StopTimer(CStatForm::STI_SOUNDMIXING);
    return;
  }

  // prepare mixer buffer
  _pfSoundProfile.IncrementCounter(CSoundProfile::PCI_MIXINGS, 1);
  ResetMixer(_pSound->sl_pInterface->m_pslMixerBuffer, slDataToMix);

  BOOL bGamePaused = _pNetwork->IsPaused() || (_pNetwork->IsServer() && _pNetwork->GetLocalPause());

  // for each sound
  FOREACHINLIST( CSoundData, sd_Node, _pSound->sl_ClhAwareList, itCsdSoundData) {
    FORDELETELIST( CSoundObject, so_Node, itCsdSoundData->sd_ClhLinkList, itCsoSoundObject) {
      CSoundObject &so = *itCsoSoundObject;
      // if the sound is in-game sound, and the game paused
      if (!(so.so_slFlags&SOF_NONGAME) && bGamePaused) {
        // don't mix it it
        continue;
      }
      // if sound is prepared and playing
      if( so.so_slFlags&SOF_PLAY && 
          so.so_slFlags&SOF_PREPARE &&
        !(so.so_slFlags&SOF_PAUSED)) {
        // mix it
        MixSound(&so);
      }
    }
  }

  // eventually normalize mixed sounds
  snd_fNormalizer = Clamp( snd_fNormalizer, 0.0f, 1.0f);
  NormalizeMixerBuffer( snd_fNormalizer, slDataToMix, _fLastNormalizeValue);

  // TEMP! - write mixer buffer to file
  /*
  static FILE *_filMixerBuffer;
  static BOOL _bOpened = FALSE;

  if (!_bOpened) _filMixerBuffer = FileSystem::Open("d:\\MixerBufferDump.raw", "wb");
  fwrite((void *)sl_pInterface->sl_pslMixerBuffer, 1, slDataToMix, _filMixerBuffer);
  _bOpened = TRUE;
  */

  // copy mixer buffer to buffers buffer(s)
  _pSound->sl_pInterface->CopyMixerBuffer(slDataToMix);

  // all done
  _pfSoundProfile.StopTimer(CSoundProfile::PTI_MIXSOUNDS);
  _sfStats.StopTimer(CStatForm::STI_SOUNDMIXING);
};

//
//  Sound mode awareness functions
//

/*
 *  Add sound in sound aware list
 */
void CSoundLibrary::AddSoundAware(CSoundData &CsdAdd) {
  // [Cecil] Ignore sounds on a dedicated server
  if (_SE1Setup.IsAppServer()) return;

  // add sound to list tail
  sl_ClhAwareList.AddTail(CsdAdd.sd_Node);
};

/*
 *  Remove a display mode aware object.
 */
void CSoundLibrary::RemoveSoundAware(CSoundData &CsdRemove) {
  // [Cecil] Ignore sounds on a dedicated server
  if (_SE1Setup.IsAppServer()) return;

  // remove it from list
  CsdRemove.sd_Node.Remove();
};

// listen from this listener this frame
void CSoundLibrary::Listen(CSoundListener &sl)
{
  // [Cecil] Ignore sounds on a dedicated server
  if (_SE1Setup.IsAppServer()) return;

  // just add it to list
  if (sl.sli_lnInActiveListeners.IsLinked()) {
    sl.sli_lnInActiveListeners.Remove();
  }
  sl_lhActiveListeners.AddTail(sl.sli_lnInActiveListeners);
};
