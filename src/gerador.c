#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <stdlib.h>
#include <time.h>
#include <sys/file.h>
#include <sys/stat.h>
#include <error.h>
#include <errno.h>
#include <time.h>
#include <string.h>
#include <pthread.h>
#include "common.h"
#include "utils.h"

#define PARK_TIME_MULTIPLES             10


const char VEHICLE_FIFO_PREFIX[] = "/tmp/fifo_vh";


typedef struct {
    char controller_fifo_name[CONTROLLER_NAME_LEN];
    info_t info;
} vehicle_t;


/* Global Variables */
static int tGeracao;
static clock_t uRelogio;
static int numberVehicles = 0;
static pthread_mutex_t veiculo_lock = PTHREAD_MUTEX_INITIALIZER;


/* Functions */
void* veiculo(void* arg);
vehicle_t create_vehicle(int ID);
void wait_ticks_random();


int main(int argc, char **argv)
{
    clock_t start;
    double timeElapsed = 0;
    pthread_t detach_thread;
    
    srand(time(NULL));
    
    if (argc != 3) 
    {
        fprintf(stderr, "Usage: gerador <T_GERACAO> <U_RELOGIO>\n");
        exit(1);
    }
    
    if (convert_str_to_int(argv[1], &tGeracao) == -1)
    {
        fprintf(stderr, "Invalid entry for T_GERACAO\n");
        exit(2);			
    }
    
    if (convert_str_to_int(argv[2], (int *) &uRelogio) == -1)
    {
        fprintf(stderr, "Invalid entry for U_RELOGIO\n");
        exit(3);		
    }
    
    start = clock();
    while (timeElapsed < tGeracao)
    {
        wait_ticks_random();
        pthread_create(&detach_thread, NULL, veiculo, NULL);
        timeElapsed = (double) (clock() - start) / CLOCKS_PER_SEC; 
    }
    
    pthread_exit(0);
}




void* veiculo(void* arg) {
    vehicle_t vehicle;
    int fd_vehicle, fd_controller;
    feedback_t feedback;
    
    
    pthread_detach(pthread_self());
    
    
    pthread_mutex_lock(&veiculo_lock);
    numberVehicles++;
    vehicle = create_vehicle(numberVehicles);
    pthread_mutex_unlock(&veiculo_lock);
    
    
    if ( (fd_controller = open(vehicle.controller_fifo_name, O_WRONLY)) == -1 )
    {
        perror(strcat(vehicle.controller_fifo_name, " FIFO opening failed on veiculo"));
        return NULL;
    }
    
    
    if ( (fd_vehicle = init_fifo(vehicle.info.vehicle_fifo_name)) == -1 )
        return NULL;

    
    if (write(fd_controller, &vehicle.info, sizeof(vehicle.info)) == -1)
    {
        perror("Error writing to controller");
        close(fd_vehicle);
        close(fd_controller);
        return NULL;
    }
    
    if (read(fd_vehicle, &feedback, sizeof(feedback)) == -1)
    {
        perror("Error reading controller's feedback");
        unlink_fifo(vehicle.info.vehicle_fifo_name);
        close(fd_vehicle);
        close(fd_controller);
        return NULL;
    }
    
    
    if (strcmp(feedback.msg, ACCEPTED_STR) == 0)
    {
        //printf("%s\n", vehicle.info.vehicle_fifo_name);
        
        if (read(fd_vehicle, &feedback, sizeof(feedback)) == -1)
        {
            perror("Error reading controller's second feedback");
            unlink_fifo(vehicle.info.vehicle_fifo_name);
            close(fd_vehicle);
            close(fd_controller);
            return NULL;
        }
        
        //    printf("%s\n", response);
    }
    
    close(fd_vehicle);
    close(fd_controller);
    
    unlink_fifo(vehicle.info.vehicle_fifo_name);
    return NULL;
}






vehicle_t create_vehicle(int ID) {
    vehicle_t vehicle;
    
    strcpy(vehicle.controller_fifo_name, controller_name[rand() % NUM_CONTROLLERS]);            // generate entry point (controller)
    vehicle.info.vehicle_id = ID;                                                               // set ID
    vehicle.info.parking_time =  uRelogio * ( (rand() % PARK_TIME_MULTIPLES) + 1 );             // time in the parking lot
    sprintf(vehicle.info.vehicle_fifo_name, "%s_%d", VEHICLE_FIFO_PREFIX, ID);                  // sets vehicle's fifo name
    
    return vehicle;
}



void wait_ticks_random() {
    int generateTimeGap, p;
    
    p = rand() % 10;                                           // wait probability
    if (p < 5)
        generateTimeGap = 0;
    else if (p < 8)
        generateTimeGap = 1;
    else
        generateTimeGap = 2;
    
    wait_ticks(generateTimeGap * uRelogio);
}