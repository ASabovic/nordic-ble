#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
//#include "nrf_delay.h"
//#include "boards.h"

#include "cJSON.h"

char *create_flowJSON(){

    char *flow_out;
    cJSON *flow_json, *taskflows, *taskflow, *wait, *condition;

    flow_json = cJSON_CreateObject();
    taskflows = cJSON_CreateArray();

    cJSON_AddItemToObject(flow_json, "TaskFlows", taskflows);
  
    // Task flows part

    // First task flow 
    cJSON_AddItemToArray(taskflows, taskflow = cJSON_CreateObject());
    cJSON_AddStringToObject(taskflow, "source", "f_temp");
    cJSON_AddStringToObject(taskflow, "destination", "f_temp");
    cJSON_AddItemToObject(taskflow, "wait", wait = cJSON_CreateObject());
    cJSON_AddNumberToObject(wait, "value", 10);
    cJSON_AddStringToObject(wait, "unit", "seconds");

    // Second task flow 
    cJSON_AddItemToArray(taskflows, taskflow = cJSON_CreateObject());
    cJSON_AddStringToObject(taskflow, "source", "f_temp");
    cJSON_AddStringToObject(taskflow, "destination", "f_store");
    
    // Third task flow 
    cJSON_AddItemToArray(taskflows, taskflow = cJSON_CreateObject());
    cJSON_AddStringToObject(taskflow, "source", "f_store");
    cJSON_AddStringToObject(taskflow, "destination", "f_compute");
    cJSON_AddItemToObject(taskflow, "condition", condition = cJSON_CreateObject());
    cJSON_AddStringToObject(condition, "variable", "temp_size");
    cJSON_AddNumberToObject(condition, "value", 3);
    cJSON_AddStringToObject(condition, "typeCondition", "GE");

    // Fourth task flow 
    cJSON_AddItemToArray(taskflows, taskflow = cJSON_CreateObject());
    cJSON_AddStringToObject(taskflow, "source", "f_compute");
    cJSON_AddStringToObject(taskflow, "destination", "f_transmit");
    cJSON_AddItemToObject(taskflow, "condition", condition = cJSON_CreateObject());
    cJSON_AddBoolToObject(condition, "typeCondition", true);

    flow_out = cJSON_Print(flow_json);

    cJSON_Delete(flow_json);

    return flow_out;
}

void parse_flowJson(char * json_file){

    cJSON *taskflow, *source, *destination, *wait, *condition, *value_wait, *unit_wait, *variable, *value_condition, 
    *type_condition;

    cJSON * taskflowsJson = cJSON_Parse(json_file);

    cJSON * taskflows = cJSON_GetObjectItemCaseSensitive(taskflowsJson, "TaskFlows");

    cJSON_ArrayForEach(taskflow, taskflows) 
    {
    
        source = cJSON_GetObjectItemCaseSensitive(taskflow, "source");
        destination = cJSON_GetObjectItemCaseSensitive(taskflow, "destination");
        wait = cJSON_GetObjectItemCaseSensitive(taskflow, "wait");
        value_wait = cJSON_GetObjectItemCaseSensitive(wait, "value");
        unit_wait = cJSON_GetObjectItemCaseSensitive(wait, "unit");
        condition = cJSON_GetObjectItemCaseSensitive(taskflow, "condition");
        variable = cJSON_GetObjectItemCaseSensitive(condition, "variable");
        value_condition = cJSON_GetObjectItemCaseSensitive(condition, "value");
        type_condition = cJSON_GetObjectItemCaseSensitive(condition, "typeCondition");

	if ((source->valuestring = "f_store") && (value_condition->valuedouble == 3))
    	{
        
        	printf("It's temperature part!\n");
    	}
    }

    cJSON_Delete(taskflowsJson);
}


int main(){

    char * flow_out = create_flowJSON();
    printf("%s\n", flow_out);
    parse_flowJson(flow_out);
    free(flow_out);
    return 0;    
}