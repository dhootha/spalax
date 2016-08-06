#!/bin/bash

# Log commands and exit on error
set -xe

# Test-builds the firmware on Travis.
# Required packages: gcc-arm-none-eabi libnewlib-arm-none-eabi

cd Avionics.Firmware
make all VERBOSE=1

cd ../Avionics.GUI
mkdir build
cd build
cmake -G "Unix Makefiles" ..
make Avionics.GUI.Launcher VERBOSE=1