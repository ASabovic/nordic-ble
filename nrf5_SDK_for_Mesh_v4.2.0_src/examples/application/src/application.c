#include "application.h"

MESH_CONFIG_FILE(m_app_file, M_APP_FILE_ID, MESH_CONFIG_STRATEGY_CONTINUOUS);

/* Temperature array */
int m_live_temp[MAX_TEMP_AMOUNT];

uint32_t m_app_temp_setter(mesh_config_entry_id_t id, const void *p_entry)
{
    if (!IS_IN_RANGE(id.record, TEMP_RECORD_START, TEMP_RECORD_END))
    {
        return NRF_ERROR_NOT_FOUND;
    }

    uint16_t idx = id.record - TEMP_RECORD_START;

    const int *p_app_temp = (const int *)p_entry;

    m_live_temp[idx] = *p_app_temp;

    return NRF_SUCCESS;
}

void m_app_temp_getter(mesh_config_entry_id_t id, void *p_entry)
{
    NRF_MESH_ASSERT_DEBUG(IS_IN_RANGE(id.record, TEMP_RECORD_START, TEMP_RECORD_END));

    uint16_t idx = id.record - TEMP_RECORD_START;

    int *p_app_temp = (int *)p_entry;

    *p_app_temp = m_live_temp[idx];
}

MESH_CONFIG_ENTRY(m_app_temp,        // Name
                  APP_ENTRY_TEMP_ID, // Entry id
                  MAX_TEMP_AMOUNT,   // Amount of items
                  sizeof(int),       // Size of item
                  m_app_temp_setter, // Setter
                  m_app_temp_getter, // Getter
                  NULL,              // Deleter
                  false);            // Has default value

/* Amount of measured temperature values */
int m_live_temp_am;

uint32_t m_app_temp_am_setter(mesh_config_entry_id_t id, const void *p_entry)
{
    const int *p_app_temp_am = (const int *)p_entry;
    m_live_temp_am = *p_app_temp_am;
    return NRF_SUCCESS;
}

void m_app_temp_am_getter(mesh_config_entry_id_t id, void *p_entry)
{
    int *p_app_temp_am = (int *)p_entry;
    *p_app_temp_am = m_live_temp_am;
}

MESH_CONFIG_ENTRY(m_app_temp_am,
                  APP_ENTRY_TEMP_AM_ID,
                  1,
                  sizeof(int),
                  m_app_temp_am_setter,
                  m_app_temp_am_getter,
                  NULL,
                  false);

/* Average temperature value */
int m_live_avg_temp;

uint32_t m_app_avg_temp_setter(mesh_config_entry_id_t id, const void *p_entry)
{
    const int *p_app_avg_temp = (const int *)p_entry;
    m_live_avg_temp = *p_app_avg_temp;
    return NRF_SUCCESS;
}

void m_app_avg_temp_getter(mesh_config_entry_id_t id, void *p_entry)
{
    int *p_app_avg_temp = (int *)p_entry;
    *p_app_avg_temp = m_live_avg_temp;
}

MESH_CONFIG_ENTRY(m_app_avg_temp,
                  APP_ENTRY_AVG_TEMP_ID,
                  1,
                  sizeof(int),
                  m_app_avg_temp_setter,
                  m_app_avg_temp_getter,
                  NULL,
                  false);
int temperature;
int average_temperature;
int temperature_size;

simple_message_client_t m_client2;
bool m_device_provisioned;

nrf_mesh_evt_handler_t m_mesh_core_event_handler = { .evt_cb = app_mesh_core_event_cb };

/* SAADC part */
/* Define saadc_callback_handler function */
void saadc_callback_handler(nrf_drv_saadc_evt_t const * p_event)
{
 // Empty handler function
}

/* SAADC initialization */
void saadc_init(void)
{

    ret_code_t err_code;

    nrf_saadc_channel_config_t channel_config = NRFX_SAADC_DEFAULT_CHANNEL_CONFIG_SE(SAADC_CH_PSELP_PSELP_VDD);

    err_code = nrf_drv_saadc_init(NULL, saadc_callback_handler);
    ERROR_CHECK(err_code);

    err_code = nrfx_saadc_channel_init(0, &channel_config);
    ERROR_CHECK(err_code);
}

void device_identification_start_cb(uint8_t attention_duration_s)
{
    __LOG(LOG_SRC_APP, LOG_LEVEL_INFO, "Device identified \n");
}

void provisioning_aborted_cb(void)
{
    __LOG(LOG_SRC_APP, LOG_LEVEL_INFO, "Provisioning aborted \n");
}

void unicast_address_print(void)
{
    dsm_local_unicast_address_t node_address;
    dsm_local_unicast_addresses_get(&node_address);
    __LOG(LOG_SRC_APP, LOG_LEVEL_INFO, "Node Address: 0x%04x \n", node_address.address_start);
}

void provisioning_complete_cb(void)
{
    __LOG(LOG_SRC_APP, LOG_LEVEL_INFO, "Successfully provisioned\n");

    /* Restores the application parameters after switching from the Provisioning
     * service to the Proxy  */
    gap_params_init();
    conn_params_init();

    unicast_address_print();
}

