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

#include "StdH.h"

#include "SoundDecoder.h"

// [Cecil] Include miniaudio and explicitly link libvorbis
#include <ThirdParty/miniaudio/miniaudio.c>
#include <ThirdParty/miniaudio/extras/decoders/libvorbis/miniaudio_libvorbis.c>

#if SE1_WIN
  #pragma comment(lib, "libogg.lib")
  #pragma comment(lib, "libvorbis.lib")
  #pragma comment(lib, "libvorbisfile.lib")
#endif

// [Cecil] Decode data for miniaudio
struct DecodeData {
  // This must go first in order to be able to cast DecodeData into
  // ma_vfs that always expects ma_vfs_callbacks in the beginning
  ma_vfs_callbacks vfs;

  enum DecodeMode {
    E_NONE,
    E_MEMORY, // Decoded after being fully loaded into memory (i.e. not streamed)
    E_FILE,   // Decoded from a physical file (streamed from OGG, MP3 etc.)
    E_VFS,    // Decoded by the custom VFS (e.g. streamed from OGG or from an uncompressed archive entry)
  } eMode;

  static ma_decoder_config dcConfig; // Common config for loading any audio
  ma_decoder *pDecoder; // Actual decoder for specific audio

  // Common information about encoded audio
  size_t iDataSize; // Size of encoded audio in bytes (setting it to -1 figures out the size upon decoder initialization)
  SEAudioFormat afFormat; // Format of the loaded audio
  FLOAT fLength; // Audio length in seconds

  // eMode == E_MEMORY
  UBYTE *pubData; // Buffer that holds encoded audio in memory with the length of iDataSize

  // eMode == E_VFS
  CTString fnmStreamed; // Absolute path to the file on disk to stream audio from
  size_t iDataOffset; // Where the encoded audio starts inside loaded data (e.g. uncompressed file inside an archive)

  DecodeData() : eMode(E_NONE), pDecoder(NULL), pubData(NULL), fnmStreamed(""), iDataOffset(0), iDataSize(0), fLength(0) {};
};

ma_decoder_config DecodeData::dcConfig;

// Decoder functions used by DecodeData::vfs
static ma_result DecoderOpen(ma_vfs *pVFS, const char *pFilePath, ma_uint32 iOpenMode, ma_vfs_file *pFile) {
  ASSERT(iOpenMode & MA_OPEN_MODE_READ);

  DecodeData *pData = (DecodeData *)pVFS;
  FILE *fHandle = FileSystem::Open(pData->fnmStreamed, "rb");

  if (fHandle == NULL) {
    ThrowF_t(TRANS("Cannot open encoded audio file: %s"), strerror(errno));
    return MA_INVALID_FILE;
  }

  // Figure out the size (only if it's loaded from a singular file)
  if (pData->iDataSize == -1) {
    ASSERT(pData->iDataOffset == 0);
    fseek(fHandle, 0, SEEK_END);
    pData->iDataSize = ftell(fHandle);
    fseek(fHandle, 0, SEEK_SET);
  }

  // Seek to the requested offset
  if (pData->iDataOffset != 0) {
    fseek(fHandle, (long)pData->iDataOffset, SEEK_SET);
  }

  *pFile = fHandle;
  return MA_SUCCESS;
};

static ma_result DecoderClose(ma_vfs *pVFS, ma_vfs_file file) {
  FILE *fHandle = (FILE *)file;
  if (fHandle != NULL) fclose(fHandle);
  return MA_SUCCESS;
};

static ma_result DecoderRead(ma_vfs *pVFS, ma_vfs_file file, void *pDest, size_t iBytesToRead, size_t *pBytesRead) {
  DecodeData *pData = (DecodeData *)pVFS;
  FILE *fHandle = (FILE *)file;

  // Don't read more data than how much is left
  const size_t iCurrentPos = ftell(fHandle) - pData->iDataOffset;
  const size_t iDataLeft = pData->iDataSize - iCurrentPos;
  iBytesToRead = ClampUp(iBytesToRead, iDataLeft);

  if (iBytesToRead != 0) {
    *pBytesRead = fread(pDest, 1, iBytesToRead, fHandle);
  } else {
    *pBytesRead = 0;
  }

  return MA_SUCCESS;
};

