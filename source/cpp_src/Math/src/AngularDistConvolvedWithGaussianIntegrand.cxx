#include "AngularDistConvolvedWithGaussianIntegrand.hh"

AngularDistConvolvedWithGaussianIntegrand::AngularDistConvolvedWithGaussianIntegrand():
GSLIntegrationInterface("AngularDistConvolvedWithGaussianIntegrand")
{

}

AngularDistConvolvedWithGaussianIntegrand::~AngularDistConvolvedWithGaussianIntegrand(){;};

void 
AngularDistConvolvedWithGaussianIntegrand::SetGaussianPDF(GaussianPDF* gaussian)
{
    fGaussian = gaussian;
}

void 
AngularDistConvolvedWithGaussianIntegrand::SetAngularDistPDF(AngularDistPDF* AngularDist)
{
    fAngularDist = AngularDist;
}

double
AngularDistConvolvedWithGaussianIntegrand::f(double x) //const
{
    return (fAngularDist->Evaluate(x))*(fGaussian->Evaluate(fT - x));
}
