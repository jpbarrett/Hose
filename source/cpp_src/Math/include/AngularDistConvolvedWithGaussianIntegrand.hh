#ifndef __AngularDistConvolvedWithGaussianIntegrand_H__
#define __AngularDistConvolvedWithGaussianIntegrand_H__



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
#include "GaussianPDF.hh"
#include "AngularDistPDF.hh"


class AngularDistConvolvedWithGaussianIntegrand : public GSLIntegrationInterface
{
    public:
        AngularDistConvolvedWithGaussianIntegrand();
        virtual ~AngularDistConvolvedWithGaussianIntegrand();

        void SetGaussianPDF(GaussianPDF* gaussian);
        void SetAngularDistPDF(AngularDistPDF* AngularDist);

        GaussianPDF* GetGaussianPDF(){return fGaussian;};        
        AngularDistPDF* GetAngularDistPDF(){return fAngularDist;};

        void SetT(double t){fT = t;}; 
        //sets the evaluation point t, whereas x is the dummy value that we will integrate over to form the convolution
        //for the owner PDF: AngularDistConvolvedWithGaussianPDF

        double f(double x);// const;

    private:

    GaussianPDF* fGaussian;
    AngularDistPDF* fAngularDist;
    double fT;

};


#endif /* __AngularDistConvolvedWithGaussianIntegrand_H__ */

