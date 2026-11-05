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

public:
  // [Cecil] Initialize/clean up the new decoder
  static void InitPlugins(void);
  static void EndPlugins(void);

  CSoundDecoder() : sdc_pData(NULL) {};
  ~CSoundDecoder(void) { Clear(); };

  // [Cecil] Initialize the decoder for a specific file
  void Initialize(const CTString &fnmStream);

  // Clear the decoder
  void Clear(void);

  // Check if the decoder opened successfully
  BOOL IsOpen(void) const;

  // Get audio format from the decoder (only if it's open)
  void GetFormat(WAVEFORMATEX &wfe) const;

  // [Cecil] Retrieve length of the encoded audio in seconds
  SECOND GetSoundLength(void) const;

  // [Cecil] Decode a block of samples and return amount of read samples
  ULONG Decode(void *pvDestBuffer, ULONG ctSamplesToDecode);

  // Reset decoder to the first sample
  void Reset(void);
};