static ma_result DecoderSeek(ma_vfs *pVFS, ma_vfs_file file, ma_int64 iOffset, ma_seek_origin eOrigin) {
  DecodeData *pData = (DecodeData *)pVFS;
  FILE *fHandle = (FILE *)file;

  switch (eOrigin) {
    case ma_seek_origin_current: fseek(fHandle, (long)iOffset, SEEK_CUR); break;
    case ma_seek_origin_end:     fseek(fHandle, (long)(pData->iDataOffset + pData->iDataSize) + (long)iOffset, SEEK_SET); break;
    case ma_seek_origin_start:   fseek(fHandle, (long)pData->iDataOffset + (long)iOffset, SEEK_SET); break;
    default: return MA_INVALID_OPERATION;
  }

  return MA_SUCCESS;
};

static ma_result DecoderTell(ma_vfs *pVFS, ma_vfs_file file, ma_int64 *pCursor) {
  DecodeData *pogg = (DecodeData *)pVFS;
  FILE *fHandle = (FILE *)file;

  *pCursor = ftell(fHandle) - pogg->iDataOffset;
  return MA_SUCCESS;
};

// [Cecil] Initialize the new decoder
void CSoundDecoder::InitPlugins(void)
{
  // Specify additional backends for decoding extra formats
  static ma_decoding_backend_vtable *aCustomBackends[] = {
    ma_decoding_backend_libvorbis, // OGG
  };

  // Force-load everything in [signed 16-bit, stereo, max sample rate] format that the engine mixer expects
  DecodeData::dcConfig = ma_decoder_config_init(ma_format_s16, 2, CSoundLibrary::sl_ulMaxSampleRate);
  DecodeData::dcConfig.pCustomBackendUserData = NULL;
  DecodeData::dcConfig.ppCustomBackendVTables = aCustomBackends;
  DecodeData::dcConfig.customBackendCount = ARRAYCOUNT(aCustomBackends);
};

// [Cecil] Clean up the new decoder
void CSoundDecoder::EndPlugins(void)
{
};

