#ifndef GSLIntegrationInterface_HH
#define GSLIntegrationInterface_HH

#include <gsl/gsl_integration.h>
#include <iostream>

#include "<string>"
#include "TMath.h"
#include "NSObject.hh"

/*////////////////////////////////////////////////////////////
* @file GSLIntegrationInterface.hh
* @brief 
* @details
*
*
* <b>Revision History:</b>
* Date   Name   Brief description
* 01 Jan 2011   J. Barrett  First version
*
*/////////////////////////////////////////////////////////////

class GSLIntegrationInterface : public NSObject
{
	public:
		GSLIntegrationInterface(std::string name);
		GSLIntegrationInterface(std::string name, double accuracy, double space_size);
		
		virtual ~GSLIntegrationInterface ();
		
		double Evaluate(const double x) ;//const; //evaluate the probability distribution at x
		double Integrate(const double xlow, const double xhigh);// const; 
		//the integral over the interval [xlow, xhigh]
		double CDF(const double x);// const; //the cumulative distribution function
		
		std::string GetName() const {return fName;};

//		double GetNormalization();

		virtual double f(double x) = 0;
		//must be a function defined on (-inf, inf) with an integral of 1.0 over (-inf,inf)


	protected:
		/* data */
		std::string fName;

		gsl_integration_workspace* fGSLWork1;
		gsl_integration_workspace* fGSLWork2;
		double fAccuracy;
		double fWorkSpaceSize;

		//scratch space
		mutable gsl_function fGSLFunc1;
		mutable gsl_function fGSLFunc2;

		//scratch space		
		mutable double result1;
		mutable double error1;
		mutable double result2;
		mutable double error2; 


};

extern "C"
{
	inline double GSLIntegrationInterface_call(double x, void* params)
	{
		GSLIntegrationInterface* p = (GSLIntegrationInterface*) params;
		return p->f(x);
	}
}


#endif /* GSLIntegrationInterface_HH */




