#ifndef HWhiteNoiseSignal_HH__
#define HWhiteNoiseSignal_HH__

#include "HSimulatedAnalogSignalSampleGenerator.hh"

#include <random>


namespace hose
{

class HWhiteNoiseSignal: public HSimulatedAnalogSignalSampleGenerator
{
    public:
        HWhiteNoiseSignal();
        ~HWhiteNoiseSignal();

        void SetRandomSeed(unsigned int seed){fSeed = seed;}

        //set valid time range, or period of the signal
        //signal generation always starts at t=0, period is forced to be positive
        virtual void SetTimePeriod(double period);

        //we modify this, signal is never periodic, but can continue for any amount of time
        virtual bool IsPeriodic() const;

        //implementation specific
        virtual void Initialize();

        virtual bool GetSample(const double& sample_time, double& sample) const;
        virtual double GenerateSample(const double& sample_time) const;

    protected:

        std::mt19937::result_type fSeed;
        std::uniform_real_distribution<double>* fDistribution;
};


}

#endif /* end of include guard: HWhiteNoiseSignal_HH__ */
