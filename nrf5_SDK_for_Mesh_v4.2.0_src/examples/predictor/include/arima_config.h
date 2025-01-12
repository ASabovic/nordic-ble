#ifndef ARIMA_CONFIG_H
#define ARIMA_CONFIG_H

//Auto generated by python script
 
/*!
*\file arima_config.h
*\brief Defines the parameters for the ARIMA model
*\details This file is autogenerated by python script and contains the P, D, Q 
		values for the arima model along with the coefficients and intercept 
		values.
*\note To be used with the supporting python scripts
*\author Anuj Justus (justus98@imec.be)
*/
	
#define P 3
#define D 1
#define Q 2
#define SAMPLE_PERIOD 30

static const double AR_coef[] = {0.5985155286244926,
0.313442230152267,-0.052167384808971984};
static const double MA_coef[] = {-0.19151768237588296,
-0.34995905307880637};
#define intercept -4.593012021122454e-08

#endif
