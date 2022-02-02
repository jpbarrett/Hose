#ifndef __BoxConvolvedWithGaussianPDF_H__
#define __BoxConvolvedWithGaussianPDF_H__



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
#include "BoxConvolvedWithGaussianIntegrand.hh"
#include "GaussianPDF.hh"
#include "BoxPDF.hh"


class BoxConvolvedWithGaussianPDF: public GSLIntegrationInterface
{
    public:
        BoxConvolvedWithGaussianPDF();
        virtual ~BoxConvolvedWithGaussianPDF();

        void SetGaussianPDF(GaussianPDF* gaussian);
        void SetBoxPDF(BoxPDF* box);


        GaussianPDF* GetGaussianPDF(){return fIntegrand->GetGaussianPDF();};        
        BoxPDF* GetBoxPDF(){return fIntegrand->GetBoxPDF();};


        double f(double x);// const;

    private:

    BoxConvolvedWithGaussianIntegrand* fIntegrand;


};


#endif /* __BoxConvolvedWithGaussianPDF_H__ */

