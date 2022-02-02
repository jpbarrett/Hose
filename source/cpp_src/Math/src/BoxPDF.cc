#include "BoxPDF.hh"

BoxPDF::BoxPDF():
GSLIntegrationInterface("Box")
{

}

BoxPDF::~BoxPDF(){;};

void 
BoxPDF::SetCenter(double center)
{
	fCenter = center;
};

void 
BoxPDF::SetWidth(double width)
{
	fWidth = width;
}; 



double
BoxPDF::f(double x) //const
{
	if( TMath::Abs(fCenter - x) <= (fWidth/2.0) )
    {
        return 1.0/fWidth;
    }
    else
    {
        return 0;
    }
};
