
#ifndef ARIMA_MODEL_H
#define ARIMA_MODEL_H

/*!
 \defgroup  ARIMA_MATH
 \details   This module contains the non-seasonal ARIMA(P,D,Q) function 
            implemented in general C language and referred throught this project
            . The mathematical representation for the model is, 
            \f[
                (1-( \sum_{n=1}^{P}B^{n}\phi_{n}))(1-B)^{D}Y_{t}=
                (1+\sum_{n=1}^{Q}B^{n}\theta_{n})e_{t}+C
            \f]
            Here, 
            \li P,D,Q are user defined hyper parameters (Basic grid search 
                algorithm is available for enhancing the selection process)
            \li B is the backshift operator (e.g. \f$B^{n}Y_{t}=Y_{t-n}\f$ and 
                \f$B^{n}e_{t}=e_{t-n}\f$)
            \li \f$ \phi\f$<SUB>n</SUB> and \f$ \theta\f$<SUB>n</SUB> are the 
                coefficients of the auto-regressive (AR) and moving average (MA) 
                components of the ARIMA model, respectively
            \li Y<SUB>t</SUB> is the univariable being forecasted and 
                backshifted by B and
            \li e<SUB>t</SUB> is the error/resdual data which is also 
                backshifted by B (Observed - Predicted).
            \li C is the intercept/constant

            For more information on the mathematical representation visit 
            <a href="http://people.duke.edu/~rnau/Mathematical_structure_of_ARIMA_models--Robert_Nau.pdf">
            "The mathematical structure of ARIMA models"</a> and for more 
            information visit 
            <a href="https://otexts.com/fpp2/non-seasonal-arima.html">
            "Forecasting: Principles and Practice"</a>
*/

/*!
 *\file     arima_model.h
 *\brief    ARIMA model in general C
 *\author   Anuj Justus (justus98@imec.be)          
 *\details  This file contains the non-seasonal actual ARIMA model implemented 
            in general C. It is capable of accomodating various values for P, D 
            and Q. The coefficients of the model is obtained through the python 
            scripts. For more information on the actual ARIMA model algorithm 
            implemented in C, refer \ref ARIMA_MATH
 *\note		Empirical analysis showed that AR and Differencing part of ARIMA
            model in C is a perfect match to the model employed by STATSMODEL 
            in python. The MA part could not be verified due to lack of control
            in the initialization values of the model trained by STATSMODEL. But
            its effect on the RMSE is in the order of 10^-12. For now the entire 
            ARIMA algorithm in C is considered to be verified with STATSMODEL's 
            ARIMA algorithm in python and it's assumed to be correct.
 *\warning  To be used with the supporting python scripts
 *\todo     Add variable forecasting horizon. Currently it just predicts one 
            step ahead.
 *\todo     Auto gain values detection for quantization
 */
#include <stdio.h>
#include "arima_config.h"

// current values are obtained from time_series_data/ixys_2021-01-10.csv 
// using arima_coefficients_generator.py
// Static variable are placed in data memory for faster access (yet to be 
// tested, platform dependent), also for scoping.


#define max(a,b) (((a)>(b))?(a):(b))//!< Find the maximum

/*!
 * \defgroup buffers
 * @{
 * \details Contains all the buffers needed to hold the necessary univariable 
 *          data for computing with trained model, mechanism to detect the 
 *          necessary buffer length and pointer to the index containing latest 
 *          buffer value
 * \note    It's assumed that static variables are auto initialized to zero, 
 *          according to standard.
 */


//! Set the buffer length to accomodate necessary observed time series data for 
//! the given P,D and Q values
/*!
    P+D = total number of time-series values required for proper prediction
    e.g. when P=3, we need Yt-1, Yt-2, Yt-3 values assuming D=0. If D=1, we need
    (Yt-1)-(Yt-2),(Yt-2)-(Yt-3),(Yt-3)-(Yt-4) to predict using AR(3) algorithm
    Which means len(Yt-1..Yt-4) = 4 => P+D. Can also be evident from the formula
*/
#define BUF_LEN max((P+D),Q)

// Buffers for holding the minimum incoming data required for prediction
// Static variable are auto initialized to 0 (According to standard)


