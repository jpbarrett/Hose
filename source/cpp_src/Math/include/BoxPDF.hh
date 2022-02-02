#ifndef __BoxPDF_H__
#define __BoxPDF_H__



/*///////////////////////////////////////////////////
*
*@file BoxPDF.h
*@brief
*@details
*
*<b>Revision History:<b>
*Date Name Brief Description
*Tue Oct 11 11:40:40 EDT 2011 J. Barrett (barrettj@mit.edu) First Version
*
*//////////////////////////////////////////////////////

#include "GSLIntegrationInterface.hh"

class BoxPDF : public GSLIntegrationInterface
{
    public:
        BoxPDF();
        virtual ~BoxPDF();

        void SetCenter(double center);
        void SetWidth(double width);
        
        double GetCenter() const {return fCenter;};
        double GetWidth() const {return fWidth;};

        double f(double x);// const;

    private:

    double fCenter;
    double fWidth;

};



#endif /* __BoxPDF_H__ */

