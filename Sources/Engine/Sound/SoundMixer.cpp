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

// [Cecil] Some useful fixed-point constants
static const ufix16_t fixZero(0);
static const ufix16_t fixHalf(0.5);
static const sfix16_t fixHalfS(0.5);
static const ufix16_t fixOne(1);
static const sfix16_t fixOneS(1);

// a bunch of local vars coming up

static ULONG ulMixerBufferSampleRate;  // quality of destination buffer
static size_t ctMixerBufferSize;       // size in samples per channel of the destination buffers
static void *pvMixerBuffer;            // pointer to the start of the destination buffers

static SWORD *pswSrcBuffer;
static ULONG ulLeftVolume, ulRightVolume; // [Cecil] Volume may only be positive
static SLONG slLeftFilter, slRightFilter;
static SLONG slLastLeftSample, slLastRightSample;

static ufix16_t fixSoundBufferSize;
static ufix16_t fixLeftStep, fixRightStep, fixLeftOfs, fixRightOfs, fixRelLeftOfs, fixRelRightOfs;

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

  const sfix16_t fixLFactor(fixRelLeftOfs.GetFraction().holder, true);
  const sfix16_t fixRFactor(fixRelRightOfs.GetFraction().holder, true);

  // Interpolate between samples
  slLeftSample  = Floor(sfix16_t(slLeftSample)  * (fixOneS - fixLFactor) + sfix16_t(left.slNext)  * fixLFactor);
  slRightSample = Floor(sfix16_t(slRightSample) * (fixOneS - fixRFactor) + sfix16_t(right.slNext) * fixRFactor);

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

