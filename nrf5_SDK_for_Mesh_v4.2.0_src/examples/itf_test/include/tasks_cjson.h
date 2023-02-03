#ifndef TASKS_CJSON_H__
#define TASKS_CJSON_H__

#include "cJSON.h"

#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include <stdbool.h>
#include <math.h> 
#include <stdlib.h>
#include <string.h>

typedef enum{nocondition, wait, higherorequal, available}constraintType;
typedef enum {novariable, temp, t_size, t_average}constraintVar;
typedef enum{f_Temp, f_Store, f_Compute, f_Transmit, f_Led}taskName;

extern enum constraintType type;
extern enum constraintVar var;
extern enum taskName task_name;

taskName char_to_enum(char *task_name);

struct Task{

    taskName task_name;
    int task_energy;
    int execution_time;
    int task_priority;
    int first_task;
    constraintType type[2];
    constraintVar var[2];
    int constraint_value[2];
    int child_id[2];
    int required_voltage;
};

struct TaskInstance{

    int start_time;
    int task_id; 
};

extern struct Task tasks[5];
//extern int reqVoltage[4];

void application_doit(char *json_file, char *json_file2);

void application_dofile(char *filename1, char *filename2);

/** @} end of TASKS_CJSON */

#endif /* TASKS_CJSON_H__ */