#include <dirent.h>
#include <vector>
#include <string>
#include <iostream>
#include <fstream>
#include <algorithm>
#include <utility>
#include <stdint.h>
#include <sstream>
#include <ctime>
#include <getopt.h>
#include <iomanip>

//single header JSON lib
#include <json.hpp>
using json = nlohmann::json;

extern "C"
{
    #include "HBasicDefines.h"
    #include "HSpectrumFile.h"
    #include "HNoisePowerFile.h"
}

#include "HArrayWrapper.hh"

//TODO fix these headers
#include "HPowerLawNoiseSignal.hh"
#include "HGaussianWhiteNoiseSignal.hh"
#include "HSwitchedSignal.hh"
#include "HSummedSignal.hh"
#include "HSimpleAnalogToDigitalConverter.hh"

#include "HSpectrumFileStructWrapper.hh"
using namespace hose;




#include "TCanvas.h"
#include "TApplication.h"
#include "TStyle.h"
#include "TColor.h"
#include "TGraph.h"
#include "TGraph2D.h"
#include "TH2D.h"
#include "TH1D.h"
#include "TVirtualFFT.h"
#include "TF1.h"

union raw_data_type
{
    int16_t value;
    char char_arr[sizeof(int16_t)] ;
};

struct less_than_file
{
    inline bool operator() (const std::pair< std::string, std::pair<uint64_t, uint64_t > >& file_int1, const std::pair< std::string, std::pair<uint64_t, uint64_t > >& file_int2)
    {
        if(file_int1.second.first < file_int2.second.first )
        {
            return true;
        }
        else if (file_int1.second.first == file_int2.second.first )
        {
            return (file_int1.second.second < file_int2.second.second );
        }
        else
        {
            return false;
        }
    }
};


void
get_time_stamped_files(std::string fext, std::string delim, const std::vector<std::string>& file_list, std::vector< std::pair< std::string, std::pair< uint64_t, uint64_t > > >& stamped_files)
{
    for(auto it = file_list.begin(); it != file_list.end(); it++)
    {
        if( it->find(fext) != std::string::npos)
        {
            std::string fname =  it->substr(it->find_last_of("\\/")+1, it->size() - 1);
            size_t fext_location = fname.find( fext );
            if( fext_location != std::string::npos)
            {
                size_t delim_loc = fname.find( delim );
                if(delim_loc != std::string::npos)
                {
                    //strip the acquisition second
                    std::string acq_second = fname.substr(0, delim_loc);
                    std::string remaining = fname.substr(delim_loc+1, fname.size() );
                    //get the leading sample index
                    size_t next_delim_loc = remaining.find( delim );
                    if(next_delim_loc != std::string::npos)
                    {
                        std::string sample_index = remaining.substr(0, next_delim_loc);
                        std::stringstream ss1;
                        ss1 << acq_second;
                        uint64_t val_sec;
                        ss1 >> val_sec;
                        std::stringstream ss2;
                        ss2 << sample_index;
                        uint64_t val_si;
                        ss2 >> val_si;
                        stamped_files.push_back( std::pair< std::string, std::pair<uint64_t, uint64_t > >(*it, std::pair< uint64_t, uint64_t >(val_sec, val_si ) ) );
                    }
                }
            }
        }
    }
}


