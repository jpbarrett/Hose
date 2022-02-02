#include "BoxConvolvedWithGaussianPDF.hh"

BoxConvolvedWithGaussianPDF::BoxConvolvedWithGaussianPDF():
GSLIntegrationInterface("BoxConvolvedWithGaussian")
{
    fIntegrand = new BoxConvolvedWithGaussianIntegrand();

}

BoxConvolvedWithGaussianPDF::~BoxConvolvedWithGaussianPDF(){;};

void 
BoxConvolvedWithGaussianPDF::SetGaussianPDF(GaussianPDF* gaussian)
{
    fIntegrand->SetGaussianPDF(gaussian);
}

void 
BoxConvolvedWithGaussianPDF::SetBoxPDF(BoxPDF* box)
{
    fIntegrand->SetBoxPDF(box);
}

double
BoxConvolvedWithGaussianPDF::f(double x) //const
{
    double c = ( fIntegrand->GetBoxPDF() )->GetCenter();
    double w = ( fIntegrand->GetBoxPDF() )->GetWidth();
    double s = ( fIntegrand->GetGaussianPDF() )->GetSigma();

    //want the limits on the integration to be far enough out that we don't accidentally cut off any piece of the dist
    double taulow = (c - w/2.0) - 10*s;
    double tauhigh = (c + w/2.0) + 10*s;

    fIntegrand->SetT(x);
    return (fIntegrand->Integrate(taulow, tauhigh));
}
