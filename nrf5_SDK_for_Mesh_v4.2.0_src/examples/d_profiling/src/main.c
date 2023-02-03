#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include <stdbool.h>
#include <math.h> 

/* HAL */
#include "boards.h"
//#include "simple_hal.h"
#include "app_timer.h"

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
//#include "rtt_input.h"

/* Example specific includes */
#include "app_config.h"
#include "nrf_mesh_config_examples.h"
#include "example_common.h"
#include "ble_softdevice_support.h"
#include "ble_dfu_support.h"

/* nRF5 SDK */
#include "nrf_soc.h"
#include "nrf_pwr_mgmt.h"
#include "nrf.h"
#include "nrf_delay.h"
#include "app_util_platform.h"
#include "nrf_sdh.h"
#include "nrf_sdh_soc.h"

/*****************************************************************************
 * Definitions
 *****************************************************************************/
/* The maximum duration to scan for incoming Friend Offers. */
#define FRIEND_REQUEST_TIMEOUT_MS (MESH_LPN_FRIEND_REQUEST_TIMEOUT_MAX_MS)

/* The upper limit for two subsequent Friend Polls. */
#define POLL_TIMEOUT_MS (SEC_TO_MS(17))

/* The time between LPN sending a request and listening for a response. */
#define RECEIVE_DELAY_MS (100)

/* The time between two temperature measurements */
#define TEMP_TIMEOUT (SEC_TO_MS(10))

/* Flash states definition */
/* Flash idle */
#define FLASH_IDLE 0

/* Flash - tasks part */
#define TASKS_WRITE 1

/* Flash - pointer part */
#define POINTER_WRITE 2

/* Flash - temperature part */
#define TEMP_WRITE 3

/* Flash - new pointer part */
#define NEW_POINTER_INCREASE 4
#define NEW_POINTER_WRITE 5

/* Flash - compute part */
#define AVG_WRITE 6
#define AVG_READ 7

/*****************************************************************************
 * Forward declaration of static functions
 *****************************************************************************/
static void app_mesh_core_event_cb (const nrf_mesh_evt_t * p_evt);
static void send_simple_message(uint8_t *data);

static void simple_message_client_status_cb(const simple_message_client_t * p_self, 
                                            simple_message_status_t status, 
                                            uint16_t src);

static void simple_message_client_set_cb (const simple_message_client_t * p_self,
                                              nrf_mesh_address_t src,
                                              nrf_mesh_address_t dst,  
                                              uint8_t *data, 
                                              uint8_t length);

/*****************************************************************************
 * Static variables
 *****************************************************************************/

// Global variables

uint8_t flash_status = 0;
uint32_t new_pointer_counter;

// Global temporary temperature value 

int temperature;
int temp_size = 3;
int tmp_temp;
int tmp_temp_array[3];
uint32_t zero[];
int avg_temp[];
//uint32_t new_pointer[];
//uint32_t zero_value = 0;

// Store memory part 
// Temperatures
uint32_t pg_size_temp;
uint32_t pg_num_temp; 
uint32_t *pointer_temp;

// Counter
uint32_t pg_size_counter;
uint32_t pg_num_counter; 
uint32_t *pointer_counter;

// Compute 
uint32_t pg_size_compute;
uint32_t pg_num_compute;
uint32_t *pointer_compute;


/* Initilize LOG */

static void initilize(void)
{
    __LOG_INIT(LOG_SRC_APP | LOG_SRC_ACCESS, LOG_LEVEL_INFO, LOG_CALLBACK_DEFAULT);
}

static simple_message_client_t m_client2;
static bool                   m_device_provisioned;

/* temperature timer - The timer allows our board to measure the temperature every X seconds */
APP_TIMER_DEF(temperature_timer);

static nrf_mesh_evt_handler_t m_mesh_core_event_handler = { .evt_cb = app_mesh_core_event_cb };

static void device_identification_start_cb(uint8_t attention_duration_s)
{
    __LOG(LOG_SRC_APP, LOG_LEVEL_INFO, "Device identified \n");
}

static void provisioning_aborted_cb(void)
{
    __LOG(LOG_SRC_APP, LOG_LEVEL_INFO, "Provisioning aborted \n");
}

static void unicast_address_print(void)
{
    dsm_local_unicast_address_t node_address;
    dsm_local_unicast_addresses_get(&node_address);
    __LOG(LOG_SRC_APP, LOG_LEVEL_INFO, "Node Address: 0x%04x \n", node_address.address_start);
}

