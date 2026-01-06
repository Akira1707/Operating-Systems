#!/bin/bash
rm -rf CMakeFiles CMakeCache.txt Makefile cmake_install.cmake host_* client_*
cmake .
make
