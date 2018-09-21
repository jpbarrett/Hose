#ifndef HColoredNoiseSignal_HH__
#define HColoredNoiseSignal_HH__

#include <vector>
#include <complex>
#include <random>

#include "HMessaging.hh"
#include "HSimulatedAnalogSignal.hh"
#include "HMultidimensionalFastFourierTransform.hh"
#ifdef H_USE_FFTW
    #include "HMultidimensionalFastFourierTransformFFTW.hh"
    #define FFT_TYPE HMultidimensionalFastFourierTransformFFTW<1>
#else
    #define FFT_TYPE HMultidimensionalFastFourierTransform<1>
#endif

/*
*File: HColoredNoiseSignal.hh
*Class: HColoredNoiseSignal
*Author: J. Barrett
*Email: barrettj@mit.edu
*Date:
*Description: Generates a time-series of colored noise samples (1/f^{\alpha} noise), with 0 <= alpha <= 2
*This method is somewhat memory intensive, with memory use which grows linearly with
*the time period and the sampling frequency (bandwidth). May need to make this more efficient, signal
*is not guaranteed to be bandlimited.
*Algorithm according to the paper:

@article{timmer1995generating,
  title={On generating power law noise.},
  author={Timmer, J and K{\"o}nig, M},
  journal={Astronomy and Astrophysics},
  volume={300},
  pages={707},
  year={1995}
}

*/

template< XFloatType = double >
class HColoredNoiseSignal: public HSimulatedAnalogSignal< XFloatType >
{
    public:
        HColoredNoiseSignal():HSimulatedAnalogSignal< XFloatType >():
            fWrapper()
        {
            fFFTCalculator = new FFT_TYPE();
        };

        virtual ~HColoredNoiseSignal()
        {
            delete fFFTCalculator;
        };

        void SetRandomSeed(unsigned int seed){fSeed = seed;}
        void SetPowerLawExponent(XFloatType alpha)
        {
            if(alpha < 0.0){fAlpha = 0.0;}
            else if(alpha > 2.0){fAlpha = 2.0;}
            else{fAlpha = alpha;}
        }
        
        virtual void Initialize()
        {
            //determine number of random samples and generate them
            fSamplePeriod = 1.0/(this->fNyquistFrequency);
            fNSamples = fPeriod/fSamplePeriod;
            fDim[0] = fNSamples;
            
            //construct a Gaussian random number generator
            auto real_rand = std::bind(std::normal_distribution< XFloatType >(0.0,1.0), std::mt19937(fSeed));

            //assign frequency samples and weight according to power spectrum,
            //we assume we want real time-series output, so X[-k] = X[N-k] = X*[k]
            //and we only need unique complex samples up to N/2
            fSamples.clear();
            fSamples.resize(fNSamples);
            XFloatType rs, is, omega, weight;
            for(unsigned int i=0; i<fNSamples/2; i++)
            {
                //frequency weight (we are bandwidth limited to fNyquistFrequency/2.0)
                omega = i*(this->fNyquistFrequency/fNSamples);
                weight = std::pow( 1.0/omega, fAlpha/2.0);

                rs = weight*real_rand();
                is = weight*real_rand();

                fSamples[i] = std::complex< XFloatType > (rs,is);
                fSamples[fNSamples - i] = std::complex< XFloatType >(rs, -is);
            }

            //number of samples is odd, so we need to ensure X[N/2+1] is real
            if(fNSamples%2 == 1)
            {
                unsigned int i = fNSamples/2+1;
                omega = i*(this->fNyquistFrequency/fNSamples);
                weight = std::pow( 1.0/omega, fAlpha/2.0);
                rs = weight*real_rand();
                fSamples[i] = std::complex< XFloatType > (rs,0);
            }

            //wrap the array (needed for FFT interface
            fWrapper->SetData(&(fSamples[0]));
            fWrapper->SetArrayDimensions(fDim);

            //obtain time series by backward FFT
            fFFTCalculator->SetBackward();
            fFFTCalculator->SetInput(&fWrapper);
            fFFTCalculator->SetOutput(&fWrapper);
            fFFTCalculator->Initialize();
            fFFTCalculator->ExecuteOperation();
        }

    protected:

        virtual XFloatType GenerateSample(XFloatType& sample_time) const
        {
            //just need to figure out where in the array we want to pull from
            unsigned int index = sample_time/fSamplePeriod;

            //dead simple, sample and hold
            return (fWrapper->GetData())[index];

            //TODO FIXME upgrade this so we can use linear/cubic interpolation (directly in time domain) between samples.
            //Interpolating in the time domain may introduce higher-frequency componehose which cause aliasing 
            //but since we don't expect a real world noise signal to be bandlimited this is not a problem at the moment 
            //If the system to be simulated has a band-limiting filter, then this needs to be dealt with separately.
        }

        //data
        unsigned int fNSamples;
        unsigned int fDim[1];
        std::mt19937::result_type fSeed;
        XFloatType fAlpha;
        XFloatType fSamplePeriod;
        std::vector< std::complex< XFloatType > > fSamples;
        HArrayWrapper< std::complex< XFloatType >, 1 > fWrapper;

        FFT_TYPE* fFFTCalculator;

};

#endif /* end of include guard: HColoredNoiseSignal */
