#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
//#include "nrf_delay.h"
//#include "boards.h"

#include "cJSON.h"

char *create_taskJSON(){

    char *print_out;
    cJSON *application, *tasks, *task, *energyConsumption, *executionTime, *inputs, *output;

    //Create application, tasks and taskflows
    application = cJSON_CreateObject();
    tasks = cJSON_CreateArray();

    //Add tasks and taskflows arrays to application object 
    cJSON_AddItemToObject(application, "Tasks", tasks);
  
    //Tasks part 

    //Add first task to tasks array - Temperature measurement 
    cJSON_AddItemToArray(tasks, task = cJSON_CreateObject());
    cJSON_AddStringToObject(task, "functionName", "f_temp");
    cJSON_AddItemToObject(task, "energyConsumption", energyConsumption = cJSON_CreateObject());
    cJSON_AddNumberToObject(energyConsumption, "value", 5);
    cJSON_AddStringToObject(energyConsumption, "unit", "mA");
    cJSON_AddItemToObject(task, "executionTime", executionTime = cJSON_CreateObject());
    cJSON_AddNumberToObject(executionTime, "value", 0.1);
    cJSON_AddStringToObject(executionTime, "unit", "milliseconds");
    cJSON_AddNumberToObject(task, "priority", 5);
    cJSON_AddItemToObject(task, "output", output = cJSON_CreateObject());
    cJSON_AddStringToObject(output, "name", "temperature");
    cJSON_AddStringToObject(output, "typeOutput", "float");

    //Add second taks to tasks array - Saving temperature values in flash 
    cJSON_AddItemToArray(tasks, task = cJSON_CreateObject());
    cJSON_AddStringToObject(task, "functionName", "f_store");
    cJSON_AddItemToObject(task, "energyConsumption", energyConsumption = cJSON_CreateObject());
    cJSON_AddNumberToObject(energyConsumption, "value", 7);
    cJSON_AddStringToObject(energyConsumption, "unit", "mA");
    cJSON_AddItemToObject(task, "executionTime", executionTime = cJSON_CreateObject());
    cJSON_AddNumberToObject(executionTime, "value", 1);
    cJSON_AddStringToObject(executionTime, "unit", "milliseconds");
    cJSON_AddNumberToObject(task, "priority", 5);
    cJSON_AddItemToObject(task, "inputs", inputs = cJSON_CreateObject());
    cJSON_AddStringToObject(inputs, "name", "temperature");
    cJSON_AddStringToObject(inputs, "typeInput", "float");
    cJSON_AddItemToObject(task, "output", output = cJSON_CreateObject());
    cJSON_AddStringToObject(output, "name", "temp_size");
    cJSON_AddStringToObject(output, "typeOutput", "integer");

    //Add third task to tasks array - Compute the average temperature value
    cJSON_AddItemToArray(tasks, task = cJSON_CreateObject());
    cJSON_AddStringToObject(task, "functionName", "f_compute");
    cJSON_AddItemToObject(task, "energyConsumption", energyConsumption = cJSON_CreateObject());
    cJSON_AddNumberToObject(energyConsumption, "value", 10);
    cJSON_AddStringToObject(energyConsumption, "unit", "mA");
    cJSON_AddItemToObject(task, "executionTime", executionTime = cJSON_CreateObject());
    cJSON_AddNumberToObject(executionTime, "value", 5);
    cJSON_AddStringToObject(executionTime, "unit", "milliseconds");
    cJSON_AddNumberToObject(task, "priority", 8);
    cJSON_AddItemToObject(task, "inputs", inputs = cJSON_CreateObject());
    cJSON_AddStringToObject(inputs, "name", "temp_size");
    cJSON_AddStringToObject(inputs, "typeInput", "integer");
    cJSON_AddItemToObject(task, "output", output = cJSON_CreateObject());
    cJSON_AddStringToObject(output, "name", "avg_temp");
    cJSON_AddStringToObject(output, "typeOutput", "float");

    //Add fourth task to tasks array - Send the average temperature value to server node (from LPN to server/receiver node using BLE mesh)
    cJSON_AddItemToArray(tasks, task = cJSON_CreateObject());
    cJSON_AddStringToObject(task, "functionName", "f_transmit");
    //energyConsumption = cJSON_CreateObject();
    cJSON_AddItemToObject(task, "energyConsumption", energyConsumption = cJSON_CreateObject());
    cJSON_AddNumberToObject(energyConsumption, "value", 12);
    cJSON_AddStringToObject(energyConsumption, "unit", "mA");
    cJSON_AddItemToObject(task, "executionTime", executionTime = cJSON_CreateObject());
    cJSON_AddNumberToObject(executionTime, "value", 10);
    cJSON_AddStringToObject(executionTime, "unit", "milliseconds");
    cJSON_AddNumberToObject(task, "priority", 10);
    cJSON_AddItemToObject(task, "inputs", inputs = cJSON_CreateObject());
    cJSON_AddStringToObject(inputs, "name", "avg_temp");
    cJSON_AddStringToObject(inputs, "typeInput", "float");
   
    print_out = cJSON_Print(application);

   /* free all objects under root and root itself */
    cJSON_Delete(application);

    return print_out;
}

void parse_taskJson(char * json_file){

    cJSON *task, *functionName, *energyConsumption, *value_energy, *unit_energy, *executionTime, *value_time, *unit_time,
    *priority, *inputs, *input_name, *input_type, *output, *output_name, *output_type;

    cJSON * tasksJson = cJSON_Parse(json_file);

    cJSON * tasks = cJSON_GetObjectItemCaseSensitive(tasksJson, "Tasks");

    cJSON_ArrayForEach(task, tasks) 
    {
    
        functionName = cJSON_GetObjectItemCaseSensitive(task, "functionName");
        energyConsumption = cJSON_GetObjectItemCaseSensitive(task, "energyConsumption");
        value_energy = cJSON_GetObjectItemCaseSensitive(energyConsumption, "value");
        unit_energy = cJSON_GetObjectItemCaseSensitive(energyConsumption, "unit");
        executionTime = cJSON_GetObjectItemCaseSensitive(task, "executionTime");
        value_time = cJSON_GetObjectItemCaseSensitive(executionTime, "value");
        unit_time = cJSON_GetObjectItemCaseSensitive(executionTime, "unit");
        priority = cJSON_GetObjectItemCaseSensitive(task, "priority");
        inputs = cJSON_GetObjectItemCaseSensitive(task, "inputs");
        input_name = cJSON_GetObjectItemCaseSensitive(inputs, "name");
        input_type = cJSON_GetObjectItemCaseSensitive(inputs, "typeInput");
        output = cJSON_GetObjectItemCaseSensitive(task, "output");
        output_name = cJSON_GetObjectItemCaseSensitive(output, "name");
        output_type = cJSON_GetObjectItemCaseSensitive(output, "typeOutput");

        if ((functionName->valuestring = "f_compute") && (value_time->valuedouble == 10) && (unit_energy->valuestring = "mA"))
        {
            printf("It's transmit part!\n");
        }
  
    }

    cJSON_Delete(tasksJson);
}


int main(){

    char * print_out = create_taskJSON();
    printf("%s\n", print_out);
    parse_taskJson(print_out);
    free(print_out);
    return 0;    
}