#ifndef SOUND_VOC_H
#define SOUND_VOC_H

#include "../util.h"

namespace Common { class ReadStream; }

namespace Audio {

class AudioStream;


#if !defined(__GNUC__)
#pragma START_PACK_STRUCTS
#endif

struct VocFileHeader {
	uint8 desc[20];
	uint16 datablock_offset;
	uint16 version;
	uint16 id;
} GCC_PACK;

struct VocBlockHeader {
	uint8 blocktype;
	uint8 size[3];
	uint8 sr;
	uint8 pack;
} GCC_PACK;

#if !defined(__GNUC__)
#pragma END_PACK_STRUCTS
#endif

/**
 * Take a sample rate parameter as it occurs in a VOC sound header, and
 * return the corresponding sample frequency.
 *
 * This method has special cases for the standard rates of 11025 and 22050 kHz,
 * which due to limitations of the format, cannot be encoded exactly in a VOC
 * file. As a consequence, many game files have sound data sampled with those
 * rates, but the VOC marks them incorrectly as 11111 or 22222 kHz. This code
 * works around that and "unrounds" the sampling rates.
 */
extern int getSampleRateFromVOCRate(int vocSR);

/**
 * Try to load a VOC from the given seekable stream. Returns a pointer to memory
 * containing the PCM sample data (allocated with malloc). It is the callers
 * responsibility to dellocate that data again later on! Currently this
 * function only supports uncompressed raw PCM data.
 */
extern byte *loadVOCFromStream(Common::ReadStream &stream, int &size, int &rate);

/**
 * Try to load a VOC from the given seekable stream and create an AudioStream
 * from that data. Currently this function only supports uncompressed raw PCM
 * data. Looping is not supported.
 *
 * This function uses loadVOCFromStream() internally.
 */
AudioStream *makeVOCStream(Common::ReadStream &stream);

} // End of namespace Audio

#endif
