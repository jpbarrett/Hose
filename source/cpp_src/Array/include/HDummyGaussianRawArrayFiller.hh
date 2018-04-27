#ifndef HDummyGaussianRawArrayFiller_HH__
#define HDummyGaussianRawArrayFiller_HH__

/*
*File: HDummyGaussianRawArrayFiller.hh
*Class: HDummyGaussianRawArrayFiller
*Author: J. Barrett
*Email: barrettj@mit.edu
*Date:
*Description:
*/

#include <random>
#include <type_traits>

namespace hose
{

template< typename XSampleType >
struct HDummyGaussianRawArrayFiller
{
    void Fill(XSampleType* array, XSampleType mean, XSampleType sigma, size_t len)
    {
        //random number generator
        std::random_device r_dev;
        std::mt19937 mt_generator( r_dev() );
        std::normal_distribution< double > r_dist(mean, sigma);

        for(size_t i=0; i<len; i++)
        {
            array[i] = r_dist( mt_generator );
        }
    }
};


}

#endif /* end of include guard: HDummyGaussianRawArrayFiller */