static double Y[BUF_LEN];//!< Buffer to store observed time series data
static double E[Q];//!< Buffer to store residual (observed-predicted) terms

//! Point to the head of the time-series data buffer
/*!
    \warning -1 initialization is needed for proper operation in first iteration
*/
static int Y_latest_val_at_index = -1;

//! Point to the head of the residual data buffer
/*!
    \warning -1 initialization is needed for proper operation in first iteration
*/
static int E_latest_val_at_index = -1;

/**@}*/

//! Container to store predicted value
static double Yt_predicted = 0;

//! Time_series data length tracker
static int ts_length = 0;

//! Function to process new observed value
/*!
    This function is responsible for processing the new incoming observed value 
    of the univariable being forecasted. This value and the corresponding 
    residual gets added to the corresponding buffer also updating the buffer 
    head pointers. Also refer \ref buffers.
    \warning Each call to the function will increment the series length tracker
    \p ts_length
*/

void handle_new_data_core(double b_Yt,int *y_latest_val_at_index_ptr, int *e_latest_val_at_index_ptr, double y[], double e[], double yt_predicted){

    //Load the value at proper index
    // assert (0=<Y_latest_val_at_index<BUF_LEN) - won't pass in first iteration
    int y_latest_val_at_index = *y_latest_val_at_index_ptr;
    int e_latest_val_at_index = *e_latest_val_at_index_ptr;
    
    #if BUF_LEN != 0 
    y_latest_val_at_index ++;    
    if(y_latest_val_at_index == BUF_LEN) y_latest_val_at_index = 0;
    y[y_latest_val_at_index] = b_Yt;
    #endif
    
    #if Q != 0 
    e_latest_val_at_index ++;    
    if(e_latest_val_at_index == Q) e_latest_val_at_index = 0;
    e[e_latest_val_at_index] = b_Yt - yt_predicted;
    #endif

    *y_latest_val_at_index_ptr = y_latest_val_at_index;
    *e_latest_val_at_index_ptr = e_latest_val_at_index;

}

void handle_new_data(double B_Yt/**< [in] The latest observed value. */){
    ts_length++;
    handle_new_data_core(B_Yt, &Y_latest_val_at_index, &E_latest_val_at_index, Y
                        , E, Yt_predicted);
}

//!Predict the next value of the univariable using ARIMA model
/*!
    Uses the global variables - buffers and buffer head pointers
    \return the predicted value
*/

double predict_th1(int y_latest_val_at_index, int e_latest_val_at_index, double y[], double e[]){    
    // Assume time horizon = 1, just one step ahead,

    int idx = y_latest_val_at_index;
    
    double AR = 0; // Container for the AR part of the algorithm

    #if P != 0
        for (int p=0;p<P;p++){
            
            // // When D=1
            // int iddx = idx - 1;
            // if(iddx<0) iddx = BUF_LEN-1;
            // // When D=2, include below
            // int idddx = iddx - 1;
            // if(idddx<0) idddx = BUF_LEN-1;
            // AR += ar_coef[p]*(y[idx]-y[iddx])-(y[iddx]-y[idddx]));

            //Macro to avoid unecessary array G loading + compute when D=0
            #if D == 0
                AR += AR_coef[p]*y[idx];
            #else
                // D parameter calculation
                double G[BUF_LEN]; 
                // double G[BUF_LEN] = all the values loaded in respective index
                //                     such that G[0]=Yt-1, G[2] = Yt-2...
                
                int idxd = idx;
                for(int x=0;x<BUF_LEN;x++){
                    G[x]=y[idxd];
                    idxd--;
                    if(idxd<0) idxd = BUF_LEN-1;
                }

                // Calculate the variable for coefficients () of AR function, 
                // for varying D values 
                for(int x=D;x>0;x--)
                {
                    for (int y=0;y<x;y++){
                        G[y] = G[y] - G[y+1];
                    }
                }
                AR += (AR_coef[p]*G[0]);
            #endif

            //Move back a step
            idx--;
            if(idx<0) idx = BUF_LEN-1;
        }
    #endif

    #if D != 0
        // Difference Y is the univariable. Below algorithm will give the value 
        // to be added to the differenced Y to get Y

        double G[BUF_LEN]={0}; 
        // double G[BUF_LEN] = {all the values loaded in respective index 
        //                      such that G[0] = 0, G[0] = Yt-1, G[2] = Yt-2...}
        int idxd = y_latest_val_at_index;
        for(int x=1;x<BUF_LEN;x++){
            G[x]=y[idxd];
            idxd--;
            if(idxd<0) idxd = BUF_LEN-1;
        }

        for(int x=D;x>0;x--)
        {
            for (int y=0;y<x;y++){
                G[y] = G[y] - G[y+1];
            }
        }
        AR -= G[0];
    #endif 

    double MA = 0;
    #if Q != 0 
        idx = e_latest_val_at_index;
        for (int q=0;q<Q;q++){
            //Removed negative sign !!! It works now...
            MA += MA_coef[q]*e[idx];
            //Move back a step
            idx--;
            if(idx<0) idx = Q-1;
        }
    #endif

    return (AR+MA+intercept);
}

