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
	
typedef double fptype;

#define _P 4
#define _D 2
#define _Q 2
#define SAMPLE_PERIOD 5

static const fptype AR_coef[] = {-0.9945487778942391,
-0.15803582543073375,0.026788706154235307,0.019883188340543163};
static const fptype MA_coef[] = {-0.03909781416333791,
-0.6476484986429807};
#define intercept -6.061237241210344e-10

#endif