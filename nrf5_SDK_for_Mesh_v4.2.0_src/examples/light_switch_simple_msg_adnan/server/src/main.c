#include <stdint.h>
#include <string.h>

/* HAL */
#include "boards.h"
#include "simple_hal.h"
#include "app_timer.h"

/* Core */
#include "nrf_mesh_config_core.h"
#include "nrf_mesh_gatt.h"
#include "nrf_mesh_events.h"
#include "nrf_mesh_configure.h"
#include "nrf_mesh.h"
#include "mesh_stack.h"
#include "device_state_manager.h"
#include "access_config.h"
#include "proxy.h"

/* Provisioning and configuration */
#include "mesh_provisionee.h"
#include "mesh_app_utils.h"

/* Models */
#include "simple_message_server.h"
#include "simple_message_common.h"

/* Logging and RTT */
#include "log.h"
//#include "rtt_input.h"

/* Example specific includes */
#include "app_config.h"
#include "example_common.h"
#include "nrf_mesh_config_examples.h"
#include "light_switch_example_common.h"
#include "app_onoff.h"
#include "ble_softdevice_support.h"

/*****************************************************************************
 * Definitions
 *****************************************************************************/
#define APP_ONOFF_ELEMENT_INDEX     (0)

/*****************************************************************************
 * Forward declaration of static functions
 *****************************************************************************/
static void app_mesh_core_event_cb(const nrf_mesh_evt_t * p_evt);

static void simple_message_client_set_cb(const simple_message_server_t * p_server, nrf_mesh_address_t src, nrf_mesh_address_t dst,uint8_t *data, uint8_t length);
static void simple_message_client_get_cb(const simple_message_server_t * p_self,nrf_mesh_address_t src,  uint8_t *data, uint8_t length);

/*****************************************************************************
 * Static variables
 *****************************************************************************/
static bool m_device_provisioned;

static simple_message_server_t m_simple_message_server_0; 

static nrf_mesh_evt_handler_t m_mesh_core_event_handler = { .evt_cb = app_mesh_core_event_cb };                

static void simple_message_client_set_cb(const simple_message_server_t * p_server, nrf_mesh_address_t src, nrf_mesh_address_t dst,uint8_t *data, uint8_t length)
{
 // TODO
 __LOG(LOG_SRC_APP, LOG_LEVEL_INFO, "Received message: %s, Received length: %d\n", data, strlen(data));
}
static void simple_message_client_get_cb(const simple_message_server_t * p_self,nrf_mesh_address_t src,  uint8_t *data, uint8_t length)
{
  //TODO
  __LOG(LOG_SRC_APP, LOG_LEVEL_INFO, "Received message: %s, Received lengt: %d \n", data, strlen(data));
}

static void app_model_init(void)
{
    m_simple_message_server_0.set_cb = &simple_message_client_set_cb;
    m_simple_message_server_0.get_cb = &simple_message_client_get_cb;

    ERROR_CHECK(simple_message_server_init(&m_simple_message_server_0, APP_ONOFF_ELEMENT_INDEX+1));
    __LOG(LOG_SRC_APP, LOG_LEVEL_INFO, "Simple Message Model Handle: %d\n", m_simple_message_server_0.model_handle);
}

/*************************************************************************************************/

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

static void device_identification_start_cb(uint8_t attention_duration_s)
{
    //Empty function
}

static void provisioning_aborted_cb(void)
{
    //Empty function
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

#if MESH_FEATURE_GATT_ENABLED
    /* Restores the application parameters after switching from the Provisioning
     * service to the Proxy  */
    gap_params_init();
    conn_params_init();
#endif

    unicast_address_print();
}

static void models_init_cb(void)
{
    __LOG(LOG_SRC_APP, LOG_LEVEL_INFO, "Initializing and adding models\n");
    app_model_init();
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
}

static void initialize(void)
{
    __LOG_INIT(LOG_SRC_APP | LOG_SRC_FRIEND, LOG_LEVEL_DBG1, LOG_CALLBACK_DEFAULT);
    __LOG(LOG_SRC_APP, LOG_LEVEL_INFO, "----- BLE Mesh Light Switch Server Demo -----\n");

    ERROR_CHECK(app_timer_init());

    ble_stack_init();

#if MESH_FEATURE_GATT_ENABLED
    gap_params_init();
    conn_params_init();
#endif

    mesh_init();
    ERROR_CHECK(sd_power_dcdc_mode_set(NRF_POWER_DCDC_ENABLE));  
}

static void start(void)
{

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
            .p_device_uri = EX_URI_LS_SERVER
        };
        ERROR_CHECK(mesh_provisionee_prov_start(&prov_start_params));
    }
    else
    {
        unicast_address_print();
    }

    mesh_app_uuid_print(nrf_mesh_configure_device_uuid_get());

    ERROR_CHECK(mesh_stack_start());
}

static void app_mesh_core_event_cb(const nrf_mesh_evt_t * p_evt)
{
  /* USER_NOTE: User can insert mesh core event proceesing here */
    switch (p_evt->type)
    {
         case NRF_MESH_EVT_FRIENDSHIP_ESTABLISHED:
        {
            const nrf_mesh_evt_friendship_established_t *p_est =
                    &p_evt->params.friendship_established;
            (void) p_est;

            __LOG(LOG_SRC_APP, LOG_LEVEL_INFO,
                  "Friendship established with: 0x%04X\n",
                  p_est->friend_src);
            break;
         }

         case NRF_MESH_EVT_FRIENDSHIP_TERMINATED:
        {
            const nrf_mesh_evt_friendship_terminated_t *p_term = &p_evt->params.friendship_terminated;
            UNUSED_VARIABLE(p_term);

            __LOG(LOG_SRC_APP, LOG_LEVEL_INFO,
                  "Friendship with 0x%04X terminated. Reason: %d\n",
                  p_term->friend_src, p_term->reason);

            break;

         }

    }

}


int main(void)
  {
    initialize();
    (void) proxy_stop();
    start();

    if (!mesh_stack_is_device_provisioned())
    {
        __LOG(LOG_SRC_APP, LOG_LEVEL_INFO, "The device is not provisioned.\n");
    }
    else
    {
        __LOG(LOG_SRC_APP, LOG_LEVEL_INFO, "----- Node is already Provisioned  -----\n");
    }

    for (;;)
    {
        (void)sd_app_evt_wait();
    }
}
