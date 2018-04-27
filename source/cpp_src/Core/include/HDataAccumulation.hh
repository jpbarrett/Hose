#ifndef HDataAccumulation_HH__
#define HDataAccumulation_HH__

/*
*File: HDataAccumulation.hh
*Class: HDataAccumulation
*Author: J. Barrett
*Email: barrettj@mit.edu
*Date:
*Description:
*/

struct HDataAccumulation
{
    double sum_x;
    double sum_x2;
    double count;
    uint64_t start_index;
    uint64_t stop_index;
};

#endif /* end of include guard: HDataAccumulation */
