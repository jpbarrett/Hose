#include <vector>
#include <string>
#include <iostream>
#include <iomanip>
#include <stdint.h>
#include <getopt.h>
#include <ctime>

#include "TCanvas.h"
#include "TApplication.h"
#include "TStyle.h"
#include "TColor.h"
#include "TGraph.h"
#include "TMultiGraph.h"
#include "TH1D.h"


#include "HLinearBuffer.hh"

#include "HPeriodicPowerCalculator.hh"

#include "HPowerLawNoiseSignal.hh"
#include "HGaussianWhiteNoiseSignal.hh"
#include "HSwitchedSignal.hh"
#include "HSummedSignal.hh"
#include "HSimpleAnalogToDigitalConverter.hh"

using namespace hose;


#define SAMPLE_TYPE uint16_t

int main(int argc, char** argv)
{

    std::string usage =
    "\n"
    "Usage: TestPeriodicPowerCalculator <options> \n"
    "\n"
    "Generate one second of noise samples with the specified power distribution.\n"
    "\tOptions:\n"
    "\t -h, --help               (shows this message and exits)\n"
    "\t -g, --gaussian           (generate gaussian white noise)\n"
    "\t -f, --frequency          (frequency (sampling))\n"
    "\t -r, --ratio              (ratio of switched signal (voltage) amplitude to constant signal amplitude)\n"
    "\t -s, --switch             (switching frequency, noise signal with be turned on/off with 50% duty-cycle)\n"
    "\t -t, --time               (time period to simulation, default: 1 sec)\n"
    ;

    static struct option longOptions[] =
    {
        {"help", no_argument, 0, 'h'},
        {"gaussian", no_argument, 0, 'g'},
        {"ratio", required_argument, 0, 'r'},
        {"frequency", required_argument, 0, 'f'},
        {"switch", required_argument, 0, 's'},
        {"time", required_argument, 0, 't'}
    };

    static const char *optString = "hgr:f:s:t:";

    bool do_gaussian = false;
    double sampling_frequency = 1e5;
    bool do_switching = false;
    double switching_frequency = 10;
    double ratio = 1.0;
    double sample_time = 1.0; //1 second

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
            case('t'):
            sample_time = atof(optarg);
            break;
            default:
            std::cout<<usage<<std::endl;
            return 1;
        }
    }

    double sample_frequency = sampling_frequency;
    double delta = 1.0/sample_frequency;

    //total number of samples
    size_t num_samples = sample_time*sample_frequency;
    size_t dim[1];
    dim[0] = num_samples;

    std::vector<double> noise_samples;
    std::vector<SAMPLE_TYPE> digi_samples;
    digi_samples.resize(num_samples,0);
    noise_samples.resize(num_samples,0);

    std::vector< std::complex<double> > noise_xform_in; noise_xform_in.resize(num_samples);
    std::vector< std::complex<double> > noise_xform_out; noise_xform_out.resize(num_samples);
    bool testval;

    //generate gaussian white noise
    HGaussianWhiteNoiseSignal* gnoise1 = new HGaussianWhiteNoiseSignal();
    gnoise1->SetRandomSeed(time(NULL));
    gnoise1->Initialize();

    HGaussianWhiteNoiseSignal* gnoise2 = new HGaussianWhiteNoiseSignal();
    gnoise2->SetRandomSeed(time(NULL)+1);
    gnoise2->Initialize();

    HSwitchedSignal* snoise = new HSwitchedSignal();
    snoise->SetSwitchingFrequency(switching_frequency);
    snoise->SetSignalGenerator(gnoise2);

    HSummedSignal* summed_noise = new HSummedSignal();
    summed_noise->AddSignalGenerator(gnoise1, 1.0);
    summed_noise->AddSignalGenerator(snoise, ratio);

    HSimpleAnalogToDigitalConverter<double, SAMPLE_TYPE, 14> simple14bitADC(-8,8);

    double samp;
    double type_max = std::numeric_limits<SAMPLE_TYPE>::max();
    double type_min = std::numeric_limits<SAMPLE_TYPE>::min();
    double type_range = type_max - type_min;
    double mid = (type_max + type_min +1.0)/2.0;

    for(size_t i=0; i<num_samples; i++)
    {
        testval = summed_noise->GetSample(i*delta, samp);
        (void) testval;
        double conv = simple14bitADC.Convert(samp);
        noise_samples[i] = conv;
        digi_samples[i] = conv;
        double rescaled = (conv - mid)/type_range;
        noise_xform_in[i] = std::complex<double>(rescaled, 0.0);
    }

    delete summed_noise;
    delete snoise;
    delete gnoise2;
    delete gnoise1;

    HLinearBuffer< SAMPLE_TYPE >* ppBuff = new HLinearBuffer< SAMPLE_TYPE >( &(digi_samples[0]), digi_samples.size() );
    ppBuff->GetMetaData()->SetLeadingSampleIndex(0);

    //now build a power calculator to test the calculation
    HPeriodicPowerCalculator< SAMPLE_TYPE >* ppCalc = new HPeriodicPowerCalculator< SAMPLE_TYPE >();
    ppCalc->SetSamplingFrequency(sample_frequency);
    ppCalc->SetSwitchingFrequency(switching_frequency);
    ppCalc->SetBlankingPeriod(0);

    ppCalc->SetBuffer(ppBuff);

    ppCalc->Calculate();

    std::vector< struct HDataAccumulationStruct >* accums = ppBuff->GetMetaData()->GetAccumulations();

    double total_on_x = 0;
    double total_on_x2 = 0;
    double total_off_x = 0;
    double total_off_x2 = 0;
    double total_on_count = 0;
    double total_off_count = 0;
    for(size_t i=0; i<accums->size(); i++)
    {
        if(accums->at(i).state_flag == 1)
        {
            //diode on
            total_on_x += accums->at(i).sum_x;
            total_on_x2 += accums->at(i).sum_x2;
            total_on_count += accums->at(i).count;
        }
        else
        {
            //diode off
            total_off_x += accums->at(i).sum_x;
            total_off_x2 += accums->at(i).sum_x2;
            total_off_count += accums->at(i).count;
        }
    }

    std::cout<<std::setprecision(15)<<std::endl;

    std::cout<<"total on x = "<<total_on_x<<std::endl;
    std::cout<<"total on x2 = "<<total_on_x2<<std::endl;
    std::cout<<"total on count = "<<total_on_count<<std::endl;

    std::cout<<"total off x = "<<total_off_x<<std::endl;
    std::cout<<"total off x2 = "<<total_off_x2<<std::endl;
    std::cout<<"total off count = "<<total_off_count<<std::endl;

    double on_var = total_on_x2/total_on_count - (total_on_x/total_on_count)*(total_on_x/total_on_count);
    double off_var = total_off_x2/total_off_count - (total_off_x/total_off_count)*(total_off_x/total_off_count);

    std::cout<<"on var = "<<on_var<<std::endl;
    std::cout<<"off var = "<<off_var<<std::endl;
    

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

