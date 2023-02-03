#include "application.h"
#include "tasks_cjson.h"
#include "arima_model.h"
#include "arima_config.h"

/* Maximum number of tasks in task list */
#define MAX_TASK_LIST_SIZE 5

/* Occupied and task list */
int occup_list[MAX_TASK_LIST_SIZE] = {0};
struct TaskInstance task_instances[MAX_TASK_LIST_SIZE];

/* Selected task for execution - task with the highest priority */
struct TaskInstance *selected_task;

/* Task instances - flash part */
#define M_TSK_FILE_ID (M_APP_FILE_ID + 1)
MESH_CONFIG_FILE(m_tsk_file, M_TSK_FILE_ID, MESH_CONFIG_STRATEGY_CONTINUOUS);

/* Task instances array */
#define TSK_RECORD_START (0x0001)
#define TSK_RECORD_END (TSK_RECORD_START + MAX_TASK_LIST_SIZE - 1)
#define ENTRY_TSK_ID MESH_CONFIG_ENTRY_ID(M_TSK_FILE_ID, TSK_RECORD_START)
static struct TaskInstance m_live_tsk[MAX_TASK_LIST_SIZE];

static uint32_t m_tsk_setter(mesh_config_entry_id_t id, const void *p_entry)
{
    if (!IS_IN_RANGE(id.record, TSK_RECORD_START, TSK_RECORD_END))
    {
        return NRF_ERROR_NOT_FOUND;
    }

    uint16_t idx = id.record - TSK_RECORD_START;

    const struct TaskInstance *p_tsk = (const struct TaskInstance *)p_entry;

    m_live_tsk[idx] = *p_tsk;

    return NRF_SUCCESS;
}

static void m_tsk_getter(mesh_config_entry_id_t id, void *p_entry)
{
    NRF_MESH_ASSERT_DEBUG(IS_IN_RANGE(id.record, TSK_RECORD_START, TSK_RECORD_END));

    uint16_t idx = id.record - TSK_RECORD_START;

    struct TaskInstance *p_tsk = (struct TaskInstance *)p_entry;

    *p_tsk = m_live_tsk[idx];
}


MESH_CONFIG_ENTRY(m_tsk,
                  ENTRY_TSK_ID,
                  MAX_TASK_LIST_SIZE,
                  sizeof(struct TaskInstance),
                  m_tsk_setter,
                  m_tsk_getter,
                  NULL,
                  false);

/* Occupied list */
#define OCC_RECORD_START (TSK_RECORD_END + 1)
#define OCC_RECORD_END (OCC_RECORD_START + MAX_TASK_LIST_SIZE - 1)
#define ENTRY_OCC_ID MESH_CONFIG_ENTRY_ID(M_TSK_FILE_ID, OCC_RECORD_START)
static int m_live_occ[MAX_TASK_LIST_SIZE];

static uint32_t m_occ_setter(mesh_config_entry_id_t id, const void *p_entry)
{
    if (!IS_IN_RANGE(id.record, OCC_RECORD_START, OCC_RECORD_END))
    {
        return NRF_ERROR_NOT_FOUND;
    }

    uint16_t idx = id.record - OCC_RECORD_START;

    const int *p_occ = (const int *)p_entry;

    m_live_occ[idx] = *p_occ;

    return NRF_SUCCESS;
}

static void m_occ_getter(mesh_config_entry_id_t id, void *p_entry)
{
    NRF_MESH_ASSERT_DEBUG(IS_IN_RANGE(id.record, OCC_RECORD_START, OCC_RECORD_END));

    uint16_t idx = id.record - OCC_RECORD_START;

    int *p_occ = (int *)p_entry;

    *p_occ = m_live_occ[idx];
}

MESH_CONFIG_ENTRY(m_occ,
                  ENTRY_OCC_ID,
                  MAX_TASK_LIST_SIZE,
                  sizeof(int),
                  m_occ_setter,
                  m_occ_getter,
                  NULL,
                  false);

/* Measured voltage */
double voltage;
double in_voltage;
int in_step = 1;

int predictor;

int sleep_time=5000;
int a=0;

int sleep_time_schedule;
int sleep_time_check;

/* Voltage check period */
#define SLEEP_TIMEOUT_MS (1000) // sleep time in milliseconds

/* Define a sleep timer */
APP_TIMER_DEF(sleep_timer);

/* Define a task timer */
APP_TIMER_DEF(task_timer);

