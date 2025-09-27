#!/bin/bash
set -euo pipefail

OUT="./mydaemon"
CXXFLAGS="-std=c++17 -Wall -Werror -O2"

echo "Cleaning..."
rm -f "${OUT}" *.o

echo "Building..."
g++ ${CXXFLAGS} -o "${OUT}" main.cpp MyDaemon.cpp Config.cpp utils.cpp

echo "Build complete: ${OUT}"