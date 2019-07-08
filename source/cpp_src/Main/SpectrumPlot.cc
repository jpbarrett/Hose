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

#include "HSpectrumFileStructWrapper.hh"
using namespace hose;

#include "TCanvas.h"
#include "TApplication.h"
#include "TStyle.h"
#include "TColor.h"
#include "TGraph.h"
#include "TGraph2D.h"
#include "TH2D.h"



double eps = 1e-15;

struct less_than_spec
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
    "Usage: SpectrumPlot <options> <directory>\n"
    "\n"
    "Plot spectrum from scan data in the <directory>.\n"
    "\tOptions:\n"
    "\t -h, --help               (shows this message and exits)\n"
    "\t -d, --data-dir           (path to the directory containing scan data, mandatory)\n"
    "\t -n, --n-averages         (number of spectra files (.spec) to average together, optional)\n"
    ;

    //set defaults
    unsigned int n_averages = 0; //default behavior is to average all spectra together
    bool have_data = false;
    std::string data_dir = STR2(DATA_INSTALL_DIR);
    std::string o_dir = STR2(DATA_INSTALL_DIR);

    static struct option longOptions[] =
    {
        {"help", no_argument, 0, 'h'},
        {"data_dir", required_argument, 0, 'd'},
        {"n-averages", required_argument, 0, 'n'}
    };

    static const char *optString = "hd:n:";

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
            case('n'):
            n_averages = atoi(optarg);
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


    std::string spec_ext = ".spec";
    std::string npow_ext = ".npow";
    std::string meta_data_name = "meta_data.json";
    std::string udelim = "_";
    std::vector< std::string > allFiles;
    std::vector< std::pair< std::string, std::pair< uint64_t, uint64_t > > > specFiles;
    std::vector< std::pair< std::string, std::pair< uint64_t, uint64_t > > > powerFiles;
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
    get_time_stamped_files(spec_ext, udelim, allFiles, specFiles);
    get_time_stamped_files(npow_ext, udelim, allFiles, powerFiles);

    //look for meta_data file
    for(auto it = allFiles.begin(); it != allFiles.end(); it++)
    {
        if( it->find(meta_data_name) != std::string::npos)
        {
            have_meta_data = true;
            metaDataFile = *it;
        }
    }

    //now sort the spec and npow files by time stamp (filename)
    std::sort( specFiles.begin(), specFiles.end() , less_than_spec() );
    std::sort( powerFiles.begin(), powerFiles.end() , less_than_spec() );

    // if(have_meta_data)
    // {
    //     std::cout<<"meta data file = "<< metaDataFile<<std::endl;
    //     std::ifstream metadata(metaDataFile.c_str());
    //     json j;
    //     metadata >> j;
    //     for (json::iterator it = j.begin(); it != j.end(); ++it) {
    //       std::cout << (*it)["measurement"] << '\n';
    //     }
    // }


    // for(auto it = specFiles.begin(); it != specFiles.end(); it++)
    // {
    //     std::cout<<"spec file: "<< it->first << " @ " << it->second.first <<", "<< it->second.second<<std::endl;
    // }
    //
    // for(auto it = powerFiles.begin(); it != powerFiles.end(); it++)
    // {
    //     std::cout<<"npow file: "<< it->first << " @ " << it->second.first <<", "<< it->second.second<<std::endl;
    // }

