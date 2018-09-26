#ifndef HSimulatedAnalogSignalSampleGenerator_HH__
#define HSimulatedAnalogSignalSampleGenerator_HH__

#include <cmath>

namespace hose
{

/*
*
*@file HSimulatedAnalogSignalSampleGenerator.hh
*@class HSimulatedAnalogSignalSampleGenerator
*@brief abstract class template for an analog signal
* that is to be sampled by a digitizer at some frequency
* it is expected that the signal is bandwidth limited
*@details
*/

class HSimulatedAnalogSignalSampleGenerator
{
    public:
        HSimulatedAnalogSignalSampleGenerator(){};
        virtual ~HSimulatedAnalogSignalSampleGenerator(){};

        //tells the sample generation implementation what the expected the sampling frequency is
        virtual void SetSamplingFrequency(double max_freq){fSamplingFrequency = max_freq;};
        virtual double GetSamplingFrequency() const {return fSamplingFrequency;};

        //implementation specific
        virtual void Initialize(){;};

        bool GetSample(const double& sample_time, double& sample) const
        {
            return GenerateSample(sample_time, sample);
        }

    protected:

        virtual bool GenerateSample(const double& sample_time, double& sample) const = 0;

        //data
        double fSamplingFrequency;
};

}


#endif /* HSimulatedAnalogSignalSampleGenerator_H__ */
