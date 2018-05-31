#ifndef HNoisePowerFileStruct_H__
#define HNoisePowerFileStruct_H__

#include "HNoisePowerHeaderStruct.h"
#include "HDataAccumulationStruct.h"

/*
*File: HNoisePowerFileStruct.h

*Author: J. Barrett
*Email: barrettj@mit.edu
*Date:
*Description:
*/

struct HSpectrumFileStruct
{
    //header data
    struct HNoisePowerHeaderStruct fHeader;
    //on/off statistics data
    struct HDataAccumulationStruct* OnAccumulations;
    struct HDataAccumulationStruct* OffAccumulations;
};




#endif /* end of include guard: HNoisePowerFileStruct */
