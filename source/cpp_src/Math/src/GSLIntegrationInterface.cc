#include "GSLIntegrationInterface.hh"
#include <iostream>

GSLIntegrationInterface::GSLIntegrationInterface(std::string name):
fName(name)
{
	fAccuracy = 1e-10;
	fWorkSpaceSize = 100000000;
	fGSLWork1 = gsl_integration_workspace_alloc(fWorkSpaceSize);
	fGSLWork2 = gsl_integration_workspace_alloc(fWorkSpaceSize); 
}

GSLIntegrationInterface::GSLIntegrationInterface(std::string name, double accuracy, double space_size):
fName(name),
fAccuracy(accuracy),
fWorkSpaceSize(space_size)
{
	fGSLWork1 = gsl_integration_workspace_alloc(fWorkSpaceSize);
	fGSLWork2 = gsl_integration_workspace_alloc(fWorkSpaceSize);
}


GSLIntegrationInterface::~GSLIntegrationInterface ()
{
	gsl_integration_workspace_free(fGSLWork1);
	gsl_integration_workspace_free(fGSLWork2);
};

double GSLIntegrationInterface::Evaluate(double x) //const
{
	return f(x);
}

double GSLIntegrationInterface::Integrate(double xlow, double xhigh)// const
{

	fGSLFunc2.function = &GSLIntegrationInterface_call;
	fGSLFunc2.params = static_cast<void*>(this);

	gsl_integration_qags(&fGSLFunc2, xlow, xhigh, 0, fAccuracy, fWorkSpaceSize, fGSLWork2, &result2, &error2); 

	return result2;

}

double GSLIntegrationInterface::CDF(double x)
{
	fGSLFunc2.function = &GSLIntegrationInterface_call;
	fGSLFunc2.params = static_cast<void*>(this);

	gsl_integration_qagil(&fGSLFunc2, x, 0, fAccuracy, fWorkSpaceSize, fGSLWork2, &result2, &error2); 

	return result2;

}

//double GSLIntegrationInterface::GetNormalization()
//{

//	fGSLFunc1.function = &GSLIntegrationInterface_call;
//	fGSLFunc1.params = static_cast<void*>(this);

//	//gsl_integration_qagil(&fGSLFunc1, 0, 0, fAccuracy, fWorkSpaceSize, fGSLWork1, &result1, &error1); 
//	//gsl_integration_qagiu(&fGSLFunc1, 0, 0, fAccuracy, fWorkSpaceSize, fGSLWork2, &result2, &error2); 

//	gsl_integration_qagi(&fGSLFunc1, 1e-6, fAccuracy, fWorkSpaceSize, fGSLWork1, &result1, &error1);



//	return  result1;

//}