double predict_next_one(){
    Yt_predicted = predict_th1(Y_latest_val_at_index, E_latest_val_at_index, Y, 
                               E);
    return Yt_predicted;
}


// // To avoid re-predicting inbetween if incrementing TH values are sent in sequentially
// static unsigned int virt_ts_length = 0;
// void predict_vth_reset(){
//     virt_ts_length = 0;
// }

//th value > 1, if th =  one, its same as predict next one...
double predict_vth(int th){
    // if(virt_ts_length == 0) {
    
    static double Y_virt[BUF_LEN];
    for(int x = 0;x<BUF_LEN;x++) Y_virt[x] = Y[x];
    
    static double E_virt[Q];
    for(int x = 0;x<Q;x++) E_virt[x] = E[x];

    int Y_latest_val_at_index_virt = Y_latest_val_at_index;
    int E_latest_val_at_index_virt = E_latest_val_at_index;
    double Yt_predicted_virt = Yt_predicted;
    // static double [Yt];
    // }
    
    for(int y = 0; y<th; y++){
        // virt_ts_length++;
        Yt_predicted_virt = predict_th1(Y_latest_val_at_index_virt, E_latest_val_at_index_virt, Y_virt, 
                              E_virt);
        handle_new_data_core(Yt_predicted_virt, &Y_latest_val_at_index_virt, &E_latest_val_at_index_virt, Y_virt
                        , E_virt, Yt_predicted_virt);
    }
    return Yt_predicted_virt;
}

#define MAX_TIME_HORIZON_STEPS 10
#define SAMPLING_TIME_PERIOD_S 30
#define MAX_HARVESTING_POWER_W 1.5
#define MIN_HARVESTING_POWER_W 0

//Returns time steps to wait
int predict_when(double joule_required){
    // if(virt_ts_length == 0) {
    
    static double Y_virt[BUF_LEN];
    for(int x = 0;x<BUF_LEN;x++) Y_virt[x] = Y[x];
    
    static double E_virt[Q];
    for(int x = 0;x<Q;x++) E_virt[x] = E[x];

    int Y_latest_val_at_index_virt = Y_latest_val_at_index;
    int E_latest_val_at_index_virt = E_latest_val_at_index;
    double Yt_predicted_virt = Yt_predicted;
    // static double [Yt];
    // }
    
    double total_energy = 0;
    for(int y = 0; y<MAX_TIME_HORIZON_STEPS; y++){
        // virt_ts_length++;
        Yt_predicted_virt = predict_th1(Y_latest_val_at_index_virt, E_latest_val_at_index_virt, Y_virt, 
                              E_virt);
        
        if(Yt_predicted_virt<MIN_HARVESTING_POWER_W) Yt_predicted_virt = MIN_HARVESTING_POWER_W;
        else if(Yt_predicted_virt>MAX_HARVESTING_POWER_W) Yt_predicted_virt = MAX_HARVESTING_POWER_W;

        handle_new_data_core(Yt_predicted_virt, &Y_latest_val_at_index_virt, &E_latest_val_at_index_virt, Y_virt
                        , E_virt, Yt_predicted_virt);
        
        total_energy += Yt_predicted_virt*SAMPLING_TIME_PERIOD_S;

        if (total_energy >= joule_required) return (y+1);
    }
    return MAX_TIME_HORIZON_STEPS;
}

#endif
