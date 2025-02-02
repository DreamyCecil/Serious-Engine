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

#ifndef SE_INCL_SOUNDLIBRARY_H
#define SE_INCL_SOUNDLIBRARY_H
#ifdef PRAGMA_ONCE
  #pragma once
#endif


#include <Engine/Base/Lists.h>
#include <Engine/Base/Timer.h>
#include <Engine/Base/Synchronization.h>
#include <Engine/Templates/StaticArray.h>
#include <Engine/Templates/StaticStackArray.h>
#include <Engine/Templates/DynamicArray.h>

// [Cecil] Sound interfaces
#include <Engine/Sound/SoundAPI.h>

// Mixer
// set master volume and resets mixer buffer (wipes it with zeroes and keeps pointers)
void ResetMixer( const SLONG *pslBuffer, const SLONG slBufferSize);
// copy mixer buffer to the output buffer(s)
void CopyMixerBuffer_stereo( const SLONG slSrcOffset, void *pDstBuffer, const SLONG slBytes);
void CopyMixerBuffer_mono(   const SLONG slSrcOffset, void *pDstBuffer, const SLONG slBytes);
// normalize mixed sounds
void NormalizeMixerBuffer( const FLOAT snd_fNormalizer, const SLONG slBytes, FLOAT &_fLastNormalizeValue);
// mix in one sound object to mixer buffer
void MixSound( class CSoundObject *pso);


/*
 * Timer handler for sound mixing.
 */
class ENGINE_API CSoundTimerHandler : public CTimerHandler {
public:
  /* This is called every TickQuantum seconds. */
  virtual void HandleTimer(void);
};

/*
 *  Sound Library class
 */
class ENGINE_API CSoundLibrary {
public:
  enum SoundFormat {
    SF_NONE     = 0,
    SF_11025_16 = 1,
    SF_22050_16 = 2,
    SF_44100_16 = 3,
    SF_ILLEGAL  = 4
  };

//private:
public:
  CTCriticalSection sl_csSound;          // sync. access to sounds
  CSoundTimerHandler sl_thTimerHandler;  // handler for mixing sounds in timer

  SoundFormat sl_EsfFormat; // sound format (external)
  CAbstractSoundAPI *sl_pInterface; // [Cecil] Currently used interface
  WAVEFORMATEX sl_SwfeFormat; // primary sound buffer format

  CListHead sl_ClhAwareList; // list of sound mode aware objects
  CListHead sl_lhActiveListeners; // active listeners for current frame of listening

  /* Return library state (active <==> format <> NONE */
  inline BOOL IsActive(void) {return sl_EsfFormat != SF_NONE;};
  /* Clear Library WaveOut */
  void ClearLibrary(void);

private:
  // [Cecil] Destroy current sound interface
  void DestroyInterface(void);

  // [Cecil] Internal functionality
  void SetWaveFormat(SoundFormat EsfFormat);
  void SetLibraryFormat(void);
  void SetFormat_internal(SoundFormat EsfNew, BOOL bReport);

public:
  /* Constructor */
  CSoundLibrary(void);
  /* Destructor */
  ~CSoundLibrary(void);
  DECLARE_NOCOPYING(CSoundLibrary);

  /* Initialization */
  void Init(void);
  /* Clear Sound Library */
  void Clear(void);

  /* Set Format */
  SoundFormat SetFormat( SoundFormat EsfNew, BOOL bReport=FALSE);
  /* Get Format */
  inline SoundFormat GetFormat(void) { return sl_EsfFormat; };

  /* Update all 3d effects and copy internal data. */
  static void UpdateSounds(void);
  /* Update Mixer */
  static void MixSounds(void);
  /* Mute output until next UpdateSounds() */
  static void Mute(void);

  /* Add sound in sound aware list */
  void AddSoundAware( CSoundData &CsdAdd);
  /* Remove a sound mode aware object */
  void RemoveSoundAware( CSoundData &CsdRemove);

  // listen from this listener this frame
  void Listen(CSoundListener &sl);
};


// pointer to global sound library object
ENGINE_API extern CSoundLibrary *_pSound;


#endif  /* include-once check. */

