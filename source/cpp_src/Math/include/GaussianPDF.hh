#ifndef GaussianPDF_HH
#define GaussianPDF_HH

#include "GSLIntegrationInterface.hh"

#include "TMath.h"

/*////////////////////////////////////////////////////////////
* @file GaussianPDF.hh
* @brief 
* @details
*
*
* <b>Revision History:</b>
* Date   Name   Brief description
* 01 Jan 2011   J. Barrett  First version
*
*/////////////////////////////////////////////////////////////

class GaussianPDF: public GSLIntegrationInterface
{
	public:
		GaussianPDF();
		virtual ~GaussianPDF();
		
		void SetMean(double mean);
		void SetSigma(double sigma); 

        double GetMean() const {return m;};
        double GetSigma() const {return s;};

		double f(double x);// const;

	private:
		double m;
		double s;

};




#endif /* GaussianPDF_HH */