void led_timeout_handler(){

    ret_code_t err_code;
    err_code = app_timer_stop(led_timer_id);
    APP_ERROR_CHECK(err_code);
    nrf_drv_gpiote_out_toggle(PIN_OUT_LED);
}

void simple_message_client_set_cb (const simple_message_client_t * p_self,
                                              nrf_mesh_address_t src,
                                              nrf_mesh_address_t dst,  
                                              uint8_t *data, 
                                              uint8_t length)
{
__LOG(LOG_SRC_APP, LOG_LEVEL_INFO,"received Simple data %d %s \n",length, data);
}

void simple_message_client_status_cb(const simple_message_client_t * p_self, 
                                            simple_message_status_t status, 
                                            uint16_t src)
{
    __LOG(LOG_SRC_APP, LOG_LEVEL_INFO,"received Simple data %d %x \n",status, src);
}

void node_reset(void)
{
    __LOG(LOG_SRC_APP, LOG_LEVEL_INFO, "----- Node reset  -----\n");

    /* This function may return if there are ongoing flash operations. */
    mesh_stack_device_reset();
}

void config_server_evt_cb(const config_server_evt_t * p_evt)
{
    if (p_evt->type == CONFIG_SERVER_EVT_NODE_RESET)
    {
        node_reset();
    }
}

/* Fourth task - send the average temperature value */
void f_transmit(uint8_t *data)
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

void f_friendship()
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

void terminate_friendship()
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

void app_mesh_core_event_cb(const nrf_mesh_evt_t * p_evt)
{
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

void models_init_cb(void)
{
    __LOG(LOG_SRC_APP, LOG_LEVEL_INFO, "Initializing and adding models\n");

    m_client2.status_cb = &simple_message_client_status_cb;
    m_client2.set_cb = &simple_message_client_set_cb;
    
    ERROR_CHECK(simple_message_client_init(&m_client2, 2));
}

void mesh_init(void)
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

/* First task - temperature measurement */
void f_temp()
{

    int32_t raw_temp;
    sd_temp_get(&raw_temp);
    temperature = raw_temp >> 2;
}

/* Second task - Store temperature values in flash */
void f_store()
{
    mesh_config_entry_get(APP_ENTRY_TEMP_AM_ID, &temperature_size);
    mesh_config_entry_get(APP_ENTRY_TEMP_ID, &temperature);

    mesh_config_entry_id_t temp_id = APP_ENTRY_TEMP_ID;
    temp_id.record += temperature_size++;
    mesh_config_entry_set(temp_id, &temperature);
    mesh_config_entry_set(APP_ENTRY_TEMP_AM_ID, &temperature_size);
}

/* Third task - compute the average temperature value */
void f_compute()
{
    mesh_config_entry_id_t temp_id = APP_ENTRY_TEMP_ID;
    average_temperature = 0;

    for(int k=0; k < MAX_TEMP_AMOUNT; k++){
    
        int temp_sum = 0;
        temp_id.record = TEMP_RECORD_START + k;
        mesh_config_entry_get(temp_id, &temp_sum);
        average_temperature += temp_sum;
    }

    average_temperature /= MAX_TEMP_AMOUNT;
    mesh_config_entry_set(APP_ENTRY_AVG_TEMP_ID, &average_temperature);
    temperature_size = 0;
    mesh_config_entry_set(APP_ENTRY_TEMP_AM_ID, &temperature_size);
}

void led_task(){

    nrf_drv_gpiote_out_set(PIN_OUT_LED);
    ret_code_t err_code = app_timer_start(led_timer_id,HAL_MS_TO_RTC_TICKS(LED_ENABLE_TIME),NULL);
    APP_ERROR_CHECK(err_code);
}

void initialize(void)
{
    __LOG_INIT(LOG_SRC_APP | LOG_SRC_ACCESS, LOG_LEVEL_INFO, LOG_CALLBACK_DEFAULT);
    __LOG(LOG_SRC_APP, LOG_LEVEL_INFO, "----- BLE Mesh LPN Demo -----\n");

    ERROR_CHECK(app_timer_init());

    ret_code_t err_code;
    err_code = nrf_drv_gpiote_init();

    nrf_drv_gpiote_out_config_t     out_config = GPIOTE_CONFIG_OUT_SIMPLE(false);
    err_code = nrf_drv_gpiote_out_init(PIN_OUT_LED, &out_config);
    APP_ERROR_CHECK(err_code);

    ble_stack_init();
    gap_params_init();
    conn_params_init();

    mesh_init();
    ERROR_CHECK(sd_power_dcdc_mode_set(NRF_POWER_DCDC_ENABLE));

    mesh_lpn_init();
    saadc_init();
}

void start(void)
{
    ERROR_CHECK(app_timer_create(&led_timer_id,APP_TIMER_MODE_SINGLE_SHOT,led_timeout_handler));
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

}