APP_TIMER_DEF(cap_timer);

APP_TIMER_DEF(in_timer);


char temp_avg[10];

void execute(){

    switch(tasks[selected_task->task_id].task_name){
    
    case f_Temp:

        /* Temperature measurement -> function defined in application.c file */
        f_temp();
        break;

    case f_Store:

        /* Store the temperature value -> function defined in application.c file */
        f_store();
        break;

    case f_Compute:

        /* Compute the average temperature -> function defined in application.c file */
        f_compute();
        break;

    case f_Transmit:

        /* Transmit the average temperature to the mesh node -> function defined in application.c file */
        sprintf(temp_avg, "%d", average_temperature);
        f_transmit(&temp_avg[0]);
        break;

    case f_Led:

        /* Confirm the successful transmission by briefly turning on the LED -> function defined in application.c file */
        led_task();
        break;

    default:
        break;
    }
}
    
/* Returns the initial task/s of each application */
void getfirstTask(){
    
    /* Read and parse a JSON file/s into a struct/s -> function defined in tasks_cjson.c file */
    application_dofile("C:/Users/IDLab guest/Documents/GitHub/BLUESS_BLE/nrf5_SDK_for_Mesh_v4.2.0_src/examples/epeas_setup/src/tasks.json", "C:/Users/IDLab guest/Documents/GitHub/BLUESS_BLE/nrf5_SDK_for_Mesh_v4.2.0_src/examples/epeas_setup/src/tasksflow.json");
    
    struct TaskInstance temporarily;

    int position = 0;

    /* Select all initial tasks, add them into task instances, and store them in the flash */
    for(int i=0; i<(sizeof(tasks) / sizeof(tasks[0])); i++){
      
        if(tasks[i].first_task == 1){
            temporarily.start_time = 0;
            temporarily.task_id = i;
            task_instances[position] = temporarily;
            mesh_config_entry_id_t instances_id = ENTRY_TSK_ID;
            instances_id.record += position;
            mesh_config_entry_set(instances_id, &task_instances[position]);

            occup_list[position] = 1;
            mesh_config_entry_id_t occ_id = ENTRY_OCC_ID;
            occ_id.record += position;
            mesh_config_entry_set(occ_id, &occup_list[position]);

            position++;
        }
    }

}

/* Set up the scheduler */
void setupSchedule(){

    int pos = 0;
    struct TaskInstance tempo;

    /* Check if the flash is empty */
    if((struct Task *)(pointer_tasks->task_priority) == 0xFFFFFFFF){

        __LOG(LOG_SRC_APP, LOG_LEVEL_INFO, "Read data from json file/s!\n");
        getfirstTask();

        if(flash_status != FLASH_IDLE){
    
            __LOG(LOG_SRC_APP, LOG_LEVEL_INFO, "Flash is busy in setupSchedule function in flash status %d. Try again later! \n", flash_status); 
            return 1;
        }else{

            uint32_t err_code = sd_flash_page_erase(pg_num_tasks);
            if (err_code != 0){
            
                __LOG(LOG_SRC_APP, LOG_LEVEL_INFO, "Fatal error in task list erase function: err_code is %d \n", err_code);
            }
            flash_status = TASKS_WRITE; 
         }
    }else{

        __LOG(LOG_SRC_APP, LOG_LEVEL_INFO, "Read data from flash!\n");

        for(int i=0; i<MAX_TASK_LIST_SIZE; i++){
        
            //temperature_size = 0;
            tasks[i] = pointer_tasks[i];

            //if(tasks[i].first_task == 1){
            //    tempo.start_time = 0;
            //    tempo.task_id = i;
            //    task_instances[pos] = tempo;
            //    occup_list[pos] = 1;
            //    pos++;
            //}

            mesh_config_entry_id_t instances_id = ENTRY_TSK_ID;
            instances_id.record += i;
            mesh_config_entry_get(instances_id, &task_instances[i]);

            mesh_config_entry_id_t occ_id = ENTRY_OCC_ID;
            occ_id.record += i;
            mesh_config_entry_get(occ_id, &occup_list[i]);        
        }
    }
}

