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
#include <cmath>

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
#include "TMath.h"

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////




void
blackman_harris_coeff(unsigned int num, std::vector<double>& pOut, double& s1, double& s2)
{
    pOut.resize(num);
    double a0 = 0.35875f;
    double a1 = 0.48829f;
    double a2 = 0.14128f;
    double a3 = 0.01168f;

    s1 = 0;
    s2 = 0;

    unsigned int idx = 0;
    while( idx < num )
    {
        pOut[idx]   = a0 - (a1 * std::cos( (2.0f * M_PI * idx) / (num - 1) )) + (a2 * std::cos( (4.0f * M_PI * idx) / (num - 1) )) - (a3 * std::cos( (6.0f * M_PI * idx) / (num - 1) ));
        s1 += pOut[idx];
        s2 += pOut[idx]*pOut[idx];
        idx++;
    }
};



class MetaDataContainer
{
    public:
        MetaDataContainer(){};
        ~MetaDataContainer(){};

    public:

        double fSpectrumLength;
        double fNAverages;
        double fSampleRate;
        double fSamplePeriod;
        double fNSampleLength;
        double fNSamplesPerSpectrum;

        double fBinDelta;
        double fFrequencyDeltaMHz;
        double fReferenceBinIndex;
        double fReferenceBinCenterSkyFrequencyMHz;
        double fDuration;

        double fMedianOnVariance;
        double fMedianOffVariance;
        double fMeanOnVariance;
        double fMeanOffVariance;
        double fKFactor;

        std::string fRA;
        std::string fDEC;
        std::string fEpoch;
        std::string fSourceName;
        std::string fExperimentName;
        std::string fScanName;
        std::string fStartTime;
};


////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

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


////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

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

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////