// [Cecil] Initialize the decoder for a specific file or for an optional stream to immediately read data from
void CSoundDecoder::Initialize(const CTString &fnmFile, CTStream *istrReadFrom) {
  // Ignore sounds on a dedicated server
  if (_SE1Setup.IsAppServer()) return;

  // [Cecil] Locate a new file with the audio if there's no open stream to read existing data from
  ExpandPath expath;

  if (istrReadFrom == NULL) {
    // No file to read
    if (!expath.ForReading(fnmFile, 0)) {
      CPrintF(TRANS("Cannot open encoded audio '%s' for streaming: %s\n"), fnmFile.ConstData(), TRANS("file not found"));
      return;
    }
  }

  // Set up decode data for this decoder
  sdc_pData = new DecodeData;

  // Set up VFS for streaming from disk
  sdc_pData->vfs.onOpen  = DecoderOpen;
  sdc_pData->vfs.onOpenW = NULL;
  sdc_pData->vfs.onClose = DecoderClose;
  sdc_pData->vfs.onRead  = DecoderRead;
  sdc_pData->vfs.onWrite = NULL;
  sdc_pData->vfs.onSeek  = DecoderSeek;
  sdc_pData->vfs.onTell  = DecoderTell;
  sdc_pData->vfs.onInfo  = NULL;

  try {
    // Load data into memory from a stream
    if (istrReadFrom != NULL) {
      sdc_pData->iDataSize = istrReadFrom->GetStreamSize();
      sdc_pData->pubData = (UBYTE *)AllocMemory(sdc_pData->iDataSize);
      istrReadFrom->Read_t(sdc_pData->pubData, sdc_pData->iDataSize);

      sdc_pData->eMode = DecodeData::E_MEMORY;

    // Load data from an archive
    } else if (expath.IsFromArchive()) {
      const SLONG slDataSize = expath.pZipEntry->GetUncompressedSize();

      // Make sure it's not an empty file
      if (slDataSize <= 0) ThrowF_t(TRANS("Archived file is empty"));

      // If the entry is compressed, uncompress it into memory instead
      if (!expath.pZipEntry->IsStored()) {
        IZip::Handle_t pZipHandle = IZip::Open_t(expath.pZipEntry);

        sdc_pData->iDataSize = slDataSize;
        sdc_pData->pubData = (UBYTE *)AllocMemory(slDataSize);
        IZip::ReadBlock_t(pZipHandle, sdc_pData->pubData, 0, slDataSize);
        IZip::Close(pZipHandle);

        sdc_pData->eMode = DecodeData::E_MEMORY;

      // Otherwise, remember path to the archive to stream an uncompressed entry from
      } else {
        sdc_pData->fnmStreamed = expath.pZipEntry->GetArchive();
        sdc_pData->iDataOffset = expath.pZipEntry->GetDataOffset();
        sdc_pData->iDataSize = slDataSize;

        sdc_pData->eMode = DecodeData::E_VFS;
      }

    // Load data from a physical file
    } else {
      // Remember path to the file to stream from the beginning
      sdc_pData->fnmStreamed = expath.fnmExpanded;
      sdc_pData->iDataOffset = 0;
      sdc_pData->iDataSize = -1; // Calculated upon decoder opening

      sdc_pData->eMode = DecodeData::E_FILE; // May also be set to E_VFS but it's not necessary
    }

    // Initialize the decoder for specific mode
    sdc_pData->pDecoder = new ma_decoder;
    ma_result result;

    switch (sdc_pData->eMode) {
      case DecodeData::E_MEMORY: {
        result = ma_decoder_init_memory(sdc_pData->pubData, sdc_pData->iDataSize, &DecodeData::dcConfig, sdc_pData->pDecoder);

        if (result != MA_SUCCESS) {
          ThrowF_t(TRANS("Cannot initialize audio decoder for memory: %s"), ma_result_description(result));
        }
      } break;

      case DecodeData::E_FILE: {
        result = ma_decoder_init_file(sdc_pData->fnmStreamed.ConstData(), &DecodeData::dcConfig, sdc_pData->pDecoder);

        if (result != MA_SUCCESS) {
          ThrowF_t(TRANS("Cannot initialize audio decoder for a file: %s"), ma_result_description(result));
        }
      } break;

      case DecodeData::E_VFS: {
        // [Cecil] NOTE: Passing the original filename because miniaudio may utilize its extension to determine the format and sometimes
        // the game needs to stream audio files from archives, which will have to use the archive itself in place of the audio file.
        // The original filename will pick the appropriate decoder, whereas the provided VFS will open the appropriate file from the
        // DecodeData::fnmStreamed field, which may either be the audio file itself or an archive that contains it at specific offset.
        result = ma_decoder_init_vfs(sdc_pData, expath.fnmExpanded.NoDir().ConstData(), &DecodeData::dcConfig, sdc_pData->pDecoder);

        if (result != MA_SUCCESS) {
          ThrowF_t(TRANS("Cannot initialize audio decoder for VFS: %s"), ma_result_description(result));
        }
      } break;

      default: ThrowF_t(TRANS("Unknown mode %d for the audio decoder"), sdc_pData->eMode);
    }

    // Retrieve format
    ma_format eFormat;
    ma_uint32 iChannels;
    ma_uint32 iSampleRate;
    result = ma_data_source_get_data_format(sdc_pData->pDecoder, &eFormat, &iChannels, &iSampleRate, NULL, 0);

    if (result != MA_SUCCESS) {
      ThrowF_t(TRANS("Failed to retrieve data format"));
    }

    // Must be in a very specific format
    if (eFormat != ma_format_s16 || iChannels != 2 || iSampleRate != CSoundLibrary::sl_ulMaxSampleRate) {
      ThrowF_t(TRANS("Encoded audio must be signed 16-bit, stereo, %u Hz!"), CSoundLibrary::sl_ulMaxSampleRate);
    }

    SEAudioFormat &af = sdc_pData->afFormat;
    af.iChannels = iChannels;
    af.iSampleRate = iSampleRate;
    af.iBytesPerSample = ma_get_bytes_per_sample(eFormat);

    // Retrieve length
    result = ma_data_source_get_length_in_seconds(sdc_pData->pDecoder, &sdc_pData->fLength);

    if (result != MA_SUCCESS) {
      ThrowF_t(TRANS("Failed to determine audio length"));
    }

  } catch (char *strError) {
    CPrintF(TRANS("Cannot open encoded audio '%s' for streaming: %s\n"), fnmFile.ConstData(), strError);
    Clear();
  }
};

