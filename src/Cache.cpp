#include <iostream>
#include <cmath>
#include "Cache.h"

Cache::Cache(MemoryManager *memory, uint32_t hitLatency, uint32_t cacheSize, uint32_t blockSize, uint32_t associativity,
             bool writeBack, bool writeAllocate, bool exclusive,  Cache *lowerCache, Cache *higherCache) {
    this->cacheSize = cacheSize;
    this->blockSize = blockSize;
    this->associativity = associativity;
    this->numBlocks = cacheSize / blockSize;
    this->writeBack = writeBack;
    this->writeAllocate = writeAllocate;
    this->lowerCache = lowerCache;
    this->victim = nullptr;
    this->higherCache = higherCache;
    this->exclusive = exclusive;
    this->memory = memory;
    this->numAccesses = 0;
    this->numHit = 0;
    this->numMiss = 0;
    this->baseCycles = 0;
    this->missCycles = 0;
    this->hitLatency = hitLatency;
    this->missLatency = 100;
    // initialize the cache
    this->initializeCache();
}

uint8_t Cache::get_byte(uint32_t addr, uint32_t *cycles) {
    this->numAccesses++;
    if (cycles != nullptr) this->baseCycles += this->hitLatency;
    uint32_t index = this->getIndex(addr);
    uint32_t tag = this->getTag(addr);
    uint32_t offset = this->getOffset(addr);

    int blockId = this->findInCache(addr);
    if (blockId != -1) {
        // if the block is in cache
        this->numHit++;
        this->blocks[blockId].lastAccess = this->numAccesses;
        return this->blocks[blockId].data[offset];
    } else {
        // cache miss
        if (this->victim != nullptr) {
            // check in victim cache
            int victimBlockId = this->victim->findInCache(addr); // the blockId in victim that contains addr
            // this->missCycles += this->victim->hitLatency;
            if (victimBlockId != -1) {
                this->victim->blocks[victimBlockId].lastAccess = this->numAccesses;
                return this->victim->blocks[victimBlockId].data[offset];
            }
        }
        this->numMiss++;
        Block block;
        block = this->getBlockFromLowerLevel(addr, cycles);  // construct a block with data from lower-level cache.
        if (cycles != nullptr) this->missCycles += this->missLatency;

        // find the blockId to place the new block
        uint32_t replacedBlockId = findReplacedBlockId(addr);
        if ((this->blocks[replacedBlockId].valid && this->blocks[replacedBlockId].dirty && !this->exclusive) || 
            (this->blocks[replacedBlockId].valid && this->exclusive)) {

            writeBlockToLowerLevel(&this->blocks[replacedBlockId], addr, cycles);
            if (cycles != nullptr) this->missCycles += this->missLatency;
        }

        if (this->victim != nullptr && this->blocks[replacedBlockId].valid) {
            this->insertToVictim(&this->blocks[replacedBlockId], this->getAddrFromBlockId(replacedBlockId));
            // this->missCycles += this->victim->hitLatency;
        }
        this->blocks[replacedBlockId] = block;
        return block.data[offset];
    }
}

void Cache::set_byte(uint32_t addr, uint8_t val, uint32_t *cycles) {
    this->numAccesses++;
    if (cycles != nullptr) this->baseCycles += this->hitLatency;
    uint32_t index = this->getIndex(addr);
    uint32_t tag = this->getTag(addr);
    uint32_t offset = this->getOffset(addr);

    int blockId = this->findInCache(addr);
    if (blockId != -1) {
        // if the block is in cache
        this->numHit++;
        this->blocks[blockId].lastAccess = this->numAccesses;
        this->blocks[blockId].dirty = true;
        this->blocks[blockId].data[offset] = val; // modify the data in cache
        if (!this->writeBack) {
            // if write through
            this->writeBlockToLowerLevel(&this->blocks[blockId], addr, cycles); // modify the data in lower level
            if (cycles != nullptr) this->missCycles += missLatency;
        }
    } else {
        // cache miss
        this->numMiss++;
        if (writeAllocate) {
            Block block = this->getBlockFromLowerLevel(addr, cycles);  // construct a block with data from lower-level cache.
            if (cycles != nullptr) this->missCycles += this->missLatency;
            block.data[offset] = val; // change the data in cache
            block.dirty = true;
            uint32_t replacedBlockId = findReplacedBlockId(addr); // find the blockId to place the new block
            if ((this->blocks[replacedBlockId].valid && this->blocks[replacedBlockId].dirty && !this->exclusive) || 
                (this->blocks[replacedBlockId].valid && this->exclusive)) {
                writeBlockToLowerLevel(&this->blocks[replacedBlockId], addr, cycles);
                if (cycles != nullptr) this->missCycles += this->missLatency;
            }
            this->blocks[replacedBlockId] = block;
        } else {
            if (cycles != nullptr) this->missCycles += missLatency;
            if (this->lowerCache == nullptr) {
                this->memory->setByteNoCache(addr, val);
            } else {
                this->lowerCache->set_byte(addr, val, cycles);
            }
        }
    }
}

