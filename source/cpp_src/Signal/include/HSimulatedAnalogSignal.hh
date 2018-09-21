#ifndef HSimulatedAnalogSignal_HH__
#define HSimulatedAnalogSignal_HH__

#include <cmath>

namespace hose
{

/*
*
*@file HSimulatedAnalogSignal.hh
*@class HSimulatedAnalogSignal
*@brief abstract class template for an analog signal
* that is to be sampled by a digitizer at or below some frequency
* over a limited time range
*@details
*/

class HSimulatedAnalogSignal: 
{
    public:

        HSimulatedAnalogSignal():fIsPeriodicSignal(false),fPeriod(0),fNyquistFrequency(0){};
        virtual ~HSimulatedAnalogSignal(){};

        //tells the sample generation implementation what the expected the nyquist frequency is
        //this is important for bandwidth limited signals
        //TODO: Possibly move this to sub-classes?
        virtual void SetNyquistFrequency(double max_freq) {fNyquistFrequency = std::fabs(max_freq);};
        double GetNyquistFrequency() const {return fNyquistFrequency;};

        //set valid time range, or period of the signal
        //signal generation always starts at t=0, period is forced to be positive
        virtual void SetTimePeriod(double period) {fPeriod = std::fabs(period);};
        double GetTimePeriod() const {return fPeriod;}; 

        //allows the signal to repeat indefinitely if true
        //if false, the signal is time limited, and zero outside of the time limits
        void SetPeriodic(bool is_periodic) {fIsPeriodicSignal = is_periodic;};
        void SetPeriodicTrue() {fIsPeriodicSignal = true;};
        void SetPeriodicFalse() {fIsPeriodicSignal = false;};
        bool IsPeriodic() const {return fIsPeriodicSignal;};

        //implementation specific
        virtual void Initialize(){;};
        
        virtual bool GetSample(const double& sample_time, double& sample) const
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

    protected:

        virtual double GenerateSample(double& sample_time) const = 0;

        //data
        bool fIsPeriodicSignal;
        double fNyquistFrequency;
        double fPeriod;

};

}


#endif /* HSimulatedAnalogSignal_H__ */
