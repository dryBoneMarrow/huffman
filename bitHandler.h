// bitHandler provides functions to write single bits to a file
#ifndef BITHANDLER_H
#define BITHANDLER_H

#include <stdint.h>
#include <stdio.h>

// bitBuffer_t contains all necessary infos to read / write bits from / to a file
typedef struct bitBuffer {
    unsigned char* buffer;
    size_t bufferSize;
    unsigned int bufferPosition;
    int bitPosition;
    FILE* file;
} bitBuffer_t;

extern void writeBit(int bit, bitBuffer_t* bitBuffer);
extern void writeBits(uint64_t bits, int nbits, bitBuffer_t* bitBuffer);
extern void writeByte(unsigned char byte, bitBuffer_t* bitBuffer);
extern void flush(bitBuffer_t* bitBuffer);

#endif
