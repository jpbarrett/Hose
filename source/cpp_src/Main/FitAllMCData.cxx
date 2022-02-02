#include "BoxPDF.hh"
#include "GaussianPDF.hh"
#include "BoxConvolvedWithGaussianPDF.hh"

#include "TH1D.h"
#include "TTree.h" 
#include "TFile.h"
#include "TBranch.h"
#include "TBasket.h" 
#include "TMath.h"
#include "TCanvas.h"
#include "TGraph.h"
#include "TMultiGraph.h"
#include "TAxis.h"
#include "TApplication.h"
#include "TRandom3.h"
#include "TMinuit.h"
#include "TStyle.h"
#include "TColor.h"


#include <iostream>
#include <string>

void SetUpStyle(TStyle* myStyle)
{
    myStyle->SetCanvasBorderMode(0);
    myStyle->SetPadBorderMode(0);
    myStyle->SetPadColor(0);
    myStyle->SetCanvasColor(0);
    myStyle->SetTitleColor(1);
    myStyle->SetPalette(1,0);   // nice color scale for z-axis
    myStyle->SetCanvasBorderMode(0); // gets rid of the stupid raised edge around the canvas
    myStyle->SetFrameBorderMode(0);
    myStyle->SetTitleFillColor(0); //turns the default dove-grey background to white 
    myStyle->SetCanvasColor(0);
    myStyle->SetPadColor(0);
    myStyle->SetTitleFillColor(0);
    myStyle->SetStatColor(0); //this one may not work
    
    const Int_t NRGBs = 5;
    const Int_t NCont = 255;

    Double_t stops[NRGBs] = { 0.00, 0.34, 0.61, 0.84, 1.00 };
    Double_t red[NRGBs]   = { 0.00, 0.00, 0.87, 1.00, 0.51 };
    Double_t green[NRGBs] = { 0.00, 0.81, 1.00, 0.20, 0.00 };
    Double_t blue[NRGBs]  = { 0.51, 1.00, 0.12, 0.00, 0.00 };
    TColor::CreateGradientColorTable(NRGBs, stops, red, green, blue, NCont);
    myStyle->SetNumberContours(NCont);
};

//Global variables need by the fit function which is used by the 'C' style function called by TMinuit
//This is ugly, but the easiest way to use TMinuit with resorting to 
//functors and other such complications in order to call member functions of classes
BoxPDF* ProtonPulseBox;
GaussianPDF* ProtonPulseBlurring;
BoxConvolvedWithGaussianPDF* ProtonPulse;
TH1D* TheData;

//the function to be fit by TMinuit, this is our maximum likelihood function
void fcn(Int_t &npar, Double_t *gin, Double_t &f, Double_t *par, Int_t iflag)
{
    //set the parameters
    ProtonPulse->GetBoxPDF()->SetCenter(par[0]);

    Int_t nbins = TheData->GetNbinsX();
    Double_t LogLikelihood = 0.0;
    Double_t BinWidth;
    Double_t BinLowEdge;
    Double_t BinProb;
    Double_t BinContent;

    for(Int_t i=1; i <= nbins; i++)
    {
        BinContent = TheData->GetBinContent(i);
        BinLowEdge = TheData->GetBinLowEdge(i);
        BinWidth = TheData->GetBinWidth(i);
        BinProb = ProtonPulse->Integrate(BinLowEdge, BinLowEdge + BinWidth);
        if ( BinProb > 0.0) 
        {
            LogLikelihood += BinContent*TMath::Log(BinProb);
        }
        else if( BinProb < 0.0 )
        {
            std::cout << "WARNING::Probability is negative! This should not happen!" << std::endl;
        }
    }

    std::cout<<"ll = "<<-2.0*LogLikelihood<<", dt = "<<par[0]<<std::endl;

    f = -2.0*LogLikelihood;


}

int main(int argc, char *argv[]) 
{
    
    if (argc < 2)
    {
        std::cout<<"Usage:  FitMCData <full path to input file>"<<std::endl;
        std::cout<<""<<std::endl;
        return 0;
    }

    //read in the data
    std::string infile(argv[1]);
    TFile* F = new TFile(infile.c_str());
    TTree* T = (TTree*)F->Get("Tree");
    T->SetBranchAddress("MCData", &TheData);
    T->GetEntry(0);

    //We start with the same assumptions about the proton pulse shape, since this is known.
    //We are only interested in determining the most likely time off-set that the fake neutrino 
    //data represents.

    //the width of the proton pulse
    Double_t PulseWidth = 10.5*1e-6; //10.5 microseconds, page 4, paragraph 2

    //sigma for the drop off of the square proton pulse, we could find no clear 
    //value stated in the paper for this property of the proton pulse.
    //We estimate this value from the four subfigures of fig 12. on page 19 to be roughly
    //FWHM = 800ns, so sigma ~400ns
    Double_t PulseSigma = 400*1e-9;

    //build the classes we need for the fit function
    ProtonPulseBox = new BoxPDF();
    ProtonPulseBox->SetCenter(0);
    ProtonPulseBox->SetWidth(PulseWidth);

    ProtonPulseBlurring = new GaussianPDF();
    ProtonPulseBlurring->SetMean(0);
    ProtonPulseBlurring->SetSigma(PulseSigma);

    ProtonPulse = new BoxConvolvedWithGaussianPDF();
    ProtonPulse->SetBoxPDF(ProtonPulseBox);
    ProtonPulse->SetGaussianPDF(ProtonPulseBlurring);


    const int NParameters = 1; // the number of parameters, just the time displacement
    TMinuit Minuit(NParameters);
    Minuit.SetFCN(fcn);

    Double_t Parameters[NParameters];               // the start values
    Double_t StepSize[NParameters];          // step sizes 

    Parameters[0] = 100*1e-9; //start some where off by 1000ns
    StepSize[0] = 1e-9;

    Minuit.DefineParameter(0, "delta_t", Parameters[0], StepSize[0], 0, 0);

    //Minimize
    Minuit.Migrad();



    return 0;
}
