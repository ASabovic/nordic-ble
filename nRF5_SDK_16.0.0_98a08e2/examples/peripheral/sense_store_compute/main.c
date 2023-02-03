#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include "nrf.h"
#include "nrf_drv_saadc.h"
#include "nrf_drv_ppi.h"
#include "nrf_drv_timer.h"
#include "boards.h"
#include "nrf_delay.h"
#include "nrf_temp.h"
#include "app_error.h"
#include "bsp.h"
#include "app_util_platform.h"
#include "nrf_pwr_mgmt.h"
#include "nrf_nvmc.h"
#include "nordic_common.h"

#include "nrf_log.h"
#include "nrf_log_ctrl.h"
#include "nrf_log_default_backends.h"

#include "app_timer.h"
#include "nrf_drv_clock.h"

#include "nrf_cli.h"
#include "nrf_cli_uart.h"

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

float f_temp(){


    float temp; 

    NRF_TEMP->TASKS_START = 1;

    while (NRF_TEMP->EVENTS_DATARDY == 0)
    {
        // Do nothing.
    }
    
    NRF_TEMP->EVENTS_DATARDY = 0;

    temp = (nrf_temp_read() / 4);

    NRF_TEMP->TASKS_STOP = 1;
    
    NRF_LOG_INFO("First temperature is: " NRF_LOG_FLOAT_MARKER " \n", NRF_LOG_FLOAT(temp));

    return temp;

}

float f_store(float temp, uint32_t *i, int temp_size){


    uint32_t tmp_temp = temp;

    uint32_t f_addr = 0x0007f000;

    uint32_t * p_addr = (uint32_t *)f_addr;

    uint32_t tmp_temp_array[*i+1];

    for (int k = 0; k<=*i; k++) {
      
      tmp_temp_array[k] = *(p_addr+k);
      
    }

    tmp_temp_array[*i] = tmp_temp;

    nrf_nvmc_page_erase(f_addr);
    nrf_nvmc_write_words(f_addr, tmp_temp_array, *i+1);
    
   
    for (int j = 0; j<=*i; j++){
      NRF_LOG_INFO("i memory is: " NRF_LOG_FLOAT_MARKER " \n", NRF_LOG_FLOAT(*i));
      NRF_LOG_INFO("j is: " NRF_LOG_FLOAT_MARKER " \n", NRF_LOG_FLOAT(j));
      NRF_LOG_INFO("In memory data is: " NRF_LOG_FLOAT_MARKER " \n", NRF_LOG_FLOAT(*(p_addr+j)));
    }

    NRF_LOG_INFO("i at end of f_store is: " NRF_LOG_FLOAT_MARKER " \n", NRF_LOG_FLOAT(*i));

}

float f_compute(int temp_size){

  uint32_t f_addr = 0x0007f000;

  uint32_t * p_addr = (uint32_t *)f_addr;

  uint32_t avg_addr = 0x0007d000;

  uint32_t * avg_pointer = (uint32_t *)avg_addr;

  float temp_sum = 0;

  for (int k = 0; k<temp_size; k++){
 
    temp_sum = temp_sum + (*(p_addr+k));
  }

  float average_temp = temp_sum / temp_size;

  NRF_LOG_INFO("Average temperature is: " NRF_LOG_FLOAT_MARKER " \n", NRF_LOG_FLOAT(average_temp));

  nrf_nvmc_page_erase(avg_addr);
  nrf_nvmc_write_word(avg_addr, average_temp);

  nrf_nvmc_page_erase(f_addr);
}

int main(void)
{
    saadc_init();

    nrf_saadc_value_t adc_val;


    nrf_temp_init();
    APP_ERROR_CHECK(NRF_LOG_INIT(NULL));
    NRF_LOG_DEFAULT_BACKENDS_INIT();
    
    int temp_size = 10;

    uint32_t i_addr = 0x0007e000;
    uint32_t * i_pointer = (uint32_t *)i_addr;

    NRF_LOG_INFO("Test is: " NRF_LOG_FLOAT_MARKER " \n", NRF_LOG_FLOAT(*i_pointer));

    if (*i_pointer == -1){
      nrf_nvmc_write_word(i_addr, 0);
    }
    
    NRF_LOG_INFO("after Test is: " NRF_LOG_FLOAT_MARKER " \n", NRF_LOG_FLOAT(*i_pointer));

    while (true)
    {
        

        nrfx_saadc_sample_convert(0, &adc_val);

        float voltage = adc_val * 3.6 / 1024;

        NRF_LOG_INFO("Volts: " NRF_LOG_FLOAT_MARKER " \n", NRF_LOG_FLOAT(voltage));

        //NRF_LOG_INFO("Hello!\n");

        if (voltage > 2.5){
          
          NRF_LOG_INFO("i: " NRF_LOG_FLOAT_MARKER " \n", NRF_LOG_FLOAT(*i_pointer));

          if (*i_pointer < temp_size){
            NRF_LOG_INFO("uso u if\n");
            
            f_store(f_temp(), i_pointer, temp_size);
            const uint32_t new_i = *(i_pointer) + 1;
            NRF_LOG_INFO("i after f-store() is: " NRF_LOG_FLOAT_MARKER " \n", NRF_LOG_FLOAT(new_i));
            nrf_nvmc_page_erase(i_addr);
            nrf_nvmc_write_word(i_addr, new_i);
            NRF_LOG_INFO("after new i: " NRF_LOG_FLOAT_MARKER " \n", NRF_LOG_FLOAT(*i_pointer));
            //nrf_nvmc_write_word(i_addr, 2);
            //NRF_LOG_INFO("after second new i: " NRF_LOG_FLOAT_MARKER " \n", NRF_LOG_FLOAT(*i_pointer));
          } else {
            NRF_LOG_INFO("uso u else\n");
            f_compute(temp_size); //in this function, erase from memory
            //i = 0;
            nrf_nvmc_page_erase(i_addr);
            nrf_nvmc_write_word(i_addr, 0);
          }


        }
        else{
        
          NRF_LOG_INFO("Not possible!\n");
        }

        nrf_delay_ms(5000);

        NRF_LOG_FLUSH();
    }
}



