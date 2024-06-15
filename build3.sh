#!/bin/bash

# Create a build directory for the out-of-source build
if [ ! -d "build" ]; then
  mkdir build
fi

# Move into the build directory
cd build

# Configure the project with CMake
cmake ..

# Build the project
make

# Move back to the original directory
cd ..

echo "Build finished."
