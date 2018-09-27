#ifndef HSimpleAnalogToDigitalConverter_HH__
#define HSimpleAnalogToDigitalConverter_HH__

#include <cmath>
#include <limits>

#include <iostream>

namespace hose{

/*
*
*@file HSimpleAnalogToDigitalConverter
*@class HSimpleAnalogToDigitalConverter
*@brief bare-bones class to take analog (double or float precision) samples
* and convert them to an integer output type by applying a linear set of thresholds.
* The number of bits determines the number of thresholds, while the thresholds
* themselves are determined by the lower/upper limits
*@details
*
*/

//TODO ensure that 8*sizeof(XOutputDataType) >= XNBits at compile time

template< typename XInputFloatType, typename XOutputDataType, size_t XNBits >
class HSimpleAnalogToDigitalConverter
{
    public:

        HSimpleAnalogToDigitalConverter():
            fNLevels(HArrayMath::PowerOfTwo< XNBits >::value)
        {
            if(std::numeric_limits<XOutputDataType>::is_signed)
            {
                //working with a signed output type (need 1 bit for sign info)
                fLowerOutputLimit = -1*HArrayMath::PowerOfTwo< XNBits-1 >::value;
                fUpperOutputLimit = HArrayMath::PowerOfTwo< XNBits-1 >::value;
            }
            else
            {
                //unsigned output
                fLowerOutputLimit = 0;
                fUpperOutputLimit = HArrayMath::PowerOfTwo< XNBits >::value;
            }
            //default input limits
            SetLimits( -1, 1 );


            std::cout<<"n level = "<<fNLevels<<std::endl;
            std::cout<<"low out = "<<fLowerOutputLimit<<std::endl;
            std::cout<<"upp out = "<<fUpperOutputLimit<<std::endl;
            std::cout<<"level width = "<<fInputLevelWidth<<std::endl;
        }

        HSimpleAnalogToDigitalConverter(XInputFloatType lower_limit, XInputFloatType upper_limit):
            fNLevels(HArrayMath::PowerOfTwo< XNBits >::value)
        {
            if(std::numeric_limits<XOutputDataType>::is_signed)
            {
                //working with a signed output type (need 1 bit for sign info)
                fLowerOutputLimit = -1*HArrayMath::PowerOfTwo< XNBits-1 >::value;
                fUpperOutputLimit = HArrayMath::PowerOfTwo< XNBits-1 >::value;
            }
            else
            {
                //unsigned output
                fLowerOutputLimit = 0;
                fUpperOutputLimit = HArrayMath::PowerOfTwo< XNBits >::value;
            }
            SetLimits(lower_limit, upper_limit);

            std::cout<<"n level = "<<fNLevels<<std::endl;
            std::cout<<"low out = "<<fLowerOutputLimit<<std::endl;
            std::cout<<"upp out = "<<fUpperOutputLimit<<std::endl;
            std::cout<<"level width = "<<fInputLevelWidth<<std::endl;
        }

        ~HSimpleAnalogToDigitalConverter(){};

        void SetLimits(XInputFloatType low, XInputFloatType up)
        {
            if(low <= up)
            {
                fLowerInputLimit = low;
                fUpperInputLimit = up;
            }
            else
            {
                fLowerInputLimit = up;
                fUpperInputLimit = low;
            }
            fInputRange = std::fabs(fUpperInputLimit - fLowerInputLimit);
            fInputLevelWidth = fInputRange/fNLevels;
        };

        XOutputDataType Convert(XInputFloatType sample)
        {
            XInputFloatType normed_sample = (InputClip(sample) - fLowerInputLimit); //normed sample is now in range [0, fInputRange]
            XOutputDataType quant_level = std::floor(normed_sample/fInputLevelWidth); //quantization level in range [0, fNLevels]
            return quant_level + fLowerOutputLimit; 
        }

    protected:

        XInputFloatType InputClip(XInputFloatType sample)
        {
            if(sample <= fLowerInputLimit){return fLowerInputLimit;};
            if(sample >= fUpperInputLimit){return fUpperInputLimit;};
            return sample;
        }

        const unsigned int fNLevels; //number of quantization levels

        //clipping leves on input
        XInputFloatType fLowerInputLimit;
        XInputFloatType fUpperInputLimit;
        //total input range
        XInputFloatType fInputRange;
        //width of input quant level
        XInputFloatType fInputLevelWidth;

        XOutputDataType fLowerOutputLimit;
        XOutputDataType fUpperOutputLimit;

        

};

}

#endif /* end of include guard: HSimpleAnalogToDigitalConverter_HH__ */
