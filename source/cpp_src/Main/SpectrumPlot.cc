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

    if(have_meta_data)
    {
        std::cout<<"meta data file = "<< metaDataFile<<std::endl;
        std::ifstream metadata(metaDataFile.c_str());
        json j;
        metadata >> j;
        for (json::iterator it = j.begin(); it != j.end(); ++it) {
          std::cout << (*it)["measurement"] << '\n';
        }
    }


    for(auto it = specFiles.begin(); it != specFiles.end(); it++)
    {
        std::cout<<"spec file: "<< it->first << " @ " << it->second.first <<", "<< it->second.second<<std::endl;
    }

    for(auto it = powerFiles.begin(); it != powerFiles.end(); it++)
    {
        std::cout<<"npow file: "<< it->first << " @ " << it->second.first <<", "<< it->second.second<<std::endl;
    }

    //now open up all the spec data (hope it fits in memory)
    std::vector< HSpectrumFileStructWrapper< float > > spectrum_vec;
    //spectrum_vec.resize(specFiles.size());
    for(size_t i = 0; i < specFiles.size(); i++)
    {
        spectrum_vec.push_back( HSpectrumFileStructWrapper<float>(specFiles[i].first) ) ;
    }

/*


    //now open up all the spec data (hope it fits in memory)
    std::vector< HSpectrumObject<float> > spectrum_vec;
    for(size_t i=0; i<specFiles.size(); i++)
    {
        HSpectrumObject<float> obj;
        spectrum_vec.push_back(obj);
        spectrum_vec.back().ReadFromFile(specFiles[i].first);
        std::cout<<"spec file: "<<i<<" = "<<specFiles[i].first<< " has spectrum of length: "<<spectrum_vec.back().GetSpectrumLength()<<std::endl;
    }

    size_t acq_start = spectrum_vec[0].GetStartTime();
    std::time_t start_time = (std::time_t) acq_start;
    std::cout << "Acq start time = "<< std::asctime(std::gmtime(&start_time));


    //look at noise statistics data
    std::vector< HDataAccumulation > on_accum;
    std::vector< HDataAccumulation > off_accum;
    for(unsigned int i=0; i<spectrum_vec.size(); i++)
    {
        std::vector<HDataAccumulation>* vec = nullptr;

        vec = spectrum_vec[i].GetOnAccumulations();
        on_accum.insert( on_accum.end(), vec->begin(), vec->end() );

        vec = spectrum_vec[i].GetOffAccumulations();
        off_accum.insert( off_accum.end(), vec->begin(), vec->end() );




        //if(i < 10)
        {
            vec = spectrum_vec[i].GetOnAccumulations();

            std::cout<<"i = "<<i<<std::endl;
            std::cout<<"spec file leading index "<<spectrum_vec[i].GetLeadingSampleIndex()<<std::endl;
            std::cout<<"spec sideband: "<<spectrum_vec[i].GetSidebandFlag()<<std::endl;
            std::cout<<"spec pol: "<<spectrum_vec[i].GetPolarizationFlag()<<std::endl;

            for(unsigned int j=0; j<vec->size(); j++)
            {
                std::cout<<"j = "<<i<<std::endl;
                std::cout<<"on ["<<vec->at(j).start_index<<", "<<vec->at(j).stop_index<<"]"<<std::endl;
            }

            vec = spectrum_vec[i].GetOffAccumulations();


            for(unsigned int j=0; j<vec->size(); j++)
            {
                std::cout<<"j = "<<i<<std::endl;
                std::cout<<"off ["<<vec->at(j).start_index<<", "<<vec->at(j).stop_index<<"]"<<std::endl;
            }

        }

    }

    // //print out the first few on/off accumulation periods
    // for(unsigned int i=0; i<10; i++)
    // {
    //     std::cout<<"i = "<<i<<std::endl;
    //     std::cout<<"on ["<<on_accum[i].start_index<<", "<<on_accum[i].stop_index<<"]"<<std::endl;
    //     std::cout<<"off ["<<off_accum[i].start_index<<", "<<off_accum[i].stop_index<<"]"<<std::endl;
    // }

    double onx, onx2, onN;
    for(unsigned int i=0; i<on_accum.size(); i++)
    {
        onx += on_accum[i].sum_x;
        onx2 += on_accum[i].sum_x2;
        onN += on_accum[i].count;
    }

    double offx, offx2, offN;
    for(unsigned int i=0; i<off_accum.size(); i++)
    {
        offx += off_accum[i].sum_x;
        offx2 += off_accum[i].sum_x2;
        offN += off_accum[i].count;
    }


    std::cout<<"N on accum = "<<on_accum.size()<<std::endl;
    std::cout<<"N off accum = "<<off_accum.size()<<std::endl;

    std::cout<<"on x: "<<onx<<" on x2: "<<onx2<<" on count "<<onN<<std::endl;
    std::cout<<"off x: "<<offx<<" off x2: "<<offx2<<" off count "<<offN<<std::endl;

    double onrms = (onx2/onN) -(onx/onN)*(onx/onN);
    double offrms = (offx2/offN) -(offx/offN)*(offx/offN);

    double Tsys = std::fabs( offrms/(onrms - offrms) );
    std::cout<<"tsys in noise diode units (uncal.) = "<<Tsys<<std::endl;





*/



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
    c->cd();
    c->SetFillColor(0);
    c->SetRightMargin(0.2);

    // std::stringstream ss;
    // ss <<"Averaged spectrum, acquisition start time: ";
    // ss << std::asctime(std::gmtime(&start_time));



    std::cout<<"sample length = "<< spectrum_vec[0].GetSampleLength() <<std::endl;
    std::cout<<"n averages = "<<spectrum_vec[0].GetNAverages() << std::endl;
    double sample_rate = spectrum_vec[0].GetSampleRate();
    std::cout<<"sample rate = "<<sample_rate<<std::endl;

    double n_samples_per_spec = spectrum_vec[0].GetSampleLength() / spectrum_vec[0].GetNAverages();
    double spec_res = sample_rate / n_samples_per_spec;

    std::cout<<"n samples per spec = "<<n_samples_per_spec<<std::endl;
    std::cout<<"spec_res= "<<spec_res<<std::endl;

    for(unsigned int n=0; n < spectrum_vec.size(); n++)
    {
        TGraph* g = new TGraph();
        graph.push_back(g);
        float* spec_val = spectrum_vec[n].GetSpectrumData();
        uint64_t spec_length = spectrum_vec[n].GetSpectrumLength();
        for(unsigned int j=0; j<spec_length; j++)
        {
            g->SetPoint(j, j*spec_res/1e6, 10.0*std::log10( spec_val[j] + eps ) );
        }
        if(n == 0){g->Draw("ALP");}
        else{g->Draw("ALP SAME");}

        // g->SetTitle(ss.str().c_str());
        g->GetXaxis()->SetTitle("Frequency (MHz)");
        g->GetYaxis()->SetTitle("Relative Power (dB)");
        g->GetYaxis()->CenterTitle();
        g->GetXaxis()->CenterTitle();

        if(n == 0){g->Draw("ALP");}
        else{g->Draw("ALP SAME");}

        c->Update();
    }



    //
    // size_t start_sec = spectrum_vec[0].GetStartTime();
    // size_t now_sec = spectrum_vec.back().GetStartTime();
    //
    // //calculate the start time of this spectral averages
    // double sample_index = spectrum_vec.back().GetLeadingSampleIndex() + spectrum_vec.back().GetSampleLength();
    // double stime = sample_index / sample_rate;
    //
    // double time_diff = (double)(now_sec - start_sec) + stime;
    //
    //
    // std::cout<<"seconds difference = "<<(double)(now_sec - start_sec)<<std::endl;
    // std::cout<<"sub sec difference = "<<stime<<std::endl;
    // std::cout<<"total time from first to last sample (s): "<<time_diff<<std::endl;