bool ReadDataDirectory(std::string data_dir, bool toggle_diode,  MetaDataContainer& meta_data, std::vector<double>& freq_axis, std::vector<double>& average_spectrum )
{

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

    bool map_to_sky_frequency = false;
    bool source_info = false;
    bool recording_status_info = false;


    if(have_meta_data)
    {
        std::cout<<"meta data file = "<< metaDataFile<<std::endl;
        std::ifstream metadata(metaDataFile.c_str());
        json j;
        metadata >> j;

        for (json::iterator it = j.begin(); it != j.end(); ++it)
        {
            std::string measurement_name = (*it)["measurement"].get<std::string>();
            if( measurement_name == std::string("frequency_map") )
            {
                auto js2 = (*it)["fields"];
                meta_data.fBinDelta  = js2["bin_delta"].get<int>();
                meta_data.fFrequencyDeltaMHz = js2["frequency_delta_MHz"].get<double>();
                meta_data.fReferenceBinCenterSkyFrequencyMHz = js2["reference_bin_center_sky_frequency_MHz"].get<double>();
                meta_data.fReferenceBinIndex = js2["reference_bin_index"].get<int>();

                std::cout << "bin_delta = " << meta_data.fBinDelta << std::endl;
                std::cout << "frequency delta (MHZ) = "<< meta_data.fFrequencyDeltaMHz << std::endl;
                std::cout << "reference_bin_center_sky_frequency (MHz) " << meta_data.fReferenceBinCenterSkyFrequencyMHz  << std::endl;
                std::cout << "reference_bin_index =  " << meta_data.fReferenceBinIndex << std::endl;
                map_to_sky_frequency = true;
            }

            if( measurement_name == std::string("source_status") )
            {
                auto js2 = (*it)["fields"];
                meta_data.fSourceName = js2["source"].get<std::string>();
                meta_data.fRA = js2["ra"].get<std::string>();
                meta_data.fDEC = js2["dec"].get<std::string>();
                meta_data.fEpoch = js2["epoch"].get<std::string>();
                source_info = true;
            }

            if( measurement_name == std::string("recording_status") )
            {
                auto js2 = (*it)["fields"];
                std::string recording_state = js2["recording"].get<std::string>();
                if(recording_state == std::string("on"))
                {
                    meta_data.fExperimentName = js2["experiment_name"].get<std::string>();
                    meta_data.fScanName = js2["scan_name"].get<std::string>();
                    std::string tmptime = (*it)["time"].get<std::string>();
                    meta_data.fStartTime = tmptime.substr(0, tmptime.find("."));
                    recording_status_info = true;
                }
            }
        }
    }


    //if(!map_to_sky_frequency || ! source_info || !recording_status_info)
    //{
    //    std::cout<<"Error: missing meta-data!"<<std::endl;
    //    return false;
    //}

////////////////////////////////////////////////////////////////////////////////
//compute an averaged spectrum

    //space to accumulate the raw spectrum

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
            meta_data.fDuration = time_diff;
            average_spectrum.resize(spec_length, 0);
        }

        float* spec_data = tempFile.GetSpectrumData();
        for(size_t j=0; j<spec_length; j++)
        {
             average_spectrum[j] += spec_data[j]; //here spec_data is actually the average of 32 complete'gpu-spectra', where gpu-spectra are accumulations of 16 FFT's worth of data
        }
        spec_count += 1.0;
    }

    //compute the average over all the spectra
    //TODO FIXME...this is because of the 1st pass accumulation on the GPU, (no averaging/normalization is done)
    //HARD_CODED, number of accumulations we didn't normalize for in GPU code:
    double gpu_ave_factor = 16.0;
    for(size_t j=0; j<spec_length; j++)
    {
         average_spectrum[j] *= (1.0/gpu_ave_factor)*(1.0/spec_count);
    }

    //export some meta data
    meta_data.fSpectrumLength = spec_length;
    meta_data.fNAverages = n_ave;
    meta_data.fSampleRate = sample_rate;
    meta_data.fSamplePeriod = sample_period;
    meta_data.fNSampleLength = n_samples;
    meta_data.fNSamplesPerSpectrum = n_samples_per_spec;
    meta_data.fSamplePeriod = sample_period;

    //zero-out the first and last few bins
    for(size_t k=0; k<10; k++)
    {
        average_spectrum[k] = 0.0;
        average_spectrum[spec_length-1-k] = 0.0;
    }

    //compute the frequency axis
    freq_axis.resize(spec_length);
    for(unsigned int j=0; j<spec_length; j++)
    {
        double index = j;
        double freq = (index - meta_data.fReferenceBinIndex)*meta_data.fFrequencyDeltaMHz + meta_data.fReferenceBinCenterSkyFrequencyMHz;
        freq_axis[j] = freq;
    }

    //now normalize for the combined effect of the FFT/blackman_harris window
    ////////////////////////////////////////////////////////////////////////////////
    //calculate the missing weight factors from the blackman_harris window

    std::vector<double> bh_coeff;
    double s1;
    double s2;
    blackman_harris_coeff(meta_data.fNSamplesPerSpectrum, bh_coeff, s1, s2);
    std::cout<<"samples per spec "<<meta_data.fNSamplesPerSpectrum<<std::endl;
    std::cout<<"blackman-harris scale factors: s1, s2 = "<<s1<<", "<<s2<<std::endl;
    std::cout<<"NENBW = N*(s2/(s1*s1)) = "<<meta_data.fNSamplesPerSpectrum*(s2/(s1*s1))<<std::endl;
    std::cout<<"ENBW = NENBW*f_res = "<< (meta_data.fSampleRate)*(s2/(s1*s1))<<std::endl;

    //this normalizes for the FFT, to give the power spectrum
    for(size_t j=0; j<spec_length; j++)
    {
         average_spectrum[j] *= 2.0/(s1*s1);
    }

