#include <zmq.hpp>
#include <string>
#include <iostream>
#include <unistd.h>
#include <sstream>
#include <thread>
#include <chrono>

// ROOT headers
#include "TStyle.h"
#include "TSystem.h"
#include "TCanvas.h"
#include "TGraph.h"
#include "TApplication.h"
#include "TAxis.h"
#include "TROOT.h"

#include "HTokenizer.hh"
#include "HTimer.hh"

#define NBINS 256
using namespace hose;

int main(int argc, char* argv[])
{
    //root app
    TApplication rootapp("spectrum monitor", &argc, argv);

    //canvas
    auto c1 = new TCanvas("c1", "Spectrum Plot");
    c1->SetWindowSize(1550, 700);

    //create a plot for the continuum noise power
    auto g1 = new TGraph();
    g1->SetTitle("Spectral Power");
    g1->GetYaxis()->SetTitle("Relative Power (dB)");
    g1->GetXaxis()->SetTitle("Frequency (MHz)");
    g1->SetMarkerStyle(5);

    c1->cd(1); 
    c1->Pad()->SetLeftMargin(0.08);
    c1->Pad()->SetRightMargin(0.01);
    g1->Draw("A");

    //create a plot for the narrow band noise power
    auto g2 = new TGraph();
    g2->SetTitle("Averaged Spectral Power");
    g2->GetYaxis()->SetTitle("Relative Power (dB)");
    g2->GetXaxis()->SetTitle("Frequency (MHz)");
    g2->SetMarkerStyle(4);

    //divide canvas and set plots
    c1->Divide(1, 2);
    c1->cd(1); 
    c1->Pad()->SetLeftMargin(0.08);
    c1->Pad()->SetRightMargin(0.01);
    g1->Draw("A");
    c1->cd(2); 
    c1->Pad()->SetLeftMargin(0.08);
    c1->Pad()->SetRightMargin(0.01);
    g2->Draw("A");


    //set up ZeroMQ UDP client to grab packets
    zmq::context_t context(1);
    zmq::socket_t subscriber(context, ZMQ_DISH);
    subscriber.bind("udp://*:8181");
    subscriber.join("spectrum");


    //hard-coded frequency map
    double sample_rate = 1.25e9;
    double n_samples_per_spec = NBINS;
    double flip = -1.0;
    double spec_res = (sample_rate/2.)/n_samples_per_spec;
    double Hz_perMHz = 1e6;

    float spec[NBINS];
    double sum_spec[NBINS];
    double count = 0;

    for(int i=0; i<NBINS; i++)
    {
        spec[i] = 0.0;
        sum_spec[i] = 1e-30;
    }

    while(subscriber.connected())
    {
        //make sure canvas hasn't been closed
        if(gROOT->GetListOfCanvases()->FindObject("c1") == nullptr){break;}

        zmq::message_t update;
        subscriber.recv(&update);

        if(update.size() == sizeof(float)*NBINS)
        {
            //copy in spectrum data 
            memcpy(&spec, update.data<float>(), update.size());

            count += 1;
            g1->Set(0);
            g2->Set(0);
            for(int i=NBINS-1; i>0; i--)
            {
                sum_spec[i] += spec[i]; 
                g1->SetPoint(g1->GetN(), i*spec_res/Hz_perMHz, 20.0*std::log10(spec[i]) );
                g2->SetPoint(g2->GetN(), i*spec_res/Hz_perMHz, 20.0*std::log10(sum_spec[i]/count) );
            }

            //update the plots in the window
            c1->cd(1);
            //c1->SetLogy();
            g1->Draw("AP");
            g1->GetYaxis()->SetTitle("Relative Power (dB)");
            g1->GetXaxis()->SetTitle("Frequency (MHz)");
            c1->Update();
            c1->Pad()->Draw();
            c1->cd(2);
            g2->GetYaxis()->SetTitle("Relative Power (dB)");
            g2->GetXaxis()->SetTitle("Frequency (MHz)");
            g2->Draw("ALP");
            c1->Update();
            c1->Pad()->Draw();
            gSystem->ProcessEvents();
        }
        else
        {
            std::cout<<"error message size of "<<update.size()<<" != "<<sizeof(float)*NBINS<<std::endl;
            break;
        }

    }











































}