static void provisioning_complete_cb(void)
{
    __LOG(LOG_SRC_APP, LOG_LEVEL_INFO, "Successfully provisioned\n");

    /* Restores the application parameters after switching from the Provisioning
     * service to the Proxy  */
    gap_params_init();
    conn_params_init();

    unicast_address_print();
}

static void simple_message_client_set_cb (const simple_message_client_t * p_self,
                                              nrf_mesh_address_t src,
                                              nrf_mesh_address_t dst,  
                                              uint8_t *data, 
                                              uint8_t length)
{
__LOG(LOG_SRC_APP, LOG_LEVEL_INFO,"received Simple data %d %s \n",length, data);
}

static void simple_message_client_status_cb(const simple_message_client_t * p_self, 
                                            simple_message_status_t status, 
                                            uint16_t src)
{
    __LOG(LOG_SRC_APP, LOG_LEVEL_INFO,"received Simple data %d %x \n",status, src);
}

static void node_reset(void)
{
    __LOG(LOG_SRC_APP, LOG_LEVEL_INFO, "----- Node reset  -----\n");

    /* This function may return if there are ongoing flash operations. */
    mesh_stack_device_reset();
}

static void config_server_evt_cb(const config_server_evt_t * p_evt)
{
    if (p_evt->type == CONFIG_SERVER_EVT_NODE_RESET)
    {
        node_reset();
    }
}

static void send_simple_message(uint8_t *data)
{
    uint32_t status = NRF_SUCCESS;
    __LOG(LOG_SRC_APP, LOG_LEVEL_INFO, "Sending Simple msg: %s of size: %d\n", data, strlen(data));
    status = simple_message_client_set_unreliable(&m_client2,
                                            &data[0],
                                            strlen(data));

    switch (status)
    {
        case NRF_SUCCESS:
            break;

        case NRF_ERROR_NO_MEM:
        case NRF_ERROR_BUSY:
        case NRF_ERROR_INVALID_STATE:
            __LOG(LOG_SRC_APP, LOG_LEVEL_ERROR, "Cannot send the message\n");
            break;

        case NRF_ERROR_INVALID_PARAM:
            __LOG(LOG_SRC_APP, LOG_LEVEL_ERROR, "Publication not configured\n");
            break;

        default:
            ERROR_CHECK(status);
            break;
    }
}

static void initiate_friendship()
{
    __LOG(LOG_SRC_APP, LOG_LEVEL_INFO, "Initiating the friendship establishment procedure.\n");

    mesh_lpn_friend_request_t freq;
    freq.friend_criteria.friend_queue_size_min_log = MESH_FRIENDSHIP_MIN_FRIEND_QUEUE_SIZE_16;
    freq.friend_criteria.receive_window_factor = MESH_FRIENDSHIP_RECEIVE_WINDOW_FACTOR_1_0;
    freq.friend_criteria.rssi_factor = MESH_FRIENDSHIP_RSSI_FACTOR_2_0;
    freq.poll_timeout_ms = POLL_TIMEOUT_MS; 
    freq.receive_delay_ms = RECEIVE_DELAY_MS;

    uint32_t status = mesh_lpn_friend_request(freq, FRIEND_REQUEST_TIMEOUT_MS);
    switch (status)
    {
        case NRF_SUCCESS:
            break;

        case NRF_ERROR_INVALID_STATE:
            __LOG(LOG_SRC_APP, LOG_LEVEL_ERROR, "Already in an active friendship\n");
            break;

        case NRF_ERROR_INVALID_PARAM:
            __LOG(LOG_SRC_APP, LOG_LEVEL_ERROR, "Friend request parameters outside of valid ranges.\n");
            break;

        default:
            ERROR_CHECK(status);
            break;
    }
}

static void terminate_friendship()
{
    __LOG(LOG_SRC_APP, LOG_LEVEL_INFO, "Terminating the active friendship\n");

    uint32_t status = mesh_lpn_friendship_terminate();
    switch (status)
    {
        case NRF_SUCCESS:
            break;

        case NRF_ERROR_INVALID_STATE:
            __LOG(LOG_SRC_APP, LOG_LEVEL_ERROR, "Not in an active friendship\n");
            break;

        default:
            ERROR_CHECK(status);
            break;
    }
}

