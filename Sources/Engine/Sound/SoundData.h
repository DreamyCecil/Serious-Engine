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

#ifndef SE_INCL_SOUNDDATA_H
#define SE_INCL_SOUNDDATA_H
#ifdef PRAGMA_ONCE
  #pragma once
#endif

#include <Engine/Base/Lists.h>
#include <Engine/Base/Serial.h>

class ENGINE_API CSoundData : public CSerial {
public:
  enum ESoundDataFlags {
    // Indicates an encoded sound that requires a dedicated decoder to play. When CSoundData is marked with this flag,
    // CSoundObject will create and utilize its own decoder for the specified file using CSoundObject::so_psdcDecoder
    SDF_ENCODED   = (1UL << 0),

    // Indicates that a sound is being played directly from disk instead of being fully loaded into memory
    // Currently, only used by encoded files, if needed. Regular unencoded sounds don't support streaming
    SDF_STREAMING = (1UL << 1),
  };

public:
  // Sound Mode Aware class (notify class when sound mode change)
  CListNode   sd_Node;        // for linking in list
  ULONG sd_ulFlags;           // flags

//private:
public:
  // Call-back called when sound mode changes.
  void ModeChanged(void);

  inline BOOL IsHooked(void) const { return sd_Node.IsLinked(); };
  CListHead sd_ClhLinkList;			// list of objects linked to data

  void PausePlayingObjects(void);
  void ResumePlayingObjects(void);

  // Sound Buffer
  SEAudioFormat sd_afFormat;     // primary sound buffer format
  SWORD *sd_pswBuffer;           // pointer on buffer
  SLONG  sd_slBufferSampleSize;  // buffer sample size
  SECOND sd_dSecondsLength;      // sound length in seconds

  // [Cecil] Static decoder for any sound object for decoding audio from memory instead of streaming it from disk
  class CSoundDecoder *sd_psdcStaticDecoder;

  // free Buffer (and all linked Objects)
  void ClearBuffer(void);
  // Add object in sound aware list
  void AddObjectLink(CSoundObject &CsoAdd);
  // Remove an object from aware list
  void RemoveObjectLink(CSoundObject &CsoRemove);
  // reference counting functions
  void AddReference(void);
  void RemReference(void);
public:
  // Constructor
  CSoundData();
  // Destructor
  ~CSoundData();

  inline bool IsEncoded(void) const {
    return !!(sd_ulFlags & SDF_ENCODED);
  };

  inline bool IsStreamed(void) const {
    return !!(sd_ulFlags & SDF_STREAMING);
  };

  // get sound length in seconds
  SECOND GetSecondsLength(void);
  // read sound from file and convert it to the current sound format
  void  Read_t(CTStream *inFile);        // throw char *
  // write sound to file (not implemented)
  void  Write_t(CTStream *outFile);       // throw char *
  /* Get the description of this object. */
  CTString GetDescription(void);
  // free allocated memory for sound and Sound in DXBuffer
  void  Clear(void);
  // check if this kind of objects is auto-freed
  virtual BOOL IsAutoFreed(void);
  // get amount of memory used by this object
  SLONG GetUsedMemory(void);
};


#endif  /* include-once check. */

