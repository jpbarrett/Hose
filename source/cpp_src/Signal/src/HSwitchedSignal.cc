#include "HSwitchedSignal.hh"

namespace hose
{


HSwitchedSignal::HSwitchedSignal(): 
    HSimulatedAnalogSignalSampleGenerator(),
    fSwitchingFrequency(0),
    fSwitchingPeriod(0),
    fSignalGenerator(nullptr)
{};


HSwitchedSignal::~HSwitchedSignal(){};

//underlying signal type that is to be masked by the switch
void 
HSwitchedSignal::SetSignalGenerator(HSimulatedAnalogSignalSampleGenerator* sig_gen)
{
    fSignalGenerator = sig_gen;
}

void 
HSwitchedSignal::SetSwitchingFrequency(double frequency)
{
    fSwitchingFrequency = frequency;
    fSwitchingPeriod = 1.0/fSwitchingFrequency;
}

bool HSwitchedSignal::GenerateSample(const double& sample_time, double& sample) const
{
    bool retval = true;
    sample = 0.0; //mask signal to zero
    //determine if the sample time is in an on-period
    //this is easy for 50 duty cycle
    unsigned int n_half_periods = std::floor(sample_time/(fSwitchingPeriod/2.0));
    if(n_half_periods%2 == 0)
    {
        retval = fSignalGenerator->GetSample(sample_time, sample);
    }
    return retval;
}

}