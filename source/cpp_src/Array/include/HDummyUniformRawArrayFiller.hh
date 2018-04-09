#ifndef HDummyUniformRawArrayFiller_HH__
#define HDummyUniformRawArrayFiller_HH__

/*
*File: HDummyUniformRawArrayFiller.hh
*Class: HDummyUniformRawArrayFiller
*Author: J. Barrett
*Email: barrettj@mit.edu
*Date:
*Description:
*/

#include <random>
#include <type_traits>

namespace hose
{

template< typename XSampleType, typename XEnableType = void >
struct HDummyUniformRawArrayFiller
{
    void Fill(XSampleType* /*array*/, XSampleType /*lower_limit*/, XSampleType /*upper_limit*/, size_t /*len*/){}; //unspecialized default is to do nothing
};

//partial specialization for floating point types
template< typename XSampleType >
struct HDummyUniformRawArrayFiller< XSampleType, typename std::enable_if< std::is_floating_point< XSampleType >::value >::type >
{
    void Fill(XSampleType* array, XSampleType lower_limit, XSampleType upper_limit, size_t len)
    {
        //random number generator
        std::random_device r_dev;
        std::mt19937 mt_generator( r_dev() );
        std::uniform_real_distribution< XSampleType > r_dist(lower_limit, upper_limit);

        for(size_t i=0; i<len; i++)
        {
            array[i] = r_dist( mt_generator );
        }
    }
};

//partial specialization for integral types
template< typename XSampleType >
struct HDummyUniformRawArrayFiller< XSampleType, typename std::enable_if< std::is_integral< XSampleType >::value >::type >
{
    void Fill(XSampleType* array, XSampleType lower_limit, XSampleType upper_limit, size_t len)
    {
        //random number generator
        std::random_device r_dev;
        std::mt19937 mt_generator( r_dev() );
        std::uniform_int_distribution< XSampleType > r_dist(lower_limit, upper_limit);

        for(size_t i=0; i<len; i++)
        {
            array[i] = r_dist( mt_generator );
        }
    }
};

}

#endif /* end of include guard: HDummyUniformRawArrayFiller */
