#include "AngularDistConvolvedWithGaussianPDF.hh"

AngularDistConvolvedWithGaussianPDF::AngularDistConvolvedWithGaussianPDF():
GSLIntegrationInterface("AngularDistConvolvedWithGaussian")
{
    fIntegrand = new AngularDistConvolvedWithGaussianIntegrand();

}

AngularDistConvolvedWithGaussianPDF::~AngularDistConvolvedWithGaussianPDF(){;};

void 
AngularDistConvolvedWithGaussianPDF::SetGaussianPDF(GaussianPDF* gaussian)
{
    fIntegrand->SetGaussianPDF(gaussian);
}

void 
AngularDistConvolvedWithGaussianPDF::SetAngularDistPDF(AngularDistPDF* AngularDist)
{
    fIntegrand->SetAngularDistPDF(AngularDist);
}

double
AngularDistConvolvedWithGaussianPDF::f(double x) //const
{

    double s = ( fIntegrand->GetGaussianPDF() )->GetSigma();

    //want the limits on the integration to be far enough out that we don't accidentally cut off any piece of the dist
    double taulow = 10*s;
    double tauhigh = 2*TMath::Pi() + 10*s;

    fIntegrand->SetT(x);
    return (fIntegrand->Integrate(taulow, tauhigh));
}