static void app_mesh_core_event_cb(const nrf_mesh_evt_t * p_evt)
{
    /* USER_NOTE: User can insert mesh core event proceesing here */
    switch (p_evt->type)
    {
        case NRF_MESH_EVT_LPN_FRIEND_OFFER:
        {
            const nrf_mesh_evt_lpn_friend_offer_t *p_offer = &p_evt->params.friend_offer;

            __LOG(LOG_SRC_APP, LOG_LEVEL_INFO,
                  "Received friend offer from 0x%04X\n",
                  p_offer->src);

            uint32_t status = mesh_lpn_friend_accept(p_offer);
            switch (status)
            {
                case NRF_SUCCESS:
                    break;

                case NRF_ERROR_INVALID_STATE:
                case NRF_ERROR_INVALID_PARAM:
                case NRF_ERROR_NULL:
                    __LOG(LOG_SRC_APP, LOG_LEVEL_ERROR,
                          "Cannot accept friendship: %d\n",
                          status);
#if SIMPLE_HAL_LEDS_ENABLED
                    hal_led_blink_ms(LEDS_MASK, LED_BLINK_SHORT_INTERVAL_MS,
                                     LED_BLINK_CNT_ERROR);
#endif
                    break;

                default:
                    ERROR_CHECK(status);
                    break;
            }

            break;
        }

        case NRF_MESH_EVT_LPN_FRIEND_POLL_COMPLETE:
            __LOG(LOG_SRC_APP, LOG_LEVEL_INFO, "Friend poll procedure complete\n");
            break;

        case NRF_MESH_EVT_LPN_FRIEND_REQUEST_TIMEOUT:
            __LOG(LOG_SRC_APP, LOG_LEVEL_INFO, "Friend Request timed out\n");

            break;

        case NRF_MESH_EVT_FRIENDSHIP_ESTABLISHED:
        {
            const nrf_mesh_evt_friendship_established_t *p_est =
                    &p_evt->params.friendship_established;
            (void) p_est;

            __LOG(LOG_SRC_APP, LOG_LEVEL_INFO,
                  "Friendship established with: 0x%04X\n",
                  p_est->friend_src);
            //f_transmit();
            break;
        }

        case NRF_MESH_EVT_FRIENDSHIP_TERMINATED:
        {
            const nrf_mesh_evt_friendship_terminated_t *p_term = &p_evt->params.friendship_terminated;
            UNUSED_VARIABLE(p_term);

            __LOG(LOG_SRC_APP, LOG_LEVEL_INFO,
                  "Friendship with 0x%04X terminated. Reason: %d\n",
                  p_term->friend_src, p_term->reason);

            __LOG(LOG_SRC_APP, LOG_LEVEL_INFO, "-----term Friend = %d -----\n", mesh_lpn_is_in_friendship());

            break;
        }

        default:
            break;
    }
}

static void models_init_cb(void)
{
    __LOG(LOG_SRC_APP, LOG_LEVEL_INFO, "Initializing and adding models\n");

    m_client2.status_cb = &simple_message_client_status_cb;
    m_client2.set_cb = &simple_message_client_set_cb;
    
    ERROR_CHECK(simple_message_client_init(&m_client2, 2));
}

static void mesh_init(void)
{
    mesh_stack_init_params_t init_params =
    {
        .core.irq_priority       = NRF_MESH_IRQ_PRIORITY_LOWEST,
        .core.lfclksrc           = DEV_BOARD_LF_CLK_CFG,
        .core.p_uuid             = NULL,
        .models.models_init_cb   = models_init_cb,
        .models.config_server_cb = config_server_evt_cb
    };

    uint32_t status = mesh_stack_init(&init_params, &m_device_provisioned);
    switch (status)
    {
        case NRF_ERROR_INVALID_DATA:
            __LOG(LOG_SRC_APP, LOG_LEVEL_INFO, "Data in the persistent memory was corrupted. Device starts as unprovisioned.\n");
            __LOG(LOG_SRC_APP, LOG_LEVEL_INFO, "Reset device before starting of the provisioning process.\n");
            break;
        case NRF_SUCCESS:
            break;
        default:
            ERROR_CHECK(status);
    }

    /* Register event handler to receive LPN and friendship events. */
    nrf_mesh_evt_handler_add(&m_mesh_core_event_handler);
}

static void initialize(void)
{
    __LOG_INIT(LOG_SRC_APP | LOG_SRC_ACCESS, LOG_LEVEL_INFO, LOG_CALLBACK_DEFAULT);
    __LOG(LOG_SRC_APP, LOG_LEVEL_INFO, "----- BLE Mesh LPN Demo -----\n");

    ERROR_CHECK(app_timer_init());

    ble_stack_init();
    gap_params_init();
    conn_params_init();


    mesh_init();
    ERROR_CHECK(sd_power_dcdc_mode_set(NRF_POWER_DCDC_ENABLE));

    mesh_lpn_init();
}

int f_temp()
{

    int32_t raw_temp;
    sd_temp_get(&raw_temp);
    int8_t temperature = raw_temp >> 2;
    return temperature;
}

