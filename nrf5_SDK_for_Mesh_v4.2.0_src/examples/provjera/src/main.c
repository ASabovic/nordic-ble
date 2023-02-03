#include "stdio.h"
/* HAL */
#include "boards.h"
#include "app_timer.h"
#include "nrf_drv_gpiote.h" 

/* Core */
#include "nrf_mesh_configure.h"
#include "nrf_mesh.h"
#include "mesh_stack.h"
#include "device_state_manager.h"
#include "access_config.h"
#include "proxy.h"

/* LPN */
#include "mesh_lpn.h"
#include "mesh_friendship_types.h"

/* Provisioning and configuration */
#include "mesh_provisionee.h"
#include "mesh_app_utils.h"

/* Models */
#include "simple_message_client.h"
#include "simple_message_common.h"

/* Logging and RTT */
#include "log.h"

/* Example specific includes */
#include "app_config.h"
#include "nrf_mesh_config_examples.h"
#include "example_common.h"
#include "ble_softdevice_support.h"
#include "ble_dfu_support.h"

/* nRF5 SDK */
#include "nrf_soc.h"
#include "nrf_pwr_mgmt.h"
#include "nrf_delay.h"

/* nRF5 SDK extra */
#include "app_util_platform.h"
#include "nrf_sdh.h"
#include "nrf_sdh_soc.h"
#include "nrf_sdh_ble.h"
#include "nrf.h"
#include "nrf_drv_ppi.h"
#include "nrf_drv_timer.h"
#include "nrf_drv_saadc.h"




double score(double * input) {   
    return (((((((((((((((((0.8431902449209346) + ((input[0]) * (0.06712345145642042))) + ((input[1]) * (-0.030426351930243897))) + ((input[2]) * (-0.2348893123877231))) + ((input[3]) * (-0.05146165828958308))) + ((input[4]) * (-0.05555122658104517))) + ((input[5]) * (1.462551158314854))) + ((input[6]) * (-0.0039713074345443964))) + ((input[7]) * (0.003971307434544172))) + ((input[8]) * (-0.1443863231973469))) + ((input[9]) * (0.14438632319734707))) + ((input[10]) * (0.07800564102994724))) + ((input[11]) * (-0.07800564102994724))) + ((input[12]) * (-0.031622947404039245))) + ((input[13]) * (0.031622947404039155))) + ((input[14]) * (-0.1600892466222542))) + ((input[15]) * (0.24681026852100735))) + ((input[16]) * (-0.10005638567400717));
}

double input[17] = { 1.24474546,  1.9817189 , -0.55448733,  3.02536229,  0.2732313 ,
        0.41173269, -0.47234264,  0.47234264, -0.72881553,  0.72881553,
        0.52836225, -0.52836225, -2.54711697,  2.54711697,  1.55889948,
       -0.7820157 , -0.70020801};
double *i = input;


int main(void){

    __LOG_INIT(LOG_SRC_APP | LOG_SRC_ACCESS, LOG_LEVEL_INFO, LOG_CALLBACK_DEFAULT);

    score(i);

    int res = score(i);

    __LOG(LOG_SRC_APP, LOG_LEVEL_INFO, "Prediction is: %d!\n", res);

    //printf("Result is: %d\n", res);

}