/*


    std::string name2("spectrogram");
    TCanvas* c2 = new TCanvas(name2.c_str(),name2.c_str(), 50, 50, 950, 850);
    c2->cd();
    c2->SetFillColor(0);
    c2->SetRightMargin(0.2);


    //double last_sample_index = spectrum_vec.back().GetLeadingSampleIndex();// + spectrum_vec.back().GetSampleLength();
    //double last_stime = last_sample_index / sample_rate;
    //TH2D* g2d = new TH2D("h2","", 256, 0.0, last_stime+1, 256, 0, (spec_length-1)*spec_res/1e6 );


    TGraph2D* g2d = new TGraph2D();
    //g2d->SetTitle("Spectrogram; Count; Frequency (MHz); Power (dB)");
    size_t count = 0;
    for(unsigned int n=0; n < spectrum_vec.size(); n++)
    {
        //calculate the start time of this spectral average
        //w.r.t to the first

        size_t start_sec = spectrum_vec[0].GetStartTime();
        size_t now_sec = spectrum_vec[n].GetStartTime();

        //calculate the start time of this spectral averages
        double sample_index = spectrum_vec[n].GetLeadingSampleIndex();
        double stime = sample_index / sample_rate;

        double time_diff = (now_sec - start_sec) + stime;

        //std::cout<<"stime = "<<stime<<std::endl;
        float* spec_valn = spectrum_vec[n].GetSpectrumData();
        double smax = 0;
        for(unsigned int j=0; j<spectrum_vec[n].GetSpectrumLength()-1; j++)
        {
            if( std::log10( spec_valn[j] ) > smax){smax = 10.0*std::log10( spec_valn[j] + eps ); } ;
            //g2d->Fill(stime, j*spec_res/1e6, 10.0*std::log10(spec_valn[j] + 0.0001 )  );
            g2d->SetPoint(count++, time_diff, j*spec_res/1e6, 10.0*std::log10( spec_valn[j] + eps) );
        }
        //std::cout<<"smax = "<<smax<<std::endl;
    }

    g2d->SetNpx(100);
    g2d->SetNpy(100);
    g2d->Draw("COLZ");
    std::stringstream ss2;
    ss2 <<"Spectrogram, acquisition start time: ";
    ss2 << std::asctime(std::gmtime(&start_time));
    g2d->SetTitle(ss.str().c_str());
    g2d->GetHistogram()->GetYaxis()->SetTitle("Frequency (MHz)");
    g2d->GetHistogram()->GetYaxis()->CenterTitle();
    g2d->GetHistogram()->GetYaxis()->SetTitleOffset(1.4);
    g2d->GetHistogram()->GetZaxis()->SetTitle("Power (dB)");
    g2d->GetHistogram()->GetZaxis()->CenterTitle();
    g2d->GetHistogram()->GetZaxis()->SetTitleOffset(1.4);
    g2d->GetHistogram()->GetXaxis()->SetTitle("Time (s)");
    g2d->GetHistogram()->GetXaxis()->CenterTitle();
    g2d->GetHistogram()->GetXaxis()->SetTitleOffset(1.4);
    g2d->Draw("P0");

    c2->Update();

*/

    App->Run();

    return 0;
}
