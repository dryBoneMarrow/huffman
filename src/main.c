#include "huffman.h"
#include <stdio.h>
#include <string.h>

void printHelp()
{
    printf("\nEncodes and decodes data using huffman algorithm.\n\n"

           "Usage:\n"
           " huffman encode|decode [INFILE [OUTFILE]]\n"
           " huffman help\n\n"

           "Note:\n"
           " - may be used for stdin / stdout\n"
           " When omitting INFILE/OUTFILE, stdin/stdout is used\n\n");
}

// open input file, - for stdin
FILE* getInput(char* filename)
{
    FILE* input;
    if (!strcmp(filename, "-")) {
        // Both the encode and decode functions change the file pointer position which is not
        // possible in stdin. That's why we copy the stdin into a temporary file first
        int currChar;
        input = tmpfile();
        while ((currChar = fgetc(stdin)) != EOF) {
            fputc(currChar, input);
        }
        rewind(input);
    } else {
        input = fopen(filename, "rb");
    }
    return input;
}

// open output file, - for stdout
FILE* getOutput(char* filename)
{
    FILE* output;
    if (!strcmp(filename, "-")) {
        output = stdout;
    } else {
        output = fopen(filename, "wb");
    }
    return output;
}

int main(int argc, char* argv[])
{
    int exitCode = 0;

    // select operation
    int (*operation)(FILE*, FILE*);
    if (argc < 2) {
        fprintf(stderr, "\nNot enough arguments\n");
        printHelp();
        exitCode = 1;
        goto cleanup;
    } else if (!strcmp(argv[1], "help")) {
        printHelp();
        goto cleanup;
    } else if (!strcmp(argv[1], "encode")) {
        operation = &encode;
    } else if (!strcmp(argv[1], "decode")) {
        operation = &decode;
    } else {
        printHelp();
        exitCode = 1;
        goto cleanup;
    }

    // get and validate INFILE and OUTFILE
    FILE *input, *output;
    char* inputPath;
    char* outputPath;
    switch (argc) {
    case 2:
        inputPath = "-";
        outputPath = "-";
        break;
    case 3:
        inputPath = argv[2];
        outputPath = "-";
        break;
    default:
        inputPath = argv[2];
        outputPath = argv[3];
        break;
    }
    input = getInput(inputPath);
    if (!input) {
        fprintf(stderr, "can't open INFILE\n");
        exitCode = 1;
        goto cleanup;
    }
    output = getOutput(outputPath);
    if (!output) {
        fprintf(stderr, "can't open OUTFILE\n");
        exitCode = 1;
        goto cleanupOutput;
    }

    // run the actual program
    exitCode = operation(input, output);

    fclose(output);
cleanupOutput:
    fclose(input);
cleanup:
    return exitCode;
}
