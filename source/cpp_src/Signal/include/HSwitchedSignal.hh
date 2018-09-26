#ifndef HSwitchedSignal_HH__
#define HSwitchedSignal_HH__

#include "HSimulatedAnalogSignalSampleGenerator.hh"


namespace hose
{

class HSwitchedSignal: public HSimulatedAnalogSignalSampleGenerator
{
    public:
        HSwitchedSignal();
        ~HSwitchedSignal();

        //underlying signal type that is to be masked by the switch, must be initialized beforehand
        void SetSignalGenerator(HSimulatedAnalogSignalSampleGenerator* sig_gen);

        virtual void SetSwitchingFrequency(double frequency); 
        double GetSwitchingFrequency() const {return fSwitchingFrequency;};

    protected:

        virtual bool GenerateSample(const double& sample_time, double& sample) const override;

        //currently this signal simulation assumes 50% duty cycle and that signal is switch on at start
        //TODO implement duty-cycle factor, and phase (on/off) at start

        double fSwitchingFrequency;
        double fSwitchingPeriod;
        HSimulatedAnalogSignalSampleGenerator* fSignalGenerator;

};


}

#endif /* end of include guard: HSwitchedSignal_HH__ */