/* Add a child task/s that satisfy the constraint/s to the task list */
void addTask(){

    int m = 0;

    for(int i=0; i<MAX_TASK_LIST_SIZE; i++){
        
        if(occup_list[i] == 0){

            if(tasks[selected_task->task_id].child_id[m] == 5000 || m>1){

                occup_list[i] = 0;
            }else{

               mesh_config_entry_id_t instances_id = ENTRY_TSK_ID;
               mesh_config_entry_id_t occ_id = ENTRY_OCC_ID;

                switch(tasks[selected_task->task_id].type[m])
                {
                case nocondition:

                    task_instances[i].start_time = tasks[selected_task->task_id].execution_time;
                    task_instances[i].task_id = tasks[selected_task->task_id].child_id[m];
                    instances_id = ENTRY_TSK_ID;
                    instances_id.record += i;
                    mesh_config_entry_set(instances_id, &task_instances[i]);

                    occup_list[i] = 1;
                    occ_id = ENTRY_OCC_ID;
                    occ_id.record += i;
                    mesh_config_entry_set(occ_id, &occup_list[i]);
                    
                    m=m+1;
                    break;

                case wait:
                
                    task_instances[i].start_time = tasks[selected_task->task_id].execution_time + tasks[selected_task->task_id].constraint_value[m];
                    //task_instances[i].start_time = tasks[selected_task->task_id].execution_time + 3000;
                    task_instances[i].task_id = tasks[selected_task->task_id].child_id[m];
                    instances_id = ENTRY_TSK_ID;
                    instances_id.record += i;
                    mesh_config_entry_set(instances_id, &task_instances[i]);

                    occup_list[i] = 1;
                    occ_id = ENTRY_OCC_ID;
                    occ_id.record += i;
                    mesh_config_entry_set(occ_id, &occup_list[i]);

                    m=m+1;
                    break;

                case higherorequal:
   
                    if(temperature_size >= tasks[selected_task->task_id].constraint_value[m]){

                        task_instances[i].start_time = tasks[selected_task->task_id].execution_time;
                        task_instances[i].task_id = tasks[selected_task->task_id].child_id[m];
                        instances_id = ENTRY_TSK_ID;
                        instances_id.record += i;
                        mesh_config_entry_set(instances_id, &task_instances[i]);

                        occup_list[i] = 1;
                        occ_id = ENTRY_OCC_ID;
                        occ_id.record += i;
                        mesh_config_entry_set(occ_id, &occup_list[i]);
                        
                        m=m+1;
                    }else{
                    
                        occup_list[i] = 0;
                    }
                    break;
                
                case available:

                    task_instances[i].start_time = tasks[selected_task->task_id].execution_time;
                    task_instances[i].task_id = tasks[selected_task->task_id].child_id[m];
                    instances_id = ENTRY_TSK_ID;
                    instances_id.record += i;
                    mesh_config_entry_set(instances_id, &task_instances[i]);

                    occup_list[i] = 1;
                    occ_id = ENTRY_OCC_ID;
                    occ_id.record += i;
                    mesh_config_entry_set(occ_id, &occup_list[i]);

                    m=m+1;
                    break;

                default:

                    break;
                }
            }
        }
        
    }
}

/* Choose the highest priority task and remove it from the task list */
struct TaskInstance *removeTask(){

    int high_priority = -1;
    int high_priority_2 = -1;

    for(int i=0; i<MAX_TASK_LIST_SIZE; i++){
    
        if(occup_list[i] == 1 && tasks[task_instances[i].task_id].task_priority > high_priority){
        
            high_priority = tasks[task_instances[i].task_id].task_priority;
            high_priority_2 = i;
        }
    }

    if(high_priority_2 != -1){
    
        occup_list[high_priority_2] = 0;
        //mesh_config_entry_id_t occ_id = ENTRY_OCC_ID;
        //occ_id.record += high_priority_2;
        //mesh_config_entry_set(occ_id, &occup_list[high_priority_2]);

        //mesh_config_entry_id_t instances_id = ENTRY_TSK_ID;
        //instances_id.record += high_priority_2;
        //mesh_config_entry_set(occ_id, &task_instances[high_priority_2]);

        return &task_instances[high_priority_2];
    }else{
    
        return NULL;
    }
}

unsigned int get_time(){

    unsigned int time = 240000;

    for (int i = 0; i < MAX_TASK_LIST_SIZE; i++)
    {
        if (occup_list[i])
        {
            if (time < 0 || time > task_instances[i].start_time)
            {
                time = task_instances[i].start_time;
            }
        }
    }
    for (int i = 0; i < MAX_TASK_LIST_SIZE; i++)
    {
        if (occup_list[i])
        {
            task_instances[i].start_time -= time;
        }
    }
    return time;
}

