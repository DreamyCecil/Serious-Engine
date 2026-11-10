/* Copyright (c) 2002-2012 Croteam Ltd. 
   Copyright (c) 2023 Dreamy Cecil
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

// [Cecil] ASM code translation:
// https://gitlab.com/TwilightWingsStudio/SSE/SeriousEngineE/-/blob/master/Engine/Sound/SoundMixer.cpp

#include "StdH.h"

#include <Engine/Sound/SoundProfile.h>
#include <Engine/Sound/SoundDecoder.h>
#include <Engine/Sound/SoundLibrary.h>
#include <Engine/Sound/SoundData.h>
#include <Engine/Sound/SoundObject.h>
#include <Engine/Base/Statistics_internal.h>
#include <Engine/Base/Console.h>

// console variables for volume
extern FLOAT snd_fSoundVolume;
extern FLOAT snd_fMusicVolume;
extern INDEX snd_bMono;


// a bunch of local vars coming up

static ULONG ulMixerBufferSampleRate;  // quality of destination buffer
static size_t ctMixerBufferSize;       // size in samples per channel of the destination buffers
static void *pvMixerBuffer;            // pointer to the start of the destination buffers

static SWORD *pswSrcBuffer;
static ULONG ulLeftVolume, ulRightVolume; // [Cecil] Volume may only be positive
static SLONG slLeftFilter, slRightFilter;
static SLONG slLastLeftSample, slLastRightSample;

static ufix16_t fixSoundBufferSize;
static ufix16_t fixLeftStep, fixRightStep, fixLeftOfs, fixRightOfs;

static ULONG mmSurroundFactor;
static UQUAD mmVolumeGain;
static BOOL bNotLoop, bEndOfSound;

// reset mixer buffer (wipes it with zeroes and remembers pointers in static mixer variables)
void ResetMixer(const SLONG *pslBuffer, const size_t ctBufferSize)
{
  // clamp master volumes
  snd_fSoundVolume = Clamp(snd_fSoundVolume, 0.0f, 1.0f);
  snd_fMusicVolume = Clamp(snd_fMusicVolume, 0.0f, 1.0f);

  // cache local variables
  ASSERT(ctBufferSize % 4 == 0);
  pvMixerBuffer     = (void*)pslBuffer;
  ctMixerBufferSize = ctBufferSize / 2 / 2; // because it's stereo and 16-bit dst format
  ulMixerBufferSampleRate = _pSound->sl_afFormat.iSampleRate;

  // wipe destination mixer buffer
  memset(pvMixerBuffer, 0, ctMixerBufferSize * 2 * sizeof(SLONG));
}


// copy mixer buffer to the output buffer(s)
void CopyMixerBuffer_stereo(const size_t ctSrcOffset, void *pDstBuffer, const size_t ctBytes)
{
  ASSERT(pDstBuffer != NULL);
  ASSERT(ctBytes % 4 == 0);
  if (ctBytes < 4) return;

  memcpy(pDstBuffer, ((UBYTE *)pvMixerBuffer) + ctSrcOffset, ctBytes);
}


// copy one channel from mixer buffer to the output buffer(s)
void CopyMixerBuffer_mono(const size_t ctSrcOffset, void *pDstBuffer, const size_t ctBytes)
{
  ASSERT(pDstBuffer != NULL);
  ASSERT(ctBytes % 2 == 0);
  if (ctBytes < 4) return;

  SWORD *pDst = (SWORD *)pDstBuffer;
  const SWORD *pSrc = (SWORD *)(((UBYTE *)pvMixerBuffer) + ctSrcOffset);

  const size_t ct = ctBytes / 4;

  for (size_t i = 0; i < ct; i++) {
    *pDst = *pSrc;
    pDst += 1; // Move 16 bits
    pSrc += 2; // Move 32 bits
  }
}


// plain conversion of mixer buffer from 32-bit to 16-bit clamped
static void ConvertMixerBuffer(const size_t ctBytes)
{
  ASSERT(ctBytes % 4 == 0);
  if (ctBytes < 4) return;

  SWORD *pDst = (SWORD *)pvMixerBuffer;
  const SLONG *pSrc = (SLONG *)pvMixerBuffer;

  const size_t ct = ctBytes / 2;

  for (size_t i = 0; i < ct; i++) {
    *pDst = (SWORD)Clamp(*pSrc, -32767, 32767);
    pDst++;
    pSrc++;
  }
}


// normalize mixer buffer
void NormalizeMixerBuffer(const FLOAT fNormStrength, const size_t ctBytes, FLOAT &fLastNormValue)
{
  // just convert to 16-bit if normalization isn't required
  ASSERT(ctBytes % 4 == 0);
  if (ctBytes < 8) return;

  if (fNormStrength < 0.01f) {
    ConvertMixerBuffer(ctBytes);
    return;
  }

  // well, I guess we'll might need to normalize a bit, so first - find maximum
  size_t i;
  SLONG slPeak = 0;
  SLONG *pslSrc = (SLONG*)pvMixerBuffer;

  const size_t ctSamples = ctBytes / 2; // 16-bit was assumed -> samples (treat as mono)
  for (i = 0; i < ctSamples; i++) {
    slPeak = Max(Abs(pslSrc[i]), slPeak);
  }

  // [Cecil] Avoid potential division by zero
  if (slPeak == 0) {
    fLastNormValue = 1.0f;
    ConvertMixerBuffer(ctBytes);
    return;
  }

  // determine normalize value and skip normalization if maximize is required (do not increase volume!)
  FLOAT fNormValue = 32767.0f / (FLOAT)slPeak;
  if( fNormValue>0.99f && fLastNormValue>0.99f) { // should be enough to tolerate
    fLastNormValue = 1.0f;
    ConvertMixerBuffer(ctBytes);
    return;
  }

  // adjust normalize value by strength
  ASSERT( fNormStrength>=0 && fNormStrength<=1);
  fNormValue = Lerp( 1.0f, fNormValue, fNormStrength);
  const FLOAT fNormAdd = (fNormValue - fLastNormValue) / (ctSamples / 4); 
  
  // normalize (and convert to 16-bit)
  SWORD *pswDst = (SWORD*)pvMixerBuffer;
  FLOAT fCurrentNormValue = fLastNormValue;

  for (i = 0; i < ctSamples; i++) {
    SLONG slSample = FloatToInt(pslSrc[i]*fCurrentNormValue);
    pswDst[i] = (SWORD)Clamp( slSample, -32767L, +32767L);
    fCurrentNormValue = fCurrentNormValue+fNormAdd; // interpolate normalizer
         if( fCurrentNormValue<fNormValue && fNormAdd<0) fCurrentNormValue = fNormValue; // clamp interpolated value
    else if( fCurrentNormValue>fNormValue && fNormAdd>0) fCurrentNormValue = fNormValue;
  }

  // CPrintF( "%.5f -> %.5f (%.5f) @ %.9f / %d\n", fLastNormValue, fCurrentNormValue, fNormValue, fNormAdd, iSamples);
  // remember normalization value
  fLastNormValue = fCurrentNormValue;
}

// [Cecil] Common structure for representing a mixed sample per channel
struct MixedSample {
  SLONG slCur; // Current sample value
  SLONG slNext; // Next sample value
  SWORD swSurround; // Surround factor
};

// [Cecil] Common function for mixing one sample for both channels
__forceinline void MixOneSample(MixedSample left, MixedSample right, SLONG *pslDstBuffer) {
  SLONG &slLeftSample  = left.slCur;
  SLONG &slRightSample = right.slCur;

  const ufix16_t::Type iMask = ufix16_t::iFracMask;
  const ufix16_t::Type iLFactor = fixLeftOfs.holder  & iMask;
  const ufix16_t::Type iRFactor = fixRightOfs.holder & iMask;

  // Interpolate between samples
  slLeftSample  = (slLeftSample  * (iMask - iLFactor) + left.slNext  * iLFactor) >> ufix16_t::iFracSize;
  slRightSample = (slRightSample * (iMask - iRFactor) + right.slNext * iRFactor) >> ufix16_t::iFracSize;

  // Filter samples
  slLastLeftSample  += ((slLeftSample  - slLastLeftSample)  * slLeftFilter)  >> 15;
  slLastRightSample += ((slRightSample - slLastRightSample) * slRightFilter) >> 15;

  // Use the current volume with each loop cycle
  const ULONG ulLeftVolTmp  = ulLeftVolume  >> 16;
  const ULONG ulRightVolTmp = ulRightVolume >> 16;

  // Apply stereo volume to current sample
  slLeftSample  = (slLastLeftSample  * ulLeftVolTmp)  >> 15;
  slRightSample = (slLastRightSample * ulRightVolTmp) >> 15;

  // Properly XOR with the surround factor
  slLeftSample  = SLONG((SWORD)slLeftSample  ^ left.swSurround);
  slRightSample = SLONG((SWORD)slRightSample ^ right.swSurround);

  // Mix in current sample
  slLeftSample  += pslDstBuffer[0];
  slRightSample += pslDstBuffer[1];

  // Limit samples
  slLeftSample  = Clamp(slLeftSample,  (SLONG)MIN_SWORD, (SLONG)MAX_SWORD);
  slRightSample = Clamp(slRightSample, (SLONG)MIN_SWORD, (SLONG)MAX_SWORD);

  // Store samples (both channels)
  pslDstBuffer[0] = slLeftSample;
  pslDstBuffer[1] = slRightSample;

  // Modify volume using 32-bit masks because that's how mmVolumeGain is set per channel
  ulLeftVolume  += (ULONG)((mmVolumeGain >>  0) & 0xFFFFFFFF);
  ulRightVolume += (ULONG)((mmVolumeGain >> 32) & 0xFFFFFFFF);
};

// mixes one mono 16-bit signed sound to destination buffer
inline void MixMono( CSoundObject *pso)
{
  _pfSoundProfile.StartTimer(CSoundProfile::PTI_RAWMIXER);

  // Initialize some local vars
  MixedSample left, right; // [Cecil]
  SLONG *pslDstBuffer = (SLONG *)pvMixerBuffer;

  // [Cecil] Appropriate words of the surround factor for XOR'ing each channel
  left.swSurround  = (SWORD)UWORD((mmSurroundFactor >>  0) & 0xFFFF);
  right.swSurround = (SWORD)UWORD((mmSurroundFactor >> 16) & 0xFFFF);

  // Loop through source buffer
  size_t iCt = ctMixerBufferSize;

  FOREVER {
    // If left channel reached the end of the sample buffer
    if (fixLeftOfs >= fixSoundBufferSize) {
      fixLeftOfs -= fixSoundBufferSize;
      bEndOfSound = bNotLoop; // If it's not looped, end it
    }

    // If right channel reached the end of the sample buffer
    if (fixRightOfs >= fixSoundBufferSize) {
      fixRightOfs -= fixSoundBufferSize;
      bEndOfSound = bNotLoop; // If it's not looped, end it
    }

    // End of buffer
    if (iCt == 0 || bEndOfSound) break;

    // Fetch one lineary interpolated sample on left channel
    left.slCur  = pswSrcBuffer[fixLeftOfs.Floor() + 0];
    left.slNext = pswSrcBuffer[fixLeftOfs.Floor() + 1];

    // Fetch one lineary interpolated sample on right channel
    right.slCur  = pswSrcBuffer[fixRightOfs.Floor() + 0];
    right.slNext = pswSrcBuffer[fixRightOfs.Floor() + 1];

    // [Cecil] Common function for mixing one sample for both channels
    MixOneSample(left, right, pslDstBuffer);

    // Advance to the next sample
    fixLeftOfs += fixLeftStep;
    fixRightOfs += fixRightStep;
    pslDstBuffer += 2;
    iCt--;
  }

  _pfSoundProfile.StopTimer(CSoundProfile::PTI_RAWMIXER);
}


// mixes one stereo 16-bit signed sound to destination buffer
inline void MixStereo( CSoundObject *pso)
{
  _pfSoundProfile.StartTimer(CSoundProfile::PTI_RAWMIXER);

  // Initialize some local vars
  MixedSample left, right; // [Cecil]
  SLONG *pslDstBuffer = (SLONG *)pvMixerBuffer;

  // [Cecil] Appropriate words of the surround factor for XOR'ing each channel
  left.swSurround  = (SWORD)UWORD((mmSurroundFactor >>  0) & 0xFFFF);
  right.swSurround = (SWORD)UWORD((mmSurroundFactor >> 16) & 0xFFFF);

  // Loop through source buffer
  size_t iCt = ctMixerBufferSize;

  FOREVER {
    // If left channel reached the end of the sample buffer
    if (fixLeftOfs >= fixSoundBufferSize) {
      fixLeftOfs -= fixSoundBufferSize;
      bEndOfSound = bNotLoop; // If it's not looped, end it
    }

    // If right channel reached the end of the sample buffer
    if (fixRightOfs >= fixSoundBufferSize) {
      fixRightOfs -= fixSoundBufferSize;
      bEndOfSound = bNotLoop; // If it's not looped, end it
    }

    // End of buffer
    if (iCt == 0 || bEndOfSound) break;

    // [Cecil] Samples go in pairs, so they should be offset as such
    const size_t iLeftOffset  = fixLeftOfs.Floor()  * 2;
    const size_t iRightOffset = fixRightOfs.Floor() * 2;

    // Fetch one lineary interpolated sample on left channel
    left.slCur  = pswSrcBuffer[iLeftOffset + 0];
    left.slNext = pswSrcBuffer[iLeftOffset + 2];

    // Fetch one lineary interpolated sample on right channel
    right.slCur  = pswSrcBuffer[iRightOffset + 1];
    right.slNext = pswSrcBuffer[iRightOffset + 3];

    // [Cecil] Common function for mixing one sample for both channels
    MixOneSample(left, right, pslDstBuffer);

    // Advance to the next sample
    fixLeftOfs += fixLeftStep;
    fixRightOfs += fixRightStep;
    pslDstBuffer += 2;
    iCt--;
  }

  _pfSoundProfile.StopTimer(CSoundProfile::PTI_RAWMIXER);
}


// mixes one sound to destination buffer
void MixSound( CSoundObject *pso)
{
  const CSoundData *psd = pso->so_pCsdLink;
  const BOOL bEncoded = psd->IsEncoded();

  // if don't mix encoded sounds if they are not opened properly
  if (bEncoded && (pso->so_psdcDecoder == NULL || !pso->so_psdcDecoder->IsOpen())) {
    return;
  }

  // check for supported sound formats
  const SEAudioFormat &af = psd->sd_afFormat;

  // unsupported sound formats will be ignored
  if ((af.iChannels != 1 && af.iChannels != 2) || af.iBytesPerSample != 2) return;

  // check for delay
  const DOUBLE fSecondsToMix = (DOUBLE)ctMixerBufferSize / (DOUBLE)ulMixerBufferSampleRate;
  pso->so_fDelayed += fSecondsToMix;
  if( pso->so_fDelayed < pso->so_sp.sp_fDelay) {
    _pfSoundProfile.IncrementCounter(CSoundProfile::PCI_SOUNDSDELAYED, 1);
    return;
  }
  // playing started, so skip further delays
  pso->so_fDelayed = 9999.9999f;

  // reach sound data and determine sound step, sound buffer and buffer size
  pswSrcBuffer = psd->sd_pswBuffer;
  const ufix16_t fixSoundSampleRate = (DOUBLE)af.iSampleRate * (DOUBLE)pso->so_sp.sp_fPitchShift;
  const ufix16_t fixStep = fixSoundSampleRate / ufix16_t(ulMixerBufferSampleRate);
  fixLeftStep  = fixStep;
  fixRightStep = fixStep;
  fixSoundBufferSize = psd->sd_slBufferSampleSize;

  // eliminate potentional "puck" at the of sample that hasn't loop
  if (!(pso->so_slFlags & SOF_LOOP) && fixSoundBufferSize > ufix16_t(1)) fixSoundBufferSize -= ufix16_t(1);

  // get old and new volumes
  const FLOAT fLeftVolume  = ClampDn(pso->so_fLastLeftVolume,  0.0f);
  const FLOAT fRightVolume = ClampDn(pso->so_fLastRightVolume, 0.0f);
  FLOAT fNewLeftVolume  = ClampDn(pso->so_sp.sp_fLeftVolume,  0.0f);
  FLOAT fNewRightVolume = ClampDn(pso->so_sp.sp_fRightVolume, 0.0f);

  // adjust for master volume
  if(pso->so_slFlags&SOF_MUSIC) {
    fNewLeftVolume  *= snd_fMusicVolume;
    fNewRightVolume *= snd_fMusicVolume;
  } else {
    fNewLeftVolume  *= snd_fSoundVolume;
    fNewRightVolume *= snd_fSoundVolume;
  }

  // if both channel volumes are too low
  if( fLeftVolume<0.001f && fRightVolume<0.001f && fNewLeftVolume<0.001f && fNewRightVolume<0.001f)
  {
    // [Cecil] NOTE: This code is responsible for completely skipping playback of unencoded sounds if they aren't immediately
    // audible because those sounds are usually used for momentary sound effects, whereas encoded sounds are used for music and
    // therefore they simply get paused until they're audible again, at which point they continue playing from the paused point

    // if this is not an encoded sound
    if (!bEncoded) {
      // skip mixing of this sample segment
      const ufix16_t fixOfsDelta = fixStep * ufix16_t(ulMixerBufferSampleRate * fSecondsToMix);
      pso->so_fixLeftOffset  += fixOfsDelta;
      pso->so_fixRightOffset += fixOfsDelta;

      // if looping
      if (pso->so_slFlags & SOF_LOOP) {
        // adjust offset ptrs inside sound
        while (pso->so_fixLeftOffset  >= fixSoundBufferSize) pso->so_fixLeftOffset  -= fixSoundBufferSize;
        while (pso->so_fixRightOffset >= fixSoundBufferSize) pso->so_fixRightOffset -= fixSoundBufferSize;

      // if not looping
      } else {
        // no more playing
        pso->so_slFlags  &= ~SOF_PLAY;
        pso->so_fDelayed     = 0.0f;
        pso->so_sp.sp_fDelay = 0.0f;
      }
    }

    // reset last samples
    pso->so_swLastLeftSample  = 0;
    pso->so_swLastRightSample = 0;
    // update volume
    pso->so_fLastLeftVolume  = fNewLeftVolume;
    pso->so_fLastRightVolume = fNewRightVolume;

    _pfSoundProfile.IncrementCounter(CSoundProfile::PCI_SOUNDSSKIPPED, 1);
    return;
  }

  _sfStats.IncrementCounter(CStatForm::SCI_SOUNDSMIXING);

  // cache sound object vars
  DOUBLE fPhase = pso->so_sp.sp_fPhaseShift;
  fixLeftOfs  = pso->so_fixLeftOffset;
  fixRightOfs = pso->so_fixRightOffset;
  const ufix16_t fixOfsDelta = pso->so_fixOffsetDelta;

  const FLOAT fVolumeMul = 65536 * 32767.0f;
  ulLeftVolume  = FloatToInt(fLeftVolume  * fVolumeMul);
  ulRightVolume = FloatToInt(fRightVolume * fVolumeMul);

  const FLOAT fMixBufSize = fVolumeMul / ctMixerBufferSize;
  const SLONG slLeftGain  = FloatToInt((fNewLeftVolume  - fLeftVolume)  * fMixBufSize);
  const SLONG slRightGain = FloatToInt((fNewRightVolume - fRightVolume) * fMixBufSize);
  mmVolumeGain = ((SQUAD)(slRightGain)<<32) | ((SQUAD)(slLeftGain)&0xFFFFFFFF);

  // extrapolate back new volumes because of not enough precision in interpolation!
  // (otherwise we might hear occasional pucks)
  if (fNewLeftVolume  > 0.001f) fNewLeftVolume  = (ulLeftVolume  + slLeftGain  * ctMixerBufferSize) / fVolumeMul;
  if (fNewRightVolume > 0.001f) fNewRightVolume = (ulRightVolume + slRightGain * ctMixerBufferSize) / fVolumeMul;
  //ASSERT( fNewLeftVolume>=0 && fNewRightVolume>=0);
  //CPrintF( "NV: %.4f / %.4f, GV: %.4f / %.4f\n", fNewLeftVolume,fNewRightVolume, fLeftGainedVolume,fRightGainedVolume); 

  // determine filtering and surround 
  slLeftFilter  = pso->so_sp.sp_slLeftFilter;
  slRightFilter = pso->so_sp.sp_slRightFilter;
  bNotLoop = !(pso->so_slFlags & SOF_LOOP);
  mmSurroundFactor = (pso->so_slFlags & SOF_SURROUND) ? 0x0000FFFF : 0x00000000;

  BOOL bDecodingFinished = FALSE;

  // If it's encoded audio
  if (bEncoded) {
    _pfSoundProfile.StartTimer(CSoundProfile::PTI_DECODESOUND);

    // Decode some samples from it
    const ULONG ulSampleSize = af.GetBytesPerFrame();
    const ULONG ulWantedSamples = FloatToInt(ctMixerBufferSize * (DOUBLE)fixStep);
    ASSERT(ulWantedSamples > 0);
    ASSERT(ulWantedSamples <= _pSound->sl_pInterface->m_ctDecodeBufferSize / ulSampleSize);
    UBYTE *pDecodeBuffer = (UBYTE *)_pSound->sl_pInterface->m_pswDecodeBuffer;

    ULONG ulDecodedSamples = pso->so_psdcDecoder->Decode(pDecodeBuffer, ulWantedSamples);
    ASSERT(ulDecodedSamples <= ulWantedSamples);

    // If it's looped
    if (!bNotLoop) {
      // Decode the buffer until it decodes enough samples
      while (ulDecodedSamples < ulWantedSamples) {
        pso->so_psdcDecoder->Reset();

        const ULONG ulBytesOffset = ulDecodedSamples * ulSampleSize;
        ulDecodedSamples += pso->so_psdcDecoder->Decode(pDecodeBuffer + ulBytesOffset, ulWantedSamples - ulDecodedSamples);
      }

    // If it's not looped and the decoded samples are less than the buffer
    } else if (ulDecodedSamples < ulWantedSamples) {
      // Mark it as finished
      bDecodingFinished = TRUE;
    }

    // copy first sample to the last one (this is needed for linear interpolation)
    const ULONG ulBytesEnd = ulDecodedSamples * ulSampleSize;
    memcpy(pDecodeBuffer + ulBytesEnd, pDecodeBuffer, ulSampleSize);

    // fix some mixer variables to play temporary decode buffer instead of real sound
    pswSrcBuffer = (SWORD *)pDecodeBuffer;
    fixSoundBufferSize = ulDecodedSamples;
    fixLeftOfs  = 0;
    fixRightOfs = 0;
    fPhase = 0.0f;

    _pfSoundProfile.StopTimer(CSoundProfile::PTI_DECODESOUND);
  }

  _pfSoundProfile.IncrementCounter(CSoundProfile::PCI_SOUNDSMIXED, 1);
  _pfSoundProfile.IncrementCounter(CSoundProfile::PCI_SAMPLES, (INDEX)ctMixerBufferSize);

  _pfSoundProfile.StartTimer(CSoundProfile::PTI_MIXSOUND);

  slLastLeftSample  = pso->so_swLastLeftSample;
  slLastRightSample = pso->so_swLastRightSample;

  // [Cecil] For divisions by 2
  static const ufix16_t fixHalf(0.5);

  // calculate eventual new offsets from phase shift
  const ufix16_t fixLastPhase = fixOfsDelta / fixSoundSampleRate;
  const DOUBLE fPhaseDelta = fPhase - (DOUBLE)fixLastPhase; // [Cecil] NOTE: Keep DOUBLE because it may be negative
  ufix16_t fixStepDelta = Abs(fPhaseDelta * (DOUBLE)fixSoundSampleRate / ctMixerBufferSize);

  ufix16_t fixStepDeltaL, fixStepDeltaR;

  if (fPhaseDelta > 0) {
    fixStepDeltaL = fixStepDelta * fixHalf;
    if (fixStepDeltaL > fixLeftStep * fixHalf) fixStepDeltaL = fixLeftStep * fixHalf;
    fixStepDeltaL = -fixStepDeltaL;
    fixStepDeltaR = fixStepDelta + fixStepDeltaL;

  } else {
    fixStepDeltaR = fixStepDelta * fixHalf;
    if (fixStepDeltaR > fixLeftStep * fixHalf) fixStepDeltaR = fixLeftStep * fixHalf;
    fixStepDeltaR = -fixStepDeltaR;
    fixStepDeltaL = fixStepDelta + fixStepDeltaR;
  }

  fixLeftStep  += fixStepDeltaL;
  fixRightStep += fixStepDeltaR;
  fixStepDelta = fixStepDeltaR - fixStepDeltaL;

  // if there is anything to mix (could be nothing when encoded file just finished)
  if (fixSoundBufferSize != ufix16_t(0)) {
    // adjust offset ptrs inside sound to match those of phase shift
    while (fixLeftOfs  >= fixSoundBufferSize) fixLeftOfs  -= fixSoundBufferSize;
    while (fixRightOfs >= fixSoundBufferSize) fixRightOfs -= fixSoundBufferSize;

    // if mono output is required
    if( snd_bMono) {
      // monomize channels (cool word:)
      fixLeftOfs = (fixLeftOfs + fixRightOfs) * fixHalf;
      fixRightOfs = fixLeftOfs;
      fixLeftStep = (fixLeftStep + fixRightStep) * fixHalf;
      fixRightStep = fixLeftStep;
      ulLeftVolume = (ulLeftVolume + ulRightVolume) / 2;
      ulRightVolume = ulLeftVolume;
      slLeftFilter = (slLeftFilter + slRightFilter) / 2;
      slRightFilter = slLeftFilter;
    }

    // call corresponding mixer routine for current sound format
    bEndOfSound = FALSE;

    if (af.iChannels == 2) {
      // mix as 16-bit stereo
      MixStereo(pso);
    } else {
      // mix as 16-bit mono
      MixMono(pso);
    }
  }

  // Use decoding finished flag instead of the mixer flag if it's encoded
  if (bEncoded) {
    bEndOfSound = bDecodingFinished;
  }

  // if sound ended, not buffer
  if( bEndOfSound) {
    // reset some sound vars
    slLastLeftSample  = 0;
    slLastRightSample = 0;
    pso->so_slFlags  &= ~SOF_PLAY;
    pso->so_fDelayed     = 0.0f;
    pso->so_sp.sp_fDelay = 0.0f;
  }

  // rememer last samples for the next mix in
  pso->so_swLastLeftSample  = (SWORD)slLastLeftSample;
  pso->so_swLastRightSample = (SWORD)slLastRightSample;
  // determine new phase shift offset
  pso->so_fixOffsetDelta += fixStepDelta * ufix16_t(ctMixerBufferSize);
  // update play offset for the next mix iteration
  pso->so_fixLeftOffset  = fixLeftOfs;
  pso->so_fixRightOffset = fixRightOfs;
  // update volume
  pso->so_fLastLeftVolume  = fNewLeftVolume;
  pso->so_fLastRightVolume = fNewRightVolume;

  //if( pso->so_fLastLeftVolume>0 || pso->so_fLastRightVolume>0 || fNewLeftVolume>0 || fNewRightVolume>0) {
  //  CPrintF( "SO: 0x%8X; OV: %.4f / %.4f, NV: %.4f / %.4f\n", pso,
  //            pso->so_fLastLeftVolume,pso->so_fLastRightVolume, fNewLeftVolume,fNewRightVolume);
  //}
  _pfSoundProfile.StopTimer(CSoundProfile::PTI_MIXSOUND);
}