void Cache::insertToVictim(Block *evictedBlock, uint32_t addr) {
    // find a free block in victim and replace it by evictedBlock
    int replaceIdx = -1;
    for (uint32_t i = 0; i < this->victim->associativity; i++) {
        if (!this->victim->blocks[i].valid) {
            replaceIdx = i;
            break;
        }
    }
    if (replaceIdx == -1) {
        replaceIdx = 0;
        uint32_t current = this->victim->blocks[0].lastAccess;
        for (uint32_t i = 0; i < this->victim->associativity; i++) {
            if (this->victim->blocks[i].lastAccess < current) {
                replaceIdx = i;
                current = this->victim->blocks[i].lastAccess;
            }
        }
    }
    this->victim->blocks[replaceIdx].data = evictedBlock->data;
    this->victim->blocks[replaceIdx].valid = true;
    this->victim->blocks[replaceIdx].tag = this->victim->getTag(addr);
    this->victim->blocks[replaceIdx].setNum = 0;
    this->victim->blocks[replaceIdx].lastAccess = this->numAccesses;
}

uint32_t Cache::getAddrFromBlockId(uint32_t blockId) {
    uint32_t offsetDigit = log2(this->blockSize);
    uint32_t indexDigit = log2(this->numBlocks / this->associativity);
    Block &b = this->blocks[blockId];
    uint32_t tag = b.tag;
    uint32_t index = b.setNum;
    uint32_t address = (tag << (indexDigit + offsetDigit)) | (index << offsetDigit);
    return address;
}

void Cache::initializeCache() {
    this->blocks.resize(this->numBlocks);
    for (uint32_t i = 0; i < this->numBlocks; i++) {
        Block &block = this->blocks[i];
        block.valid = false;
        block.tag = 0;
        block.setNum = i / this->associativity;
        block.data.resize(this->blockSize);
        block.lastAccess = 0;
        block.dirty = false;
    }
    // std::cout << "-----initialization success-----" << std::endl;
}

void Cache::set_lower_cache(Cache *cache) {
    this->lowerCache = cache;
    cache->higherCache = this;
    this->missLatency = cache->hitLatency;
}

void Cache::set_victim(Cache *victim) {
    this->victim = victim;
}

uint32_t Cache::findReplacedBlockId(uint32_t addr) {
    uint32_t index = this->getIndex(addr);
    uint32_t start = this->associativity * index;
    uint32_t end = this->associativity * (index + 1);

    // find invalid blocks
    for (uint32_t i = start; i < end; i++) {
        if (!this->blocks[i].valid) {
            return i;
        }
    }

    // the set in cache is full, evict a block using LRU, return the blockId with the lowest reference count
    uint32_t evictedBlockId = this->associativity * index;
    uint32_t current = this->blocks[evictedBlockId].lastAccess;
    for (uint32_t i = start; i < end; i++) {
        if (this->blocks[i].lastAccess < current) {
            evictedBlockId = i;
            current = this->blocks[i].lastAccess;
        }
    }

    if (!this->exclusive) {this->evictBlockFromHigherCaches(getAddrFromBlockId(evictedBlockId));}

    return evictedBlockId;
}

void Cache::evictBlockFromHigherCaches(uint32_t addr) {
    if (this->higherCache != nullptr) { 
        uint32_t blockId = this->higherCache->findInCache(addr);
        if (blockId != -1) {
            std::cout << "-----back invalidate!-----" << std::endl;
            // Evict the block from the higher cache
            this->higherCache->evictBlock(blockId);
            // Recursive call to ensure the block is removed from all higher levels
            this->higherCache->evictBlockFromHigherCaches(addr);
        }
    }
}

void Cache::evictBlock(uint32_t blockId) {
    // Evicts the block at blockId from this cache
    if (this->blocks[blockId].valid && this->blocks[blockId].dirty && this->lowerCache == nullptr) {
        uint32_t addr = this->getAddrFromBlockId(blockId);
        uint32_t blockBegin = getMemBegin(addr);
        for (uint32_t i = blockBegin; i < blockBegin + this->blockSize; i++) {
            this->memory->setByteNoCache(i, this->blocks[blockId].data[i-blockBegin]);
        }
    }
    // Invalidate the block
    this->blocks[blockId].valid = false;
    this->blocks[blockId].dirty = false;
}

