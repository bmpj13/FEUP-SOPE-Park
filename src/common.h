#ifndef COMMON_H
#define COMMON_H

#include <time.h>

#define NUM_CONTROLLERS             4
#define CONTROLLER_NAME_LEN         50
#define VEHICLE_NAME_LEN            100
#define MESSAGE_LEN                 100

const char FULL_STR[] = "cheio!";
const char ACCEPTED_STR[] = "entrada";
const char EXITING_STR[] = "saida";
const char CLOSED_STR[] = "encerrado";
const char FINISH_STR[] = "ACABAR";


char* controller_name[NUM_CONTROLLERS] = {"/tmp/fifoN", "/tmp/fifoE", "/tmp/fifoS", "/tmp/fifoO"};


typedef struct {
    clock_t parking_time;
    int vehicle_id;
    char vehicle_fifo_name[VEHICLE_NAME_LEN];
} info_t;


typedef struct {
    char msg[MESSAGE_LEN];
} feedback_t;


#endif