#include <dirent.h>
#include <vector>
#include <string>
#include <iostream>
#include <algorithm>
#include <utility>
#include <stdint.h>
#include <sstream>
#include <ctime>

#include "HSpectrumObject.hh"

#include "TCanvas.h"
#include "TApplication.h"
#include "TStyle.h"
#include "TColor.h"
#include "TGraph.h"
#include "TGraph2D.h"
#include "TH2D.h"

using namespace hose;

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


int main(int argc, char** argv)
{
    std::string usage =
    "\n"
    "Usage: TestSpectrumPlot <data_dir>\n"
    "\n"
    "Compute the average spectrum of the files stored in data_dir\n";

    if(argc != 2)
    {
        std::cout<<usage<<std::endl;
        return 1;
    }


    std::vector< std::string > allFiles;
    std::vector< std::pair< std::string, std::pair< uint64_t, uint64_t > > > specFiles;

    std::string data_dir(argv[1]);

    //look for spectrum files in this directory
    DIR *dpdf;
    struct dirent* epdf = NULL;
    dpdf = opendir(data_dir.c_str());
    if (dpdf != NULL)
    {
        do
        {
            epdf = readdir(dpdf);
            //std::cout << epdf->d_name << std::endl;
            if(epdf != NULL)
            {
                allFiles.push_back( data_dir  + std::string(epdf->d_name) );
            }
        }
        while(epdf != NULL);
    }
    closedir(dpdf);

    //strip everything that isnt a bin file
    for(auto it = allFiles.begin(); it != allFiles.end(); it++)
    {
        std::cout<< *it <<std::endl;
        std::string fname =  it->substr(it->find_last_of("\\/")+1, it->size() - 1);
        std::cout<<"fname = "<<fname<<std::endl;
        size_t fext_location = fname.find( std::string(".bin") );
        std::cout<<"fext location = "<<fext_location<<std::endl;
        if( fext_location != std::string::npos)
        {
            size_t underscore_location = fname.find(std::string("_"));
            //std::cout<<"fext_location = "<<fext_location<<std::endl;
            //std::cout<<"underscore_location = "<<underscore_location<<std::endl;

            //strip out the leading integer
            std::string acq_second = fname.substr(0, underscore_location);
            std::string n_samples = fname.substr(underscore_location+1, fname.size() - 1 );
            n_samples = n_samples.substr(0, n_samples.size() - 4 );

            //std::cout<<"acq sec = "<<acq_second<<std::endl;
            //std::cout<<"n samp = "<<n_samples<<std::endl;

            std::stringstream ss1;
            ss1 << acq_second;
            uint64_t val_sec;
            ss1 >> val_sec;
            std::stringstream ss2;
            ss2 << n_samples;
            uint64_t val_ns;
            ss2 >> val_ns;
            std::cout<<"adding file at: sec:"<<val_sec<<" and ns: "<<val_ns<<std::endl;
            specFiles.push_back( std::pair< std::string, std::pair<uint64_t, uint64_t > >(*it, std::pair< uint64_t, uint64_t >(val_sec, val_ns ) ) ); 
        }
    }
    
    if(specFiles.size() == 0)
    {
        std::cout<<"Error, could not find any files"<<std::endl;
        return 1;
    }

    // //now sort the spec files by time stamp (filename)
    std::sort( specFiles.begin(), specFiles.end() , less_than_spec() );

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
        std::vector<HDataAccumulation>* vec = spectrum_vec[i].GetOnAccumulations();
        on_accum.insert( on_accum.end(), vec->begin(), vec->end() );

        vec = spectrum_vec[i].GetOffAccumulations();
        off_accum.insert( off_accum.end(), vec->begin(), vec->end() );
    }

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

    std::cout<<"on x: "<<onx<<" on x2: "<<onx2<<" on count "<<onN<<std::endl;
    std::cout<<"off x: "<<offx<<" off x2: "<<offx2<<" off count "<<offN<<std::endl;

    double onrms = (onx2/onN) -(onx/onN)*(onx/onN);
    double offrms = (offx2/offN) -(offx/offN)*(offx/offN);

    double Tsys = std::fabs( offrms/(onrms - offrms) );
    std::cout<<"tsys in noise diode units (uncal.) = "<<Tsys<<std::endl;






    
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

    TGraph* g = new TGraph();
    std::stringstream ss;
    ss <<"Averaged spectrum, acquisition start time: ";
    ss << std::asctime(std::gmtime(&start_time));
    graph.push_back(g);

    double n_samples_per_spec = spectrum_vec[0].GetSampleLength() / spectrum_vec[0].GetNAverages();
    double sample_rate = spectrum_vec[0].GetSampleRate();    
    double spec_res = sample_rate / n_samples_per_spec; 

    std::cout<<"sample length = "<< spectrum_vec[0].GetSampleLength() <<std::endl;
    std::cout<<"n averages = "<<spectrum_vec[0].GetNAverages() << std::endl;
    std::cout<<"n samples per spec = "<<n_samples_per_spec<<std::endl;
    std::cout<<"sample rate = "<<sample_rate<<std::endl;
    std::cout<<"spec_res= "<<spec_res<<std::endl;

    size_t spec_length = spectrum_vec[0].GetSpectrumLength();

    std::vector<double> avespec;
    avespec.resize(spec_length,0);
    for(unsigned int n=0; n < spectrum_vec.size(); n++)
    {
        float* spec_val = spectrum_vec[n].GetSpectrumData();
        for(unsigned int j=0; j<spec_length; j++)
        {
            avespec[j] += spec_val[j];
        }
    }

    for(unsigned int j=0; j<spec_length-1; j++)
    {
        avespec[j] /= (double) spectrum_vec.size();
    }
    

    for(unsigned int n=0; n < spectrum_vec.size(); n++)
    {
        for(unsigned int j=0; j<spec_length-1; j++)
        {
            g->SetPoint(j, j*spec_res/1e6, 10.0*std::log10( avespec[j] + eps ) );
        }
    }
    
    g->Draw("ALP");
    c->Update();

    g->SetTitle(ss.str().c_str());
    g->GetXaxis()->SetTitle("Frequency (MHz)");
    g->GetYaxis()->SetTitle("Relative Power (dB)");
    g->GetYaxis()->CenterTitle();
    g->GetXaxis()->CenterTitle();

    g->Draw("ALP");
    c->Update();


    
    size_t start_sec = spectrum_vec[0].GetStartTime();
    size_t now_sec = spectrum_vec.back().GetStartTime();

    //calculate the start time of this spectral averages
    double sample_index = spectrum_vec.back().GetLeadingSampleIndex() + spectrum_vec.back().GetSampleLength();
    double stime = sample_index / sample_rate;

    double time_diff = (double)(now_sec - start_sec) + stime;


    std::cout<<"seconds difference = "<<(double)(now_sec - start_sec)<<std::endl;
    std::cout<<"sub sec difference = "<<stime<<std::endl;
    std::cout<<"total time from first to last sample (s): "<<time_diff<<std::endl;

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
}
