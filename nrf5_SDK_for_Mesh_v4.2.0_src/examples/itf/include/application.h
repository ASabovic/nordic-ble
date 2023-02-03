#ifndef APPLICATION_H__
#define APPLICATION_H__

#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include <stdbool.h>
#include <math.h> 

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

/* Flash states definition */
/* Flash idle */
#define FLASH_IDLE 0

/* Flash - tasks part */
#define TASKS_WRITE 1

/* The maximum duration to scan for incoming Friend Offers. */
#define FRIEND_REQUEST_TIMEOUT_MS (MESH_LPN_FRIEND_REQUEST_TIMEOUT_MAX_MS)

/* The upper limit for two subsequent Friend Polls. */
#define POLL_TIMEOUT_MS (SEC_TO_MS(17))

/* The time between LPN sending a request and listening for a response. */
#define RECEIVE_DELAY_MS (100)

#ifdef BOARD_PCA10059
#define PIN_OUT_BUZZER NRF_GPIO_PIN_MAP(1,15)
#else
#define PIN_OUT_LED NRF_GPIO_PIN_MAP(1,15)
#define PIN_OUT_LED2 NRF_GPIO_PIN_MAP(1,14)
#define PIN_OUT NRF_GPIO_PIN_MAP(1,13)
#define PIN_OUT_SENSE NRF_GPIO_PIN_MAP(1, 10)
#define PIN_OUT_SEND NRF_GPIO_PIN_MAP(1, 12)

#endif

#define LED_ENABLE_TIME (3*1000)
#define LED2_ENABLE_TIME (110*1000)
#define SENSE_ENABLE_TIME (100)
#define SEND_ENABLE_TIME (100)

#define PIN_CAP_READ SAADC_CH_PSELP_PSELP_AnalogInput5
#define GPIO_ENABLE_TIME (5)

/* LED timer */
APP_TIMER_DEF(led_timer_id);
APP_TIMER_DEF(led2_timer_id);

APP_TIMER_DEF(gpio_timer_id);

APP_TIMER_DEF(sense_timer_id);
APP_TIMER_DEF(send_timer_id);

/* Create file ID */
#define M_APP_FILE_ID (0x0010)

/* Temperature array */
#define MAX_TEMP_AMOUNT (3)
#define TEMP_RECORD_START (0x0001)
#define TEMP_RECORD_END (TEMP_RECORD_START + MAX_TEMP_AMOUNT - 1)
#define APP_ENTRY_TEMP_ID MESH_CONFIG_ENTRY_ID(M_APP_FILE_ID, TEMP_RECORD_START)
extern int m_live_temp[MAX_TEMP_AMOUNT];

extern uint32_t m_app_temp_setter(mesh_config_entry_id_t id, const void *p_entry);
extern void m_app_temp_getter(mesh_config_entry_id_t id, void *p_entry);

/* Amount of measured temperature values */
#define TEMP_AM_RECORD (TEMP_RECORD_END + 1)
#define APP_ENTRY_TEMP_AM_ID MESH_CONFIG_ENTRY_ID(M_APP_FILE_ID, TEMP_AM_RECORD)
extern int m_live_temp_am;

extern uint32_t m_app_temp_am_setter(mesh_config_entry_id_t id, const void *p_entry);
extern void m_app_temp_am_getter(mesh_config_entry_id_t id, void *p_entry);

/* Average temperature value */
#define AVG_TEMP_RECORD (TEMP_AM_RECORD + 1)
#define APP_ENTRY_AVG_TEMP_ID MESH_CONFIG_ENTRY_ID(M_APP_FILE_ID, AVG_TEMP_RECORD)
extern int m_live_avg_temp;

extern uint32_t m_app_avg_temp_setter(mesh_config_entry_id_t id, const void *p_entry);
extern void m_app_avg_temp_getter(mesh_config_entry_id_t id, void *p_entry);

extern int temperature;
extern int average_temperature;
extern int temperature_size;

/* Variables related to the flash part */
extern uint8_t flash_status;

/* Flash parts */
/* Tasks part */
extern uint32_t pg_size_tasks;
extern uint32_t pg_num_tasks;
extern struct Task *pointer_tasks;

extern simple_message_client_t m_client2;
extern bool m_device_provisioned;

extern nrf_mesh_evt_handler_t m_mesh_core_event_handler;

/* SAADC part */
/* SAADC callback handler function */
extern void saadc_callback_handler(nrf_drv_saadc_evt_t const * p_event);

/* SAADC initialization function */
extern void saadc_init(void);

/* Sending data function */
extern void f_transmit(uint8_t *data);
extern void simple_message_client_status_cb(const simple_message_client_t * p_self, simple_message_status_t status, uint16_t src);
extern void simple_message_client_set_cb (const simple_message_client_t * p_self, nrf_mesh_address_t src, nrf_mesh_address_t dst, uint8_t *data, uint8_t length);

extern void models_init_cb(void);

/* Friendship part */
extern void f_friendship();
extern void terminate_friendship();
extern void app_mesh_core_event_cb (const nrf_mesh_evt_t * p_evt);

/* Mesh init part */
extern void mesh_init(void);

/* Initialize function */
extern void initialize(void);

/* Temperature measurement function */
extern void f_temp();

/* Store the temperature and pointer values in the flash */
extern void f_store();

/* Compute the average temperature value */
extern void f_compute();

/* LED timer function */
extern void led_timeout_handler();
extern void led2_timeout_handler();
extern void led_task();
extern void led2_task();
extern void gpio_timeout_handler();
extern void gpio_init_cap(void);
extern void create_timers_cap(void);
nrf_saadc_value_t get_cap_val();
extern uint8_t if_energy(uint16_t v_th);
extern float cap_task();

extern void sensing_timeout_handler();
extern void send_timeout_handler();
extern void sensing_gpio_task();
extern void send_gpio_task();

/* Start function */
extern void provisioning_complete_cb(void);
extern void unicast_address_print(void);
extern void device_identification_start_cb(uint8_t attention_duration_s);
extern void provisioning_aborted_cb(void);
extern void start(void);

extern float read_Voltage();

/** @} end of APPLICATION */

#endif /* APPLICATION_H__ */