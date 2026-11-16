/* Copyright (c) 2002-2012 Croteam Ltd.
   Copyright (c) 2026 Dreamy Cecil
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

#pragma once

class CSoundDecoder {
private:
  struct DecodeData *sdc_pData;
  CTString sdc_strError; // [Cecil] Contains error message upon failure to initialize

public:
  // [Cecil] Initialize/clean up the new decoder
  static void InitPlugins(void);
  static void EndPlugins(void);

  CSoundDecoder() : sdc_pData(NULL) {};
  ~CSoundDecoder(void) { Clear(); };

  // [Cecil] Initialize the decoder for a specific file
  // If bLoadIntoMemory is set to true, always reads encoded audio directly
  // into memory to decode from there instead of streaming it from disk
  void Initialize(const CTString &fnmFile, bool bLoadIntoMemory);

  // [Cecil] Initialize the decoder for a stream to immediately read data from
  // If bLoadIntoMemory is set to true, always reads encoded audio directly
  // into memory to decode from there instead of streaming it from disk
  void Initialize(CTStream *istrReadFrom, bool bLoadIntoMemory);

  // [Cecil] Internal initialization routine that sets up the decoder for streaming from a file on disk
  // or inside an archive only if an expanded path to the file has been provided (otherwise falling back
  // to loading audio into memory if the file needs to be decompressed or only the stream is provided)
  void Initialize_internal(CTStream *istrReadFrom, const ExpandPath *pExpath);

  // [Cecil] Get error message
  inline const CTString &GetError(void) const {
    return sdc_strError;
  };

  // Clear the decoder
  void Clear(void);

  // Check if the decoder opened successfully (if it returns FALSE, use GetError() to find out why)
  BOOL IsOpen(void) const;

  // Get audio format from the decoder (only if it's open)
  void GetFormat(SEAudioFormat &af) const;

  // [Cecil] Check whether the sound is decoded from memory instead of streamed from disk
  BOOL IsInMemory(void) const;

  // [Cecil] Retrieve length of the encoded audio in seconds
  SECOND GetSoundLength(void) const;

  // [Cecil] Retrieve length of the encoded audio in frames (samples * channels)
  ULONG GetLengthInFrames(void) const;

  // [Cecil] Decode a block of frames and return amount of read frames
  ULONG Decode(void *pvDestBuffer, ULONG ctFramesToDecode);

  // [Cecil] Decode as many frames as there are left, up to GetLengthInFrames(), and return amount of read frames
  ULONG DecodeAll(void *pvDestBuffer);

  // [Cecil] Set next decoding position to a specific frame
  void SeekToFrame(ULONG iFrame);
};
