#ifndef CACHE_H
#define CACHE_H

#include <vector>
#include "MemoryManager.h"

class MemoryManager;

class Cache
{
public:
    uint32_t cacheSize;
    uint32_t blockSize;
    uint32_t associativity;
    uint32_t numBlocks;
    bool writeBack;
    bool writeAllocate;
    bool exclusive;
    Cache *lowerCache;
    Cache *victim;
    Cache *higherCache;
    MemoryManager *memory;
    uint32_t numAccesses;
    uint32_t numHit;
    uint32_t numMiss;
    uint64_t baseCycles;
    uint64_t missCycles;
    uint32_t hitLatency;
    uint32_t missLatency;

    struct Block
    {
        bool valid;   // valid bit
        bool dirty;   // dirty bit
        uint32_t tag; // tag
        uint32_t setNum;
        uint32_t lastAccess;
        std::vector<uint8_t> data; // data in each block, an array of uint_8
    };

    Cache(MemoryManager *memory, uint32_t hitLatency, uint32_t cacheSize, uint32_t blockSize,
          uint32_t associativity, bool writeBack = true, bool writeAllocate = true, bool exclusive = false,
          Cache *lowerCache = nullptr, Cache *higherCache = nullptr);
    void set_lower_cache(Cache *cache);
    uint8_t get_byte(uint32_t addr, uint32_t *cycles);
    void set_byte(uint32_t addr, uint8_t val, uint32_t *cycles = nullptr);
    void set_victim(Cache *victim);
    uint32_t get_total_cycles();

private:
    void initializeCache();
    bool inCache(uint32_t addr);
    uint32_t getTag(uint32_t addr);
    uint32_t getIndex(uint32_t addr);
    uint32_t getOffset(uint32_t addr);
    int findInCache(uint32_t addr);
    Block getBlockFromLowerLevel(uint32_t addr, uint32_t *cycles = nullptr);
    void writeBlockToLowerLevel(Block *block, uint32_t addr, uint32_t *cycles = nullptr);
    uint32_t getMemBegin(uint32_t addr);
    uint32_t findReplacedBlockId(uint32_t addr);
    uint32_t getAddrFromBlockId(uint32_t blockId);
    void evictBlockFromHigherCaches(uint32_t addr);
    void evictBlock(uint32_t blockId);
    void insertToVictim(Block *evictedBlock, uint32_t addr);
    std::vector<Block> blocks; // an array of blocks, the cache consists of caches, the size is cacheSize/blockSize
};

#endif