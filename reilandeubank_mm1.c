// Reiland Eubank, 12183371


#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/wait.h>
#define FRAME_SIZE 256 /*Defining the size of a frame in bytes*/
#define MEM_SIZE 256 /*size of physical memory and page table*/

int getPageNumber(int address) {
    return (address & 0xFF00) >> 8;
}

int getOffset(int address) {
    return address & 0x00FF;
}

int getPhysAddress(int frame, int offset) {
    // printf("Frame: %d, Offset: %d, PhysAddress: %d\n", frame, offset,(frame << 8) | offset);
    return (frame << 8) | offset;
}

int main(int argc, char *argv[]) {
    int TLB[2][16];
    int pagetable[MEM_SIZE];
    int present[MEM_SIZE];
    char physicalMemory[MEM_SIZE][256];
    int count = 0;
    int inputSize = 0;

    for (int i = 0; i < MEM_SIZE; i++) {
        present[i] = 0;
        pagetable[i] = -1;
    }
    for (int i = 0; i < 16; i++){
        TLB[0][i] = -1;
        TLB[1][i] = -1;
    }

    char *fileName;

    if (argc >= 2) {
        fileName = argv[1];
    }

    int input[10000];
    FILE *fin;
    fin = fopen(fileName, "r");

    while (fscanf(fin, "%d", &input[inputSize]) != EOF) { // Read until end of file
        inputSize++;
    }
    
    fclose(fin);

    FILE *binFile;
    binFile = fopen("BACKING_STORE.bin", "rb");

    char string[256];
    int physicalAddress;
    int frameNum;
    int TLBCounter = 0;
    int TLBHits = 0;
    int pageFaults = 0;

    for (int i = 0; i< inputSize; i++) {
        int pageNum = getPageNumber(input[i]);
        frameNum = -1;

        for (int j = 0; j < 16; j++){
            if (TLB[0][j] == pageNum) {
                frameNum = TLB[1][j];
                TLBHits++;
                // printf("%d %d\n", input[i], frameNum);
                break;
            }
        }
        
        if (frameNum != -1) {                                                           // TLB Hit
            physicalAddress = getPhysAddress(frameNum, getOffset(input[i]));
        }
        else if (pagetable[pageNum] == -1) {                                            // Page Fault
            pagetable[pageNum] = count;
            count++;
            physicalAddress = getPhysAddress(pagetable[pageNum], getOffset(input[i]));
            TLB[0][TLBCounter] = pageNum;
            TLB[1][TLBCounter] = pagetable[pageNum];
            TLBCounter = (TLBCounter + 1) % 16;
            pageFaults++;
        }
        else {                                                                          // TLB Miss + Page Table Hit
            frameNum = pagetable[pageNum];
            physicalAddress = getPhysAddress(frameNum, getOffset(input[i]));
            TLB[0][TLBCounter] = pageNum;
            TLB[1][TLBCounter] = pagetable[pageNum];
            TLBCounter = (TLBCounter + 1) % 16;
        }

        //physicalAddress = getPhysAddress(pagetable[pageNum], getOffset(input[i]));
        if (!present[frameNum]) {
            //printf("not present in memory, pulling from BACKING_STORE\n");
            fseek(binFile, pageNum*256, SEEK_SET);
            fread(&string, FRAME_SIZE, 1, binFile);
            for(int i = 0; i < FRAME_SIZE; i++) {
                physicalMemory[frameNum][i] = string[i];
            }
            present[frameNum] = 1;
        }

        printf("Virtual Address: %d Physical Address: %d Value: %d\n", input[i], physicalAddress, physicalMemory[frameNum][getOffset(input[i])]);
    }
    printf("Number of Translated Addresses = %d\n", inputSize);
    printf("Page Faults = %d\n", pageFaults);
    printf("Page Fault Rate = %.3f\n", (float)pageFaults / (float)inputSize);
    printf("TLB Hits = %d\n", TLBHits);
    printf("TLB Hit Rate = %.3f\n", (float)TLBHits / (float)inputSize);
    return 0;
}