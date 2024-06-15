#!/bin/bash

# Define the output binary name
output="single"

# Move into the src directory to locate the source files
cd src

# Compile the project, specify the include directory, and link necessary files
g++ -g -o $output MainSinCache.cpp Cache.cpp MemoryManager.cpp -I../include

# Move back to the project root directory
cd ..

# Run the compiled executable with the trace file
./src/$output ./cache-trace/trace1.trace