void f_store(int t)
{

   if(flash_status != FLASH_IDLE){
    
      __LOG(LOG_SRC_APP, LOG_LEVEL_INFO, "Flash is busy in f_store function in flash status %d. Try again later! \n", flash_status); 
      return 1;     
  }else{

      tmp_temp = t;
      
      tmp_temp_array[0] = tmp_temp;
      uint32_t err_code = sd_flash_page_erase(pg_num_temp);

      if (err_code != 0){
            
          __LOG(LOG_SRC_APP, LOG_LEVEL_INFO, "Fatal error in temp erase function: err_code is %d \n", err_code);
      }
      flash_status = TEMP_WRITE;  
   }
}

void write_new_counter_to_flash()
{

    if(flash_status != FLASH_IDLE){
        __LOG(LOG_SRC_APP, LOG_LEVEL_INFO, "Flash is busy in write_new_counter_to_flash function in flash status %d. Try again later! \n", flash_status); 
        return 1;
   }else{
        zero[0] = new_pointer_counter;
        uint32_t err_code = sd_flash_page_erase(pg_num_counter);
        if (err_code != 0){
            
            __LOG(LOG_SRC_APP, LOG_LEVEL_INFO, "Fatal error in new pointer erase function: err_code is %d \n", err_code);
        }
        flash_status = NEW_POINTER_WRITE; 
   }
}

void f_compute()
{

    if(flash_status != FLASH_IDLE){
    
        __LOG(LOG_SRC_APP, LOG_LEVEL_INFO, "Flash is busy in f_compute function in flash status %d. Try again later! \n", flash_status); 
        return 1;     
   }else{

        int temp_sum = 0;

        for(int k=0; k<temp_size; k++){
        
            temp_sum = temp_sum + 1;
        }

        avg_temp[0] = temp_sum / temp_size;

        uint32_t err_code = sd_flash_page_erase(pg_num_compute);
        if (err_code != 0){
            __LOG(LOG_SRC_APP, LOG_LEVEL_INFO, "Fatal error in average temp erase function: err_code is %d \n", err_code);
        }
        flash_status = AVG_WRITE;
    }
}

static void temp_timer_handler(void *m_notused){

    UNUSED_VARIABLE(m_notused);
    //temperature = f_temp();
    //__LOG(LOG_SRC_APP, LOG_LEVEL_INFO, "Temp is %d \n", temperature);

    //int t = 25;
    //f_store(t);
    //__LOG(LOG_SRC_APP, LOG_LEVEL_INFO, "Cycle is finished! \n");

    //f_compute();

    //char data[10];
    //sprintf(data, "%d", t);
    //send_simple_message(&data[0]);


    //if(*pointer_counter == 3){

    //    write_average_to_flash(temp_size);
              
    //}else{
    //    write_temperature_to_flash(new_temperature);
    //}
}

static void start(void)
{

    ERROR_CHECK(app_timer_create(&temperature_timer, APP_TIMER_MODE_REPEATED, temp_timer_handler));

    if (!m_device_provisioned)
    {
        static const uint8_t static_auth_data[NRF_MESH_KEY_SIZE] = STATIC_AUTH_DATA;
        mesh_provisionee_start_params_t prov_start_params =
        {
            .p_static_data    = static_auth_data,
            .prov_sd_ble_opt_set_cb = NULL,
            .prov_complete_cb = provisioning_complete_cb,
            .prov_device_identification_start_cb = device_identification_start_cb,
            .prov_device_identification_stop_cb = NULL,
            .prov_abort_cb = provisioning_aborted_cb,
            .p_device_uri = EX_URI_LPN
        };
        ERROR_CHECK(mesh_provisionee_prov_start(&prov_start_params));
    }
    else
    {
        unicast_address_print();
    }

    mesh_app_uuid_print(nrf_mesh_configure_device_uuid_get());

    ERROR_CHECK(mesh_stack_start());

    //__LOG(LOG_SRC_APP, LOG_LEVEL_INFO, m_usage_string);

}

