#include "huffman.h"
#include "bitHandler.h"
#include <inttypes.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

// Only works when node is initialized with 0
#define isLeaf(a) (!(a)->left)

typedef struct node {
    uint64_t frequency;
    unsigned char symbol;
    struct node* left;
    struct node* right;
} node_t;

// Dictionary is made of codeWords
typedef struct codeWord {
    unsigned char codeLength;
    uint64_t code;
} code_t;

// Sort nodes, used with qsort()
static int compareNodes(const void* a, const void* b)
{
    if ((*(node_t* const*)a)->frequency > (*(node_t* const*)b)->frequency) return -1;
    return 1;
}

// Create dictionary, dictionary[symbol] returns the corresponding code
// https://de.wikipedia.org/w/index.php?title=Huffman-Kodierung&stable=0#Erzeugen_des_Codebuchs
static void createDictionary(node_t* node, code_t* dictionary, uint64_t code, int depth)
{
    if (isLeaf(node)) {
        dictionary[node->symbol].code = code;
        dictionary[node->symbol].codeLength = depth;
        return;
    }
    // Writing code from left to right
    code &= ~(1ull << (63 - depth));
    createDictionary(node->left, dictionary, code, 1 + depth);
    code |= 1ull << (63 - depth);
    createDictionary(node->right, dictionary, code, 1 + depth);
}

// Write tree to file
// https://stackoverflow.com/a/759766/15833045
static void writeTree(node_t* node, bitBuffer_t* bitBuffer)
{
    if (isLeaf(node)) {
        writeBit(1, bitBuffer);
        writeByte(node->symbol, bitBuffer);
        return;
    }
    writeBit(0, bitBuffer);
    writeTree(node->left, bitBuffer);
    writeTree(node->right, bitBuffer);
}

// print representation of tree
static void printTree(node_t* node, int depth)
{
    int i;
    for (i = 0; i < depth; ++i) {
        printf("│  ");
    }
    // Frequency is set if tree was generated but not if tree was read off a file
    if (node->frequency) {
        if (isLeaf(node)) {
            printf("(%c: %" PRIuFAST64 ")\n", node->symbol, node->frequency);
            return;
        }
        printf("(%" PRIu64 ")\n", node->frequency);
    } else {
        if (isLeaf(node)) {
            printf("(%c)\n", node->symbol);
            return;
        }
        printf("*\n");
    }
    printTree(node->left, depth + 1);
    printTree(node->right, depth + 1);
}

// Read tree from file
int readTree(bitBuffer_t* bitBuffer, node_t* node, int* nodeNum)
{
    // We don't have to care for a potential buffer overflow as the maximal theoretical depth of
    // tree is 256, hence the maximum number of nodes 511 and it is assumed that the buffer in
    // bitBuffer is >= 511

    int currNode = *nodeNum;
    ++*nodeNum;

    // bit is 1
    if (bitBuffer->buffer[bitBuffer->bufferPosition] & 1 << (7 - bitBuffer->bitPosition)) {
        // update bitBuffer
        switch (bitBuffer->bitPosition) {
        case 7:
            bitBuffer->bitPosition = 0;
            ++bitBuffer->bufferPosition;
            break;
        default:
            ++bitBuffer->bitPosition;
        }

        // Assign symbol to leaf
        node[currNode].symbol = bitBuffer->buffer[bitBuffer->bufferPosition]
                << bitBuffer->bitPosition
            | bitBuffer->buffer[bitBuffer->bufferPosition + 1] >> (8 - bitBuffer->bitPosition);
        ++bitBuffer->bufferPosition;
        return currNode;
    }

    // bit is 0
    // update bitBuffer
    switch (bitBuffer->bitPosition) {
    case 7:
        bitBuffer->bitPosition = 0;
        bitBuffer->bufferPosition++;
        break;
    default:
        bitBuffer->bitPosition++;
    }

    // Assign left and right to node
    node[currNode].left = &node[readTree(bitBuffer, node, nodeNum)];
    node[currNode].right = &node[readTree(bitBuffer, node, nodeNum)];

    return currNode;
}

