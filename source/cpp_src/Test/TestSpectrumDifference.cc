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

double eps = 1e-4;

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
    "Usage: TestSpectrumDifference <dirA> <dirB>\n"
    "\n"
    "Compute the difference of the average spectrum (A-B) stored in dirA and dirB.\n";

    if(argc != 3)
    {
        std::cout<<usage<<std::endl;
        return 1;
    }

    std::vector< std::vector< std::string > > allFiles; allFiles.resize(2);
    std::vector< std::vector< std::pair< std::string, std::pair< uint64_t, uint64_t > > >  > specFiles; specFiles.resize(2);
    std::vector< std::vector< HSpectrumObject<float> > > spectrum_vec; spectrum_vec.resize(2);
    std::vector< std::vector< double > > avespec; avespec.resize(2);
    double spec_res = 1;

    std::vector< std::string > dir; dir.resize(2);
    dir[0] = std::string(argv[1]);
    dir[1] = std::string(argv[2]);

    for(unsigned int d=0; d<2; d++)
    {
        //look for spectrum files in dirA
        DIR *dpdf;
        struct dirent *epdf;
        dpdf = opendir(dir[d].c_str());
        if (dpdf != NULL)
        {
            while (epdf = readdir(dpdf))
            {
                allFiles[d].push_back( dir[d] + "/" + std::string(epdf->d_name) );
            }
        }
        closedir(dpdf);

        //strip everything that isnt a bin file
        for(auto it = allFiles[d].begin(); it != allFiles[d].end(); it++)
        {
            //std::cout<< *it <<std::endl;
            size_t fext_location = it->find( std::string(".bin") );
            if( fext_location != std::string::npos)
            {
                size_t underscore_location = it->find(std::string("_"));
                //strip out the leading integer
                std::string acq_second = it->substr(0, underscore_location);
                std::string n_samples = it->substr(underscore_location, fext_location);
                std::stringstream ss1;
                ss1 << acq_second;
                uint64_t val_sec;
                ss1 >> val_sec;
                std::stringstream ss2;
                ss2 << n_samples;
                uint64_t val_ns;
                ss2 >> val_ns;
                specFiles[d].push_back( std::pair< std::string, std::pair<uint64_t, uint64_t > >(*it, std::pair< uint64_t, uint64_t >(val_sec, val_ns ) ) ); 
            }
        }
        

        if(specFiles[d].size() == 0)
        {
            std::cout<<"Error, could not find any files in directory: "<<dir[d]<<std::endl;
            return 1;
        }

        // //now sort the spec files by time stamp (filename)
        std::sort( specFiles[d].begin(), specFiles[d].end() , less_than_spec() );

        //now open up all the spec data (hope it fits in memory)
        for(size_t i=0; i<specFiles[d].size(); i++)
        {
            HSpectrumObject<float> obj;
            spectrum_vec[d].push_back(obj);
            spectrum_vec[d].back().ReadFromFile(specFiles[d][i].first);
            std::cout<<"spec file: "<<i<<" = "<<specFiles[d][i].first<< " has spectrum of length: "<<spectrum_vec[d].back().GetSpectrumLength()<<std::endl;
        }

        size_t acq_start = spectrum_vec[d][0].GetStartTime();
        std::time_t start_time = (std::time_t) acq_start;
        std::cout << "Acq start time = "<< std::asctime(std::gmtime(&start_time));

        double n_samples_per_spec = spectrum_vec[d][0].GetSampleLength() / spectrum_vec[d][0].GetNAverages();
        double sample_rate = spectrum_vec[d][0].GetSampleRate();    
        spec_res = sample_rate / n_samples_per_spec; 

        std::cout<<"sample length = "<< spectrum_vec[d][0].GetSampleLength() <<std::endl;
        std::cout<<"n averages = "<<spectrum_vec[d][0].GetNAverages() << std::endl;
        std::cout<<"n samples per spec = "<<n_samples_per_spec<<std::endl;
        std::cout<<"sample rate = "<<sample_rate<<std::endl;
        std::cout<<"spec_res= "<<spec_res<<std::endl;

        size_t spec_length = spectrum_vec[d][0].GetSpectrumLength();

        avespec[d].resize(spec_length,0);
        for(unsigned int n=0; n < spectrum_vec[d].size(); n++)
        {
            float* spec_val = spectrum_vec[d][n].GetSpectrumData();
            for(unsigned int j=0; j<spec_length; j++)
            {
                avespec[d][j] += spec_val[j];
            }
        }

        for(unsigned int j=0; j<spec_length-1; j++)
        {
            avespec[d][j] /= (double) spectrum_vec.size();
        }
    }

    if(avespec[0].size() != avespec[1].size())
    {
        std::cout<<"error cannot compute difference between spectrums of different lengths."<<std::endl;
        return 1;
    }

    //compute the spectrum difference 
    std::cout<<"clamping minimum difference to 1e-4"<<std::endl;
    std::vector< double > spec_diff;
    spec_diff.resize(avespec[0].size());
    for(unsigned int i=0; i<spec_diff.size(); i++)
    {
        spec_diff[i] = avespec[0][i] - avespec[1][i];
        if(spec_diff[i] < 0){spec_diff[i] = 0;};//1e-4;}; //clamp positive
    }

    std::cout<<"starting root plotting"<<std::endl;

        
    //ROOT stuff for plots
    TApplication* App = new TApplication("TestSpectrumDifference",&argc,argv);
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
    c->Divide(1,3);
    c->cd(1);
    c->SetFillColor(0);
    c->SetRightMargin(0.2);

    TGraph* ga = new TGraph();
    std::stringstream ssa;
    ssa <<"Averaged spectrum: ";
    ssa << dir[0];
    ga->SetTitle(ssa.str().c_str());
    graph.push_back(ga);
    for(unsigned int j=0; j<avespec[0].size(); j++)
    {
        //ga->SetPoint(j, j*spec_res/1e6, avespec[0][j] );
        ga->SetPoint(j, j*spec_res/1e6, 10.0*std::log10(avespec[0][j] + eps ) );
    }
    ga->Draw("ALP");
    c->Update();
    ga->GetXaxis()->SetTitle("Frequency (MHz)");
    ga->GetYaxis()->SetTitle("Power (a.u)");
    ga->GetYaxis()->CenterTitle();
    ga->GetXaxis()->CenterTitle();
    ga->Draw("ALP");
    c->Update();

    c->cd(2);

    TGraph* gb = new TGraph();
    std::stringstream ssb;
    ssb <<"Averaged spectrum: ";
    ssb << dir[1];
    gb->SetTitle(ssb.str().c_str());
    graph.push_back(gb);
    for(unsigned int j=0; j<avespec[1].size(); j++)
    {
        //gb->SetPoint(j, j*spec_res/1e6,  avespec[1][j] );
        gb->SetPoint(j, j*spec_res/1e6, 10.0*std::log10(avespec[1][j] + eps ) );
    }
    gb->Draw("ALP");
    c->Update();
    gb->GetXaxis()->SetTitle("Frequency (MHz)");
    gb->GetYaxis()->SetTitle("Power (a.u)");
    gb->GetYaxis()->CenterTitle();
    gb->GetXaxis()->CenterTitle();
    gb->Draw("ALP");
    c->Update();

    c->cd(3);

    TGraph* gc = new TGraph();
    std::stringstream ss;
    ss <<"Spectrum difference";
    gc->SetTitle(ss.str().c_str());

    graph.push_back(gc);
    for(unsigned int j=0; j<spec_diff.size(); j++)
    {
        // gc->SetPoint(j, j*spec_res/1e6, spec_diff[j]  );
        gc->SetPoint(j, j*spec_res/1e6, 10.0*std::log10( spec_diff[j] + eps ) );
    }
    gc->Draw("ALP");
    c->Update();
    gc->GetXaxis()->SetTitle("Frequency (MHz)");
    gc->GetYaxis()->SetTitle("Power (a.u)");
    gc->GetYaxis()->CenterTitle();
    gc->GetXaxis()->CenterTitle();
    gc->Draw("ALP");
    c->Update();


    App->Run();
}
