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
    int fake_argc = 0;
    char** fake_argv = nullptr;
    //ROOT stuff for plots
    TApplication app("noise_monitor",&fake_argc,fake_argv);

    //canvas
    auto c1 = new TCanvas("c1", "Noise Power Plot");
    c1->SetWindowSize(1550, 700);

    //create a plot for the continuum noise power
    auto g1 = new TGraph();
    g1->SetTitle("Continuum Noise Power");
    g1->GetYaxis()->SetTitle("Power (a.u.)");
    g1->GetXaxis()->SetTitle("Time since start (s)");
    g1->SetMarkerStyle(4);
    // g1->SetMinimum(0);
    // g1->SetMaximum(100);

    //create a plot for the narrow band noise power
    auto g2 = new TGraph();
    g2->SetTitle("Narrow Band Noise Power");
    g2->GetYaxis()->SetTitle("Power (a.u)");
    g2->GetXaxis()->SetTitle("Time since start (s)");
    g2->SetMarkerStyle(4);
    // g2->SetMinimum(-1);
    // g2->SetMaximum(1);

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
    subscriber.join("noise_power");

    //tokenizer and timer utils 
    HTokenizer tokenizer;
    tokenizer.SetDelimiter(";");
    std::vector< std::string > tokens;

    HTimer timer;
    timer.MeasureWallclockTime();

    uint64_t prev_start_index = 0;
    uint64_t prev_start_sec = 0;
    double reset_interval = 30; //reset the plots every 5 minutes
    double prev_chunk_time = 0;

    while(subscriber.connected())
    {
        //make sure canvas hasn't been closed
        if(gROOT->GetListOfCanvases()->FindObject("c1") == nullptr){break;}

        zmq::message_t update;
        subscriber.recv(&update);
        std::string text(update.data<const char>(), update.size());
        std::cout << text << std::endl;

        tokenizer.SetString(&text);
        tokenizer.GetTokens(&tokens);

        if(tokens.size() >= 11)
        {
        
            uint64_t start_sec = atoll(tokens[0].c_str());  

            if(prev_start_sec != start_sec)
            {
                //reset the plots, clear all points 
                g1->Set(0);
                g2->Set(0);
                prev_start_sec = start_sec;
            }

            uint64_t sample_rate = atoll(tokens[1].c_str());  
            uint64_t start_index = atoll(tokens[2].c_str());  
            uint64_t stop_index = atoll(tokens[3].c_str());  
            double sum = atof(tokens[4].c_str());  
            double sum_x2 = atof(tokens[5].c_str());  
            double delta = atof(tokens[6].c_str());  
            int low_bin = atof(tokens[8].c_str());  
            int high_bin = atof(tokens[9].c_str());  
            double bin_sum = atof(tokens[10].c_str());  

            //compute time of this data chunk and noise variance
            double chunk_time = (double)start_index/(double)sample_rate;

            if( (chunk_time - prev_chunk_time) > reset_interval)
            {
                //reset the plots, clear all points
                g1->Set(0);
                g2->Set(0);
                prev_chunk_time = chunk_time;
            }


            double var = sum_x2/delta - (sum/delta)*(sum/delta);
            g1->SetPoint(g1->GetN(), chunk_time, var );
        
            g2->SetPoint(g2->GetN(), chunk_time, bin_sum);

            //update the plots in the window
            c1->cd(1);
            g1->Draw("AP");
            g1->GetYaxis()->SetTitle("Power (a.u.)");
            g1->GetXaxis()->SetTitle("Time since start (s)");
            c1->Update();
            c1->Pad()->Draw();
            c1->cd(2);
            g2->GetYaxis()->SetTitle("Power (a.u)");
            g2->GetXaxis()->SetTitle("Time since start (s)");
            g2->Draw("AP");
            c1->Update();
            c1->Pad()->Draw();
            gSystem->ProcessEvents();
            
            prev_start_index = start_index;
        }

        //every some x amount of time we ought to clear the graphs

    }











































}
