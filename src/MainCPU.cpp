/*
 * Created by He, Hao at 2019-3-11
 */

#include <cstdarg>
#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <string>

#include <elfio/elfio.hpp>

#include "BranchPredictor.h"
#include "Cache.h"
#include "Debug.h"
#include "MemoryManager.h"
#include "Simulator.h"

bool parseParameters(int argc, char **argv);
void printUsage();
void printElfInfo(ELFIO::elfio *reader);
void loadElfToMemory(ELFIO::elfio *reader, MemoryManager *memory);

char *elfFile = nullptr;
bool verbose = 0;
bool isSingleStep = 0;
bool dumpHistory = 0;
bool withCache = 0;
uint32_t stackBaseAddr = 0x80000000;
uint32_t stackSize = 0x400000;
MemoryManager memory;
Cache *cache1, *cache2, *cache3;
BranchPredictor::Strategy strategy = BranchPredictor::Strategy::NT;
BranchPredictor branchPredictor;
Simulator simulator(&memory, &branchPredictor);

int main(int argc, char **argv)
{
  if (!parseParameters(argc, argv))
  {
    printUsage();
    exit(-1);
  }

  // Init cache
  cache1 = new Cache(&memory, 1, 16 * 1024, 64, 1, true, true);
  cache2 = new Cache(&memory, 8, 128 * 1024, 64, 8, true, true);
  cache3 = new Cache(&memory, 20, 2 * 1024 * 1024, 64, 16, true, true);
  cache1->set_lower_cache(cache2);
  cache2->set_lower_cache(cache3);

  if (withCache) memory.setCache(cache1);

  // Read ELF file
  ELFIO::elfio reader;
  if (!reader.load(elfFile))
  {
    fprintf(stderr, "Fail to load ELF file %s!\n", elfFile);
    return -1;
  }

  if (verbose)
  {
    printElfInfo(&reader);
  }

  loadElfToMemory(&reader, &memory);

  simulator.isSingleStep = isSingleStep;
  simulator.verbose = verbose;
  simulator.shouldDumpHistory = dumpHistory;
  simulator.branchPredictor->strategy = strategy;
  simulator.pc = reader.get_entry();
  simulator.initStack(stackBaseAddr, stackSize);
  simulator.simulate();

  if (dumpHistory)
  {
    printf("Dumping history to dump.txt...\n");
    simulator.dumpHistory();
  }

  delete cache1;
  delete cache2;
  delete cache3;
  return 0;
}

bool parseParameters(int argc, char **argv)
{
  // Read Parameters
  for (int i = 1; i < argc; ++i)
  {
    if (argv[i][0] == '-')
    {
      switch (argv[i][1])
      {
      case 'c':
        withCache = 1;
        break;
      case 'v':
        verbose = 1;
        break;
      case 's':
        isSingleStep = 1;
        break;
      case 'd':
        dumpHistory = 1;
        break;
      case 'b':
        if (i + 1 < argc)
        {
          std::string str = argv[i + 1];
          i++;
          if (str == "AT")
          {
            strategy = BranchPredictor::Strategy::AT;
          }
          else if (str == "NT")
          {
            strategy = BranchPredictor::Strategy::NT;
          }
          else if (str == "BTFNT")
          {
            strategy = BranchPredictor::Strategy::BTFNT;
          }
          else if (str == "BPB")
          {
            strategy = BranchPredictor::Strategy::BPB;
          }
          else
          {
            return false;
          }
        }
        else
        {
          return false;
        }
        break;
      default:
        return false;
      }
    }
    else
    {
      if (elfFile == nullptr)
      {
        elfFile = argv[i];
      }
      else
      {
        return false;
      }
    }
  }
  if (elfFile == nullptr)
  {
    return false;
  }
  return true;
}

void printUsage()
{
  printf("Usage: Simulator riscv-elf-file [-v] [-s] [-d] [-b param]\n");
  printf("Parameters: \n\t[-v] verbose output \n\t[-s] single step\n \t[-c] withCache\n");
  printf("\t[-d] dump memory and register trace to dump.txt\n");
  printf("\t[-b param] branch perdiction strategy, accepted param AT, NT, "
         "BTFNT, BPB\n");
}

void printElfInfo(ELFIO::elfio *reader)
{
  printf("==========ELF Information==========\n");

  if (reader->get_class() == ELFCLASS32)
  {
    printf("Type: ELF32\n");
  }
  else
  {
    printf("Type: ELF64\n");
  }

  if (reader->get_encoding() == ELFDATA2LSB)
  {
    printf("Encoding: Little Endian\n");
  }
  else
  {
    printf("Encoding: Large Endian\n");
  }

  if (reader->get_machine() == EM_RISCV)
  {
    printf("ISA: RISC-V(0x%x)\n", reader->get_machine());
  }
  else
  {
    dbgprintf("ISA: Unsupported(0x%x)\n", reader->get_machine());
    exit(-1);
  }

  ELFIO::Elf_Half sec_num = reader->sections.size();
  printf("Number of Sections: %d\n", sec_num);
  printf("ID\tName\t\tAddress\tSize\n");

  for (int i = 0; i < sec_num; ++i)
  {
    const ELFIO::section *psec = reader->sections[i];

    printf("[%d]\t%-12s\t0x%llx\t%lld\n", i, psec->get_name().c_str(),
           psec->get_address(), psec->get_size());
  }

  ELFIO::Elf_Half seg_num = reader->segments.size();
  printf("Number of Segments: %d\n", seg_num);
  printf("ID\tFlags\tAddress\tFSize\tMSize\n");

  for (int i = 0; i < seg_num; ++i)
  {
    const ELFIO::segment *pseg = reader->segments[i];

    printf("[%d]\t0x%x\t0x%llx\t%lld\t%lld\n", i, pseg->get_flags(),
           pseg->get_virtual_address(), pseg->get_file_size(),
           pseg->get_memory_size());
  }

  printf("===================================\n");
}

void loadElfToMemory(ELFIO::elfio *reader, MemoryManager *memory)
{
  ELFIO::Elf_Half seg_num = reader->segments.size();
  for (int i = 0; i < seg_num; ++i)
  {
    const ELFIO::segment *pseg = reader->segments[i];

    uint64_t fullmemsz = pseg->get_memory_size();
    uint64_t fulladdr = pseg->get_virtual_address();
    // Our 32bit simulator cannot handle this
    if (fulladdr + fullmemsz > 0xFFFFFFFF)
    {
      dbgprintf(
          "ELF address space larger than 32bit! Seg %d has max addr of 0x%lx\n",
          i, fulladdr + fullmemsz);
      exit(-1);
    }

    uint32_t filesz = pseg->get_file_size();
    uint32_t memsz = pseg->get_memory_size();
    uint32_t addr = (uint32_t)pseg->get_virtual_address();

    for (uint32_t p = addr; p < addr + memsz; ++p)
    {
      if (!memory->isPageExist(p))
      {
        memory->addPage(p);
      }

      if (p < addr + filesz)
      {
        memory->setByteNoCache(p, pseg->get_data()[p - addr]);
      }
      else
      {
        memory->setByteNoCache(p, 0);
      }
    }
  }
}