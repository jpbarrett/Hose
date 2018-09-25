#ifndef HPulseCalibrationSignal_HH__
#define HPulseCalibrationSignal_HH__

#include "HSimulatedAnalogSignalSampleGenerator.hh"

namespace hose
{

//simple pulse cal signal (rectangular pulse with specified width), 
//pulse is normalized: fPulseDuration*fPulseHeight = 1.0

class HPulseCalibrationSignal: public HSimulatedAnalogSignalSampleGenerator
{
    public:
        HPulseCalibrationSignal();
        ~HPulseCalibrationSignal();

        //this is the repetition frequency (same as tone spacing)
        void SetPulseFrequency(double pulse_freq);
        double GetPulseFrequency() const;

        //width of the rectangle pulse
        void SetPulseDuration(double pulse_duration) {fPulseDuration = pulse_duration; fPulseHeight = 1.0/fPulseDuration;}
        double GetPulseDuration() const {return fPulseDuration;};
        double GetPulseHeight() const {return fPulseHeight;};

        //not used, no period
        virtual void SetTimePeriod(double /*period*/) override;

        //override this, signal is always periodic
        virtual bool IsPeriodic() const override;

        //implementation specific
        virtual void Initialize();

        virtual bool GetSample(const double& sample_time, double& sample) const;

    protected:

        virtual double GenerateSample(const double& sample_time) const override;

        double fPulseRepetitionFrequency;
        double fPulseDuration;
        double fPulseHeight; 
};


}

#endif /* end of include guard: HPulseCalibrationSignal_HH__ */
