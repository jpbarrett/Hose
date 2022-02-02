#ifndef __AngularDistPDF_H__
#define __AngularDistPDF_H__



/*///////////////////////////////////////////////////
*
*@file AngularDistPDF.h
*@brief
*@details
*
*<b>Revision History:<b>
*Date Name Brief Description
*Tue Oct 11 11:40:40 EDT 2011 J. Barrett (barrettj@mit.edu) First Version
*
*//////////////////////////////////////////////////////

#include "GSLIntegrationInterface.hh"

class AngularDistPDF : public GSLIntegrationInterface
{
	public:
		AngularDistPDF();
		virtual ~AngularDistPDF(){;};

		void SetNormalization(double norm){fNorm = norm;};
		void SetConstant(double constant){fConstant = constant;};
		void SetQuadratic(double quad){fQuadratic = quad;};
		void SetQuartic(double quart){fQuartic = quart;};


		double GetNormalization() const {return fNorm;};
		double GetConstant() const {return fConstant;};
		double GetQuadratic() const {return fQuadratic;};
		double GetQuartic() const {return fQuartic;};

		double f(double x);// const;

		private:

		double fNorm;
		double fConstant;
		double fQuadratic;
		double fQuartic;

};



#endif /* __AngularDistPDF_H__ */

