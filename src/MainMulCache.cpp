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
void simulate_single(Cache *cache, MemoryManager *memory, std::ofstream &csvFile);
void simulate_multi(Cache *cache1, Cache *cache2, Cache *cache3, MemoryManager *memory, std::ofstream &csvFile);
void compare1(std::ofstream &csvFile);
void compare2(std::ofstream &csvFile);
void compare3(std::ofstream &csvFile);

const char *traceFilePath;

int main(int argc, char **argv) {
    if (!parseParameters(argc, argv)) {
        return -1;
    }
    std::ofstream csvFile("./src/analysis_p2.csv");
    compare1(csvFile);
    compare2(csvFile);
    compare3(csvFile);
    csvFile.close();
    return 0;
}

void compare1(std::ofstream &csvFile) {
    MemoryManager *memory_single = new MemoryManager();
    Cache *cache = new Cache(memory_single, 1, 16 * 1024, 64, 1, true, true);
    simulate_single(cache, memory_single, csvFile);
    csvFile << std::endl;

    csvFile << "inclusive three-level cache without victim:" << std::endl;
    MemoryManager *memory_mul = new MemoryManager();
    Cache *cache1 = new Cache(memory_mul, 1, 16 * 1024, 64, 1, true, true);
    Cache *cache2 = new Cache(memory_mul, 8, 128 * 1024, 64, 8, true, true);
    Cache *cache3 = new Cache(memory_mul, 20, 2 * 1024 * 1024, 64, 16, true, true);
    cache1->set_lower_cache(cache2);
    cache2->set_lower_cache(cache3);
    simulate_multi(cache1, cache2, cache3, memory_mul, csvFile);
    csvFile << std::endl;
}

void compare2(std::ofstream &csvFile) {
    csvFile << "exclusive three-level cache without victim:" << std::endl;
    MemoryManager *memory_exclusive = new MemoryManager();
    Cache *cache1_exclusive = new Cache(memory_exclusive, 1, 16 * 1024, 64, 1, true, true, true);
    Cache *cache2_exclusive = new Cache(memory_exclusive, 8, 128 * 1024, 64, 8, true, true, true);
    Cache *cache3_exclusive = new Cache(memory_exclusive, 20, 2 * 1024 * 1024, 64, 16, true, true, true);
    cache1_exclusive->set_lower_cache(cache2_exclusive);
    cache2_exclusive->set_lower_cache(cache3_exclusive);
    simulate_multi(cache1_exclusive, cache2_exclusive, cache3_exclusive, memory_exclusive, csvFile);
    csvFile << std::endl;
}

void compare3(std::ofstream &csvFile) {
    csvFile << "inclusive three-level cache with victim:" << std::endl;
    MemoryManager *memory_victim = new MemoryManager();
    Cache *cache1 = new Cache(memory_victim, 1, 16 * 1024, 64, 1, true, true);
    Cache *cache2 = new Cache(memory_victim, 8, 128 * 1024, 64, 8, true, true);
    Cache *cache3 = new Cache(memory_victim, 20, 2 * 1024 * 1024, 64, 16, true, true);
    Cache *victim = new Cache(memory_victim, 2, 8 * 64, 64, 8, true, true);
    cache1->set_lower_cache(cache2);
    cache1->set_victim(victim);
    cache2->set_lower_cache(cache3);
    simulate_multi(cache1, cache2, cache3, memory_victim, csvFile);
    csvFile << std::endl;
}

void simulate_multi(Cache *cache1, Cache *cache2, Cache *cache3, MemoryManager *memory, std::ofstream &csvFile) {
    // open the trace file
    std::ifstream traceFile(traceFilePath);
    if (!traceFile.is_open()) {
        printf("Unable to open file %s\n", traceFilePath);
        exit(-1);
    }

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
            uint8_t result = cache1->get_byte(address, &cycles);
            // std::cout << result << std::endl;
        } else if (operation == 'w') {
            cache1->set_byte(address, 6, &cycles);
        }
    }
    // uint32_t totalCycles = cache1->baseCycles + cache1->missCycles + cache2->missCycles + cache3->missCycles;
    uint32_t totalCycles = cache1->get_total_cycles();
    float avgCycles = (float )totalCycles / count;
    csvFile << "totalCycles: " << totalCycles << "  "
        << "average cycles: " << avgCycles << std::endl;
}

void simulate_single(Cache *cache, MemoryManager *memory, std::ofstream &csvFile) {
    // open the trace file
    std::ifstream traceFile(traceFilePath);
    if (!traceFile.is_open()) {
        printf("Unable to open file %s\n", traceFilePath);
        exit(-1);
    }

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
    float missRate = (float)cache->numMiss / cache->numAccesses;
    uint32_t totalCycles = cache->baseCycles + cache->missCycles;
    float avgCycles = (float )totalCycles / count;
    csvFile << "single-level cache:" << std::endl;
    csvFile << "totalCycles: " << totalCycles << "  "
            << "average cycles: " << avgCycles << std::endl;
}

bool parseParameters(int argc, char **argv) {
    traceFilePath = argv[1];
    if (traceFilePath == nullptr) {
        return false;
    }
    return true;
}
