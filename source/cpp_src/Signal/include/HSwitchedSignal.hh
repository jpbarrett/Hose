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

        //underlying signal type that is to be masked by the switch
        void SetSignalGenerator(HSimulatedAnalogSignalSampleGenerator* sig_gen);

        virtual void SetSwitchingFrequency(double frequency); 
        double GetSwitchingFrequency() const {return fSwitchingFrequency;};

        //tells the sample generation implementation what the expected the sampling frequency is
        virtual void SetSamplingFrequency(double max_freq) override;
        virtual double GetSamplingFrequency() const override;

        //set valid time range, or period of the signalatedAnalogSignalSampleGenerator* fSignalGenerator;
        //signal generation always starts at t=0, period is forced to be positive
        virtual void SetTimePeriod(double period) override;
        virtual double GetTimePeriod() const override;

        //allows the signal to repeat indefinitely if true
        //if false, the signal is time limited, and zero outside of the time limits
        virtual void SetPeriodic(bool is_periodic) override;
        virtual void SetPeriodicTrue() override;
        virtual void SetPeriodicFalse() override;
        virtual bool IsPeriodic() const override;

        //implementation specific
        virtual void Initialize() override;

        virtual bool GetSample(const double& sample_time, double& sample) const override;

    protected:

        //not used
        virtual double GenerateSample(const double& /*sample_time*/) const {return 0;};

        //currently this signal simulation assumes 50% duty cycle and that signal is switch on at start
        //TODO implement duty-cycle factor, and phase (on/off) at start

        double fSwitchingFrequency;
        double fSwitchingPeriod;
        HSimulatedAnalogSignalSampleGenerator* fSignalGenerator;

};


}

#endif /* end of include guard: HSwitchedSignal_HH__ */
