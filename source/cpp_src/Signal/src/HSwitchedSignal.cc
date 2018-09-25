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


//tells the sample generation implementation what the expected the sampling frequency is
void 
HSwitchedSignal::SetSamplingFrequency(double max_freq)
{
    if(fSignalGenerator != nullptr)
    {
        fSignalGenerator->SetSamplingFrequency(max_freq);
    }
}


double 
HSwitchedSignal::GetSamplingFrequency() const
{
    if(fSignalGenerator != nullptr)
    {
        return fSignalGenerator->GetSamplingFrequency();
    }
    return 0;
}


//set valid time range, or period of the signal
//signal generation always starts at t=0, period is forced to be positive
void 
HSwitchedSignal::SetTimePeriod(double period)
{
    if(fSignalGenerator != nullptr)
    {
        fSignalGenerator->SetTimePeriod(period);
    }
}

double 
HSwitchedSignal::GetTimePeriod() const
{
    if(fSignalGenerator != nullptr)
    {
        return fSignalGenerator->GetTimePeriod();
    }
    return 0;
}

//allows the signal to repeat indefinitely if true
//if false, the signal is time limited, and zero outside of the time limits
void 
HSwitchedSignal::SetPeriodic(bool is_periodic)
{
    if(fSignalGenerator != nullptr)
    {
        return fSignalGenerator->SetPeriodic(is_periodic);
    }
}

void 
HSwitchedSignal::SetPeriodicTrue()
{
    if(fSignalGenerator != nullptr)
    {
        return fSignalGenerator->SetPeriodicTrue();
    }
}

void 
HSwitchedSignal::SetPeriodicFalse()
{
    if(fSignalGenerator != nullptr)
    {
        return fSignalGenerator->SetPeriodicFalse();
    }
}

bool 
HSwitchedSignal::IsPeriodic() const
{
    if(fSignalGenerator != nullptr)
    {
        return fSignalGenerator->IsPeriodic();
    }
    else
    {
        return false;
    }
}

//implementation specific
void HSwitchedSignal::Initialize()
{
    if(fSignalGenerator != nullptr)
    {
        fSignalGenerator->Initialize();
    }
}

bool 
HSwitchedSignal::GetSample(const double& sample_time, double& sample) const
{
    bool retval = true;
    sample = 0.0; //mask signal to zero
    //determine if the sample time is in an on-period
    //this is easy for 50 duty cycle
    unsigned int n_half_periods = std::floor(sample_time/(fSwitchingPeriod/2.0));
    if( n_half_periods%2 == 0)
    {
        std::cout<<sample_time<<std::endl;
        std::cout<<fSwitchingPeriod<<std::endl;
        retval = fSignalGenerator->GetSample(sample_time, sample);
    }
    return retval;
}

}