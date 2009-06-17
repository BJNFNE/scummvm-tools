/* compress_gob - .stk/.itk archive creation tool, based on a conf file.
 * Copyright (C) 2007 The ScummVM project
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.

 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 * $URL$
 * $Id$
 *
 */

#include "util.h"
#define confSTK21 "STK21"
#define confSTK10 "STK10"

struct Chunk {
	char name[64];
	uint32 size, offset;
	bool packed;

	Chunk *next;

	Chunk() : next(0) { }
	~Chunk() { delete next; }
};

Chunk *readChunkConf(FILE *gobconf, uint16 &chunkCount);
void writeEmptyHeader(FILE *stk, uint16 chunkCount);
void writeBody(FILE *stk, uint16 chunkcount, Chunk *chunks);
uint32 writeBodyFile(FILE *stk, FILE *src);
uint32 writeBodyPackFile(FILE *stk, FILE *src);
void rewriteHeader(FILE *stk, uint16 chunkCount, Chunk *chunks);
bool checkDico(byte *unpacked, uint32 unpackedIndex, int32 counter, byte *dico, uint16 currIndex, uint16 &pos, uint8 &length);

byte *packData(byte *src, uint32 &size);

int main(int argc, char **argv) {
	char *outFilename;
	char *tmpStr;
	Chunk *chunks;
	FILE *stk;
	FILE *gobConf;
	uint16 chunkCount;

	if ((argc < 2) || !strcmp(argv[1], "-h") || !strcmp(argv[1], "--help")) {
		printf("Usage: %s <Conf file>\n\n", argv[0]);
		printf("The archive will be created into the current directory.\n");
		return -1;
	}

	if (!(gobConf = fopen(argv[1], "r")))
		error("Couldn't open conf file \"%s\"", argv[1]);

	outFilename = new char[strlen(argv[1]) + 5];
	getFilename(argv[1], outFilename);

	tmpStr = strstr(outFilename, ".");
	if (tmpStr != 0)
		strcpy(tmpStr, ".stk");
	else
		strcat(outFilename, ".stk");

	if (!(stk = fopen(outFilename, "wb")))
		error("Couldn't create file \"%s\"", outFilename);

	chunks = readChunkConf(gobConf, chunkCount);
	fclose(gobConf);

	writeEmptyHeader (stk, chunkCount);
	writeBody(stk, chunkCount, chunks);
	rewriteHeader(stk, chunkCount, chunks);

	fflush(stk);
	delete chunks;
	fclose(stk);
	return 0;
}

void extractError(FILE *f1, FILE *f2, Chunk *chunks, const char *msg) {
	if (f1)
		fclose(f1);
	if (f2)
		fclose(f2);
	delete chunks;

	error(msg);
}

Chunk *readChunkConf(FILE *gobConf, uint16 &chunkCount) {
	Chunk *chunks = new Chunk;
	Chunk *curChunk = chunks;
	char buffer [1024];

	chunkCount = 1;

// first read: signature
	fscanf(gobConf, "%s", buffer);
	if (!strcmp(buffer, confSTK21))
		error("STK21 not yet handled");
	else if (strcmp(buffer, confSTK10))
		error("Unknown format signature");

// All the other reads concern file + compression flag
	fscanf(gobConf, "%s", buffer);
	while (!feof(gobConf)) {
		strcpy(curChunk->name, buffer);
		fscanf(gobConf, "%s", buffer);
		if (strcmp(buffer, "1") == 0)
			curChunk->packed = true;
		else
			curChunk->packed = false;

		fscanf(gobConf, "%s", buffer);
		if (!feof(gobConf)) {
			curChunk->next = new Chunk;
			curChunk = curChunk->next;
			chunkCount++;
		}
	}
	return chunks;
}

void writeEmptyHeader(FILE *stk, uint16 chunkCount) {
	int count;

// Write empty header dummy header, which will be overwritten
// at the end of the program execution.
	for (count = 0; count < 2 + (chunkCount * 22); count++)
		fputc(0, stk);

	return;
}

void writeBody(FILE *stk, uint16 chunkCount, Chunk *chunks) {
	Chunk *curChunk = chunks;
	FILE *src;
	uint32 realSize;
	int count;
	char buffer[4096];
	uint32 tmpSize;

	while(curChunk) {
		if (!(src = fopen(curChunk->name, "rb")))
			error("Couldn't open conf file \"%s\"", curChunk->name);

		realSize = fileSize(src);

		if (curChunk->packed) {
			printf("Compressing %12s\t", curChunk->name);
			curChunk->size = writeBodyPackFile(stk, src);
			printf("%d -> %d bytes\n", realSize, curChunk->size);
		} else {
			tmpSize = 0;
			printf("Storing %12s\t", curChunk->name);
			do {
				count = fread(buffer, 1, 4096, src);
				fwrite(buffer, 1, count, stk);
				tmpSize += count;
			} while (count == 4096);
			curChunk->size = tmpSize;
			printf("%d bytes\n", tmpSize);
		}

		fclose(src);
		curChunk = curChunk->next;
	}
	return;
}

uint32 writeBodyFile(FILE *stk, FILE *src) {
	int count;
	char buffer[4096];
	uint32 tmpSize;

	tmpSize = 0;
	do {
		count = fread(buffer, 1, 4096, src);
		fwrite(buffer, 1, count, stk);
		tmpSize += count;
	} while (count == 4096);
	return tmpSize;
}