////////////////////////////////////////////////////////////////////////////////
//compute an averaged spectrum

    //space to accumulate the raw spectrum
    std::vector<float> raw_accumulated_spec;
    size_t spec_length = 0;
    double spec_res = 0;
    double sample_rate = 0;
    double n_samples = 0;
    double n_ave = 0;
    double n_samples_per_spec = 0;
    double spec_count = 0;
    double sample_period = 0;
    //now open up all the spec data one by one and accumulate

    std::cout<<"number of spec files = "<<specFiles.size()<<std::endl;

    std::pair<uint64_t, uint64_t >  begin_time_stamp = specFiles.begin()->second;
    std::pair<uint64_t, uint64_t >  end_time_stamp = specFiles.rbegin()->second;

    std::cout<<"starting time stamp = "<<begin_time_stamp.first<<", "<<begin_time_stamp.second<<std::endl;
    std::cout<<"ending time stamp = "<<end_time_stamp.first<<", "<<end_time_stamp.second<<std::endl;


    double sec_diff = end_time_stamp.first - begin_time_stamp.first;
    double sample_diff = end_time_stamp.second - begin_time_stamp.second;

    //now open up all the spectrum files one by one and sum them
    for(size_t i = 0; i < specFiles.size(); i++)
    {
        HSpectrumFileStructWrapper<float> tempFile(specFiles[i].first);

        if(i==0)
        {
            spec_length = tempFile.GetSpectrumLength();
            n_ave = tempFile.GetNAverages();
            sample_rate = tempFile.GetSampleRate();
            sample_period = 1.0/sample_rate;
            n_samples = tempFile.GetSampleLength();
            n_samples_per_spec = n_samples / n_ave;
            spec_res = sample_rate/n_samples_per_spec;
            std::cout<<"num samples = "<<n_samples<<std::endl;
            std::cout<<"n averages = "<<n_ave<< std::endl;
            std::cout<<"sample rate = "<<sample_rate<<std::endl;
            std::cout<<"sample period = "<<sample_period<<std::endl;
            std::cout<<"n samples per spec = "<<n_samples_per_spec<<std::endl;
            std::cout<<"spec_res= "<<spec_res<<std::endl;
            std::cout<<"spectrum length = "<<spec_length<<std::endl;
            double time_diff = sec_diff + (sample_diff + n_samples)*sample_period;
            std::cout<<"recording length (sec) = "<<time_diff<<std::endl;
            raw_accumulated_spec.resize(spec_length, 0);
        }

        float* spec_data = tempFile.GetSpectrumData();
        for(size_t j=0; j<spec_length; j++)
        {
            raw_accumulated_spec[j] += spec_data[j];
        }
        spec_count += 1.0;
    }

    //compute average
    for(size_t j=0; j<spec_length; j++)
    {
        raw_accumulated_spec[j] /= spec_count;
    }

