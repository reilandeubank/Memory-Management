// Reiland Eubank, 12183371


#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/wait.h>
#define FRAME_SIZE 256 /*Defining the size of a frame in bytes*/
#define MEM_SIZE 256 /*size of physical memory*/
#define ADDR_SPACE_SIZE 256
#define INT_MAX 2147483647

int TLB[2][16];
int pagetable[ADDR_SPACE_SIZE];
int present[ADDR_SPACE_SIZE];
int count = 0;
int inputSize = 0;


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

int leastRecentlyUsed() {
    int minVal = INT_MAX;
    for (int j = 0; j < ADDR_SPACE_SIZE; j++) {
        if (present[j] < minVal && present[j] != 0) {
            minVal = j;
        }
    }
    return minVal;
}

void clockCycle() {
    for (int j = 0; j < ADDR_SPACE_SIZE; j++) {
        if (present[j]) {
            present[j]++;
        }
    }
}

void updateTLB() {
    for (int i = 0; i < 16; i++) {
        if (!present[TLB[0][i]]) {
            TLB[0][i] = -1;
            TLB[1][i] = -1;
        }
    }
}

int main(int argc, char *argv[]) {

    for (int i = 0; i < MEM_SIZE; i++) {
        present[i] = 0;
        pagetable[i] = -1;
    }
    for (int i = 0; i < 16; i++){
        TLB[0][i] = -1;
        TLB[1][i] = -1;
    }

    char *fileName;
    int memSize;

    if (argc >= 3) {
        fileName = argv[1];
        memSize = atoi(argv[2]);
    }
    else if (argc == 2) {
        memSize = 256;
    }
    char physicalMemory[memSize][FRAME_SIZE];

    int input[10000];
    FILE *fin;
    fin = fopen(fileName, "r");

    while (fscanf(fin, "%d", &input[inputSize]) != EOF) { // Read until end of file
        inputSize++;
    }
    
    fclose(fin);

    FILE *binFile;
    binFile = fopen("BACKING_STORE.bin", "rb");

    char string[FRAME_SIZE];
    int physicalAddress;
    int frameNum;
    int TLBCounter = 0;
    int TLBHits = 0;
    int pageFaults = 0;
    int valAtAddress;

    for (int i = 0; i< inputSize; i++) {
        int pageNum = getPageNumber(input[i]);
        frameNum = -1;

        if (count < memSize) {
            //printf("count %d < memsize\n", count);

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
            else if (pagetable[pageNum] == -1) {                                            // TLB Miss + Page Fault
                pagetable[pageNum] = count;
                frameNum = pagetable[pageNum];
                count++;
                physicalAddress = getPhysAddress(pagetable[pageNum], getOffset(input[i]));
                TLB[0][TLBCounter] = pageNum;
                TLB[1][TLBCounter] = pagetable[pageNum];
                TLBCounter = (TLBCounter + 1) % 16;
                pageFaults++;
            }
            else {                                                                          // TLB Miss + Page Table Hit
                //printf("pagetable hit\n");
                frameNum = pagetable[pageNum];
                // printf("frame number: %d\n", frameNum);
                physicalAddress = getPhysAddress(frameNum, getOffset(input[i]));
                TLB[0][TLBCounter] = pageNum;
                TLB[1][TLBCounter] = pagetable[pageNum];
                TLBCounter = (TLBCounter + 1) % 16;
            }
            if (!present[pageNum]) {
                fseek(binFile, pageNum*FRAME_SIZE, SEEK_SET);
                fread(&string, FRAME_SIZE, 1, binFile);
                for(int k = 0; k < FRAME_SIZE; k++) {
                    physicalMemory[frameNum][k] = string[k];
                }
                valAtAddress = physicalMemory[frameNum][getOffset(input[i])];
                present[pageNum] = 1;
            }
            else {
                valAtAddress = physicalMemory[frameNum][getOffset(input[i])];
            }
        }
        else {
            if (present[pageNum]){
                // printf("present\n\n");
                for (int j = 0; j < 16; j++){
                    if (TLB[0][j] == pageNum) {
                        frameNum = TLB[1][j];
                        TLBHits++;
                        continue;
                    }
                }
                if (frameNum != -1) {                                                           // TLB Hit
                    physicalAddress = getPhysAddress(frameNum, getOffset(input[i]));
                }
                else {                                                                          // TLB Miss + Page Table Hit
                    frameNum = pagetable[pageNum];
                    physicalAddress = getPhysAddress(frameNum, getOffset(input[i]));
                    TLB[0][TLBCounter] = pageNum;
                    TLB[1][TLBCounter] = pagetable[pageNum];
                    TLBCounter = (TLBCounter + 1) % 16;
                }
                valAtAddress = physicalMemory[frameNum][getOffset(input[i])];
            }
            else {
                //printf("Else else\n");
                int lruFrame = leastRecentlyUsed();
                fseek(binFile, pageNum*FRAME_SIZE, SEEK_SET);
                fread(&string, FRAME_SIZE, 1, binFile);
                for(int k = 0; k < FRAME_SIZE; k++) {
                    physicalMemory[lruFrame][k] = string[k];
                }
                pagetable[pageNum] = lruFrame;
                TLB[0][TLBCounter] = pageNum;
                TLB[1][TLBCounter] = lruFrame;
                TLBCounter = (TLBCounter + 1) % 16;
                present[pageNum] = 1;
                valAtAddress = physicalMemory[lruFrame][getOffset(input[i])];
                pageFaults++;
            }
        }
        
        updateTLB();
        clockCycle();
        printf("Virtual Address: %d Physical Address: %d Value: %d\n", input[i], physicalAddress, valAtAddress);
    }
    printf("Number of Translated Addresses = %d\n", inputSize);
    printf("Page Faults = %d\n", pageFaults);
    printf("Page Fault Rate = %.3f\n", (float)pageFaults / (float)inputSize);
    printf("TLB Hits = %d\n", TLBHits);
    printf("TLB Hit Rate = %.3f\n", (float)TLBHits / (float)inputSize);
    return 0;
}