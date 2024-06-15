/*
 * Main entrance of the single-level cache simulator.
 * ./SinCacheSimulator path
 */

#include <iostream>
#include <cstring>
#include <cstdlib>
#include <fstream>
#include "Cache.h"
#include "MemoryManager.h"

bool parseParameters(int argc, char **argv);
void simulate(uint32_t cacheSize, uint32_t blockSize, uint32_t associativity,
              bool writeBack, bool writeAllocate, std::ofstream &csvFile);

const char *traceFilePath;

int main(int argc, char **argv) {
    if (!parseParameters(argc, argv)) {
        return -1;
    }
    std::ofstream csvFile("./src/analysis_p1.csv");
    csvFile << "cacheSize,blockSize,associativity,writeBack,writeAllocate,"
             "missRate,totalCycles,CPI" << std::endl;
    std::cout << "The tested trace file: " << traceFilePath << std::endl;
    for (uint32_t cacheSize = 4*1024; cacheSize <= 1024*1024; cacheSize *= 4) {
        for (uint32_t blockSize = 32; blockSize <= 256; blockSize *= 2) {
            for (uint32_t associativity = 2; associativity <= 32; associativity *= 2) {
                uint32_t numSets = cacheSize / blockSize;
                if (numSets < associativity != 0) continue;

                simulate(cacheSize, blockSize, associativity, false, false, csvFile);
                simulate(cacheSize, blockSize, associativity, true, false, csvFile);
                simulate(cacheSize, blockSize, associativity, false, true, csvFile);
                simulate(cacheSize, blockSize, associativity, true, true, csvFile);
            }
        }
    }
    // simulate(4*1024, 32, 4, false, false, csvFile);
    // simulate(4*1024, 32, 4, true, false, csvFile);
    // simulate(4*1024, 32, 4, false, true, csvFile);
    // simulate(4*1024, 32, 4, true, true, csvFile);
    csvFile.close();
    return 0;
}

void simulate(uint32_t cacheSize, uint32_t blockSize, uint32_t associativity, bool writeBack, bool writeAllocate, std::ofstream &csvFile) {
    // open the trace file
    std::ifstream traceFile(traceFilePath);
    if (!traceFile.is_open()) {
        printf("Unable to open file %s\n", traceFilePath);
        exit(-1);
    }

    MemoryManager *memory = new MemoryManager();
    Cache *cache = new Cache(memory, 1, cacheSize, blockSize, associativity, writeBack, writeAllocate);

    char operation;
    uint32_t address;
    uint32_t cycles = 0;
    int count = 0;
    while (traceFile >> operation >> std::hex >> address) {
        count++;
        if (!memory->isPageExist(address)) {
            memory->addPage(address);
        }
        if (operation == 'r') {
            uint8_t result = cache->get_byte(address, &cycles);
            // std::cout << result << std::endl;
        } else if (operation == 'w') {
            cache->set_byte(address, 6, &cycles);
        }
    }
    float missRate = (float) cache->numMiss / cache->numAccesses;
    uint32_t totalCycles = cache->baseCycles + cache->missCycles;
    float cpi = (float) totalCycles / count;
    csvFile << cacheSize << "," << blockSize << "," << associativity << "," << writeBack << ","
            << writeAllocate << "," << missRate << "," << totalCycles << "," << cpi << std::endl;
}

bool parseParameters(int argc, char **argv) {
    traceFilePath = argv[1];
    if (traceFilePath == nullptr) {
        return false;
    }
    return true;
}
