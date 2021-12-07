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
    g1->GetYaxis()->SetTitle("Power (a.u.)");
    g1->GetXaxis()->SetTitle("Frequency Bin");
    g1->SetMarkerStyle(4);
    // g1->SetMinimum(0);
    // g1->SetMaximum(100);

    // //create a plot for the narrow band noise power
    // auto g2 = new TGraph();
    // g2->SetTitle("Narrow Band Noise Power");
    // g2->GetYaxis()->SetTitle("Power (a.u)");
    // g2->GetXaxis()->SetTitle("Time since start (s)");
    // g2->SetMarkerStyle(4);
    // // g2->SetMinimum(-1);
    // // g2->SetMaximum(1);

    //divide canvas and set plots
    //c1->Divide(1, 2);
    c1->cd(1); 
    c1->Pad()->SetLeftMargin(0.08);
    c1->Pad()->SetRightMargin(0.01);
    g1->Draw("A");
    // c1->cd(2); 
    // c1->Pad()->SetLeftMargin(0.08);
    // c1->Pad()->SetRightMargin(0.01);
    // g2->Draw("A");

    //set up ZeroMQ UDP client to grab packets
    zmq::context_t context(1);
    zmq::socket_t subscriber(context, ZMQ_DISH);
    subscriber.bind("udp://*:8181");
    subscriber.join("spectrum");


//    HTimer timer;
//    timer.MeasureWallclockTime();

    float spec[256];
    double sum_spec[256];
    double count = 0;


    while(subscriber.connected())
    {
        //make sure canvas hasn't been closed
        if(gROOT->GetListOfCanvases()->FindObject("c1") == nullptr){break;}

        zmq::message_t update;
        subscriber.recv(&update);
    //    std::string text(update.data<const char>(), update.size());

        if(update.size() != sizeof(float)*256)
        {
            std::cout<<"error message size of "<<update.size()<<" != "<<sizeof(float)*256<<std::endl;
            break;
        }

        //copy in spectrum data 
        memcpy(&spec, update.data<float>(), update.size());

        count += 1;
        g1->Set(0);
        for(int i=0; i<256; i++)
        {
            sum_spec[i] += spec[i]; 
            g1->SetPoint(g1->GetN(), i, sum_spec[i]/count);
        }

        //g2->SetPoint(g2->GetN(), chunk_time, bin_sum);

        //update the plots in the window
        c1->cd(1);
        g1->Draw("AP");
        g1->GetYaxis()->SetTitle("Spectral Power (a.u.)");
        g1->GetXaxis()->SetTitle("Frequency Bin (s)");
        c1->Update();
        c1->Pad()->Draw();
        // c1->cd(2);
        // g2->GetYaxis()->SetTitle("Power (a.u)");
        // g2->GetXaxis()->SetTitle("Time since start (s)");
        // g2->Draw("AP");
        // c1->Update();
        // c1->Pad()->Draw();
        gSystem->ProcessEvents();

        //every some x amount of time we ought to clear the graphs

    }











































}
