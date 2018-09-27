#Hose: Haystack Observatory Spectrometer Experiment
#
# Required dependencies: 
#(1) A compiler with C++11 support, e.g gcc >= 4.8 or clang >= 3.3.
#(2) CMake 3.0 or greater (https://cmake.org/) required by the build system.
#    To configure options you may want to obtain the command line GUI
#    interface for cmake, (ccmake). On a debian base Linux system this can be 
#    done with:
#        sudo apt-get install cmake cmake-curses-gui 
#    Other systems may name these packages differently.
#
#(3) Python 2.7 (Python 3 not yet supported)
#
#Optional external dependencies
#(1) Signatec library for interfacing with PX14400 digitizer.
#(2) Teledyne SP devices library for interfacing with ADQ7 digitizer.
#(3) NVidia CUDA Toolkit (https://developer.nvidia.com/cuda-zone). 
#    Needed for GPU support (only tested with version 6.0 and 8.0).
#(4) ROOT (https://root.cern.ch/), needed for spectrum plotting.
#(5) ZeroMQ (http://zeromq.org/), needed for GPU spectrometer client/server interface.
#
#To compile and install:
#(1) Download the repository with:
#    git clone git@github.mit.edu:barrettj/Hose.git
#
#    Note: To clone  the repository without password input you may need to set 
#    up ssh-keys on your machine for your github account.
#
#(2) To compile, do the following:
#    cd ./Hose
#    mkdir build
#    cd ./build
#    ccmake ../
#    <..configure options as needed..>
#    make && make install
#
#(3) To place the necessary libraries and executables in your path, run:
#    source <Hose>/install/bin/hoseenv.sh