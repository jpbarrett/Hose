#ifndef HGaussianWhiteNoiseSignal_HH__
#define HGaussianWhiteNoiseSignal_HH__

#include "HSimulatedAnalogSignalSampleGenerator.hh"

#include <random>


namespace hose
{

class HGaussianWhiteNoiseSignal: public HSimulatedAnalogSignalSampleGenerator
{
    public:
        HGaussianWhiteNoiseSignal();
        ~HGaussianWhiteNoiseSignal();

        void SetRandomSeed(unsigned int seed){fSeed = std::mt19937::result_type(seed);}

        //not used, no period
        virtual void SetTimePeriod(double /*period*/) override;

        //override this, signal is never periodic, but can continue for any amount of time
        virtual bool IsPeriodic() const override;

        //implementation specific
        virtual void Initialize();

    protected:

        virtual double GenerateSample(const double& sample_time) const override;

        std::mt19937::result_type fSeed;
        std::mt19937* fGenerator;
        std::normal_distribution<double>* fDistribution;
};


}

#endif /* end of include guard: HGaussianWhiteNoiseSignal_HH__ */
