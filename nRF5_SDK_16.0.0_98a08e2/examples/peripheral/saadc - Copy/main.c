#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "nrf.h"
#include "nrf_drv_saadc.h"
#include "nrf_drv_ppi.h"
#include "nrf_drv_timer.h"
#include "boards.h"
#include "app_error.h"
#include "nrf_delay.h"
#include "app_util_platform.h"
#include "nrf_pwr_mgmt.h"

#include "nrf_log.h"
#include "nrf_log_ctrl.h"
#include "nrf_log_default_backends.h"

void saadc_callback_handler (nrf_drv_saadc_evt_t const * p_event){

  // Empty handler function
}

void saadc_init(void){

  ret_code_t err_code;

  // Create a config struct and assign it default values along with the pin number for ADC input
  // Configure the input as Single Ended (One Pin Reading)

  nrf_saadc_channel_config_t channel_config = NRFX_SAADC_DEFAULT_CHANNEL_CONFIG_SE(NRF_SAADC_INPUT_VDD);

  // Initilize the saadc
  err_code = nrf_drv_saadc_init(NULL, saadc_callback_handler);
  APP_ERROR_CHECK(err_code);

  // Initilize the channel which will be connected to that specific pin
  err_code = nrfx_saadc_channel_init(0, &channel_config);
  APP_ERROR_CHECK(err_code);

}

void log_init(void){

  APP_ERROR_CHECK(NRF_LOG_INIT(NULL));

  NRF_LOG_DEFAULT_BACKENDS_INIT();

}

int main(void)
{

  log_init();

  saadc_init();

  nrf_saadc_value_t adc_val;

  NRF_LOG_INFO("Application started!!");

    
    while (1)
    {

      nrfx_saadc_sample_convert(0, &adc_val);

      NRF_LOG_INFO("Sample value read: %d", adc_val);

      NRF_LOG_INFO("Volts: " NRF_LOG_FLOAT_MARKER " \r\n", NRF_LOG_FLOAT(adc_val * 3.6 / 1024));

      nrf_delay_ms(5000);
       
    }
}



