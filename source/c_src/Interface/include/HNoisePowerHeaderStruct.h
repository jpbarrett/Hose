#ifndef HNoisePowerHeaderStruct_H__
#define HNoisePowerHeaderStruct_H__

#include <stdint.h>
#include <stdlib.h>

#include "HBasicDefines.h"

/*
*File: HNoisePowerHeaderStruct.hh
*Author: J. Barrett
*Email: barrettj@mit.edu
*Date:
*Description:
*/

#define NOISE_POWER_HEADER_VERSION_NUMBER 1
#define NOISE_POWER_HEADER_VERSION #NOISE_POWER_HEADER_VERSION_NUMBER

struct HNoisePowerHeaderStruct
{
    size_t fHeaderSize; //size of the header struct in bytes
    char fVersionFlag[HFLAG_WIDTH]; //version code
    char fSidebandFlag[HFLAG_WIDTH]; //flag indicating the sideband
    char fPolarizationFlag[HFLAG_WIDTH]; //flag indicating the polarization recorded
    uint64_t fStartTime; //acquisition start time in seconds since epoch
    uint64_t fSampleRate; //may need to accomodate doubles?
    uint64_t fLeadingSampleIndex; //sample index since start of the acquisition
    size_t fSampleLength; //total number of samples used during the noise power accumulation
    size_t fAccumulationLength; //number of accumulation periods stored in this file
    double fSwitchingFrequency; //frequency at which the noise diode is switched on/off
    double fBlankingPeriod; //period of time about the switching transition which is 'masked' (discarded from the calculation)
    char fExperimentName[HNAME_WIDTH]; //experiment name
    char fSourceName[HNAME_WIDTH]; //source name
    char fScanName[HNAME_WIDTH]; //scan name
};

#endif /* end of include guard: HNoisePowerHeaderStruct */
