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


bool ReadDataDirectory
(
    std::string data_dir,
    MetaDataContainer& meta_data,
    std::vector<double>& freq_axis,
    std::vector<double>& average_spectrum
)
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


    // if(!map_to_sky_frequency || ! source_info || !recording_status_info)
    // {
    //     std::cout<<"Error: missing meta-data!"<<std::endl;
    //     return false;
    // }

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

    return true;

}







void RebinSpectra(bool toggle,
                  double desired_spec_res, 
                  std::vector<double>& rebinned_spec, 
                  std::vector<double>& rebinned_freq_axis, 
                  std::vector<double>& raw_accumulated_spec,
                  std::vector<double>& raw_freq_axis)
{
    rebinned_spec.clear();
    rebinned_freq_axis.clear();
    size_t spec_length = raw_accumulated_spec.size();
    //zero-out the first and last few bins to get rid of DC
    int bin_mask = std::max(4, (int) ( 0.0001*(double)spec_length) );
    for(size_t k=0; k<bin_mask; k++)
    {
        raw_accumulated_spec[k] = 0.0;
        raw_accumulated_spec[spec_length-1-k] = 0.0;
    }

    //determine the current spectral resolution from bin delta

    double spec_res = std::fabs(raw_freq_axis[1] - raw_freq_axis[0]);

    int n_to_merge = 1; //only if toggle is true do we actually do any rebinning
    if(toggle){ n_to_merge = std::max(1, (int)std::floor(desired_spec_res/spec_res) ); }
    std::cout<<"Rebinning frequency axis. Desired resolution: "<<desired_spec_res<<", native resolution: "<<spec_res<<std::endl;
    std::cout<<"Number of bins to merge: "<<n_to_merge<<", new spectral resolution: "<<n_to_merge*spec_res<<std::endl;
    size_t new_spec_length = spec_length/n_to_merge;
    rebinned_spec.resize(new_spec_length, 0);
    rebinned_freq_axis.resize(new_spec_length, 0);
    for(size_t j=0; j<new_spec_length; j++)
    {
        rebinned_spec[j] = 0;
        rebinned_freq_axis[j] = 0;
        for(size_t k=0; k<n_to_merge; k++)
        {
            rebinned_spec[j] += raw_accumulated_spec[j*n_to_merge + k];
            rebinned_freq_axis[j] += raw_freq_axis[j*n_to_merge + k];
        }
        rebinned_spec[j] /= (double)n_to_merge;
        rebinned_freq_axis[j] /= (double)n_to_merge;
    }
}
































////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////



