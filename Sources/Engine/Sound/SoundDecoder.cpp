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

  static ma_decoder_config dcConfig; // Common config for loading any audio
  ma_decoder *pDecoder; // Actual decoder for specific audio

  CTString fnmStreamed; // Absolute path to the file on disk to stream audio from
  size_t iDataOffset; // Where the encoded audio starts inside loaded data (e.g. uncompressed file inside an archive)
  size_t iDataSize; // Size of the encoded audio (setting it to -1 figures out the size upon decoder initialization)

  SEAudioFormat afFormat; // Format of the loaded audio
  FLOAT fLength; // Audio length in seconds

  DecodeData() : pDecoder(NULL), fnmStreamed(""), iDataOffset(0), iDataSize(0), fLength(0) {};
};

ma_decoder_config DecodeData::dcConfig;

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

// [Cecil] Initialize the decoder for a specific file
void CSoundDecoder::Initialize(const CTString &fnm) {
  // Ignore sounds on a dedicated server
  if (_SE1Setup.IsAppServer()) return;

  // [Cecil] FIXME: The sound file has already been located and opened and now the decoder tries
  // opening it again for its own handle, which does an additional expensive lookup of the ZIP entry
  ExpandPath expath;

  // No file to read
  if (!expath.ForReading(fnm, 0)) {
    CPrintF(TRANS("Cannot open encoded audio '%s' for streaming: %s\n"), fnm.ConstData(), TRANS("file not found"));
    return;
  }

  const CTString &fnmExpanded = expath.fnmExpanded;

  // Set up decode data for this decoder
  sdc_pData = new DecodeData;
  sdc_pData->vfs.onOpen  = DecoderOpen;
  sdc_pData->vfs.onOpenW = NULL;
  sdc_pData->vfs.onClose = DecoderClose;
  sdc_pData->vfs.onRead  = DecoderRead;
  sdc_pData->vfs.onWrite = NULL;
  sdc_pData->vfs.onSeek  = DecoderSeek;
  sdc_pData->vfs.onTell  = DecoderTell;
  sdc_pData->vfs.onInfo  = NULL;

  try {
    if (expath.IsFromArchive()) {
      if (!expath.pZipEntry->IsStored()) {
        ThrowF_t(TRANS("Encoded audio in archives must be uncompressed!"));
      }

      // Remember path to the archive to stream an uncompressed entry from
      sdc_pData->fnmStreamed = expath.pZipEntry->GetArchive();
      sdc_pData->iDataOffset = expath.pZipEntry->GetDataOffset();
      sdc_pData->iDataSize = expath.pZipEntry->GetUncompressedSize();

    } else {
      // Remember path to the file to stream from the beginning
      sdc_pData->fnmStreamed = fnmExpanded;
      sdc_pData->iDataOffset = 0;
      sdc_pData->iDataSize = -1; // Calculated upon decoder opening
    }

    sdc_pData->pDecoder = new ma_decoder;

    // [Cecil] NOTE: Passing the original filename because miniaudio may utilize its extension to determine the format and sometimes
    // the game needs to stream audio files from archives, which will have to use the archive itself in place of the audio file.
    // The original filename will pick the appropriate decoder, whereas the provided VFS will open the appropriate file from the
    // DecodeData::fnmStreamed field, which may either be the audio file itself or an archive that contains it at specific offset.
    ma_result result = ma_decoder_init_vfs(sdc_pData, fnmExpanded.NoDir().ConstData(), &DecodeData::dcConfig, sdc_pData->pDecoder);

    if (result != MA_SUCCESS) {
      ThrowF_t(TRANS("Cannot open audio decoder: %s"), ma_result_description(result));
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
    CPrintF(TRANS("Cannot open encoded audio '%s' for streaming: %s\n"), fnm.ConstData(), strError);
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
BOOL CSoundDecoder::SeekToFrame(ULONG iFrame) {
  if (!IsOpen()) return FALSE;

  return (ma_data_source_seek_to_pcm_frame(sdc_pData->pDecoder, iFrame) == MA_SUCCESS);
};

// reset decoder to start of sample
void CSoundDecoder::Reset(void) {
  if (!IsOpen()) return;

  // [Cecil] Seek to the first sample
  ma_result result = ma_data_source_seek_to_pcm_frame(sdc_pData->pDecoder, 0);

  if (result != MA_SUCCESS) {
    ASSERTALWAYS("Failed to seek decoder to the beginning");
    CPrintF(TRANS("Failed to seek decoder to the beginning: %s\n"), ma_result_description(result));
  }
};
