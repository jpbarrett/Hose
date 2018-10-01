# Hose: Haystack Observatory Spectrometer Experiment

# Required dependencies: 
1. A compiler with C++11 support, e.g gcc >= 4.8 or clang >= 3.3.
2. Python 2.7.x (Python 3 not yet supported).
3. CMake 3.0 or greater (https://cmake.org/) required by the build system.
    To configure options you may want to obtain the command line GUI
    interface for cmake, (ccmake). 
4. The version control software git if you wish to interface with the repository.
5. On a Debian based Linux system the basic pre-requisites can be installed with the following command:
    - sudo apt-get install git build-essential cmake cmake-curses-gui
6. On a Red Hat base Linux system (untested), they may be installed with:
    - sudo yum install git gcc cmake

# Optional external dependencies:
1. Signatec library for interfacing with PX14400 digitizer.
2. Teledyne SP devices library for interfacing with ADQ7 digitizer.
3. NVidia CUDA Toolkit (https://developer.nvidia.com/cuda-zone). 
    Needed for GPU support (only tested with version 6.0 and 8.0).
4. ROOT (https://root.cern.ch/), needed for spectrum plotting.
5. ZeroMQ (http://zeromq.org/), needed for GPU spectrometer client/server interface.

# To compile and install:
1. Download the repository with:
    - git clone git@github.mit.edu:barrettj/Hose.git
    - alternatively, you can retrieve a static version of the code via 
      a download of a .zip file at:
      https://github.mit.edu/barrettj/Hose/archive/master.zip

    Note: To clone the repository without password input you may need to set 
    up ssh-keys on your machine for your github account.
2.  Assuming you have downloaded and/or unpacked the Hose code to the relative directory ./Hose,
    to compile and install the basic libraries (namely the file I/O library) do the following:
    1. cd ./Hose && mkdir build
    2. cd ./build
    3. ccmake ../
    4. Press 'c' to configure, and 'g' to generate the build (accept the defaults to simply build the I/O library, or configure options as needed)
    5. make && make install

3. To place the necessary libraries and executables in your path, run:
    source <Hose directory>/install/bin/hoseenv.sh

# Conversion of spectrum files to SDFITS (this may change):
1. After installing Hose and setting the environmental variables with hoseenv.sh. 
You can convert a directory of spectrum files to a FITS file using the utilities in the folder <Hose directory>/ToFits.
2. First do change into the ToFits directory:
3. Make gpu_sdfits.py an executable (if not already) with:
    - chmod u+x ./gpu_sdfits.py
4. Then run:
    ./gpu_sdfits <path to spectrum files>
5. The output will be the file: <path to spectrum files>/.fits


