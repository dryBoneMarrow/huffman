#include "bitHandler.h"
#include <stdint.h>
#include <stdio.h>

// write single bits to file
extern void writeBit(int bit, bitBuffer_t* bitBuffer)
{
    // write bit
    if (bit) {
        bitBuffer->buffer[bitBuffer->bufferPosition] |= 1u << (7 - bitBuffer->bitPosition);
    } else {
        bitBuffer->buffer[bitBuffer->bufferPosition] &= ~(1u << (7 - bitBuffer->bitPosition));
    }

    // update bitBuffer
    if (bitBuffer->bitPosition == 7) {
        ++bitBuffer->bufferPosition;
        if (bitBuffer->bufferPosition == bitBuffer->bufferSize) {
            fwrite(bitBuffer->buffer, sizeof(char), bitBuffer->bufferSize, bitBuffer->file);
            bitBuffer->bufferPosition = 0;
        }
        bitBuffer->bitPosition = 0;
    } else {
        ++bitBuffer->bitPosition;
    }
}

// write n bits to buffer
// Has better performance than calling writeBit repeatedly
extern void writeBits(uint64_t bits, int nbits, bitBuffer_t* bitBuffer)
{
    //// Fill partially filled byte

    // Clear part of the byte that is going to be written
    bitBuffer->buffer[bitBuffer->bufferPosition] &= ~(255 >> bitBuffer->bitPosition);
    // Write the bits
    bitBuffer->buffer[bitBuffer->bufferPosition] |= bits >> (56 + bitBuffer->bitPosition);
    // Return if all bits have been written
    if (nbits <= 8 - bitBuffer->bitPosition) {
        bitBuffer->bitPosition += nbits;
        return;
    }
    // Remove written bits
    bits <<= 8 - bitBuffer->bitPosition;
    ++bitBuffer->bufferPosition;
    // flush buffer if full
    if (bitBuffer->bufferPosition == bitBuffer->bufferSize) {
        fwrite(bitBuffer->buffer, sizeof(char), bitBuffer->bufferSize, bitBuffer->file);
        bitBuffer->bufferPosition = 0;
    }
    nbits -= 8 - bitBuffer->bitPosition;
    bitBuffer->bitPosition = nbits % 8;

    //// Write as many bytes to buffer as needed
    while (nbits >= 8) {
        bitBuffer->buffer[bitBuffer->bufferPosition] = bits >> 56;
        bits <<= 8;
        ++bitBuffer->bufferPosition;
        nbits -= 8;
        // flush buffer if full
        if (bitBuffer->bufferPosition == bitBuffer->bufferSize) {
            fwrite(bitBuffer->buffer, sizeof(char), bitBuffer->bufferSize, bitBuffer->file);
            bitBuffer->bufferPosition = 0;
        }
    }

    //// Write left over bits
    bitBuffer->buffer[bitBuffer->bufferPosition] &= 0u;
    bitBuffer->buffer[bitBuffer->bufferPosition] |= bits >> 56;
}

// write byte to file
extern void writeByte(unsigned char byte, bitBuffer_t* bitBuffer)
{

    // Complete partially filled byte
    bitBuffer->buffer[bitBuffer->bufferPosition] &= ~(0xFFu >> bitBuffer->bitPosition);
    bitBuffer->buffer[bitBuffer->bufferPosition] |= byte >> bitBuffer->bitPosition;
    ++bitBuffer->bufferPosition;

    // flush buffer if full
    if (bitBuffer->bufferPosition == bitBuffer->bufferSize) {
        fwrite(bitBuffer->buffer, sizeof(char), bitBuffer->bufferSize, bitBuffer->file);
        bitBuffer->bufferPosition = 0;
    }

    // write left over bits
    bitBuffer->buffer[bitBuffer->bufferPosition] = byte << (8 - bitBuffer->bitPosition);
}

// flush the remaining bits in buffer to file
extern void flush(bitBuffer_t* bitBuffer)
{
    fwrite(bitBuffer->buffer, sizeof(char),
        (bitBuffer->bitPosition == 0) ? bitBuffer->bufferPosition : bitBuffer->bufferPosition + 1,
        bitBuffer->file);
    bitBuffer->bufferPosition = 0;
    bitBuffer->bitPosition = 0;
}