int main(int argc, char** argv)
{
    std::string usage =
    "\n"
    "Usage: OnMinusOff <options> <directory>\n"
    "\n"
    "Plot spectrum from scan data in the <directory>.\n"
    "\tOptions:\n"
    "\t -h, --help                (shows this message and exits)\n"
    "\t -o, --on-source-data-dir  (path to the directory containing hot-load scan data, mandatory)\n"
    "\t -f, --off-source-data-dir (path to the directory containing cold-load scan data, mandatory)\n"
    "\t -r, --resolution          (desired spectral resolution in MHz, if greater than the native spectral resolution, frequency bins will be averaged together)\n"
    ;

    //set defaults
    bool have_data = false;
    bool rebin = false;
    double desired_spec_res = 1.0;
    std::string on_src_data_dir;
    std::string off_src_data_dir;
    bool have_on_src_data = false;
    bool have_off_src_data = false;

    double sampling_rate = 1250*1e6;

    static struct option longOptions[] =
    {
        {"help", no_argument, 0, 'h'},
        {"on-source-data-dir", required_argument, 0, 'o'},
        {"off-source-data-dir", required_argument, 0, 'f'},
        {"resolution", required_argument, 0, 'r'}
    };

    static const char *optString = "ho:f:r:";

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
                on_src_data_dir = std::string(optarg);
                have_on_src_data = true;
            break;
            case('f'):
                off_src_data_dir = std::string(optarg);
                have_off_src_data = true;
            break;
            case('r'):
                desired_spec_res = 1e6*atof(optarg);
                rebin = true;
            break;
            default:
                std::cout<<usage<<std::endl;
            return 1;
        }
    }

    if(!have_off_src_data || !have_on_src_data)
    {
        std::cout<<"Data directory arguments are mandatory."<<std::endl;
        std::cout<<usage<<std::endl;
        return 1;
    }

    std::cout<<"sampling rate is: "<<sampling_rate<<std::endl;

    MetaDataContainer on_src_meta;
    std::vector<double> on_src_freq;
    std::vector<double> on_src_spectrum;
    have_on_src_data =  ReadDataDirectory(on_src_data_dir, on_src_meta, on_src_freq, on_src_spectrum);

    MetaDataContainer off_src_meta;
    std::vector<double> off_src_freq;
    std::vector<double> off_src_spectrum;
    have_off_src_data =  ReadDataDirectory(off_src_data_dir, off_src_meta, off_src_freq, off_src_spectrum);


    if(!have_on_src_data || !have_off_src_data)
    {
        std::cout<<"Missing data!"<<std::endl;
        return 0;
    }


    std::cout<<"finished reading data"<<std::endl;

    if( off_src_freq.size() != on_src_freq.size())
    {
        std::cout<<"Error, spectrum size mismatch!"<<std::endl;
        return 1;
    }


    std::vector<double> bh_coeff;
    double s1;
    double s2;
    double input_ohms = 50.0;

    blackman_harris_coeff(on_src_meta.fNSamplesPerSpectrum, bh_coeff, s1, s2);
    std::cout<<"samples per spec "<<on_src_meta.fNSamplesPerSpectrum<<std::endl;
    std::cout<<"blackman-harris scale factors: s1, s2 = "<<s1<<", "<<s2<<std::endl;
    std::cout<<"NENBW = N*(s2/(s1*s1)) = "<<on_src_meta.fNSamplesPerSpectrum*(s2/(s1*s1))<<std::endl;
    std::cout<<"ENBW = NENBW*f_res = "<< (on_src_meta.fSampleRate)*(s2/(s1*s1))<<std::endl;
    double enbw = (on_src_meta.fSampleRate)*(s2/(s1*s1));


    //now calculate the on,off, and differenced spectrum in W/Hz units
    //also plot the (ON-OFF)/OFF relative difference

    std::vector<double> norm_on_src_spec; //W/Hz
    std::vector<double> norm_off_src_spec; //W/Hz


    for(unsigned int j=0; j<on_src_freq.size(); j++)
    {
        double on_src = (on_src_spectrum[j]*enbw)/input_ohms;
        double off_src = (off_src_spectrum[j]*enbw)/input_ohms;
        norm_on_src_spec.push_back(on_src);
        norm_off_src_spec.push_back(off_src);
        // double diff = on_src - off_src;
        // double rel_diff = diff/off_src;
        // norm_diff_spec.push_back(diff);
        // relative_diff_spec.push_back(rel_diff);
    }

    std::vector<double> rebinned_on_src_spec;
    std::vector<double> rebinned_off_src_spec;
    std::vector<double> rebinned_on_freq_axis;
    std::vector<double> rebinned_off_freq_axis;

    //now we can re-bin the spectra if a different spectral resolution is desired 
    RebinSpectra(rebin, desired_spec_res, rebinned_on_src_spec, rebinned_on_freq_axis, norm_on_src_spec, on_src_freq);
    RebinSpectra(rebin, desired_spec_res, rebinned_off_src_spec, rebinned_off_freq_axis, norm_off_src_spec, off_src_freq);

    std::vector<double> norm_diff_spec; //W/Hz
    std::vector<double> relative_diff_spec; //W/Hz
    for(unsigned int j=0; j<rebinned_on_freq_axis.size(); j++)
    {
        double on_src = rebinned_on_src_spec[j];
        double off_src = rebinned_off_src_spec[j];
        double diff = on_src - off_src;
        double rel_diff = diff/off_src;
        if( std::fabs(off_src) < 1e-15*diff){rel_diff = 0.0;}
        norm_diff_spec.push_back(diff);
        relative_diff_spec.push_back(rel_diff);
    }


    ////////////////////////////////////////////////////////////////////////////////

    std::cout<<"starting root plotting"<<std::endl;

    //ROOT stuff for plots

    int fake_argc = 1;
    char fake_argv[]="Tsys";
    char* tmp = &(fake_argv[0]);

    TApplication* App = new TApplication("OnMinusOff",&fake_argc,&tmp);
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
    c->Divide(2,2);

    c->cd(1);
    TGraph* g1 = new TGraph();
    unsigned int count=0;
    for(unsigned int j=0; j<rebinned_on_freq_axis.size(); j++)
    {
        g1->SetPoint(count, rebinned_on_freq_axis[j], rebinned_on_src_spec[j] );
        count++;
    }
    g1->Draw("ALP");
    g1->SetMarkerStyle(7);
    g1->SetTitle("ON" );
    g1->GetXaxis()->SetTitle("Frequency (MHz)");
    g1->GetYaxis()->SetTitle("ON   Power spectral density W/Hz");
    // g1->GetHistogram()->SetMaximum(100.0);
    // g1->GetHistogram()->SetMinimum(0.0);
    g1->GetYaxis()->CenterTitle();
    g1->GetXaxis()->CenterTitle();

    c->cd(2);
    TGraph* g2 = new TGraph();
    count=0;
    for(unsigned int j=0; j<rebinned_off_freq_axis.size(); j++)
    {
        g2->SetPoint(count, rebinned_off_freq_axis[j], rebinned_off_src_spec[j] );
        count++;
    }
    g2->Draw("ALP");
    g2->SetMarkerStyle(7);
    g2->SetTitle("OFF" );
    g2->GetXaxis()->SetTitle("Frequency (MHz)");
    g2->GetYaxis()->SetTitle("OFF   Power spectral density W/Hz");
    // g1->GetHistogram()->SetMaximum(100.0);
    // g1->GetHistogram()->SetMinimum(0.0);
    g2->GetYaxis()->CenterTitle();
    g2->GetXaxis()->CenterTitle();

    c->cd(3);
    TGraph* g3 = new TGraph();
    count=0;
    for(unsigned int j=0; j<rebinned_off_freq_axis.size(); j++)
    {
        g3->SetPoint(count, rebinned_off_freq_axis[j], norm_diff_spec[j] );
        count++;
    }
    g3->Draw("ALP");
    g3->SetMarkerStyle(7);
    g3->SetTitle("ON-OFF" );
    g3->GetXaxis()->SetTitle("Frequency (MHz)");
    g3->GetYaxis()->SetTitle("ON-OFF   Power spectral density W/Hz");
    // g1->GetHistogram()->SetMaximum(100.0);
    // g1->GetHistogram()->SetMinimum(0.0);
    g3->GetYaxis()->CenterTitle();
    g3->GetXaxis()->CenterTitle();


    c->cd(4);
    TGraph* g4 = new TGraph();
    count=0;
    for(unsigned int j=0; j<rebinned_off_freq_axis.size(); j++)
    {
        g4->SetPoint(count,  rebinned_off_freq_axis[j], relative_diff_spec[j] );
        count++;
    }
    g4->Draw("ALP");
    g4->SetMarkerStyle(7);
    g4->SetTitle("(ON-OFF)/OFF" );
    g4->GetXaxis()->SetTitle("Frequency (MHz)");
    g4->GetYaxis()->SetTitle("Ratio (ON-OFF)/OFF");
    // g1->GetHistogram()->SetMaximum(100.0);
    // g1->GetHistogram()->SetMinimum(0.0);
    g4->GetYaxis()->CenterTitle();
    g4->GetXaxis()->CenterTitle();

    // TPaveText *pt = new TPaveText(.14,.72,.29,.88,"blNDC");
    // pt->SetBorderSize(1);
    // pt->SetFillColor(0);
    // pt->AddText( std::string( std::string( "Experiment: ") + experiment_name).c_str() );
    // pt->AddText( std::string( std::string( "Scan: ") + scan_name).c_str() );
    // pt->AddText( std::string( std::string( "Source: ") + source_name ).c_str() );
    // pt->AddText( std::string( std::string( "RA: ") + ra).c_str() );
    // pt->AddText( std::string( std::string( "DEC: ") + dec).c_str() );
    // pt->AddText( std::string( std::string( "Epoch: ") + epoch).c_str() );
    // pt->AddText( std::string( std::string( "Start Time: ") + start_time).c_str() );
    // pt->AddText( std::string( std::string( "Duration: ") + duration).c_str() );


    c->Update();


    App->Run();


    return 0;
}
