#ifndef __BoxConvolvedWithGaussianIntegrand_H__
#define __BoxConvolvedWithGaussianIntegrand_H__



/*///////////////////////////////////////////////////
*
*@file BoxConvolvedWithGaussianIntegrand.h
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
#include "BoxPDF.hh"


class BoxConvolvedWithGaussianIntegrand : public GSLIntegrationInterface
{
    public:
        BoxConvolvedWithGaussianIntegrand();
        virtual ~BoxConvolvedWithGaussianIntegrand();

        void SetGaussianPDF(GaussianPDF* gaussian);
        void SetBoxPDF(BoxPDF* box);

        GaussianPDF* GetGaussianPDF(){return fGaussian;};        
        BoxPDF* GetBoxPDF(){return fBox;};

        void SetT(double t){fT = t;}; 
        //sets the evaluation point t, whereas x is the dummy value that we will integrate over to form the convolution
        //for the owner PDF: BoxConvolvedWithGaussianPDF

        double f(double x);// const;

    private:

    GaussianPDF* fGaussian;
    BoxPDF* fBox;
    double fT;

};


#endif /* __BoxConvolvedWithGaussianIntegrand_H__ */

