#ifndef HDataAccumulationStruct_H__
#define HDataAccumulationStruct_H__

#include <stdint.h>
#include <stdlib.h>

/*
*File: HDataAccumulationStruct.h
*Author: J. Barrett
*Email: barrettj@mit.edu
*Date:
*Description:
*/

struct HDataAccumulationStruct
{
    double sum_x;
    double sum_x2;
    double count;
    uint64_t start_index;
    uint64_t stop_index;
};

#endif /* end of include guard: HDataAccumulationStruct */