int main(int argc, char** argv)
{
    std::string usage =
    "\n"
    "Usage: RawPlot <options> <directory>\n"
    "\n"
    "Plot spectrum from scan data in the <directory>.\n"
    "\tOptions:\n"
    "\t -h, --help               (shows this message and exits)\n"
    "\t -d, --data-dir           (path to the directory containing scan data, mandatory)\n"
    ;

    //set defaults
    bool have_data = false;
    std::string data_dir = STR2(DATA_INSTALL_DIR);
    std::string o_dir = STR2(DATA_INSTALL_DIR);

    static struct option longOptions[] =
    {
        {"help", no_argument, 0, 'h'},
        {"data_dir", required_argument, 0, 'd'},
    };

    static const char *optString = "hd:";

    while(1)
    {
        char optId = getopt_long(argc, argv, optString, longOptions, NULL);
        if(optId == -1) break;
        switch(optId)
        {
            case('h'): // help
            std::cout<<usage<<std::endl;
            return 0;
            case('d'):
            data_dir = std::string(optarg);
            have_data = true;
            break;
            default:
                std::cout<<usage<<std::endl;
            return 1;
        }
    }

    if(!have_data)
    {
        std::cout<<"Data directory argument is mandatory."<<std::endl;
        std::cout<<usage<<std::endl;
        return 1;
    }


    std::string raw_ext = ".bin";
    std::string udelim = "_";
    std::vector< std::string > allFiles;
    std::vector< std::pair< std::string, std::pair< uint64_t, uint64_t > > > rawFiles;
    bool have_meta_data = false;
    std::string metaDataFile = "";

    //get list of all file in directory
    DIR *dpdf;
    struct dirent* epdf = NULL;
    dpdf = opendir(data_dir.c_str());
    if (dpdf != NULL)
    {
        do
        {
            epdf = readdir(dpdf);
            if(epdf != NULL){allFiles.push_back( data_dir + "/" + std::string(epdf->d_name) );}
        }
        while(epdf != NULL);
    }
    closedir(dpdf);

    //sort files, locate .spec, .npow and .json meta data file
    get_time_stamped_files(raw_ext, udelim, allFiles, rawFiles);

    //now sort the raw files by time stamp (filename)
    std::sort( rawFiles.begin(), rawFiles.end() , less_than_file() );

    if(rawFiles.size() < 1)
    {
        std::cout<<"no raw data found"<<std::endl;
        return 1;
    }

    //read the raw data
    std::vector<double> raw_data;
    std::ifstream in_file;
    in_file.open( rawFiles.back().first,  std::ios::in);

    std::cout<<"reading raw data"<<std::endl;
    size_t count=0;
    double data_min=1e30;
    double data_max=-1e30;

    do
    {
        raw_data_type datum;
        in_file.read(datum.char_arr, sizeof(int16_t));
        count++;
        raw_data.push_back(datum.value);
        if(datum.value < data_min){data_min = datum.value;}
        if(datum.value > data_max){data_max = datum.value;}
        if(count % 100000 == 0){std::cout<<"on sample: "<<count<<" value = "<<datum.value<<std::endl;};
    }
    while(!in_file.eof() && count < 10000000);
    in_file.close();

    double sample_frequency = 1250e6;

    //ROOT stuff for plots
    TApplication* App = new TApplication("TestRawPlot",&argc,argv);
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
    std::vector< TCanvas* > canvas;
    std::vector< TGraph* > graph;
    std::vector< TGraph2D* > graph2d;

    std::string name("spec");
    TCanvas* c = new TCanvas(name.c_str(),name.c_str(), 50, 50, 950, 850);
    c->SetFillColor(0);
    c->SetRightMargin(0.2);
    c->Divide(1,3);
    c->cd(1);

    TGraph* g = new TGraph();

    std::vector< std::complex<double> > noise_xform_in;
    std::vector< std::complex<double> > noise_xform_out;
    size_t dim[1];
    dim[0] = raw_data.size();

    for(unsigned int j=0; j<raw_data.size(); j++)
    {
        g->SetPoint(j, j*(1.0/sample_frequency), raw_data[j] );
        noise_xform_in.push_back(std::complex<double>( raw_data[j], 0.0) );
        noise_xform_out.push_back(std::complex<double>( 0.0, 0.0) );
    }

    g->Draw("ALP");
    g->SetTitle("Samples");
    g->GetXaxis()->SetTitle("Time (s)");
    g->GetYaxis()->SetTitle("Sample Value");
    g->GetYaxis()->CenterTitle();
    g->GetXaxis()->CenterTitle();
    c->Update();


    //now we want to compute the spectral power density
    //wrap the array (needed for FFT interface
    HArrayWrapper< std::complex< double >, 1 > wrapperIn;
    HArrayWrapper< std::complex< double >, 1 > wrapperOut;
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

    for(unsigned int i=1; i<dim[0]/2; i++)
    {
        double omega = i*(sample_frequency/dim[0]);
        spectrum->SetPoint(spectrum->GetN(), std::log10(omega), 10.0*std::log10( norm*std::real( noise_xform_out[i]*std::conj(noise_xform_out[i]) ) ) );
    }

    c->cd(2);
    spectrum->Draw("ALP");
    c->Update();

    //
    //
    //
    //
    // //now we want to do an FFT of the raw data sample and plot the output:
    //
    // //Allocate an array big enough to hold the transform output
    // //Transform output in 1d contains, for a transform of size N,
    // //N/2+1 complex numbers, i.e. 2*(N/2+1) real numbers
    // //our transform is of size n+1, because the histogram has n+1 bins
    // int sample_len = raw_data.size();
    // double *in = new double[2*((sample_len+1)/2+1)];
    // for(size_t i=0; i<=sample_len; i++)
    // {
    //     in[i] = raw_data[i];
    // }
    // //Make our own TVirtualFFT object (using option "K")
    // //Third parameter (option) consists of 3 parts:
    // //- transform type:
    // // real input/complex output in our case
    // //- transform flag:
    // // the amount of time spent in planning
    // // the transform (see TVirtualFFT class description)
    // //- to create a new TVirtualFFT object (option "K") or use the global (default)
    // int n_size = sample_len+1;
    // TVirtualFFT *fft_own = TVirtualFFT::FFT(1, &n_size, "R2C ES K");
    // if (!fft_own) return 1;
    // fft_own->SetPoints(in);
    // fft_own->Transform();
    // //Copy all the output points:
    // fft_own->GetPoints(in);
    // //Draw the real part of the output
    // c->cd(2);
    // TH1 *hr = 0;
    // hr = TH1::TransformHisto(fft_own, hr, "MAG");
    // hr->SetTitle("Raw Spectrum");
    // hr->Draw();
    // hr->SetStats(kFALSE);
    // hr->GetXaxis()->SetLabelSize(0.05);
    // hr->GetYaxis()->SetLabelSize(0.05);
    // gPad->SetLogy();

    // //histogram the values of the on/off noise variance
    c->cd(3);
    TH1D* histo = new TH1D("sample histogram", "sample histogram", 256, data_min, data_max);
    for(size_t i=0; i<raw_data.size(); i++)
    {
        histo->Fill(raw_data[i]);
    }
    histo->Draw("");
    c->Update();

    App->Run();

    return 0;
}