// Encode file
extern int encode(FILE* input, FILE* output)
{
    int exitCode = 0;

    //// Create the frequency table
    // Buffering is used for better performance

    int leafCount = 0;
    uint64_t frequencyTable[256] = { 0 };
    unsigned char buffer[BUFSIZ];
    size_t bytesRead;

    while ((bytesRead = fread(buffer, 1, BUFSIZ, input)) > 0) {
        for (size_t i = 0; i < bytesRead; ++i) {
            if (!frequencyTable[buffer[i]]) {
                ++leafCount;
            }
            ++frequencyTable[buffer[i]];
        }
    }

    // handle empty file
    if (!leafCount) {
        fprintf(stderr, "input file is empty");
        exitCode = 1;
        goto cleanup;
    }
    // handle file with one type of symbol (there's no huffman code because the huffman tree has no
    // edges)
    if (leafCount == 1) {
        fputc(1 << 7, output);
        rewind(input);
        unsigned char symbol = fgetc(input);
        uint64_t symbolCount = frequencyTable[symbol];
        fputc(symbol, output);
        fwrite(&symbolCount, sizeof(uint64_t), 1, output);
        goto cleanupBuffer;
    }

    //// Create tree
    // trees[] stores pointers to all subtrees
    // nodes[] holds all the nodes of the trees (2 times the number of symbols minus one)

    int i;
    node_t* nodes = calloc(2 * leafCount - 1, sizeof(node_t));
    if (!nodes) {
        fprintf(stderr, "Couldn't allocate memory for nodes");
        exitCode = 1;
        goto cleanup;
    }
    node_t** trees = malloc(leafCount * sizeof(node_t**));
    if (!trees) {
        fprintf(stderr, "Couldn't allocate memory for trees");
        exitCode = 1;
        goto cleanupNode;
    }

    node_t* nodesp = nodes;
    node_t** treesp = trees;

    // Fill the nodes array with the leafs and reference them in the trees array
    for (i = 0; i < 256; ++i) {
        if (frequencyTable[i]) {
            nodesp->symbol = i;
            nodesp->frequency = frequencyTable[i];
            *treesp = nodesp;
            ++treesp;
            ++nodesp;
        }
    }
    // sort the tree array
    qsort(trees, leafCount, sizeof(node_t**), &compareNodes);
    int treeCount = leafCount;

    // Connect subtrees (leafs) to the huffman tree
    while (treeCount > 1) {
        // create node
        --treeCount;
        nodesp->frequency = trees[treeCount]->frequency + trees[treeCount - 1]->frequency;
        nodesp->left = trees[treeCount];
        nodesp->right = trees[treeCount - 1];

        // sort trees
        for (i = 0; i < treeCount - 1 && nodesp->frequency >= trees[treeCount - (i + 2)]->frequency;
             ++i) {
            trees[treeCount - (i + 1)] = trees[treeCount - (i + 2)];
        }

        // Add node to trees[]
        trees[treeCount - (1 + i)] = nodesp;
        ++nodesp;
    }

    //// Create dictionary
    code_t dictionary[256];
    createDictionary(*trees, dictionary, 0, 0);

    //// Write to outputFile
    // Create bitBuffer, used to write bitwise to output file
    bitBuffer_t bitBuffer = { malloc(BUFSIZ * sizeof(char)), BUFSIZ, 0, 0, output };
    if (!bitBuffer.buffer) {
        fprintf(stderr, "Couldn't allocate memory for buffer");
        exitCode = 1;
        goto cleanupTree;
    }

    // write tree to file
    writeTree(*trees, &bitBuffer);

    // write data to file
    rewind(input);
    while ((bytesRead = fread(buffer, sizeof(char), BUFSIZ, input)) > 0) {
        for (i = 0; i < bytesRead; ++i) {
            writeBits(dictionary[buffer[i]].code, dictionary[buffer[i]].codeLength, &bitBuffer);
        }
    }

    // The last three bits show how many bits of the last byte are significant and hence should be
    // decoded later
    if (bitBuffer.bitPosition <= 5) {
        uint64_t significantBits = (uint64_t)bitBuffer.bitPosition << 61;
        writeBits(0, 5 - bitBuffer.bitPosition, &bitBuffer);
        writeBits(significantBits, 3, &bitBuffer);
    } else {
        writeByte(0, &bitBuffer);
    }

    flush(&bitBuffer);
cleanupBuffer:
    free(bitBuffer.buffer);
cleanupTree:
    free(trees);
cleanupNode:
    free(nodes);
cleanup:

    return exitCode;
}