////////////////////////////////////////////////////////////////////////////////
//collect the noise diode data

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

            bool diode_state_is_on = false;
            if(accum_dat.state_flag == H_NOISE_DIODE_ON)
            {
                diode_state_is_on = true;
                if(toggle_diode)
                {
                    diode_state_is_on = false;
                }

            }
            else
            {
                diode_state_is_on = false;
                if(toggle_diode)
                {
                    diode_state_is_on = true;
                }
            }


            if(diode_state_is_on)
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

    std::vector<double> on_var_vec;
    std::vector<double> off_var_vec;

    for(size_t j=0; j<fOnVarianceTimePairs.size(); j++)
    {
        on_var_mean += fOnVarianceTimePairs[j].first;
        on_var_vec.push_back(fOnVarianceTimePairs[j].first);
    }
    on_var_mean /= (double)fOnVarianceTimePairs.size();

    for(size_t j=0; j<fOffVarianceTimePairs.size(); j++)
    {
        off_var_mean += fOffVarianceTimePairs[j].first;
        off_var_vec.push_back(fOffVarianceTimePairs[j].first);
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

    double on_var_median = TMath::Median(on_var_vec.size(), &(on_var_vec[0]));
    double off_var_median = TMath::Median(off_var_vec.size(), &(off_var_vec[0]));


    std::cout<<"Average Proportionality constant k = (T/T_diode) =  OFF/(ON-OFF): "<< off_var_median/(on_var_median - off_var_median)<<std::endl;

    meta_data.fMedianOnVariance = on_var_median;
    meta_data.fMedianOffVariance = off_var_median;
    meta_data.fMeanOnVariance = on_var_mean;
    meta_data.fMeanOffVariance = off_var_mean;

    meta_data.fKFactor = off_var_median/(on_var_median - off_var_median);




    return true;

}


////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////




