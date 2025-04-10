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

#include <Engine/Sound/SoundDecoder.h>
#include <Engine/Base/Stream.h>
#include <Engine/Base/Console.h>
#include <Engine/Base/ErrorReporting.h>
#include <Engine/Base/FileName.h>
#include <Engine/Base/Unzip.h>
#include <Engine/Base/Translation.h>
#include <Engine/Math/Functions.h>

// generic function called if a dll function is not found
static void FailFunction_t(const char *strName) {
  ThrowF_t(TRANS("Function %s not found."), strName);
}


// ------------------------------------ AMP11

// amp11lib vars
static BOOL _bAMP11Enabled = FALSE;
static HINSTANCE _hAmp11lib = NULL;

// amp11lib types
typedef signed char ALsint8;
typedef unsigned char ALuint8;
typedef signed short ALsint16;
typedef unsigned short ALuint16;
typedef signed int ALsint32;
typedef unsigned int ALuint32;
typedef signed int ALsize;
typedef int ALbool;
typedef float ALfloat;
#define ALtrue  1
#define ALfalse 0
typedef ALsint32 ALhandle;

// define amp11lib function pointers
#define DLLFUNCTION(dll, output, name, inputs, params, required) \
  output (__stdcall *p##name) inputs = NULL;
#include "al_functions.h"
#undef DLLFUNCTION

static void AMP11_SetFunctionPointers_t(void) {
  const char *strName;
  // get amp11lib function pointers
  #define DLLFUNCTION(dll, output, name, inputs, params, required) \
    strName = "_" #name "@" #params;  \
    p##name = (output (__stdcall *)inputs)OS::GetLibSymbol(_hAmp11lib, strName); \
    if(p##name == NULL) FailFunction_t(strName);
  #include "al_functions.h"
  #undef DLLFUNCTION
}

static void AMP11_ClearFunctionPointers(void) {
  // clear amp11lib function pointers
  #define DLLFUNCTION(dll, output, name, inputs, params, required) p##name = NULL;
  #include "al_functions.h"
  #undef DLLFUNCTION
}

class CDecodeData_MPEG {
public:
  ALhandle mpeg_hMainFile; // mainfile handle if using subfile
  ALhandle mpeg_hFile;     // file handle
  ALhandle mpeg_hDecoder;  // the decoder handle
  FLOAT mpeg_fSecondsLen;  // length of sound in seconds
  WAVEFORMATEX mpeg_wfeFormat; // format of sound
};


// ------------------------------------ Ogg Vorbis
#include <vorbis/vorbisfile.h>  // we define needed stuff ourselves, and ignore the rest

#if !defined(SE1_STATIC_BUILD)
  // vorbis vars
  static BOOL _bOVEnabled = FALSE;
  static HINSTANCE _hOV = NULL;

#elif SE1_WIN
  #pragma comment(lib, "libogg.lib")
  #pragma comment(lib, "libvorbis.lib")
  #pragma comment(lib, "libvorbisfile.lib")
#endif

class CDecodeData_OGG {
public:
  FILE *ogg_fFile;      // the stdio file that ogg is in
  SLONG ogg_slOffset;   // offset where the ogg starts in the file (!=0 for oggs in zip)
  SLONG ogg_slSize;     // size of ogg in the file (!=filesize for oggs in zip)
  OggVorbis_File *ogg_vfVorbisFile;  // the decoder file
  WAVEFORMATEX ogg_wfeFormat; // format of sound
};

// define vorbis function pointers
#define DLLFUNCTION(dll, output, name, inputs, params, required) \
  output (__cdecl *p##name) inputs = NULL;
#include "ov_functions.h"
#undef DLLFUNCTION

static void OV_SetFunctionPointers_t(void) {
  const char *strName;

  // get vo function pointers
#if !defined(SE1_STATIC_BUILD)
  #define DLLFUNCTION(dll, output, name, inputs, params, required) \
    strName = #name ;  \
    p##name = (output (__cdecl *)inputs)OS::GetLibSymbol(_hOV, strName); \
    if(p##name == NULL) FailFunction_t(strName);
#else
  #define DLLFUNCTION(dll, output, name, inputs, params, required) \
    strName = #name ;  \
    p##name = &name; \
    if(p##name == NULL) FailFunction_t(strName);
#endif

  #include "ov_functions.h"
  #undef DLLFUNCTION
}
static void OV_ClearFunctionPointers(void) {
  // clear vo function pointers
  #define DLLFUNCTION(dll, output, name, inputs, params, required) p##name = NULL;
  #include "ov_functions.h"
  #undef DLLFUNCTION
}

// ogg file reading callbacks
//

static size_t ogg_read_func  (void *ptr, size_t size, size_t nmemb, void *datasource)
{
  CDecodeData_OGG *pogg = (CDecodeData_OGG *)datasource;
  // calculate how much can be read at most
  size_t slToRead = size*nmemb;
  size_t slCurrentPos = ftell(pogg->ogg_fFile)-pogg->ogg_slOffset;
  size_t slSizeLeft = ClampDn(size_t(pogg->ogg_slSize) - slCurrentPos, (size_t)0);
  slToRead = ClampUp(slToRead, slSizeLeft);

  // rounded down to the block size
  slToRead/=size;
  slToRead*=size;
  // if there is nothing to read
  if (slToRead<=0) {
    return 0;
  }
  return fread(ptr, size, slToRead/size, pogg->ogg_fFile);
}

static int ogg_seek_func  (void *datasource, ogg_int64_t offset, int whence)
{
  return -1;
/*  !!!! seeking is evil with vorbisfile 1.0RC2
  CDecodeData_OGG *pogg = (CDecodeData_OGG *)datasource;
  SLONG slCurrentPos = ftell(pogg->ogg_fFile)-pogg->ogg_slOffset;
  if (whence==SEEK_CUR) {
    return fseek(pogg->ogg_fFile, offset, SEEK_CUR);
  } else if (whence==SEEK_END) {
    return fseek(pogg->ogg_fFile, pogg->ogg_slOffset+pogg->ogg_slSize-offset, SEEK_SET);
  } else {
    ASSERT(whence==SEEK_SET);
    return fseek(pogg->ogg_fFile, pogg->ogg_slOffset+offset, SEEK_SET);
  }
*/
}

static int ogg_close_func (void *datasource)
{
  return 0;
/* !!!! closing is evil with vorbisfile 1.0RC2
  CDecodeData_OGG *pogg = (CDecodeData_OGG *)datasource;
  fclose(pogg->ogg_fFile);
  */
}
static long ogg_tell_func  (void *datasource)
{
  return -1;
/*  !!!! seeking is evil with vorbisfile 1.0RC2
  CDecodeData_OGG *pogg = (CDecodeData_OGG *)datasource;
  ftell(pogg->ogg_fFile)-pogg->ogg_slOffset;
  */
}

static ov_callbacks ovcCallbacks = {
  ogg_read_func,
  ogg_seek_func,
  ogg_close_func,
  ogg_tell_func,
};


// initialize/end the decoding support engine(s)
void CSoundDecoder::InitPlugins(void)
{
  try {
  #if !defined(SE1_STATIC_BUILD)
    // load vorbis
    if (_hOV==NULL) {
      _hOV = OS::LoadLib("libvorbisfile.dll");
    }
    if( _hOV == NULL) {
      ThrowF_t(TRANS("Cannot load libvorbisfile.dll."));
    }
  #endif

    // prepare function pointers
    OV_SetFunctionPointers_t();

  #if !defined(SE1_STATIC_BUILD)
    // if all successful, enable mpx playing
    _bOVEnabled = TRUE;
  #endif

    CPrintF(TRANS("  libvorbisfile.dll loaded, ogg playing enabled\n"));

  } catch (char *strError) {
    CPrintF(TRANS("OGG playing disabled: %s\n"), strError);
  }

  try {
    // load amp11lib
    if (_hAmp11lib==NULL) {
      _hAmp11lib = OS::LoadLib("amp11lib.dll");
    }
    if( _hAmp11lib == NULL) {
      ThrowF_t(TRANS("Cannot load amp11lib.dll."));
    }
    // prepare function pointers
    AMP11_SetFunctionPointers_t();

    // initialize amp11lib before calling any of its functions
    palInitLibrary();

    // if all successful, enable mpx playing
    _bAMP11Enabled = TRUE;
    CPrintF(TRANS("  amp11lib.dll loaded, mpx playing enabled\n"));

  } catch (char *strError) {
    CPrintF(TRANS("MPX playing disabled: %s\n"), strError);
  }
}

void CSoundDecoder::EndPlugins(void)
{
  // cleanup amp11lib when not needed anymore
  if (_bAMP11Enabled) {
    palEndLibrary();
    AMP11_ClearFunctionPointers();
    OS::FreeLib(_hAmp11lib);
    _hAmp11lib = NULL;
    _bAMP11Enabled = FALSE;
  }

  // cleanup vorbis when not needed anymore
#if !defined(SE1_STATIC_BUILD)
  if (_bOVEnabled) {
    OV_ClearFunctionPointers();
    OS::FreeLib(_hOV);
    _hOV = NULL;
    _bOVEnabled = FALSE;
  }

#else
  OV_ClearFunctionPointers();
#endif
}

// decoder that streams from file
CSoundDecoder::CSoundDecoder(const CTFileName &fnm)
{
  sdc_pogg = NULL;
  sdc_pmpeg = NULL;

  // [Cecil] Ignore sounds on a dedicated server
  if (_SE1Setup.IsAppServer()) return;

  ExpandPath expath;

  // [Cecil] No file to read
  if (!expath.ForReading(fnm, 0)) {
    CPrintF(TRANS("Cannot open encoded audio '%s' for streaming: %s\n"), fnm.ConstData(), TRANS("file not found"));
    return;
  }

  const CTString &fnmExpanded = expath.fnmExpanded;

  // if ogg
  if (fnmExpanded.FileExt()==".ogg") {
  #if !defined(SE1_STATIC_BUILD)
    if (!_bOVEnabled) return;
  #endif

    sdc_pogg = new CDecodeData_OGG;
    sdc_pogg->ogg_fFile = NULL;
    sdc_pogg->ogg_vfVorbisFile = NULL;
    sdc_pogg->ogg_slOffset = 0;
    sdc_pogg->ogg_slSize = 0;
    IZip::Handle_t pZipHandle = NULL;

    try {
      // if in zip
      if (expath.bArchive) {
        // open it
        pZipHandle = IZip::Open_t(fnmExpanded);
        const IZip::CEntry *pEntry = IZip::GetEntry(pZipHandle);

        // if compressed
        if (!pEntry->IsStored()) {
          ThrowF_t(TRANS("encoded audio in archives must not be compressed!\n"));
        }
        // open ogg file
        sdc_pogg->ogg_fFile = FileSystem::Open(pEntry->GetArchive().ConstData(), "rb");
        // if error
        if (sdc_pogg->ogg_fFile==0) {
          ThrowF_t(TRANS("cannot open archive '%s'"), pEntry->GetArchive().ConstData());
        }
        // remember offset and size
        sdc_pogg->ogg_slOffset = pEntry->GetDataOffset();
        sdc_pogg->ogg_slSize = pEntry->GetUncompressedSize();
        fseek(sdc_pogg->ogg_fFile, pEntry->GetDataOffset(), SEEK_SET);

      // if not in zip
      } else {
        // open ogg file
        sdc_pogg->ogg_fFile = FileSystem::Open(fnmExpanded, "rb");
        // if error
        if (sdc_pogg->ogg_fFile==0) {
          ThrowF_t(TRANS("cannot open encoded audio file"));
        }
        // remember offset and size
        sdc_pogg->ogg_slOffset = 0;

        fseek(sdc_pogg->ogg_fFile, 0, SEEK_END);
        sdc_pogg->ogg_slSize = ftell(sdc_pogg->ogg_fFile);
        fseek(sdc_pogg->ogg_fFile, 0, SEEK_SET);
      }

      // initialize decoder
      sdc_pogg->ogg_vfVorbisFile = new OggVorbis_File;
      int iRes = pov_open_callbacks(sdc_pogg, sdc_pogg->ogg_vfVorbisFile, NULL, 0, ovcCallbacks);

      // if error
      if (iRes!=0) {
        ThrowF_t(TRANS("cannot open ogg decoder"));
      }

      // get info on the file
      vorbis_info *pvi = pov_info(sdc_pogg->ogg_vfVorbisFile, -1);

      // remember it's format
      WAVEFORMATEX form;
      form.wFormatTag=WAVE_FORMAT_PCM;
      form.nChannels=pvi->channels;
      form.nSamplesPerSec=pvi->rate;
      form.wBitsPerSample=16;
      form.nBlockAlign=form.nChannels*form.wBitsPerSample/8;
      form.nAvgBytesPerSec=form.nSamplesPerSec*form.nBlockAlign;
      form.cbSize=0;

      // check for stereo
      if (pvi->channels!=2) {
        ThrowF_t(TRANS("not stereo"));
      }
    
      sdc_pogg->ogg_wfeFormat = form;

    } catch (char*strError) {
      CPrintF(TRANS("Cannot open encoded audio '%s' for streaming: %s\n"), fnm.ConstData(), strError);
      if (sdc_pogg->ogg_vfVorbisFile!=NULL) {
        delete sdc_pogg->ogg_vfVorbisFile;
        sdc_pogg->ogg_vfVorbisFile = NULL;
      }
      if (sdc_pogg->ogg_fFile!=NULL) {
        fclose(sdc_pogg->ogg_fFile);
        sdc_pogg->ogg_fFile = NULL;
      }
      if (pZipHandle != NULL) {
        IZip::Close(pZipHandle);
      }
      Clear();
      return;
    }
    if (pZipHandle != NULL) {
      IZip::Close(pZipHandle);
    }

  // if mp3
  } else if (fnmExpanded.FileExt()==".mp3") {

    if (!_bAMP11Enabled) {
      return;
    }

    sdc_pmpeg = new CDecodeData_MPEG;
    sdc_pmpeg->mpeg_hMainFile = 0;
    sdc_pmpeg->mpeg_hFile = 0;
    sdc_pmpeg->mpeg_hDecoder = 0;
    IZip::Handle_t pZipHandle = NULL;

    try {
      // if in zip
      if (expath.bArchive) {
        // open it
        pZipHandle = IZip::Open_t(fnmExpanded);
        const IZip::CEntry *pEntry = IZip::GetEntry(pZipHandle);

        // if compressed
        if (!pEntry->IsStored()) {
          ThrowF_t(TRANS("encoded audio in archives must not be compressed!\n"));
        }
        // open the zip file
        sdc_pmpeg->mpeg_hMainFile = palOpenInputFile(pEntry->GetArchive().ConstData());
        // if error
        if (sdc_pmpeg->mpeg_hMainFile==0) {
          ThrowF_t(TRANS("cannot open archive '%s'"), pEntry->GetArchive().ConstData());
        }
        // open the subfile
        sdc_pmpeg->mpeg_hFile = palOpenSubFile(sdc_pmpeg->mpeg_hMainFile, pEntry->GetDataOffset(), pEntry->GetUncompressedSize());
        // if error
        if (sdc_pmpeg->mpeg_hFile==0) {
          ThrowF_t(TRANS("cannot open encoded audio file"));
        }

      // if not in zip
      } else {
        // open mpx file
        sdc_pmpeg->mpeg_hFile = palOpenInputFile(fnmExpanded.ConstData());
        // if error
        if (sdc_pmpeg->mpeg_hFile==0) {
          ThrowF_t(TRANS("cannot open mpx file"));
        }
      }

      // get info on the file
      int layer, ver, freq, stereo, rate;
      if (!palGetMPXHeader(sdc_pmpeg->mpeg_hFile, &layer, &ver, &freq, &stereo, &rate)) {
        ThrowF_t(TRANS("not a valid mpeg audio file."));
      }

      // remember it's format
      WAVEFORMATEX form;
      form.wFormatTag=WAVE_FORMAT_PCM;
      form.nChannels=stereo?2:1;
      form.nSamplesPerSec=freq;
      form.wBitsPerSample=16;
      form.nBlockAlign=form.nChannels*form.wBitsPerSample/8;
      form.nAvgBytesPerSec=form.nSamplesPerSec*form.nBlockAlign;
      form.cbSize=0;

      // check for stereo
      if (!stereo) {
        ThrowF_t(TRANS("not stereo"));
      }
    
      sdc_pmpeg->mpeg_wfeFormat = form;

      // initialize decoder
      sdc_pmpeg->mpeg_hDecoder = palOpenDecoder(sdc_pmpeg->mpeg_hFile);

      // if error
      if (sdc_pmpeg->mpeg_hDecoder==0) {
        ThrowF_t(TRANS("cannot open mpx decoder"));
      }
    } catch (char*strError) {
      CPrintF(TRANS("Cannot open mpx '%s' for streaming: %s\n"), fnm.ConstData(), strError);
      if (pZipHandle != NULL) {
        IZip::Close(pZipHandle);
      }
      Clear();
      return;
    }

    if (pZipHandle != NULL) {
      IZip::Close(pZipHandle);
    }
    sdc_pmpeg->mpeg_fSecondsLen = palDecGetLen(sdc_pmpeg->mpeg_hDecoder);
  }
}

CSoundDecoder::~CSoundDecoder(void)
{
  Clear();
}

void CSoundDecoder::Clear(void)
{
  if (sdc_pmpeg!=NULL) {
    if (sdc_pmpeg->mpeg_hDecoder!=0)  palClose(sdc_pmpeg->mpeg_hDecoder);
    if (sdc_pmpeg->mpeg_hFile!=0)     palClose(sdc_pmpeg->mpeg_hFile);
    if (sdc_pmpeg->mpeg_hMainFile!=0) palClose(sdc_pmpeg->mpeg_hMainFile);

    sdc_pmpeg->mpeg_hMainFile = 0;
    sdc_pmpeg->mpeg_hFile = 0;
    sdc_pmpeg->mpeg_hDecoder = 0;
    delete sdc_pmpeg;
    sdc_pmpeg = NULL;

  } else if (sdc_pogg!=NULL) {

    if (sdc_pogg->ogg_vfVorbisFile!=NULL) {
      pov_clear(sdc_pogg->ogg_vfVorbisFile);
      delete sdc_pogg->ogg_vfVorbisFile;
      sdc_pogg->ogg_vfVorbisFile = NULL;
    }
    if (sdc_pogg->ogg_fFile!=NULL) {
      fclose(sdc_pogg->ogg_fFile);
      sdc_pogg->ogg_fFile = NULL;
    }
    delete sdc_pogg;
    sdc_pogg = NULL;
  }
}

// reset decoder to start of sample
void CSoundDecoder::Reset(void)
{
  if (sdc_pmpeg!=NULL) {
    palDecSeekAbs(sdc_pmpeg->mpeg_hDecoder, 0.0f);
  } else if (sdc_pogg!=NULL) {
    // so instead, we reinit
    pov_clear(sdc_pogg->ogg_vfVorbisFile);
    fseek(sdc_pogg->ogg_fFile, sdc_pogg->ogg_slOffset, SEEK_SET);
    pov_open_callbacks(sdc_pogg, sdc_pogg->ogg_vfVorbisFile, NULL, 0, ovcCallbacks);
  }
}

BOOL CSoundDecoder::IsOpen(void) 
{
  if (sdc_pmpeg!=NULL && sdc_pmpeg->mpeg_hDecoder!=0) {
    return TRUE;
  } else if (sdc_pogg!=NULL && sdc_pogg->ogg_vfVorbisFile!=0) {
    return TRUE;
  } else {
    return FALSE;
  }
}

void CSoundDecoder::GetFormat(WAVEFORMATEX &wfe)
{
  if (sdc_pmpeg!=NULL) {
    wfe = sdc_pmpeg->mpeg_wfeFormat;

  } else if (sdc_pogg!=NULL) {
    wfe = sdc_pogg->ogg_wfeFormat;

  } else {
    NOTHING;
  }
}

// decode a block of bytes
INDEX CSoundDecoder::Decode(void *pvDestBuffer, INDEX ctBytesToDecode)
{
  // if ogg
  if (sdc_pogg!=NULL && sdc_pogg->ogg_vfVorbisFile!=0) {
    // decode ogg
    static int iCurrrentSection = -1; // we don't care about this
    char *pch = (char *)pvDestBuffer;
    INDEX ctDecoded = 0;
    while (ctDecoded<ctBytesToDecode) {
      long iRes = pov_read(sdc_pogg->ogg_vfVorbisFile, pch, ctBytesToDecode-ctDecoded, 
        0, 2, 1, &iCurrrentSection);
      if (iRes<=0) {
        return ctDecoded;
      }
      ctDecoded+=iRes;
      pch+=iRes;
    }
    return ctDecoded;

  // if mpeg
  } else if (sdc_pmpeg!=NULL && sdc_pmpeg->mpeg_hDecoder!=0) {
    // decode mpeg
    return palRead(sdc_pmpeg->mpeg_hDecoder, pvDestBuffer, ctBytesToDecode);

  // if no decoder
  } else {
    // play all zeroes
    memset(pvDestBuffer, 0, ctBytesToDecode);
    return ctBytesToDecode;
  }
}