////////////////////////////////////////////////////////////////////////////////
//collect the raw accumulations

    std::vector< HDataAccumulationStruct > fOnAccumulations;
    std::vector< std::pair<double,double> > fOnVarianceTimePairs;
    std::vector< HDataAccumulationStruct > fOffAccumulations;
    std::vector< std::pair<double,double> > fOffVarianceTimePairs;

    double var_min = 1e100;
    double var_max = -1e100;

    double on_sumx = 0;
    double on_sumx2 = 0;
    double on_count = 0;
    double off_sumx = 0;
    double off_sumx2 = 0;
    double off_count = 0;

    for(size_t i = 0; i < powerFiles.size(); i++)
    {
        HNoisePowerFileStruct* npow = CreateNoisePowerFileStruct();
        int success = ReadNoisePowerFile(powerFiles[i].first.c_str(), npow);

        uint64_t n_accum = npow->fHeader.fAccumulationLength;
        for(uint64_t j=0; j<n_accum; j++)
        {
            HDataAccumulationStruct accum_dat = npow->fAccumulations[j];
            if(accum_dat.state_flag == H_NOISE_DIODE_ON)
            {
                fOnAccumulations.push_back(accum_dat);
                double x = accum_dat.sum_x;
                double x2 = accum_dat.sum_x2;
                double c = accum_dat.count;
                double start = accum_dat.start_index;
                on_sumx += x;
                on_sumx2 += x2;
                on_count += c;
                double var = x2/c - (x/c)*(x/c);
                fOnVarianceTimePairs.push_back(  std::pair<double,double>(var, start*sample_period) );
                if( std::fabs(var) > 0.0)
                {
                    if(var < var_min){var_min = var;};
                    if(var > var_max){var_max = var;};
                }
            }
            else
            {
                fOffAccumulations.push_back(accum_dat);
                double x = accum_dat.sum_x;
                double x2 = accum_dat.sum_x2;
                double c = accum_dat.count;
                double start = accum_dat.start_index;
                off_sumx += x;
                off_sumx2 += x2;
                off_count += c;
                double var = x2/c - (x/c)*(x/c);
                fOffVarianceTimePairs.push_back(  std::pair<double,double>(var, start*sample_period) );
                if( std::fabs(var) > 0.0)
                {
                    if(var < var_min){var_min = var;};
                    if(var > var_max){var_max = var;};
                }
            }
        }
        DestroyNoisePowerFileStruct(npow);
    }

    std::cout<<std::setprecision(15)<<std::endl;

    std::cout<<"n accum on = "<<fOnAccumulations.size()<<std::endl;
    std::cout<<"n accum off = "<<fOnAccumulations.size()<<std::endl;

    double on_var = on_sumx2/on_count - (on_sumx/on_count)*(on_sumx/on_count);
    double off_var = off_sumx2/off_count - (off_sumx/off_count)*(off_sumx/off_count);

    std::cout<<"on_sumx = "<<on_sumx<<std::endl;
    std::cout<<"on_sumx2 = "<<on_sumx2<<std::endl;
    std::cout<<"on_count = "<<on_count<<std::endl;

    std::cout<<"off_sumx = "<<off_sumx<<std::endl;
    std::cout<<"off_sumx2 = "<<off_sumx2<<std::endl;
    std::cout<<"off_count = "<<off_count<<std::endl;

    std::cout<<"Average ON variance = "<<on_var<<std::endl;
    std::cout<<"Average OFF variance = "<<off_var<<std::endl;
    std::cout<<"Difference: ON-OFF = "<<on_var - off_var<<std::endl;
    std::cout<<"Relative Difference: (ON-OFF)/ON = "<< (on_var - off_var)/on_var <<std::endl;

    double on_var_mean = 0;
    double on_var_sigma = 0;
    double off_var_mean = 0;
    double off_var_sigma = 0;

    for(size_t j=0; j<fOnVarianceTimePairs.size(); j++)
    {
        on_var_mean += fOnVarianceTimePairs[j].first;
    }
    on_var_mean /= (double)fOnVarianceTimePairs.size();

    for(size_t j=0; j<fOffVarianceTimePairs.size(); j++)
    {
        off_var_mean += fOffVarianceTimePairs[j].first;
    }
    off_var_mean /= (double)fOffVarianceTimePairs.size();

    for(size_t j=0; j<fOnVarianceTimePairs.size(); j++)
    {
        double delta = fOnVarianceTimePairs[j].first - on_var_mean;
        on_var_sigma += delta*delta;
    }
    on_var_sigma /= (double)fOnVarianceTimePairs.size();
    on_var_sigma = std::sqrt(on_var_sigma);

    for(size_t j=0; j<fOffVarianceTimePairs.size(); j++)
    {
        double delta = fOffVarianceTimePairs[j].first - off_var_mean;
        off_var_sigma += delta*delta;
    }
    off_var_sigma /= (double)fOffVarianceTimePairs.size();
    off_var_sigma = std::sqrt(off_var_sigma);

////////////////////////////////////////////////////////////////////////////////

    std::cout<<"starting root plotting"<<std::endl;

    //ROOT stuff for plots
    TApplication* App = new TApplication("TestSpectrumPlot",&argc,argv);
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
    for(unsigned int j=0; j<spec_length; j++)
    {
        g->SetPoint(j, j*spec_res/1e6, 10.0*std::log10( raw_accumulated_spec[j] + eps ) );
    }
    g->Draw("ALP");

    g->SetTitle("Average Spectrum");
    g->GetXaxis()->SetTitle("Frequency (MHz)");
    g->GetYaxis()->SetTitle("Relative Power (dB)");
    g->GetYaxis()->CenterTitle();
    g->GetXaxis()->CenterTitle();
    c->Update();

    //histogram the values of the on/off noise variance
    c->cd(2);
    TH1D* on_histo = new TH1D("on_noise_variance histogram", "on_noise_variance histogram", 5000, on_var_mean - 1.0*on_var_sigma, on_var_mean + 1.0*on_var_sigma);
    for(size_t i=0; i<fOnVarianceTimePairs.size(); i++)
    {
        on_histo->Fill(fOnVarianceTimePairs[i].first);
    }
    on_histo->Draw("");
    c->Update();

    //histogram the values
    c->cd(3);
    TH1D* off_histo = new TH1D("off_noise_variance histogram", "off_noise_variance histogram", 5000, off_var_mean - 1.0*off_var_sigma, off_var_mean + 1.0*off_var_sigma);
    for(size_t i=0; i<fOffVarianceTimePairs.size(); i++)
    {
        off_histo->Fill(fOffVarianceTimePairs[i].first);
    }
    off_histo->Draw("");
    c->Update();

    App->Run();

    return 0;
}