/*
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
*/
    c->Update();


    //noise power calc

    std::string name2("canvas2");
    TCanvas* c2 = new TCanvas(name2.c_str(),name2.c_str(), 50, 50, 950, 850);
    c2->cd();
    c2->SetFillColor(0);
    c2->SetRightMargin(0.2);
    c2->Divide(1,3);
    c2->cd(1);
    
    //plot the the on/off power variance
    TGraph* np_time_series_on = new TGraph();
    TGraph* np_time_series_off = new TGraph();
    TMultiGraph* mg = new TMultiGraph();

    for(size_t i=0; i<accums->size(); i++)
    {
        if(accums->at(i).state_flag == H_NOISE_DIODE_ON )
        {
            //diode on
            double sum_x = accums->at(i).sum_x;
            double sum_x2 = accums->at(i).sum_x2;
            double count = accums->at(i).count;
            double var = sum_x2/count - (sum_x/count)*(sum_x/count);
            double si = accums->at(i).start_index;
            np_time_series_on->SetPoint(np_time_series_on->GetN(), si*delta, var);
        }
        else
        {
            //diode off
            double sum_x = accums->at(i).sum_x;
            double sum_x2 = accums->at(i).sum_x2;
            double count = accums->at(i).count;
            double var = sum_x2/count - (sum_x/count)*(sum_x/count);
            double si = accums->at(i).start_index;
            np_time_series_off->SetPoint(np_time_series_off->GetN(), si*delta, var);
        }
    }
    
    np_time_series_on->SetMarkerColor(2);
    np_time_series_off->SetMarkerColor(4);
    np_time_series_on->SetLineColor(2);
    np_time_series_off->SetLineColor(4);


    mg->Add(np_time_series_on);
    mg->Add(np_time_series_off);

    mg->Draw("ALP");
    c2->Update();

    // 
    // //histogram the values
    // TH1D* histo2 = new TH1D("histogram2", "histogram2", 100, min, max);
    // for(size_t i=0; i<accums->size(); i++)
    // {
    //     histo2->Fill()
    // }
    // c2->cd(2);
    // 
    // 
    // double total_on_x = 0;
    // double total_on_x2 = 0;
    // double total_off_x = 0;
    // double total_off_x2 = 0;
    // double total_on_count = 0;
    // double total_off_count = 0;
    // for(size_t i=0; i<accums->size(); i++)
    // {
    //     if(accums->at(i).state_flag == H_NOISE_DIODE_ON )
    //     {
    //         //diode on
    //         total_on_x += accums->at(i).sum_x;
    //         total_on_x2 += accums->at(i).sum_x2;
    //         total_on_count += accums->at(i).count;
    //     }
    //     else
    //     {
    //         //diode off
    //         total_off_x += accums->at(i).sum_x;
    //         total_off_x2 += accums->at(i).sum_x2;
    //         total_off_count += accums->at(i).count;
    //     }
    //     // std::cout<<"i = "<<i<<std::endl;
    //     // std::cout<<"sum_x = "<<accums->at(i).sum_x<<std::endl;
    //     // std::cout<<"sum_x2 = "<<accums->at(i).sum_x2<<std::endl;
    //     // std::cout<<"count = "<<accums->at(i).count<<std::endl;
    //     // std::cout<<"state_flag = "<<accums->at(i).state_flag<<std::endl;
    //     // std::cout<<"start_index = "<<accums->at(i).start_index<<std::endl;
    //     // std::cout<<"stop_index = "<<accums->at(i).stop_index<<std::endl;
    //     // std::cout<<"-------------------------"<<std::endl;
    // }
    // 
    // 
    // 
    //     for(size_t i=0; i<accums->size(); i++)
    //     {
    //         std::cout<<"i = "<<i<<std::endl;
    //         std::cout<<"sum_x = "<<accums->at(i).sum_x<<std::endl;
    //         std::cout<<"sum_x2 = "<<accums->at(i).sum_x2<<std::endl;
    //         std::cout<<"count = "<<accums->at(i).count<<std::endl;
    //         std::cout<<"state_flag = "<<accums->at(i).state_flag<<std::endl;
    //         std::cout<<"start_index = "<<accums->at(i).start_index<<std::endl;
    //         std::cout<<"stop_index = "<<accums->at(i).stop_index<<std::endl;
    //         std::cout<<"-------------------------"<<std::endl;
    //     }
    // 



    App->Run();

    //delete fftCalculator;

}
