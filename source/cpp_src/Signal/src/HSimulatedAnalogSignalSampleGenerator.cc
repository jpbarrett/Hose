#include "HSimulatedAnalogSignalSampleGenerator.hh"

namespace hose
{

HSimulatedAnalogSignalSampleGenerator::HSimulatedAnalogSignalSampleGenerator():
    fIsPeriodicSignal(false),
    fSamplingFrequency(0),
    fPeriod(0)
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


void 
HSimulatedAnalogSignalSampleGenerator::SetTimePeriod(double period) 
{
    fPeriod = std::fabs(period);
};

double 
HSimulatedAnalogSignalSampleGenerator::GetTimePeriod() const
{
    return fPeriod;
}; 

//allows the signal to repeat indefinitely if true
//if false, the signal is time limited, and zero outside of the time limits
void 
HSimulatedAnalogSignalSampleGenerator::SetPeriodic(bool is_periodic)
{
    fIsPeriodicSignal = is_periodic;
};

void 
HSimulatedAnalogSignalSampleGenerator::SetPeriodicTrue() 
{
    fIsPeriodicSignal = true;
};

void 
HSimulatedAnalogSignalSampleGenerator::SetPeriodicFalse() 
{
    fIsPeriodicSignal = false;
};

bool 
HSimulatedAnalogSignalSampleGenerator::IsPeriodic() const 
{
    return fIsPeriodicSignal;
};

bool 
HSimulatedAnalogSignalSampleGenerator::GetSample(const double& sample_time, double& sample) const
{
    if(fIsPeriodicSignal)
    {
        //get time into sample range and compute
        double trimmed_time = sample_time - std::floor(sample_time/fPeriod)*fPeriod;
        sample = GenerateSample(trimmed_time);
        return true;
    }
    else
    {
        if( sample_time <= fPeriod && 0 <= sample_time )
        {
            //within the sample range, ok
            sample = GenerateSample(sample_time);
            return true;
        }
        else
        {
            //not a periodic signal, bail out
            sample = 0;
            return false;
        }
    }
}


}//end of namespace

