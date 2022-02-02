#include "GaussianPDF.hh"

GaussianPDF::GaussianPDF():
GSLIntegrationInterface("Gaussian")
{

}

GaussianPDF::~GaussianPDF(){;};

void 
GaussianPDF::SetMean(double mean)
{
	m = mean;
};

void 
GaussianPDF::SetSigma(double sigma)
{
	s = sigma;
}; 



double
GaussianPDF::f(double x) //const
{
	return TMath::Gaus(x, m, s, true);
}
