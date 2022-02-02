#include "AngularDistPDF.hh"

AngularDistPDF::AngularDistPDF():
GSLIntegrationInterface("AngularDistPDF")
{
	fNorm = 1;
	fConstant = 1;
	fQuadratic = 1;
	fQuartic = 1;
}

double
AngularDistPDF::f(double x) //const
{
	if( (x <= 2*TMath::Pi() ) && (x >= 0) )
	{
		double cosx = TMath::Cos(x);
		return fNorm*( fConstant + fQuadratic*cosx*cosx + fQuartic*cosx*cosx*cosx*cosx ) ;
	}
	else
	{
		return 0;
	}

};
