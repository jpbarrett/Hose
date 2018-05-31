#ifndef HSpectrumHeaderStruct_H__
#define HSpectrumHeaderStruct_H__

#include <stdint.h>
#include <stdlib.h>

#include "HBasicSizes.h"

/*
*File: HSpectrumHeaderStruct.hh
*Author: J. Barrett
*Email: barrettj@mit.edu
*Date:
*Description:
*/

#define SPECTRUM_HEADER_VERSION 1

struct HSpectrumHeaderStruct
{
    size_t fHeaderSize; //size of the header struct in bytes
    char fVersionFlag[FLAG_WIDTH]; //version, format code (also indicates what type (e.g. double, float) the spectrum data is stored in)
    char fSidebandFlag[FLAG_WIDTH]; //flag indicating the sideband
    char fPolarizationFlag[FLAG_WIDTH]; //flag indicating the polarization recorded
    uint64_t fStartTime; //acquisition start time in seconds since epoch
    uint64_t fSampleRate; //may need to accomodate doubles?
    uint64_t fLeadingSampleIndex; //sample index since start of the acquisition
    size_t fSampleLength; //total number of samples used to compute the spectrum
    size_t fNAverages; //if spectral averaging was used, how many averaging periods were used
    size_t fSpectrumLength; //number of spectral points
    size_t fSpectrumDataTypeSize; //spectrum data point size (e.g. size of float, double, etc)
    char fExperimentName[NAME_WIDTH]; //experiment name
    char fSourceName[NAME_WIDTH]; //source name
    char fScanName[NAME_WIDTH]; //scan name
};


#endif /* end of include guard: HSpectrumHeaderStruct */