int main(int argc, char** argv)
{
    std::string usage =
    "\n"
    "Usage: CrudeOnMinusOff <options> <directory>\n"
    "\n"
    "Plot on-off spectrum from pair of scans.\n"
    "\tOptions:\n"
    "\t -h, --help               (shows this message and exits)\n"
    "\t -t, --togggle-on-off     (swap the diode state labels on <-> off)\n"
    "\t -o, --on-source-data-dir (path to the directory containing on-source scan data, mandatory)\n"
    "\t -f, --off-data-dir       (path to the directory containing off-source scan data, mandatory)\n"
    ;

    //set defaults
    unsigned int n_averages = 0; //default behavior is to average all spectra together
    bool have_on_data = false;
    bool have_off_data = false;
    std::string on_source_data_dir = STR2(DATA_INSTALL_DIR);
    std::string off_source_data_dir = STR2(DATA_INSTALL_DIR);
    std::string o_dir = STR2(DATA_INSTALL_DIR);

    static struct option longOptions[] =
    {
        {"help", no_argument, 0, 'h'},
        {"on-source-data-dir", required_argument, 0, 'o'},
        {"off-source-data-dir", required_argument, 0, 'f'},
        {"togggle-on-off", no_argument, 0, 't'}
    };

    static const char *optString = "hto:f:";

    bool togggle_on_off = false;
    while(1)
    {
        char optId = getopt_long(argc, argv, optString, longOptions, NULL);
        if(optId == -1) break;
        switch(optId)
        {
            case('h'): // help
            std::cout<<usage<<std::endl;
            return 0;
            case('o'):
                on_source_data_dir = std::string(optarg);
                have_on_data = true;
            break;
            case('f'):
                off_source_data_dir = std::string(optarg);
                have_off_data = true;
            break;
            case('t'):
                togggle_on_off = true;
            break;
            default:
                std::cout<<usage<<std::endl;
            return 1;
        }
    }


    if(!have_on_data || !have_off_data)
    {
        std::cout<<"On/Off data directory arguments is mandatory."<<std::endl;
        std::cout<<usage<<std::endl;
        return 1;
    }


    MetaDataContainer on_source_meta;
    std::vector<double> on_source_freq;
    std::vector<double> on_source_spectrum;

    bool have_on_source_data =  ReadDataDirectory(on_source_data_dir, togggle_on_off, on_source_meta, on_source_freq, on_source_spectrum);

    MetaDataContainer off_source_meta;
    std::vector<double> off_source_freq;
    std::vector<double> off_source_spectrum;

    bool have_off_source_data =  ReadDataDirectory(off_source_data_dir, togggle_on_off, off_source_meta, off_source_freq, off_source_spectrum);


    if(!have_on_source_data || !have_off_source_data)
    {
        std::cout<<"Missing data!"<<std::endl;
        return 0;
    }


    std::cout<<"read the data"<<std::endl;


////////////////////////////////////////////////////////////////////////////////

    std::cout<<"starting root plotting"<<std::endl;

    //ROOT stuff for plots
    TApplication* App = new TApplication("CrudeOnMinusOff",&argc,argv);
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

    TGraph* g = new TGraph();

    TPaveText *pt = new TPaveText(.14,.72,.29,.88,"blNDC");
    pt->SetBorderSize(1);
    pt->SetFillColor(0);
    pt->AddText( std::string( std::string( "Experiment: ") + on_source_meta.fExperimentName).c_str() );
    pt->AddText( std::string( std::string( "Scan: ") + on_source_meta.fScanName).c_str() );
    pt->AddText( std::string( std::string( "Source: ") + on_source_meta.fSourceName ).c_str() );
    pt->AddText( std::string( std::string( "RA: ") + on_source_meta.fRA).c_str() );
    pt->AddText( std::string( std::string( "DEC: ") + on_source_meta.fDEC).c_str() );
    pt->AddText( std::string( std::string( "Epoch: ") + on_source_meta.fEpoch).c_str() );
    pt->AddText( std::string( std::string( "Start Time: ") + on_source_meta.fStartTime).c_str() );

    //make a crude calculation to scale the y-axis
    double t_diode = 1.25;
    double SEFD = 2250; //assumed SEFD for band C in Jansky's (Jy)
    double t_sys_on = on_source_meta.fKFactor*t_diode;
    double t_sys_off = off_source_meta.fKFactor*t_diode;

    std::cout<<"t_sys_on = "<<t_sys_on<<std::endl;
    std::cout<<"t_sys_off = "<<t_sys_off<<std::endl;

    double gain_on = t_sys_on/SEFD;
    double gain_off = t_sys_off/SEFD;

    double MHz_to_Hz = 1e6;
    double on_integral = 0.0;
    double off_integral = 0.0;

    double input_ohms = 50.0;

    for(unsigned int j=0; j<on_source_spectrum.size(); j++)
    {
        double index = j;
        double freq = on_source_freq[j];
        double on_source_val = on_source_spectrum[j];
        double off_source_val = off_source_spectrum[j];
        on_integral += TMath::Abs(on_source_meta.fFrequencyDeltaMHz)*MHz_to_Hz*on_source_val;
        off_integral += TMath::Abs(off_source_meta.fFrequencyDeltaMHz)*MHz_to_Hz*off_source_val;
    }

    std::cout<<"on_integral = "<<on_integral<<std::endl;
    std::cout<<"off_integral = "<<off_integral<<std::endl;

    double on_src_power_per_Hz = on_integral*( 1.0/(on_source_meta.fSampleRate/2.0) );
    double off_src_power_per_Hz = off_integral*( 1.0/(off_source_meta.fSampleRate/2.0) );
    double on_norm = (t_sys_on/on_src_power_per_Hz);
    double off_norm = (t_sys_off/off_src_power_per_Hz);

    for(unsigned int j=0; j<on_source_spectrum.size(); j++)
    {
        double index = j;
        double freq = on_source_freq[j];
        double on_source_val = on_source_spectrum[j];//*on_norm/gain_on;
        double off_source_val = off_source_spectrum[j];//*off_norm/gain_off;
        double diff = (on_source_val - off_source_val);
        double scaled_point = diff;
        double point = scaled_point;
        g->SetPoint(j, freq, point );
    }


    g->Draw("ALP");
    g->SetMarkerStyle(7);
    g->SetTitle("Average On-Off Spectrum" );
    g->GetXaxis()->SetTitle("Frequency (MHz)");
    g->GetYaxis()->SetTitle("Source Flux (Jy)");
    g->GetYaxis()->CenterTitle();
    g->GetXaxis()->CenterTitle();
    g->GetYaxis()->SetTitleOffset(1.1);

    pt->Draw();

    c->Update();

    App->Run();




    return 0;
}