// decode file
extern int decode(FILE* input, FILE* output)
{
    int exitCode = 0;

    // Initialize bitBuffer to read bitwise later, buffersize has to be at least 511 (max size of
    // tree in file)
    bitBuffer_t bitBuffer = { malloc(BUFSIZ),
        (bitBuffer.buffer) ? fread(bitBuffer.buffer, sizeof(char), BUFSIZ, input) : 0, 0, 0,
        input };

    // Check for failed malloc
    if (!bitBuffer.buffer) {
        fprintf(stderr, "Couldn't allocate memory\n");
        exitCode = 1;
        goto cleanup;
    }

    // check if INFILE is empty
    if (!bitBuffer.bufferSize) {
        fprintf(stderr, "INFILE is empty\n");
        exitCode = 1;
        goto cleanupBuffer;
    }

    // Handle case when only one symbol is encoded
    if (bitBuffer.buffer[0] & (1 << 7)) {
        // File has to have 10 Bytes assuming uint64_t is 8 bytes
        if (bitBuffer.bufferSize != 10) {
            fprintf(stderr, "INFILE has invalid format\n");
            exitCode = 1;
            goto cleanupBuffer;
        }
        int currChar = bitBuffer.buffer[1];
        uint64_t i;
        uint64_t symbolFrequency = *(uint64_t*)(bitBuffer.buffer + 2);
        for (i = 0; i < symbolFrequency; i++) {
            fputc(currChar, output);
        }
        goto cleanupBuffer;
    }

    // Read tree from file
    node_t tree[511] = { 0 };
    int nodeNum = 0;
    readTree(&bitBuffer, tree, &nodeNum);

    // Decode data using the tree

    // get filesize
    size_t filePos = ftell(input);
    fseek(input, 0, SEEK_END);
    size_t bytesLeft = ftell(input);
    bytesLeft -= bitBuffer.bufferPosition;
    fseek(input, filePos, SEEK_SET);

    node_t* currNode = tree;

    // Read bit for bit and follow tree, write character if leaf is reached and begin from root of
    // tree
    while (bitBuffer.bufferSize > 0) {
        while (bitBuffer.bufferPosition < bitBuffer.bufferSize && bytesLeft > 1) {
            for (; bitBuffer.bitPosition < 8; bitBuffer.bitPosition++) {
                if (bitBuffer.buffer[bitBuffer.bufferPosition] & 1 << (7 - bitBuffer.bitPosition))
                    currNode = currNode->right;
                else
                    currNode = currNode->left;
                if (isLeaf(currNode)) {
                    fputc(currNode->symbol, output);
                    currNode = tree;
                }
            }
            ++bitBuffer.bufferPosition;
            bitBuffer.bitPosition = 0;
            --bytesLeft;
        }
        bitBuffer.bufferSize = fread(bitBuffer.buffer, sizeof(char), BUFSIZ, input);
        if (bitBuffer.bufferSize != 0) {
            bitBuffer.bufferPosition = 0;
        }
    }

    // Read bits of last byte, the number of significant bits is stored in the last three bits
    for (; bitBuffer.bitPosition < (bitBuffer.buffer[bitBuffer.bufferPosition] & 7);
         ++bitBuffer.bitPosition) {
        if (bitBuffer.buffer[bitBuffer.bufferPosition] & 1 << (7 - bitBuffer.bitPosition))
            currNode = currNode->right;
        else
            currNode = currNode->left;
        if (isLeaf(currNode)) {
            fputc(currNode->symbol, output);
            currNode = tree;
        }
    }

cleanupBuffer:
    free(bitBuffer.buffer);
cleanup:
    return exitCode;
}
