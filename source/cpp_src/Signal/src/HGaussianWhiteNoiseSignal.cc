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
    fPeriod = std::numeric_limits<double>::infinity();
    fDistribution = new std::normal_distribution<double>(0.0, 1.0);
};

HGaussianWhiteNoiseSignal::~HGaussianWhiteNoiseSignal()
{
    delete fGenerator;
    delete fDistribution;
};

void 
HGaussianWhiteNoiseSignal::HGaussianWhiteNoiseSignal::SetTimePeriod(double /*period*/){ fPeriod =  std::numeric_limits<double>::infinity();};

bool 
HGaussianWhiteNoiseSignal::IsPeriodic() const {return false;}

void 
HGaussianWhiteNoiseSignal::Initialize()
{
    if(fGenerator == nullptr)
    {
        fGenerator = new std::mt19937(fSeed);
    }
}

double 
HGaussianWhiteNoiseSignal::GenerateSample(const double& /*sample_time*/) const
{
    return (*fDistribution)( *fGenerator );
}

}//end namespace