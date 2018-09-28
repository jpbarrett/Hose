#include <vector>
#include <string>
#include <iostream>
#include <stdint.h>
#include <getopt.h>

#include "TCanvas.h"
#include "TApplication.h"
#include "TStyle.h"
#include "TColor.h"
#include "TGraph.h"
#include "TH1D.h"

#include "HPowerLawNoiseSignal.hh"
#include "HGaussianWhiteNoiseSignal.hh"
#include "HSwitchedSignal.hh"
#include "HSummedSignal.hh"
#include "HSimpleAnalogToDigitalConverter.hh"

using namespace hose;

int main(int argc, char** argv)
{

    std::string usage =
    "\n"
    "Usage: TestSummedNoiseGenerator <options> \n"
    "\n"
    "Generate one second of noise samples with the specified power distribution.\n"
    "\tOptions:\n"
    "\t -h, --help               (shows this message and exits)\n"
    "\t -g, --gaussian           (generate gaussian white noise)\n"
    "\t -f, --frequency          (frequency (sampling))\n"
    "\t -r, --ratio              (ratio of switched signal amplitude to constant signal amplitude)\n"
    "\t -s, --switch             (switching frequency, noise signal with be turned on/off with 50% duty-cycle)\n"
    ;

    static struct option longOptions[] =
    {
        {"help", no_argument, 0, 'h'},
        {"gaussian", no_argument, 0, 'g'},
        {"ratio", required_argument, 0, 'r'},
        {"frequency", required_argument, 0, 'f'},
        {"switch", required_argument, 0, 's'}
    };

    static const char *optString = "hgr:f:s:";

    bool do_gaussian = false;
    double sampling_frequency = 1e5;
    bool do_switching = false;
    double switching_frequency = 10;
    double ratio = 1.0;

    while(1)
    {
        char optId = getopt_long(argc, argv, optString, longOptions, NULL);
        if(optId == -1) break;
        switch(optId)
        {
            case('h'): // help
            std::cout<<usage<<std::endl;
            return 0;
            case('g'):
            do_gaussian = true;
            break;
            case('f'):
            sampling_frequency = atof(optarg);
            break;
            case('r'):
            ratio = atof(optarg);
            break;
            case('s'):
            do_switching = true;
            switching_frequency = atof(optarg);
            break;
            default:
            std::cout<<usage<<std::endl;
            return 1;
        }
    }

    double sample_frequency = sampling_frequency;
    double sample_time = 1.0; //1 second
    double delta = sample_time/sample_frequency;

    //total number of samples
    size_t num_samples = sample_time*sample_frequency;
    size_t dim[1];
    dim[0] = num_samples;

    std::vector<double> noise_samples;
    noise_samples.resize(num_samples,0);

    std::vector< std::complex<double> > noise_xform_in; noise_xform_in.resize(num_samples);
    std::vector< std::complex<double> > noise_xform_out; noise_xform_out.resize(num_samples);
    bool testval;

    //generate gaussian white noise
    HGaussianWhiteNoiseSignal* gnoise1 = new HGaussianWhiteNoiseSignal();
    gnoise1->SetRandomSeed(123);
    gnoise1->Initialize();

    HGaussianWhiteNoiseSignal* gnoise2 = new HGaussianWhiteNoiseSignal();
    gnoise2->SetRandomSeed(456);
    gnoise2->Initialize();

    HSwitchedSignal* snoise = new HSwitchedSignal();
    snoise->SetSwitchingFrequency(switching_frequency);
    snoise->SetSignalGenerator(gnoise2);

    HSummedSignal* summed_noise = new HSummedSignal();
    summed_noise->AddSignalGenerator(gnoise1, 1.0);
    summed_noise->AddSignalGenerator(snoise, ratio);

    HSimpleAnalogToDigitalConverter<double, int16_t, 14> simple14bitADC(-4,4);

    double samp;
    for(size_t i=0; i<num_samples; i++)
    {
        testval = summed_noise->GetSample(i*delta, samp);
        (void) testval;
        double conv = simple14bitADC.Convert(samp);
        noise_samples[i] = conv;

        std::cout<<"sample, quantized_sample = "<<samp<<", "<<conv<<std::endl;
        noise_xform_in[i] = std::complex<double>(samp, 0.0);
    }

    delete summed_noise;
    delete snoise;
    delete gnoise2;
    delete gnoise1;

    //now we are going to plot the time series, and histogram the noise
    
    //ROOT stuff for plots
    TApplication* App = new TApplication("TestNoiseGenerator",&argc,argv);
    TStyle* myStyle = new TStyle("Plain", "Plain");
    myStyle->SetCanvasBorderMode(0);
    myStyle->SetPadBorderMode(0);
    myStyle->SetPadColor(0);
    myStyle->SetCanvasColor(0);
    myStyle->SetTitleColor(1);
    myStyle->SetPalette(1,0);   // nice color scale for z-axis
    myStyle->SetCanvasBorderMode(0); // gets rid of the stupid raised edge around the canvas
    myStyle->SetTitleFillColor(0); //turns the default dove-grey background to white
    myStyle->SetCanvasColor(0);
    myStyle->SetPadColor(0);
    myStyle->SetTitleFillColor(0);
    myStyle->SetStatColor(0); //this one may not work
    const int NRGBs = 5;
    const int NCont = 48;
    double stops[NRGBs] = { 0.00, 0.34, 0.61, 0.84, 1.00 };
    double red[NRGBs]   = { 0.00, 0.00, 0.87, 1.00, 0.51 };
    double green[NRGBs] = { 0.00, 0.81, 1.00, 0.20, 0.00 };
    double blue[NRGBs]  = { 0.51, 1.00, 0.12, 0.00, 0.00 };
    TColor::CreateGradientColorTable(NRGBs, stops, red, green, blue, NCont);
    myStyle->SetNumberContours(NCont);
    myStyle->cd();
    
    //plotting objects
    std::string name("canvas");
    TCanvas* c = new TCanvas(name.c_str(),name.c_str(), 50, 50, 950, 850);
    c->cd();
    c->SetFillColor(0);
    c->SetRightMargin(0.2);
    c->Divide(1,3);
    c->cd(1);

    TGraph* time_series = new TGraph();

    double min = noise_samples[0];
    double max = noise_samples[0];
    for(size_t i=0; i<noise_samples.size(); i++)
    {
        noise_xform_in[i] = std::complex<double>(noise_samples[i], 0.0);
        if(noise_samples[i]<min){min = noise_samples[i];};
        if(noise_samples[i]>max){max = noise_samples[i];};
        time_series->SetPoint(time_series->GetN(), i*delta, noise_samples[i]);
    }

    time_series->Draw("ALP");
    c->Update();


    //histogram the values
    TH1D* histo = new TH1D("histogram", "histogram", 100, min, max);
    for(size_t i=0; i<noise_samples.size(); i++)
    {
        histo->Fill(noise_samples[i]);
    }
    c->cd(2);

    histo->Draw("");
    c->Update();

    HArrayWrapper< std::complex< double >, 1 > wrapperIn;
    HArrayWrapper< std::complex< double >, 1 > wrapperOut;

    //now we want to compute the spectral power density
    //wrap the array (needed for FFT interface
    wrapperIn.SetData(&(noise_xform_in[0]));
    wrapperIn.SetArrayDimensions(dim);

    wrapperOut.SetData(&(noise_xform_out[0]));
    wrapperOut.SetArrayDimensions(dim);

    //obtain time series by backward FFT
    FFT_TYPE* fftCalculator = new FFT_TYPE();
    fftCalculator->SetForward();
    fftCalculator->SetInput(&wrapperIn);
    fftCalculator->SetOutput(&wrapperOut);
    fftCalculator->Initialize();
    fftCalculator->ExecuteOperation();

    //normalize
    double norm = 1.0/std::sqrt((double)dim[0]);

    TGraph* spectrum = new TGraph();

    for(unsigned int i=1; i<num_samples/2; i++)
    {
        double omega = i*(sample_frequency/num_samples);
        spectrum->SetPoint(spectrum->GetN(), std::log10(omega), 10.0*std::log10( norm*std::real( noise_xform_out[i]*std::conj(noise_xform_out[i]) ) ) );
    }
    
    c->cd(3);
    spectrum->Draw("ALP");
    c->Update();

    App->Run();

    delete fftCalculator;

}