// [Cecil] Mixes one signed 16-bit chunk of audio to destination buffer for the specified amount of channels
inline void MixAudioChunk(const CSoundData *psd, const UWORD iChannels) {
  _pfSoundProfile.StartTimer(CSoundProfile::PTI_RAWMIXER);

  // [Cecil] Currently supported amounts of channels
  ASSERT(iChannels == 1 || iChannels == 2);

  // Initialize some local vars
  MixedSample left, right; // [Cecil]
  SLONG *pslDstBuffer = (SLONG *)pvMixerBuffer;
  const bool bEncoded = psd->IsEncoded();

  // [Cecil] Appropriate words of the surround factor for XOR'ing each channel
  left.swSurround  = (SWORD)UWORD((mmSurroundFactor >>  0) & 0xFFFF);
  right.swSurround = (SWORD)UWORD((mmSurroundFactor >> 16) & 0xFFFF);

  // [Cecil] Set relative offsets to the fraction between the first two frames for the current decoded chunk
  if (bEncoded) {
    // [Cecil] FIXME: Add phase difference to the channel that's ahead
    fixRelLeftOfs  = fixLeftOfs.GetFraction();
    fixRelRightOfs = fixRightOfs.GetFraction();

  // Otherwise continue from absolute offsets
  } else {
    fixRelLeftOfs  = fixLeftOfs;
    fixRelRightOfs = fixRightOfs;
  }

  // Loop through the source buffer
  size_t ct = ctMixerBufferSize;

  FOREVER {
    // [Cecil] NOTE: If non-encoded sounds reach the end of the static sound buffer, they must wrap offsets around
    // to prevent them from reading samples out of bounds. This isn't the case for encoded sounds for two reasons:
    // 1. Encoded sounds are processed one chunk at a time, so wrapping around would mean returning to the beginning
    // of that chunk instead of the beginning of the next chunk or the beginning of the audio altogether.
    // 2. Looped encoded sounds decode as much data as the mixer can take at a time, so it decodes audio in a loop
    // from the beginning inside the chunks themselves, which makes these offset loops unnecessary during mixing.

    // If left channel reached the end of the sample buffer
    if (fixRelLeftOfs >= fixSoundBufferSize) {
      // [Cecil] Only wrap it around if it's mixing a fully preloaded sound
      if (!bEncoded) fixRelLeftOfs -= fixSoundBufferSize;

      // Looped sounds will keep getting mixed until the end of the mixer buffer instead of exiting preemptively
      bEndOfSound = bNotLoop;
    }

    // If right channel reached the end of the sample buffer
    if (fixRelRightOfs >= fixSoundBufferSize) {
      // [Cecil] Only wrap it around if it's mixing a fully preloaded sound
      if (!bEncoded) fixRelRightOfs -= fixSoundBufferSize;

      // Looped sounds will keep getting mixed until the end of the mixer buffer instead of exiting preemptively
      bEndOfSound = bNotLoop;
    }

    // End of buffer
    if (ct == 0 || bEndOfSound) break;

    // [Cecil] Samples go in order of channels, so they should be offset as such
    const UQUAD iFrameL = Floor(fixRelLeftOfs)  * iChannels;
    const UQUAD iFrameR = Floor(fixRelRightOfs) * iChannels;

    // Mono: 0 1, 0 1 / Stereo: 0 2, 1 3
    const UQUAD iCurL  = iFrameL;                                         // First sample of current frame
    const UQUAD iNextL = iFrameL + iChannels;                             // First sample of next frame
    const UQUAD iCurR  = iFrameR + ClampUp(iChannels - 1, 1);             // Second sample of current frame
    const UQUAD iNextR = iFrameR + ClampUp(iChannels - 1, 1) + iChannels; // Second sample of next frame

    // Fetch one lineary interpolated sample on left channel
    left.slCur  = pswSrcBuffer[iCurL];
    left.slNext = pswSrcBuffer[iNextL];

    // Fetch one lineary interpolated sample on right channel
    right.slCur  = pswSrcBuffer[iCurR];
    right.slNext = pswSrcBuffer[iNextR];

    // [Cecil] Common function for mixing one sample for both channels
    MixOneSample(left, right, pslDstBuffer);

    // Advance to the next sample
    fixLeftOfs  += fixLeftStep;
    fixRightOfs += fixRightStep;
    fixRelLeftOfs  += fixLeftStep;
    fixRelRightOfs += fixRightStep;
    pslDstBuffer += 2;

    --ct;
  }

  _pfSoundProfile.StopTimer(CSoundProfile::PTI_RAWMIXER);
};

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

  // [Cecil] Keep track of total audio length instead of the length of the sample buffer
  ufix16_t fixTotalAudioLength = fixSoundBufferSize;

  // eliminate potentional "puck" at the of sample that hasn't loop
  if (!(pso->so_slFlags & SOF_LOOP) && fixSoundBufferSize > fixOne) fixSoundBufferSize -= fixOne;

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
        // [Cecil] Wrap offsets around based on the total audio length (and not the current chunk)
        while (pso->so_fixLeftOffset  >= fixTotalAudioLength) pso->so_fixLeftOffset  -= fixTotalAudioLength;
        while (pso->so_fixRightOffset >= fixTotalAudioLength) pso->so_fixRightOffset -= fixTotalAudioLength;

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
  const sfix16_t fixOfsDelta = pso->so_fixOffsetDelta;

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

    // [Cecil] Retrieve decoded frames so far and add [non-rounded amount that needs to be decoded right now] to it
    const ufix16_t fixReadSoFar = pso->so_fixDecodedFrames;
    pso->so_fixDecodedFrames += ufix16_t(ctMixerBufferSize) * fixStep;

    // [Cecil] Take into account all non-integer amounts of frames read so far instead of rounding up wanted amount each time
    ULONG ulWantedFrames = Ceil(pso->so_fixDecodedFrames.Ceil() - fixReadSoFar) + 1; // +1 extra frame for interpolation in the end
    ASSERT(ulWantedFrames != 0);

    // [Cecil] Reallocate decoder buffer if wanted frames don't fit
    const ULONG ulFrameSize = af.GetBytesPerFrame();

    if (ulWantedFrames > _pSound->sl_pInterface->m_ctDecodeBufferSize / ulFrameSize) {
      // Allocate another second of frames on top, just in case
      _pSound->sl_pInterface->ReallocDecoderBuffer((ulWantedFrames + af.iSampleRate) * ulFrameSize);
    }

    // [Cecil] Seek decoder to the current frame regardless of how much has been decoded last time
    // [Cecil] FIXME: Offsets per channel may not always be equal
    pso->so_psdcDecoder->SeekToFrame(Floor(fixLeftOfs));

    // Decode some frames of audio
    SWORD *pDecodeBuffer = _pSound->sl_pInterface->m_pswDecodeBuffer;
    ULONG ulDecodedFrames = pso->so_psdcDecoder->Decode(pDecodeBuffer, ulWantedFrames);
    ASSERT(ulDecodedFrames <= ulWantedFrames);

    // If it's looped
    if (!bNotLoop) {
      // Keep decoding until it reads enough frames
      while (ulDecodedFrames < ulWantedFrames) {
        // [Cecil] Reset decoder to the beginning
        pso->so_psdcDecoder->SeekToFrame(0);

        const ULONG ulSamplesOffset = ulDecodedFrames * af.iChannels;
        ulDecodedFrames += pso->so_psdcDecoder->Decode(pDecodeBuffer + ulSamplesOffset, ulWantedFrames - ulDecodedFrames);
      }

    // If it's not looped and the decoded frames are less than the buffer
    } else if (ulDecodedFrames < ulWantedFrames) {
      // Mark it as finished
      bDecodingFinished = TRUE;
    }

    // Use freshly acquired data from the decoder instead of the static buffer from sound data
    pswSrcBuffer = pDecodeBuffer;

    // Amount of frames to mix, minus one used for interpolation (very last one)
    fixSoundBufferSize = ulDecodedFrames - 1;
    fixTotalAudioLength = pso->so_psdcDecoder->GetLengthInFrames();

    // [Cecil] TEMP: Really not sure about this one, needs more specific testing regarding phase shift
    //fPhase = 0.0f;

    _pfSoundProfile.StopTimer(CSoundProfile::PTI_DECODESOUND);
  }

  _pfSoundProfile.IncrementCounter(CSoundProfile::PCI_SOUNDSMIXED, 1);
  _pfSoundProfile.IncrementCounter(CSoundProfile::PCI_SAMPLES, (INDEX)ctMixerBufferSize);

  _pfSoundProfile.StartTimer(CSoundProfile::PTI_MIXSOUND);

  slLastLeftSample  = pso->so_swLastLeftSample;
  slLastRightSample = pso->so_swLastRightSample;

  // calculate eventual new offsets from phase shift
  const DOUBLE fLastPhase = (DOUBLE)fixOfsDelta / (DOUBLE)fixSoundSampleRate;
  const DOUBLE fPhaseDelta = fPhase - fLastPhase; // [Cecil] NOTE: Keep DOUBLE because it may be negative
  sfix16_t fixStepDelta = Abs(fPhaseDelta * (DOUBLE)fixSoundSampleRate / ctMixerBufferSize);

  sfix16_t fixStepDeltaL, fixStepDeltaR;

  // [Cecil] Whatever
  sfix16_t fixSignedHalfStep;
  fixSignedHalfStep.holder = (fixLeftStep * fixHalf).holder;

  if (fPhaseDelta > 0) {
    fixStepDeltaL = fixStepDelta * fixHalfS;
    if (fixStepDeltaL > fixSignedHalfStep) fixStepDeltaL = fixSignedHalfStep;
    fixStepDeltaL = -fixStepDeltaL;
    fixStepDeltaR = fixStepDelta + fixStepDeltaL;

  } else {
    fixStepDeltaR = fixStepDelta * fixHalfS;
    if (fixStepDeltaR > fixSignedHalfStep) fixStepDeltaR = fixSignedHalfStep;
    fixStepDeltaR = -fixStepDeltaR;
    fixStepDeltaL = fixStepDelta + fixStepDeltaR;
  }

  // [Cecil] Whatever 2
  ((SQUAD &)fixLeftStep.holder)  += fixStepDeltaL.holder;
  ((SQUAD &)fixRightStep.holder) += fixStepDeltaR.holder;
  fixStepDelta = fixStepDeltaR - fixStepDeltaL;

  // if there is anything to mix (could be nothing when encoded file just finished)
  if (fixSoundBufferSize != fixZero) {
    // [Cecil] Wrap offsets around based on the total audio length (and not the current chunk)
    while (fixLeftOfs  >= fixTotalAudioLength) fixLeftOfs  -= fixTotalAudioLength;
    while (fixRightOfs >= fixTotalAudioLength) fixRightOfs -= fixTotalAudioLength;

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

    // [Cecil] Mix this audio chunk for the requested amount of channels
    MixAudioChunk(psd, af.iChannels);

    // [Cecil] Wrap offsets around if they ended up going past the total audio length (and not the current chunk)
    while (fixLeftOfs  >= fixTotalAudioLength) fixLeftOfs  -= fixTotalAudioLength;
    while (fixRightOfs >= fixTotalAudioLength) fixRightOfs -= fixTotalAudioLength;
  }

  // Use decoding finished flag instead of the mixer flag if it's encoded
  if (bEncoded) {
    // [Cecil] FIXME: This stops sound playback preemptively if mixing is delayed in any way
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
  pso->so_fixOffsetDelta += fixStepDelta * sfix16_t(ctMixerBufferSize);
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