void rewriteHeader(FILE *stk, uint16 chunkCount, Chunk *chunks) {
	uint16 i;
	char buffer[1024];
	Chunk *curChunk = chunks;
	uint32 filPos;

	rewind(stk);

//	The structure of the header is the following :
//+ 2 bytes : numbers of files archived in the .stk/.itk
//	Then, for each files :
//+ 13 bytes : the filename, terminated by '\0'. In original, there's
//  garbage after if the filename has not the maximum length
//+ 4  bytes : size of the chunk
//+ 4  bytes : start position of the chunk in the file
//+ 1  byte  : If 0 : not compressed, if 1 : compressed
	filPos = 2 + (chunkCount * 22);

	buffer[0] = chunkCount & 0xFF;
	buffer[1] = chunkCount >> 8;
	fwrite(buffer, 1, 2, stk);
	// TODO : Implement STK21
	while (curChunk) {
		for (i = 0; i < 13; i++)
			if (i < strlen(curChunk->name))
				buffer[i] = curChunk->name[i];
			else
				buffer[i] = '\0';
		fwrite(buffer, 1, 13, stk);

		buffer[0] = curChunk->size;
		buffer[1] = curChunk->size >> 8;
		buffer[2] = curChunk->size >> 16;
		buffer[3] = curChunk->size >> 24;
		buffer[4] = filPos;
		buffer[5] = filPos >> 8;
		buffer[6] = filPos >> 16;
		buffer[7] = filPos >> 24;

		buffer[8] = curChunk->packed ? 0x1 : 0x0;

		fwrite(buffer, 1, 9, stk);
		filPos += curChunk->size;

		curChunk = curChunk->next;
	}
	return;
}

uint32 writeBodyPackFile(FILE *stk, FILE *src) {
	byte dico[4114];
	byte writeBuffer[17];
	uint32 counter;
	uint16 dicoIndex;
	uint32 unpackedIndex, size;
	uint8 cmd;
	uint8 buffIndex, cpt, i;
	uint16 resultcheckpos;
	byte resultchecklength;

	size = fileSize(src);

	byte *unpacked = new byte [size + 1];
	for (int i = 0; i < 4096 - 18; i++)
		dico[i] = 0x20;

	fread(unpacked, 1, size, src);

	writeBuffer[0] = size & 0xFF;
	writeBuffer[1] = size >> 8;
	writeBuffer[2] = size >> 16;
	writeBuffer[3] = size >> 24;
	fwrite(writeBuffer, 1, 4, stk);

// TODO : check size, if too small, handle correctly

	dicoIndex = 4078;
	dico[dicoIndex] = unpacked[0];
	dico[dicoIndex+1] = unpacked[1];
	dico[dicoIndex+2] = unpacked[2];
	dicoIndex += 3;

//writeBuffer[0] is reserved for the command byte
	writeBuffer[1] = unpacked[0];
	writeBuffer[2] = unpacked[1];
	writeBuffer[3] = unpacked[2];

	counter = size - 3;
	unpackedIndex = 3;
	cpt = 3;
	buffIndex = 4;
	cmd = (1 << 3) - 1;

	size=4;
	resultcheckpos = 0;
	resultchecklength = 0;

	while (counter>0) {
		if (!checkDico(unpacked, unpackedIndex, counter, dico, dicoIndex, resultcheckpos, resultchecklength)) {
			dico[dicoIndex] = unpacked[unpackedIndex];
			writeBuffer[buffIndex] = unpacked[unpackedIndex];
			cmd |= (1 << cpt);
			unpackedIndex++;
			dicoIndex = (dicoIndex + 1) % 4096;
			buffIndex++;
			counter--;
		} else {
// Copy the string in the dictionary
			for (i=0; i < resultchecklength; i++)
				dico[((dicoIndex + i) % 4096)] = dico[((resultcheckpos + i) % 4096)];

// Write the copy string command
			writeBuffer[buffIndex] = resultcheckpos & 0xFF;
			writeBuffer[buffIndex + 1] = ((resultcheckpos & 0x0F00) >> 4) + (resultchecklength - 3);

			unpackedIndex += resultchecklength;
			dicoIndex = (dicoIndex + resultchecklength) % 4096;
			resultcheckpos = (resultcheckpos + resultchecklength) % 4096;

			buffIndex += 2;
			counter -= resultchecklength;
		}

		if ((cpt == 7) | (counter == 0)) {
			writeBuffer[0] = cmd;
			fwrite(writeBuffer, 1, buffIndex, stk);
			size += buffIndex;
			buffIndex = 1;
			cmd = 0;
			cpt = 0;
		} else
			cpt++;
	}

	delete[] unpacked;
	return size;
}

bool checkDico(byte *unpacked, uint32 unpackedIndex, int32 counter, byte *dico, uint16 currIndex, uint16 &pos, uint8 &length) {
	uint16 tmpPos, bestPos;
	uint8 tmpLength, bestLength, i;

	bestPos = 0;
	bestLength = 2;

	if (counter < 3)
		return false;

	for (tmpPos = 0; tmpPos < 0x1000; tmpPos++) {
		tmpLength = 0;
		for (i = 0; ((i < 18) & (i < counter)); i++)
			if ((unpacked[unpackedIndex + i] == dico[(tmpPos + i) % 4096]) & (((tmpPos + i) % 4096 != currIndex) | (i == 0)))
				tmpLength++;
			else
				// avoid dictionary collision
				break;
		if (tmpLength > bestLength)
		{
			bestPos = tmpPos;
			if ((bestLength = tmpLength) == 18)
				break;
		}
	}

	pos = bestPos;
	length = bestLength;

	if (bestLength > 2)
		return true;
	else {
		length = 0;
		return false;
	}
}
