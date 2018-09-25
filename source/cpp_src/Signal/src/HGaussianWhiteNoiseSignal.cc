#include "HGaussianWhiteNoiseSignal.hh"

#include <limits>

namespace hose
{

HGaussianWhiteNoiseSignal::HGaussianWhiteNoiseSignal():
    HSimulatedAnalogSignalSampleGenerator(),
    fSeed(0),
    fGenerator(nullptr),
    fDistribution(nullptr)
{
    fDistribution = new std::normal_distribution<double>(0.0, 1.0);
};

HGaussianWhiteNoiseSignal::~HGaussianWhiteNoiseSignal()
{
    delete fGenerator;
    delete fDistribution;
};

void 
HGaussianWhiteNoiseSignal::Initialize()
{
    if(fGenerator == nullptr)
    {
        fGenerator = new std::mt19937(fSeed);
    }
}

bool 
HGaussianWhiteNoiseSignal::GenerateSample(const double& /*sample_time*/, double& sample) const
{
    sample = (*fDistribution)( *fGenerator );
    return true;
}


}//end namespace