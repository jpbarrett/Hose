#include "HPowerLawNoiseSignal.hh"

#include <functional>

namespace hose
{



HPowerLawNoiseSignal::HPowerLawNoiseSignal():
    HSimulatedAnalogSignalSampleGenerator(),
    fWrapper(),
    fSeed(0)
{
    fFFTCalculator = new FFT_TYPE();
};

HPowerLawNoiseSignal::~HPowerLawNoiseSignal()
{
    delete fFFTCalculator;
};

void 
HPowerLawNoiseSignal::SetRandomSeed(unsigned int seed){fSeed = seed;}

void 
HPowerLawNoiseSignal::SetPowerLawExponent(double alpha)
{
    fAlpha = alpha;
}

void 
HPowerLawNoiseSignal::Initialize()
{
    //determine number of random samples and generate them
    fSamplePeriod = 1.0/(this->fSamplingFrequency);
    fNSamples = fPeriod/fSamplePeriod;
    fDim[0] = fNSamples;
    
    //construct a Gaussian random number generator
    auto real_rand = std::bind(std::normal_distribution< double >(0.0,1.0), std::mt19937(fSeed));

    //assign frequency samples and weight according to power spectrum,
    //we assume we want real time-series output, so X[-k] = X[N-k] = X*[k]
    //and we only need unique complex samples up to N/2
    fSamples.clear();
    fSamples.resize(fNSamples);
    double rs, is, omega, weight;
    for(unsigned int i=0; i<fNSamples/2; i++)
    {
        //frequency weight (we are bandwidth limited to fSamplingFrequency/2.0)
        omega = i*(fSamplingFrequency/fNSamples);
        weight = std::pow( 1.0/omega, fAlpha/2.0);

        rs = weight*real_rand();
        is = weight*real_rand();

        fSamples[i] = std::complex< double > (rs,is);
        fSamples[fNSamples - i] = std::complex< double >(rs, -is);
    }

    //number of samples is odd, so we need to ensure X[N/2+1] is real
    if(fNSamples%2 == 1)
    {
        unsigned int i = fNSamples/2+1;
        omega = i*(fSamplingFrequency/fNSamples);
        weight = std::pow( 1.0/omega, fAlpha/2.0);
        rs = weight*real_rand();
        fSamples[i] = std::complex< double > (rs,0);
    }

    //wrap the array (needed for FFT interface
    fWrapper.SetData(&(fSamples[0]));
    fWrapper.SetArrayDimensions(fDim);

    //obtain time series by backward FFT
    fFFTCalculator->SetBackward();
    fFFTCalculator->SetInput(&fWrapper);
    fFFTCalculator->SetOutput(&fWrapper);
    fFFTCalculator->Initialize();
    fFFTCalculator->ExecuteOperation();
}


double 
HPowerLawNoiseSignal::GenerateSample(const double& sample_time) const
{
    //just need to figure out where in the array we want to pull from
    unsigned int index = sample_time/fSamplePeriod;

    //dead simple, sample and hold
    return std::real<double>( (fWrapper.GetData())[index] );

    //TODO FIXME upgrade this so we can use linear/cubic interpolation (directly in time domain) between samples.
    //Interpolating in the time domain may introduce higher-frequency components which cause aliasing 
    //but since we don't expect a real world noise signal to be bandlimited this is not a problem at the moment 
    //If the system to be simulated has a band-limiting filter, then this needs to be dealt with separately.
}



} //end of namespace
