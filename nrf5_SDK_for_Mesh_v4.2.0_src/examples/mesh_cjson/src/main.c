#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "cJSON.h"

char *create_meshJSON(){

    char *mesh_out;
    cJSON *mesh_json, *mesh_tasks, *mesh_task, *energyConsumption, *executionTime;

    mesh_json = cJSON_CreateObject();
    mesh_tasks = cJSON_CreateArray();
 
    cJSON_AddItemToObject(mesh_json, "Mesh Tasks", mesh_tasks);
  
    //Mesh Tasks part 

    //Add first task to mesh_tasks array - Provisioning part
    cJSON_AddItemToArray(mesh_tasks, mesh_task = cJSON_CreateObject());
    cJSON_AddStringToObject(mesh_task, "functionName", "f_provisioning");
    cJSON_AddItemToObject(mesh_task, "energyConsumption", energyConsumption = cJSON_CreateObject());
    cJSON_AddNumberToObject(energyConsumption, "value", 5);
    cJSON_AddStringToObject(energyConsumption, "unit", "mA");
    cJSON_AddItemToObject(mesh_task, "executionTime", executionTime = cJSON_CreateObject());
    cJSON_AddNumberToObject(executionTime, "value", 0.1);
    cJSON_AddStringToObject(executionTime, "unit", "milliseconds");
    cJSON_AddNumberToObject(mesh_task, "priority", 5);

    //Add second task to mesh_tasks array - Initilize friendship 
    cJSON_AddItemToArray(mesh_tasks, mesh_task = cJSON_CreateObject());
    cJSON_AddStringToObject(mesh_task, "functionName", "f_friendship");
    cJSON_AddItemToObject(mesh_task, "energyConsumption", energyConsumption = cJSON_CreateObject());
    cJSON_AddNumberToObject(energyConsumption, "value", 7);
    cJSON_AddStringToObject(energyConsumption, "unit", "mA");
    cJSON_AddItemToObject(mesh_task, "executionTime", executionTime = cJSON_CreateObject());
    cJSON_AddNumberToObject(executionTime, "value", 1);
    cJSON_AddStringToObject(executionTime, "unit", "milliseconds");
    cJSON_AddNumberToObject(mesh_task, "priority", 5);
    
    mesh_out = cJSON_Print(mesh_json);

    cJSON_Delete(mesh_json);

    return mesh_out;
}

void parse_meshJson(char * json_file){

    cJSON *mesh_task, *functionName, *energyConsumption, *value_energy, *unit_energy, *executionTime, *value_time, *unit_time,
    *priority;

    cJSON * meshJson = cJSON_Parse(json_file);

    cJSON * mesh_tasks = cJSON_GetObjectItemCaseSensitive(meshJson, "Mesh Tasks");

    cJSON_ArrayForEach(mesh_task, mesh_tasks) 
    {
    
        functionName = cJSON_GetObjectItemCaseSensitive(mesh_task, "functionName");
        energyConsumption = cJSON_GetObjectItemCaseSensitive(mesh_task, "energyConsumption");
        value_energy = cJSON_GetObjectItemCaseSensitive(energyConsumption, "value");
        unit_energy = cJSON_GetObjectItemCaseSensitive(energyConsumption, "unit");
        executionTime = cJSON_GetObjectItemCaseSensitive(mesh_task, "executionTime");
        value_time = cJSON_GetObjectItemCaseSensitive(executionTime, "value");
        unit_time = cJSON_GetObjectItemCaseSensitive(executionTime, "unit");
        priority = cJSON_GetObjectItemCaseSensitive(mesh_task, "priority");

        if ((functionName->valuestring = "f_provisioning") && (value_time->valuedouble == 1) && (unit_energy->valuestring = "mA"))
        {
            printf("It's transmit part!\n");
        }
  
    }

    cJSON_Delete(meshJson);
}


int main(){

    char * mesh_out = create_meshJSON();
    printf("%s\n", mesh_out);
    parse_meshJson(mesh_out);
    free(mesh_out);
    return 0;    
}