void sleep_timer_handler(void *m_notused){

    UNUSED_VARIABLE(m_notused);

    /* Measure the voltage -> defined in application.c file */
    //voltage = cap_task();
    voltage = read_Voltage();
    __LOG(LOG_SRC_APP, LOG_LEVEL_INFO, "Voltage is:  %d ms\n", voltage);

    if(voltage < 2.8){
    
        predictor = predict_when(3.2);
        //predictor = predict_when(3.0);
        
        sleep_time_check = (sleep_time * predictor);

        in_step = in_step + predictor;
        ERROR_CHECK(app_timer_start(sleep_timer, HAL_MS_TO_RTC_TICKS(sleep_time_check), NULL));
    }else{
    
        if(a<1){
        
            __LOG(LOG_SRC_APP, LOG_LEVEL_INFO, "Sensing interrupt ready!\n");
            //sensing_gpio_task();
            led_task();
            a = 1;
            ERROR_CHECK(app_timer_start(task_timer, HAL_SECS_TO_RTC_TICKS(3), NULL));
        }else{
        
            __LOG(LOG_SRC_APP, LOG_LEVEL_INFO, "Sending interrupt ready!\n");
            //send_gpio_task();
            led_task();
            a = 0;
            ERROR_CHECK(app_timer_start(task_timer, HAL_SECS_TO_RTC_TICKS(3), NULL));
        }
    
    }
     
    
}

void schedule_Task(void *n){

    __LOG(LOG_SRC_APP, LOG_LEVEL_INFO, "Inside schedule_Task function!\n");

    predictor = predict_when(3.0);

    in_step = in_step + predictor;

    sleep_time_schedule = (sleep_time*predictor);
    
    ERROR_CHECK(app_timer_start(sleep_timer, HAL_MS_TO_RTC_TICKS(sleep_time_schedule), NULL));

}

void create_timers(){

    ERROR_CHECK(app_timer_create(&task_timer, APP_TIMER_MODE_SINGLE_SHOT, schedule_Task));
}

void collect_data(void *k){

    ///__LOG(LOG_SRC_APP, LOG_LEVEL_INFO, "Initial step is: %d\n", in_step);
    //in_voltage = cap_task();
    __LOG(LOG_SRC_APP, LOG_LEVEL_INFO, "Inside collect data function!\n");
    in_voltage = read_Voltage();

    handle_new_aperiodic_synchronous_data(in_voltage, in_step);   
    
    if(in_step<11){
    
        in_step = in_step+1;
        ERROR_CHECK(app_timer_start(in_timer, HAL_MS_TO_RTC_TICKS(sleep_time), NULL));
    }else{
    
        schedule_Task(NULL);
        
    }
}


void initial_timers(){

    ERROR_CHECK(app_timer_create(&in_timer, APP_TIMER_MODE_SINGLE_SHOT, collect_data));
}



/* Callback function used to store tasks that are read/parsed from JSON files */
void sys_evt_dispatch(uint32_t sys_evt, void * p_context)
{
    if(sys_evt == NRF_EVT_FLASH_OPERATION_SUCCESS){

        uint32_t err_code;

        switch(flash_status)
        {
        case TASKS_WRITE:

            err_code = sd_flash_write(pointer_tasks, tasks, MAX_TASK_LIST_SIZE*12);
            if(err_code != 0){
            
                __LOG(LOG_SRC_APP, LOG_LEVEL_INFO, "Fatal error in tasks write function: err_code is %d \n", err_code);
            }
            flash_status = FLASH_IDLE;
            break;
        
        default:
            break;
        }
    }
}

int main(void){

    NRF_SDH_SOC_OBSERVER(m_sys_obs, 0, sys_evt_dispatch, NULL);

    initialize();
    (void) proxy_stop();
    start();

    ERROR_CHECK(app_timer_create(&sleep_timer, APP_TIMER_MODE_SINGLE_SHOT, sleep_timer_handler));
    create_timers();
    initial_timers();

    /* Flash parts */
    /* Tasks part */
    pg_size_tasks = NRF_FICR->CODEPAGESIZE;
    pg_num_tasks = NRF_FICR->CODESIZE-4;
    pointer_tasks = (struct Task *)(pg_size_tasks * pg_num_tasks);

    collect_data(NULL);
    //schedule_Task(NULL);

    for (;;)
    {
        (void)sd_app_evt_wait(); //Put the device in Sleep mode
    } 
}