#ifndef HNoisePowerHeaderStruct_H__
#define HNoisePowerHeaderStruct_H__

#include <stdint.h>
#include <stdlib.h>

#include "HBasicSizes.h"

/*
*File: HNoisePowerHeaderStruct.hh
*Author: J. Barrett
*Email: barrettj@mit.edu
*Date:
*Description:
*/

#define NOISE_POWER_HEADER_VERSION 1

struct HNoisePowerHeaderStruct
{
    size_t fHeaderSize; //size of the header struct in bytes
    char fVersionFlag[FLAG_WIDTH]; //version code
    char fSidebandFlag[FLAG_WIDTH]; //flag indicating the sideband
    char fPolarizationFlag[FLAG_WIDTH]; //flag indicating the polarization recorded
    uint64_t fStartTime; //acquisition start time in seconds since epoch
    uint64_t fSampleRate; //may need to accomodate doubles?
    uint64_t fLeadingSampleIndex; //sample index since start of the acquisition
    size_t fSampleLength; //total number of samples used during the noise power accumulation
    size_t fOnAccumulationLength; //number of periods stored in this file with noise diode 'on'
    size_t fOffAccumulationLength; //number of periods stored in this file with noise diode 'off'
    double fSwitchingFrequency; //frequency at which the noise diode is switched on/off
    double fBlankingPeriod; //period of time about the switching transition which is 'masked' (discarded from the calculation)
    char fExperimentName[NAME_WIDTH]; //experiment name
    char fSourceName[NAME_WIDTH]; //source name
    char fScanName[NAME_WIDTH]; //scan name
};

#endif /* end of include guard: HNoisePowerHeaderStruct */
