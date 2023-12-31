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
#include "TPaveText.h"


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
    "\t -r, --resolution         (desired spectral resolution in MHz, if greater than the native spectral resolution, frequency bins will be averaged together)\n"
    "\t -o, --output             (output text file to dump spectral data)\n"
    "\t -b, --bins               (plot spectrum as function of bin number)\n"
    ;

    //set defaults
    unsigned int n_averages = 0; //default behavior is to average all spectra together
    double desired_spec_res = 0.0;
    bool rebin = false;
    bool have_data = false;
    bool use_bin_numbers = false;
    std::string data_dir = STR2(DATA_INSTALL_DIR);
    std::string o_dir = STR2(DATA_INSTALL_DIR);
    std::string output_file = "";

    static struct option longOptions[] =
    {
        {"help", no_argument, 0, 'h'},
        {"data_dir", required_argument, 0, 'd'},
        {"n-averages", required_argument, 0, 'n'},
        {"resolution", required_argument, 0, 'r'},
        {"output", required_argument, 0, 'o'},
        {"bins", no_argument, 0, 'b'}
    };

    static const char *optString = "hd:n:r:o:b";

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
            case('r'):
            desired_spec_res = 1e6*atof(optarg);
            rebin = true;
            break;
            case('o'):
            output_file = std::string(optarg);
            break;
            case('b'):
            use_bin_numbers = true;
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

    // {
    //     "fields": {
    //         "bin_delta": 1,
    //         "frequency_delta_MHz": -0.0005960464477539062,
    //         "reference_bin_center_sky_frequency_MHz": 6653.499701976776,
    //         "reference_bin_index": 524288
    //     },
    //     "measurement": "frequency_map",
    //     "time": "2018-12-18T19:13:09.34Z"
    // },


    bool map_to_sky_frequency = false;
    int binDelta = 1;
    double freqDeltaMHz = 0.0;
    int referenceBinIndex = 0;
    double referenceBinCenterSkyFreqMHz = 0.0;

    bool source_info = false;
    std::string source_name = "";
    std::string ra = "";
    std::string dec = "";
    std::string epoch = "";

    bool recording_status_info = false;
    std::string experiment_name = "";
    std::string scan_name = "";
    std::string start_time = "";
    std::string duration;

    if(have_meta_data)
    {
        std::cout<<"meta data file = "<< metaDataFile<<std::endl;
        std::ifstream metadata(metaDataFile.c_str());
        json j;
        metadata >> j;

        for (json::iterator it = j.begin(); it != j.end(); ++it)
        {
            std::string measurement_name = (*it)["measurement"].get<std::string>();
            std::cout<<"measurement name = "<<measurement_name<<std::endl;
            if( measurement_name == std::string("frequency_map") )
            {
                auto js2 = (*it)["fields"];
                binDelta = js2["bin_delta"].get<int>();
                freqDeltaMHz = js2["frequency_delta_MHz"].get<double>();
                referenceBinCenterSkyFreqMHz = js2["reference_bin_center_sky_frequency_MHz"].get<double>();
                referenceBinIndex = js2["reference_bin_index"].get<int>();

                std::cout << "bin_delta = " << binDelta << std::endl;
                std::cout << "frequency delta (MHZ) = "<< freqDeltaMHz << std::endl;
                std::cout << "reference_bin_center_sky_frequency (MHz) " << referenceBinCenterSkyFreqMHz << std::endl;
                std::cout << "reference_bin_index =  " << referenceBinIndex << std::endl;

                //std::cout<<"WARNING: IGNORING SKY FREQUENCY MAP."<<std::endl;
                map_to_sky_frequency = true;
            }

            if( measurement_name == std::string("source_status") )
            {
                auto js2 = (*it)["fields"];
                source_name = js2["source"].get<std::string>();
                ra = js2["ra"].get<std::string>();
                dec = js2["dec"].get<std::string>();
                epoch = js2["epoch"].get<std::string>();
                source_info = true;
            }

            if( measurement_name == std::string("recording_status") )
            {
                auto js2 = (*it)["fields"];
                std::string recording_state = js2["recording"].get<std::string>();
                if(recording_state == std::string("on"))
                {
                    experiment_name = js2["experiment_name"].get<std::string>();
                    scan_name = js2["scan_name"].get<std::string>();
                    std::string tmptime = (*it)["time"].get<std::string>();
                    start_time = tmptime.substr(0, tmptime.find("."));
                    recording_status_info = true;
                }
            }
        }
    }


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
    if(specFiles.size() == 0)
    {
        std::cout<<"Error: could not locate any spectrum files in directory: "<<data_dir<<std::endl;
        return 1;
    }

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
            std::stringstream tempss;
            tempss << time_diff;
            tempss >> duration;

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

    //zero-out the first and last few bins to get rid of DC
    int bin_mask = std::max(4, (int) ( 0.0001*(double)spec_length) );
    for(size_t k=0; k<bin_mask; k++)
    {
        raw_accumulated_spec[k] = 0.0;
        raw_accumulated_spec[spec_length-1-k] = 0.0;
    }

    //if the user has asked for a reduced spectral resolution, we join bins together
    std::vector<float> rebinned_spec;
    int n_to_merge = 1;
    if(rebin)
    {
        n_to_merge = std::max(1, (int)std::floor(desired_spec_res/spec_res) );
        std::cout<<"Rebinning frequency axis. Desired resolution: "<<desired_spec_res<<", native resolution: "<<spec_res<<std::endl;
        std::cout<<"Number of bins to merge: "<<n_to_merge<<", new spectral resolution: "<<n_to_merge*spec_res<<std::endl;
        size_t new_spec_length = spec_length/n_to_merge;
        rebinned_spec.resize(new_spec_length, 0);
        for(size_t j=0; j<new_spec_length; j++)
        {
            rebinned_spec[j] = 0;
            for(size_t k=0; k<n_to_merge; k++)
            {
                rebinned_spec[j] += raw_accumulated_spec[j*n_to_merge + k];
            }
            rebinned_spec[j] /= (double)n_to_merge;
        }
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
    int dummy_argc = 1;
    char* dummy_argv[] = {"myapp"};
    TApplication* App = new TApplication("TestSpectrumPlot",&dummy_argc,dummy_argv);
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
    TCanvas* c = new TCanvas(name.c_str(),name.c_str(), 50, 50, 1450, 850);
    c->SetFillColor(0);
    c->SetRightMargin(0.05);
    // c->Divide(1,3);
    // c->cd(1);

    TGraph* g = new TGraph();

    TPaveText *pt = new TPaveText(.14,.72,.29,.88,"blNDC");
    pt->SetBorderSize(1);
    pt->SetFillColor(0);
    pt->AddText( std::string( std::string( "Experiment: ") + experiment_name).c_str() );
    pt->AddText( std::string( std::string( "Scan: ") + scan_name).c_str() );
    pt->AddText( std::string( std::string( "Source: ") + source_name ).c_str() );
    pt->AddText( std::string( std::string( "RA: ") + ra).c_str() );
    pt->AddText( std::string( std::string( "DEC: ") + dec).c_str() );
    pt->AddText( std::string( std::string( "Epoch: ") + epoch).c_str() );
    pt->AddText( std::string( std::string( "Start Time: ") + start_time).c_str() );
    pt->AddText( std::string( std::string( "Duration: ") + duration).c_str() );


    //output data objects 
    std::vector<double> output_freq;
    std::vector<double> output_spectra;

    if(rebinned_spec.size() == 0)
    {
        //plot the raw averaged spectrum
        for(unsigned int j=0; j<spec_length; j++)
        {
            if(!map_to_sky_frequency)
            {
                if(use_bin_numbers)
                {
                    g->SetPoint(j, j, 10.0*std::log10(  raw_accumulated_spec[j] + eps  ) );
                }
                else 
                {
                    g->SetPoint(j, j*spec_res/1e6, 10.0*std::log10( raw_accumulated_spec[j] + eps ) );
                }
                output_freq.push_back(j*spec_res/1e6);
                output_spectra.push_back(raw_accumulated_spec[j]);
            }
            else
            {
                //std::cout<<"j, val = "<<j<<", "<<raw_accumulated_spec[j]<<std::endl;
                double index = j;
                double ref_index = referenceBinIndex;
                double freq = (index - referenceBinIndex)*freqDeltaMHz + referenceBinCenterSkyFreqMHz;


                if(use_bin_numbers)
                {
                    g->SetPoint(j, index, 10.0*std::log10( raw_accumulated_spec[j] + eps  ) );
                }
                else 
                {
                    g->SetPoint(j, freq, 10.0*std::log10( raw_accumulated_spec[j] + eps ) );
                }

                output_freq.push_back(freq);
                output_spectra.push_back(raw_accumulated_spec[j]);
            }
        }
    }
    else 
    {
        //plot the re-binned spectrum
        double new_spec_res = spec_res*n_to_merge;
        for(unsigned int j=0; j<rebinned_spec.size(); j++)
        {
            if(!map_to_sky_frequency)
            {
                if(use_bin_numbers)
                {
                    g->SetPoint(j, j*n_to_merge, 10.0*std::log10( rebinned_spec[j] + eps ) );
                }
                else 
                {
                    g->SetPoint(j, j*new_spec_res/1e6, 10.0*std::log10( rebinned_spec[j] + eps ) );
                }
                output_freq.push_back(j*spec_res/1e6);
                output_spectra.push_back(rebinned_spec[j]);
            }
            else
            {
                //std::cout<<"j, val = "<<j<<", "<<raw_accumulated_spec[j]<<std::endl;
                double index = j;
                double ref_index = referenceBinIndex;
                double freq = (index - referenceBinIndex)*(n_to_merge*freqDeltaMHz) + referenceBinCenterSkyFreqMHz;
                if(use_bin_numbers)
                {
                    g->SetPoint(j, index*n_to_merge, 10.0*std::log10( rebinned_spec[j] + eps ) );
                }
                else 
                {
                    g->SetPoint(j, freq, 10.0*std::log10( rebinned_spec[j] + eps ) );
                }
                output_freq.push_back(freq);
                output_spectra.push_back(rebinned_spec[j]);
            }
        }
    }

    if(output_file != "")
    {
        //if the output_file is defined, dump the spectra to file
        std::ofstream ofile;
        ofile.open(output_file.c_str());
        ofile << "bin \t" << "frequency \t" << "power \t" << std::endl;
        for(unsigned int j=0; j<output_spectra.size(); j++)
        {
            ofile  << j<<" \t"<<output_freq[j]<<" \t"<<output_spectra[j]<<std::endl;
        }
        ofile.close();
    }
    

    g->Draw("ALP");
    g->SetMarkerStyle(7);
    g->SetTitle("Average Spectrum" );
    if(use_bin_numbers)
    {
        g->GetXaxis()->SetTitle("Bin Number");
    }
    else 
    {
        g->GetXaxis()->SetTitle("Frequency (MHz)");
    }
    g->GetYaxis()->SetTitle("Relative Power (dB)");
    g->GetHistogram()->SetMaximum(100.0);
    g->GetHistogram()->SetMinimum(0.0);
    g->GetYaxis()->CenterTitle();
    g->GetXaxis()->CenterTitle();
    pt->Draw();
    c->Update();

    App->Run();

    return 0;
}
