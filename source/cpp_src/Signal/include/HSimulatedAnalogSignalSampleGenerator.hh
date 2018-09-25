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
* over a limited time range, it is expected that the signal is bandwidth limited
* and in the case that it is not time limited, that it is periodic
*@details
*/

class HSimulatedAnalogSignalSampleGenerator: public HSimulatedAnalogSignalSampleGenerator
{
    public:
        HSimulatedAnalogSignalSampleGenerator();
        virtual ~HSimulatedAnalogSignalSampleGenerator();

        //tells the sample generation implementation what the expected the sampling frequency is
        virtual void SetSamplingFrequency(double max_freq);
        virtual double GetSamplingFrequency() const;

        //set valid time range, or period of the signal
        //signal generation always starts at t=0, period is forced to be positive
        virtual void SetTimePeriod(double period);
        virtual double GetTimePeriod() const;

        //allows the signal to repeat indefinitely if true
        //if false, the signal is time limited, and zero outside of the time limits
        virtual void SetPeriodic(bool is_periodic);
        virtual void SetPeriodicTrue();
        virtual void SetPeriodicFalse();
        virtual bool IsPeriodic() const;

        //implementation specific
        virtual void Initialize(){;};

        virtual bool GetSample(const double& sample_time, double& sample) const;

    protected:

        virtual double GenerateSample(const double& /*sample_time*/) const = 0;

        //data
        bool fIsPeriodicSignal;
        double fSamplingFrequency;
        double fPeriod;
};

}


#endif /* HSimulatedAnalogSignalSampleGenerator_H__ */
