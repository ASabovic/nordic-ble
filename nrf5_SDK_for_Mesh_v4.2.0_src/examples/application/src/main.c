#include "application.h"

/* Sleep time before checking if the required voltage is satisfied */
#define SLEEP_TIMEOUT_MS (5000) // sleep time in milliseconds

/* Define a task timer */
APP_TIMER_DEF(task_timer);

/* Define a LED timer */
APP_TIMER_DEF(led_timer);

/* Define a store timer */
APP_TIMER_DEF(store_timer);

int read_Voltage(){

    nrf_saadc_value_t adc_val;
  
    nrfx_saadc_sample_convert(0, &adc_val);
        
    int read_voltage = adc_val * 360000 / 1024;

    return read_voltage;
}

void store_timer_handler(void *n_notused){

    UNUSED_VARIABLE(n_notused);
    f_store();
}

void led_timer_handler(void *a_notused){

    UNUSED_VARIABLE(a_notused);
    led_task();
}

char temp_avg[10];
int t = 13;

void task_timer_handler(void *m_notused){

    UNUSED_VARIABLE(m_notused);

    //f_temp();

    //if(temperature_size >= 3){

    //    //__LOG(LOG_SRC_APP, LOG_LEVEL_INFO, "If! \n");
    //    temperature_size = 0;
    //    //f_compute();
    //    //sprintf(temp_avg, "%d", t);
    //    //f_transmit(&temp_avg[0]);
    //    //ERROR_CHECK(app_timer_start(led_timer, HAL_MS_TO_RTC_TICKS(500), NULL));
    //    //__LOG(LOG_SRC_APP, LOG_LEVEL_INFO, "LED! \n");
    led_task();

    //}else{
        
    //    //__LOG(LOG_SRC_APP, LOG_LEVEL_INFO, "Else! \n");
    //    //ERROR_CHECK(app_timer_start(store_timer, HAL_MS_TO_RTC_TICKS(500), NULL));
    //    f_store();
    //}
}

int main(void){


    initialize();
    (void) proxy_stop();
    start();
    
    ERROR_CHECK(app_timer_create(&task_timer, APP_TIMER_MODE_REPEATED, task_timer_handler));
    ERROR_CHECK(app_timer_create(&led_timer, APP_TIMER_MODE_SINGLE_SHOT, led_timer_handler));
    ERROR_CHECK(app_timer_create(&store_timer, APP_TIMER_MODE_SINGLE_SHOT, store_timer_handler));

    
    //ERROR_CHECK(app_timer_start(task_timer, HAL_MS_TO_RTC_TICKS(SLEEP_TIMEOUT_MS), NULL));

    //if(mesh_stack_is_device_provisioned()){

        ERROR_CHECK(app_timer_start(task_timer, HAL_MS_TO_RTC_TICKS(SLEEP_TIMEOUT_MS), NULL));
    //}else{
    
    //    __LOG(LOG_SRC_APP, LOG_LEVEL_INFO, "Node is still not provisioned!\n");
    //}

    for (;;)
    {
        (void)sd_app_evt_wait(); //Put the device in Sleep mode
    } 
}