void Cache::writeBlockToLowerLevel(Cache::Block *block, uint32_t addr, uint32_t *cycles) {
    uint32_t blockBegin = getMemBegin(addr);
    if (this->exclusive) {
        if (this->lowerCache != nullptr) {
            uint32_t replacedBlockId = this->lowerCache->findReplacedBlockId(addr);
            if (this->lowerCache->blocks[replacedBlockId].valid) {
                this->lowerCache->writeBlockToLowerLevel(&this->lowerCache->blocks[replacedBlockId], addr, cycles);
                if (cycles != nullptr) this->missCycles += this->missLatency;
            }
            // replace the blocks[blockId] in lower cache by the block from upper cache
            this->lowerCache->blocks[replacedBlockId] = *block;
            this->lowerCache->blocks[replacedBlockId].valid = true;
            this->lowerCache->blocks[replacedBlockId].tag = this->lowerCache->getTag(addr);
            this->lowerCache->blocks[replacedBlockId].setNum = this->lowerCache->getIndex(addr);
            this->lowerCache->blocks[replacedBlockId].lastAccess = this->numAccesses;
        } else if (block->dirty && this->lowerCache == nullptr) {
            // No lower cache and block is dirty, write back to memory
            for (uint32_t i = blockBegin; i < blockBegin + this->blockSize; i++) {
                this->memory->setByteNoCache(i, block->data[i - blockBegin]);
            }
        }
    } else {

        for (uint32_t i = blockBegin; i < blockBegin + this->blockSize; i++) {
            if (this->lowerCache == nullptr) {
                this->memory->setByteNoCache(i, block->data[i-blockBegin]);
            } else {
                this->lowerCache->set_byte(i, block->data[i-blockBegin], cycles);
            }
        }
    }
}

Cache::Block Cache::getBlockFromLowerLevel(uint32_t addr, uint32_t *cycles) {
    Block newBlock;
    uint32_t blockBegin = getMemBegin(addr);
    newBlock.data = std::vector<uint8_t>(this->blockSize);
    if (this->exclusive) {
        Cache *current = this->lowerCache;
        while (current != nullptr) {
            if (cycles != nullptr) this->missCycles += current->hitLatency;

            int blockId = current->findInCache(addr);
            if (blockId != -1) {
                newBlock = current->blocks[blockId];
                current->blocks[blockId].valid = false;
                current->blocks[blockId].dirty = false;
                break;
            } else {
                current = current->lowerCache;
            }
        }
        if (current == nullptr) {
            for (uint32_t i = blockBegin; i < blockBegin + this->blockSize; i++) {
                newBlock.data[i - blockBegin] = this->memory->getByteNoCache(i);
            }
            newBlock.dirty = false;
        }
        newBlock.valid = true;
        newBlock.tag = this->getTag(addr);
        newBlock.setNum = this->getIndex(addr);
        newBlock.lastAccess = this->numAccesses;
        return newBlock;
    } else {
        // inclusive cache, get block recursively
        for (uint32_t i = blockBegin; i < blockBegin + this->blockSize; i++) {
            if (this->lowerCache == nullptr) {
                newBlock.data[i-blockBegin] = this->memory->getByteNoCache(i);
            } else {
                newBlock.data[i-blockBegin] = this->lowerCache->get_byte(i, cycles);
            }
        }
        newBlock.valid = true;
        newBlock.dirty = false;
        newBlock.tag = this->getTag(addr);
        newBlock.setNum = this->getIndex(addr);
        newBlock.lastAccess = this->numAccesses;
        return newBlock;
    }
}

// get the beginning address of the block of addr in memory
uint32_t Cache::getMemBegin(uint32_t addr) {
    return (addr / blockSize) * this->blockSize;
    // uint32_t offsetDigit = log2(this->blockSize);
    // uint32_t mask = ~(1 << offsetDigit) - 1; // 11111100000
    // return addr & mask;
}

int Cache::findInCache(uint32_t addr) {
    uint32_t index = this->getIndex(addr);
    uint32_t tag = this->getTag(addr);
    // if the block is in the cache, return the block number, otherwise return -1
    for (uint32_t i = this->associativity * index; i < this->associativity * (index + 1); i++) {
        // for blocks in the corresponding set
        if (this->blocks[i].valid && (this->blocks[i].tag == tag)) {
            // std::cout << "find in Cache!" << std::endl;
            return i;
        }
    }
    return -1;
}

uint32_t Cache::getTag(uint32_t addr) {
    uint32_t offsetDigit = log2(this->blockSize);
    uint32_t indexDigit = log2(this->numBlocks / this->associativity);
    uint32_t mask = (1 << (32 - offsetDigit - indexDigit)) - 1;
    return (addr >> (offsetDigit + indexDigit)) & mask;
}

uint32_t Cache::getIndex(uint32_t addr) {
    uint32_t offsetDigit = log2(this->blockSize);
    uint32_t indexDigit = log2(this->numBlocks / this->associativity);
    uint32_t mask = (1 << indexDigit) - 1;
    return (addr >> offsetDigit) & mask;
}

uint32_t Cache::getOffset(uint32_t addr) {
    uint32_t offsetDigit = log2(this->blockSize);
    uint32_t mask = (1 << offsetDigit) - 1;
    return addr & mask;
}

uint32_t Cache::get_total_cycles() {
    uint32_t result = this->baseCycles + this->missCycles;
    Cache *current = this->lowerCache;
    while (current != nullptr) {
        result += current->missCycles;
        current = current->lowerCache;
    }
    return result;
}
