#include "HWhiteNoiseSignal.hh"

#include <limits>

namespace hose
{

HWhiteNoiseSignal::HWhiteNoiseSignal():
    HSimulatedAnalogSignalSampleGenerator(),
    fIsPeriodicSignal(false),
    fPeriod(std::numeric_limits<double>::infinity()),
    fSeed(0),
    fGenerator(nullptr),
    fDistribution(nullptr)
{
    std::random_device rd;  //Will be used to obtain a seed for the random number engine
    std::mt19937 gen(rd()); //Standard mersenne_twister_engine seeded with rd()

    fDistribution = new std::uniform_real_distribution<double>(-1.0, 1.0);

};

HWhiteNoiseSignal::~HWhiteNoiseSignal(){};

void 
HWhiteNoiseSignal::HWhiteNoiseSignal::SetTimePeriod(double /*period*/){ fPeriod =  std::numeric_limits<double>::infinity();};

bool 
HWhiteNoiseSignal::IsPeriodic() const {return false;}

void 
HWhiteNoiseSignal::Initialize()
{
    std::mt19937* fGenerator = new std::mt19937(fSeed);
}

bool 
HWhiteNoiseSignal::GetSample(const double& sample_time, double& sample) const
{
    //to get a new sample call:
    //fDistribution(*fGenerator)
}

double 
HWhiteNoiseSignal::GenerateSample(const double& sample_time) const
{

}

}