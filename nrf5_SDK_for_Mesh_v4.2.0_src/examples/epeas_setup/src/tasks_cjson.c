#include "tasks_cjson.h"

taskName char_to_enum(char *task_name){

    if(strcmp (task_name, "f_temp") == 0){
    
        return f_Temp;
    }else if (strcmp(task_name, "f_store") == 0){
    
        return f_Store;
    }else if (strcmp(task_name, "f_compute") == 0){
    
        return f_Compute;
    }else if (strcmp(task_name, "f_transmit") == 0){
    
        return f_Transmit;
    }else if (strcmp(task_name, "f_led") == 0){

        return f_Led;
    }
}

struct Task tasks[5];
//int reqVoltage[4] = {256422, 252206, 256251, 245297};
  
void application_doit(char *json_file, char *json_file2){

    cJSON *tasksJson = cJSON_Parse(json_file);

    cJSON *Tasks = cJSON_GetObjectItemCaseSensitive(tasksJson, "Tasks");

    cJSON *task;

    int i = 0;

    cJSON_ArrayForEach(task, Tasks) 
    {
    
        /* Task name */
        cJSON *functionName = cJSON_GetObjectItemCaseSensitive(task, "functionName");
        tasks[i].task_name = char_to_enum(functionName->valuestring);

        /* Task energy/current consumption */
        cJSON *energyConsumption = cJSON_GetObjectItemCaseSensitive(task, "energyConsumption");
        Tasks = cJSON_GetObjectItemCaseSensitive(energyConsumption, "value");
        tasks[i].task_energy = (Tasks->valueint);

        /* Task execution time */
        cJSON *executionTime = cJSON_GetObjectItemCaseSensitive(task, "executionTime");
        Tasks = cJSON_GetObjectItemCaseSensitive(executionTime, "value");
        tasks[i].execution_time = (Tasks->valueint);

        /* Task priority */
        cJSON *priority = cJSON_GetObjectItemCaseSensitive(task, "priority");
        tasks[i].task_priority = (priority->valueint); 
           
        /* Initial/No-initial task */
        cJSON *firstTask = cJSON_GetObjectItemCaseSensitive(task, "firstTask");
        tasks[i].first_task = (firstTask->valueint);

        /* Required voltage */
        cJSON *requiredVoltage = cJSON_GetObjectItemCaseSensitive(task, "requiredVoltage");
        tasks[i].required_voltage = (requiredVoltage->valueint);
//          tasks[i].required_voltage = reqVoltage[i];

        i = i+1;
    }

    cJSON *flowJson = cJSON_Parse(json_file2);

    cJSON *flows = cJSON_GetObjectItemCaseSensitive(flowJson, "TaskFlows");

    cJSON *flow;

    int m=0;

    int task_size = sizeof(tasks) / sizeof(tasks[0]);//4

    for(int i=0; i<task_size; i++){
    
        m=0;
        cJSON_ArrayForEach(flow, flows) 
        {
            /* Parent task name */
            cJSON *parent_task = cJSON_GetObjectItemCaseSensitive(flow, "parent_task");
            /* Child task name */
            cJSON *child_task = cJSON_GetObjectItemCaseSensitive(flow, "child_task");
        
            /* Constraint/s part */
            cJSON *condition = cJSON_GetObjectItemCaseSensitive(flow, "condition");
            cJSON *variable = cJSON_GetObjectItemCaseSensitive(condition, "variable");
            cJSON *type = cJSON_GetObjectItemCaseSensitive(condition, "typeCondition");
            cJSON *value = cJSON_GetObjectItemCaseSensitive(condition, "value");

            if(tasks[i].task_name == char_to_enum(parent_task->valuestring)){
                
                for(int k=0; k<task_size; k++){
                    
                    if(char_to_enum(child_task->valuestring) == tasks[k].task_name){
                        
                        tasks[i].child_id[m] = k;
                        
                        if(strcmp(type->valuestring, "nocondition") == 0){
                                
                            tasks[i].constraint_value[m] = (value->valueint);
                            tasks[i].type[m] = nocondition;
                            tasks[i].var[m] = novariable;
                        }else if(strcmp(type->valuestring, "wait") == 0){
                    
                            tasks[i].constraint_value[m] = (value->valueint);
                            tasks[i].type[m] = wait;
                            tasks[i].var[m] = temp; 
                        }else if(strcmp(type->valuestring, "GE") == 0){
                                    
                            tasks[i].constraint_value[m] = (value->valueint);
                            tasks[i].type[m] = higherorequal;
                            tasks[i].var[m] = t_size;  
                        }else if(strcmp(type->valuestring, "available") == 0){
                                
                            tasks[i].constraint_value[m] = (value->valueint);
                            tasks[i].type[m] = available;
                            tasks[i].var[m] = t_average;  
                        }
                        m=m+1;
                    }
                }
            }
        
        }
        if(m<2){

            tasks[i].child_id[m] = 5000;
        }
        tasks[4].child_id[1] = 5000;
    }

    cJSON_Delete(tasksJson);
    cJSON_Delete(flowJson);
}

void application_dofile(char *filename1, char *filename2){

    FILE *f1; long len1; char *data1;
    FILE *f2; long len2; char *data2;

    f1=fopen(filename1,"rb");
    fseek(f1,0,SEEK_END);
    len1=ftell(f1);
    fseek(f1,0,SEEK_SET);

    data1=(char *)malloc((len1+1)*sizeof(char));
    fread(data1,sizeof(char),len1,f1);
    fclose(f1);

    f2=fopen(filename2,"rb");
    fseek(f2,0,SEEK_END);
    len2=ftell(f2);
    fseek(f2,0,SEEK_SET);

    data2=(char *)malloc((len2+1)*sizeof(char));
    fread(data2,sizeof(char),len2,f2);
    fclose(f2);

    application_doit(data1, data2);
    free(data1);
    free(data2);
}

