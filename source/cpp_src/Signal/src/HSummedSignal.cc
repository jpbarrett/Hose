#include "HSummedSignal.hh"

namespace hose
{


void 
HSummedSignal::AddSignalGenerator(HSimulatedAnalogSignalSampleGenerator* sig_gen, double scale_factor)
{
    if(sig_gen != nullptr)
    {
        fSignals.push_back( std::pair< HSimulatedAnalogSignalSampleGenerator*, double>(sig_gen, scale_factor) );
    }
}

bool 
HSummedSignal::GenerateSample(const double& sample_time, double& sample) const
{
    sample = 0.0;
    for(auto it = fSignals.begin(); it !=fSignals.end(); it++)
    {
        double tmp_sample = 0;
        bool success = it->first->GetSample(sample_time, tmp_sample);
        if(success)
        {
            sample += (it->second)*tmp_sample;
        }
        else
        {   
            return false;
        }
    }
    return true;
}


}