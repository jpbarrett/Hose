#include "BoxConvolvedWithGaussianIntegrand.hh"

BoxConvolvedWithGaussianIntegrand::BoxConvolvedWithGaussianIntegrand():
GSLIntegrationInterface("BoxConvolvedWithGaussianIntegrand")
{

}

BoxConvolvedWithGaussianIntegrand::~BoxConvolvedWithGaussianIntegrand(){;};

void 
BoxConvolvedWithGaussianIntegrand::SetGaussianPDF(GaussianPDF* gaussian)
{
    fGaussian = gaussian;
}

void 
BoxConvolvedWithGaussianIntegrand::SetBoxPDF(BoxPDF* box)
{
    fBox = box;
}

double
BoxConvolvedWithGaussianIntegrand::f(double x) //const
{
    return (fBox->Evaluate(x))*(fGaussian->Evaluate(fT - x));
}
