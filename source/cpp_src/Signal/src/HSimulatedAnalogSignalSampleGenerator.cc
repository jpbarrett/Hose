#include "HSimulatedAnalogSignalSampleGenerator.hh"

namespace hose
{

HSimulatedAnalogSignalSampleGenerator::HSimulatedAnalogSignalSampleGenerator():
    fSamplingFrequency(0)
{};

HSimulatedAnalogSignalSampleGenerator::~HSimulatedAnalogSignalSampleGenerator(){};

void 
HSimulatedAnalogSignalSampleGenerator::SetSamplingFrequency(double max_freq)
{
    fSamplingFrequency = std::fabs(max_freq);
};

double
HSimulatedAnalogSignalSampleGenerator::GetSamplingFrequency() const 
{
    return fSamplingFrequency;
};


}//end of namespace

