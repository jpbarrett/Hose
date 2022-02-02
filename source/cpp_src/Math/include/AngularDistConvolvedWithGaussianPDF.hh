#ifndef __AngularDistConvolvedWithGaussianPDF_H__
#define __AngularDistConvolvedWithGaussianPDF_H__



/*///////////////////////////////////////////////////
*
*@file AngularDistConvolvedWithGaussianIntegrand.h
*@brief
*@details
*
*<b>Revision History:<b>
*Date Name Brief Description
*Tue Oct 11 11:37:54 EDT 2011 J. Barrett (barrettj@mit.edu) First Version
*
*//////////////////////////////////////////////////////

#include "GSLIntegrationInterface.hh"
#include "AngularDistConvolvedWithGaussianIntegrand.hh"
#include "GaussianPDF.hh"
#include "AngularDistPDF.hh"


class AngularDistConvolvedWithGaussianPDF: public GSLIntegrationInterface
{
    public:
        AngularDistConvolvedWithGaussianPDF();
        virtual ~AngularDistConvolvedWithGaussianPDF();

        void SetGaussianPDF(GaussianPDF* gaussian);
        void SetAngularDistPDF(AngularDistPDF* AngularDist);


        GaussianPDF* GetGaussianPDF(){return fIntegrand->GetGaussianPDF();};        
        AngularDistPDF* GetAngularDistPDF(){return fIntegrand->GetAngularDistPDF();};


        double f(double x);// const;

    private:

    AngularDistConvolvedWithGaussianIntegrand* fIntegrand;


};


#endif /* __AngularDistConvolvedWithGaussianPDF_H__ */