// Clear the decoder
void CSoundDecoder::Clear(void) {
  if (sdc_pData == NULL) return;

  if (sdc_pData->pDecoder != NULL) {
    ma_decoder_uninit(sdc_pData->pDecoder);
    delete sdc_pData->pDecoder;
  }

  if (sdc_pData->pubData != NULL) {
    FreeMemory(sdc_pData->pubData);
    sdc_pData->pubData = NULL;
  }

  delete sdc_pData;
  sdc_pData = NULL;
};

// Check if the decoder opened successfully
BOOL CSoundDecoder::IsOpen(void) const {
  return (sdc_pData != NULL && sdc_pData->pDecoder != NULL);
};

// Get audio format from the decoder (only if it's open)
void CSoundDecoder::GetFormat(SEAudioFormat &af) const {
  ASSERT(sdc_pData != NULL);
  if (sdc_pData != NULL) af = sdc_pData->afFormat;
};

// [Cecil] Retrieve length of the encoded audio in seconds
DOUBLE CSoundDecoder::GetSoundLength(void) const {
  if (sdc_pData != NULL) {
    return sdc_pData->fLength;
  }

  return 0;
};

// [Cecil] Retrieve length of the encoded audio in frames (samples * channels)
ULONG CSoundDecoder::GetLengthInFrames(void) const {
  if (!IsOpen()) return 0;

  ma_uint64 ctFrames;
  ma_data_source_get_length_in_pcm_frames(sdc_pData->pDecoder, &ctFrames);
  return ctFrames;
};

// [Cecil] Decode a block of frames and return amount of read frames
ULONG CSoundDecoder::Decode(void *pvDestBuffer, ULONG ctFramesToDecode) {
  // Play all zeroes if no decoder
  if (!IsOpen()) {
    memset(pvDestBuffer, 0, ctFramesToDecode * sdc_pData->afFormat.GetBytesPerFrame());
    return ctFramesToDecode;
  }

  // [Cecil] NOTE: Allow it to return 0 frames (e.g. with MA_AT_END as the result)
  // because it may need to do this in order to determine the end of the audio
  ma_uint64 ctFramesRead;
  ma_data_source_read_pcm_frames(sdc_pData->pDecoder, pvDestBuffer, ctFramesToDecode, &ctFramesRead);
  return ctFramesRead;
};

// [Cecil] Set next decoding position to a specific frame
void CSoundDecoder::SeekToFrame(ULONG iFrame) {
  if (!IsOpen()) return;

  ma_result result = ma_data_source_seek_to_pcm_frame(sdc_pData->pDecoder, iFrame);

  if (result != MA_SUCCESS) {
    ASSERTALWAYS("Failed to seek decoder");
    CPrintF(TRANS("Failed to seek decoder to frame %u: %s\n"), iFrame, ma_result_description(result));
  }
};