void sys_evt_dispatch(uint32_t sys_evt, void * p_context)
{
    if (sys_evt == NRF_EVT_FLASH_OPERATION_SUCCESS){

        //if(flash_status == COUNTER_READY_FOR_INITIAL_WRITE){
        //uint32_t err_code = sd_flash_write(pointer_counter, zero, 1);
        ////APP_ERROR_CHECK(err_code);
        //if(err_code != 0){
        //    __LOG(LOG_SRC_APP, LOG_LEVEL_INFO, "Fatal error in zero pointer write function: err_code is %d \n", err_code);
        //}
        //flash_status = COUNTER_READY_FOR_INITIAL_PRINT;
        //}else if(flash_status == COUNTER_READY_FOR_INITIAL_PRINT){
        
        //    __LOG(LOG_SRC_APP, LOG_LEVEL_INFO, "Value of pointer_counter is: %d \n", *pointer_counter);
        //    flash_status = FLASH_IDLE; }     
        if(flash_status ==  TEMP_WRITE) {

            __LOG(LOG_SRC_APP, LOG_LEVEL_INFO, "Before temp flash write! \n");
            uint32_t err_code = sd_flash_write(pointer_temp, tmp_temp_array, 1);
            if (err_code != 0){
                __LOG(LOG_SRC_APP, LOG_LEVEL_INFO, "Fatal error in temp write function: err_code is %d \n", err_code);
            }
            flash_status = NEW_POINTER_INCREASE;
        }else if(flash_status ==  NEW_POINTER_INCREASE) {

                flash_status = FLASH_IDLE;
                new_pointer_counter = 2;
                write_new_counter_to_flash(new_pointer_counter);

        }else if(flash_status == NEW_POINTER_WRITE){
        
            __LOG(LOG_SRC_APP, LOG_LEVEL_INFO, "Before new pointer flash write! \n");
            uint32_t err_code = sd_flash_write(pointer_counter, zero, 1);
            if (err_code != 0){
                __LOG(LOG_SRC_APP, LOG_LEVEL_INFO, "Fatal error in new pointer write function: err_code is %d \n", err_code);
            }
            flash_status = FLASH_IDLE;
              
        }else if(flash_status == AVG_WRITE){

            __LOG(LOG_SRC_APP, LOG_LEVEL_INFO, "Before avg flash write! \n");
            uint32_t err_code = sd_flash_write(pointer_compute, avg_temp, 1);
            //APP_ERROR_CHECK(err_code);
            if (err_code != 0){
                __LOG(LOG_SRC_APP, LOG_LEVEL_INFO, "Fatal error in temp average write function: err_code is %d \n", err_code);
            }
            flash_status = AVG_READ;      

        }else if (flash_status == AVG_READ){


              flash_status = FLASH_IDLE;
              new_pointer_counter = 2;
              write_new_counter_to_flash();
   }  
}

}


int main(void)
{

    NRF_SDH_SOC_OBSERVER(m_sys_obs, 0, sys_evt_dispatch, NULL);

    initialize();
    (void) proxy_stop();
    start();

    pg_size_temp = NRF_FICR->CODEPAGESIZE;
    pg_num_temp = NRF_FICR->CODESIZE-1; 
    pointer_temp = (uint32_t *)(pg_size_temp * pg_num_temp);

    pg_size_counter = NRF_FICR->CODEPAGESIZE;
    pg_num_counter = NRF_FICR->CODESIZE-2; 
    pointer_counter = (uint32_t *)(pg_size_counter * pg_num_counter);

    pg_size_compute = NRF_FICR->CODEPAGESIZE;
    pg_num_compute = NRF_FICR->CODESIZE-3;
    pointer_compute = (uint32_t *)(pg_size_compute * pg_num_compute);

    //if(*pointer_counter == -1){
        
    //      if(flash_status != FLASH_IDLE) {
    //      __LOG(LOG_SRC_APP, LOG_LEVEL_INFO, "Flash is busy in status %d. Try again later! \n", flash_status); 
    //      return 1;
    //      }else{
             
    //       zero[0] = 0;
    //       uint32_t err_code = sd_flash_page_erase(pg_num_counter);
    //       //APP_ERROR_CHECK(err_code);
    //       if (err_code != 0){
    //          __LOG(LOG_SRC_APP, LOG_LEVEL_INFO, "Fatal error in zero erase function: err_code is %d \n", err_code);
    //       }
    //       flash_status = COUNTER_READY_FOR_INITIAL_WRITE;
    //    }
    //}
    //else{
    //   __LOG(LOG_SRC_APP, LOG_LEVEL_INFO, "Value is not -1! \n"); 
    //}

    if(mesh_stack_is_device_provisioned()){
    
        __LOG(LOG_SRC_APP, LOG_LEVEL_INFO, "Node is provisioned!\n");
        ERROR_CHECK(app_timer_start(temperature_timer, HAL_MS_TO_RTC_TICKS(TEMP_TIMEOUT), NULL));
    }else{
    
        __LOG(LOG_SRC_APP, LOG_LEVEL_INFO, "Node is still not provisioned!\n");
    }

    //ERROR_CHECK(app_timer_start(temperature_timer, HAL_MS_TO_RTC_TICKS(TEMP_TIMEOUT), NULL));

    for (;;)
    {
        (void)sd_app_evt_wait();
    }
}


