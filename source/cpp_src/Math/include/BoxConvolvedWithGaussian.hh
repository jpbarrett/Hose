#ifndef __BoxConvolvedWithGaussian_H__
#define __BoxConvolvedWithGaussian_H__



/*///////////////////////////////////////////////////
*
*@file BoxConvolvedWithGaussian.h
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


class BoxConvolvedWithGaussian
{
    public:
        BoxConvolvedWithGaussian();
        virtual ~BoxConvolvedWithGaussian();

        void SetGaussianPDF(GaussianPDF* gaussian);


    private:

};


#endif /* __BoxConvolvedWithGaussian_H__ */

