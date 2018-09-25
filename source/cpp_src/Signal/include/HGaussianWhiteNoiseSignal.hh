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

        //implementation specific
        virtual void Initialize();

    protected:

        virtual bool GenerateSample(const double& /*sample_time*/, double& sample) const override;

        std::mt19937::result_type fSeed;
        std::mt19937* fGenerator;
        std::normal_distribution<double>* fDistribution;
};


}

#endif /* end of include guard: HGaussianWhiteNoiseSignal_HH__ */
