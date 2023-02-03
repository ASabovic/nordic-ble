#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
//#include "nrf_delay.h"
//#include "boards.h"

#include <stdio.h>
#include <stdlib.h>
#include "cJSON.h"

char *create_JSON(){

    char *out;
    cJSON *root, *cars, *car;

    /* create root node and array */
    root = cJSON_CreateObject();
    cars = cJSON_CreateArray();

    /* add cars array to root */
    cJSON_AddItemToObject(root, "cars", cars);

    /* add 1st car to cars array */
    cJSON_AddItemToArray(cars, car = cJSON_CreateObject());
    cJSON_AddItemToObject(car, "CarType1", cJSON_CreateString("BMW"));
    cJSON_AddItemToObject(car, "carID1", cJSON_CreateString("bmw123"));

    /* add 2nd car to cars array */
    cJSON_AddItemToArray(cars, car = cJSON_CreateObject());
    cJSON_AddItemToObject(car, "CarType2", cJSON_CreateString("mercedes"));
    cJSON_AddItemToObject(car, "carID2", cJSON_CreateString("mercedes123"));

    cJSON_AddItemToArray(cars, car = cJSON_CreateObject());
    cJSON_AddItemToObject(car, "CarType", cJSON_CreateString("VW"));
    cJSON_AddItemToObject(car, "carID", cJSON_CreateString("vw123"));

    /* print everything */
    out = cJSON_Print(root);

    /* free all objects under root and root itself */
    cJSON_Delete(root);

    return out;
}

int main(){

    char *out = create_JSON();
    printf("%s\n", out);
    free(out);
    return 0;
}







