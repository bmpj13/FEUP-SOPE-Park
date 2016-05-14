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
#define LOGGER_NAME                     "gerador.log"


const char VEHICLE_FIFO_PREFIX[] = "/tmp/fifo_vh";
static FILE* fp_logger;


typedef struct {
    char controller_fifo_name[CONTROLLER_NAME_LEN];
    info_t info;
} vehicle_t;


/* Global Variables */
static int tGeracao;
static clock_t uRelogio;
clock_t start;


/* Functions */
void* veiculo(void* arg);
vehicle_t create_vehicle(int ID);
void wait_ticks_random();
FILE* init_logger(char* name);
int generator_log(vehicle_t vehicle, const char* status);
int generator_log_with_lifespan(vehicle_t vehicle, const char* status, clock_t lifespan);


int main(int argc, char **argv)
{
    double timeElapsed = 0;
    int vehicleID = 0;
    vehicle_t* vehicle;
    
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
    
    if ( (fp_logger = init_logger(LOGGER_NAME)) == NULL )
    {
        fprintf(stderr, "Error opening %s\n", LOGGER_NAME);
        exit(4);
    }
    
    
    start = clock();
    while (timeElapsed < tGeracao)
    {
        vehicle = malloc(sizeof(vehicle_t));
        *vehicle = create_vehicle(vehicleID++);
        
        wait_ticks_random();
        
        pthread_create(&detach_thread, NULL, veiculo, vehicle);
        timeElapsed = (double) (clock() - start) / CLOCKS_PER_SEC; 
    }
    
    pthread_exit(0);
}




void* veiculo(void* arg) {
    vehicle_t vehicle;
    int fd_vehicle, fd_controller;
    feedback_t feedback;
    clock_t lifespan_start, lifespan_diff;

    
    vehicle = *(vehicle_t *) arg;
    
    pthread_detach(pthread_self());
    
    if ( (fd_controller = open(vehicle.controller_fifo_name, O_WRONLY)) == -1 )
    {
        generator_log(vehicle, CLOSED_STR);
        free(arg);
        return NULL;
    }
    
    
    if ( (fd_vehicle = init_fifo(vehicle.info.vehicle_fifo_name, O_RDWR)) == -1 ) {
        free(arg);
        return NULL;
    }
    
    lifespan_start = clock();
    
    if (write(fd_controller, &vehicle.info, sizeof(vehicle.info)) == -1)
    {
        perror("Error writing to controller");
        close(fd_vehicle);
        close(fd_controller);
        free(arg);
        return NULL;
    }
    
    if (read(fd_vehicle, &feedback, sizeof(feedback)) == -1)
    {
        perror("Error reading controller's feedback");
        unlink_fifo(vehicle.info.vehicle_fifo_name);
        close(fd_vehicle);
        close(fd_controller);
        free(arg);
        return NULL;
    }
    
    
    generator_log(vehicle, feedback.msg);
    
    if (strcmp(feedback.msg, ACCEPTED_STR) == 0)
    {
        if (read(fd_vehicle, &feedback, sizeof(feedback)) == -1)
        {
            perror("Error reading controller's second feedback");
            unlink_fifo(vehicle.info.vehicle_fifo_name);
            close(fd_vehicle);
            close(fd_controller);
            free(arg);
            return NULL;
        }
        
        lifespan_diff = clock() - lifespan_start;
        generator_log_with_lifespan(vehicle, feedback.msg, lifespan_diff + vehicle.info.parking_time);
    }
    
    close(fd_vehicle);
    close(fd_controller);
    free(arg);
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




FILE* init_logger(char* name) {
    FILE* fp;
    
    if ( (fp = fopen(name, "w")) == NULL )
        return NULL;
    
    fprintf(fp, "t(ticks) ; id_viat ; destin ; t_estacion ; t_vida ; observ\n");
    
    return fp;
}



int generator_log(vehicle_t vehicle, const char* status) {
    char log_msg[LOG_LENGTH];
    
    sprintf(log_msg, "%8ld ; %7d ; %4c   ; %10ld ; %6s ; %s\n", 
            clock() - start, 
            vehicle.info.vehicle_id, 
            vehicle.controller_fifo_name[strlen(vehicle.controller_fifo_name) - 1],
            vehicle.info.parking_time,
            "?",
            status
           );
    
    return Log(fp_logger, log_msg);
}




int generator_log_with_lifespan(vehicle_t vehicle, const char* status, clock_t lifespan) {
    char log_msg[LOG_LENGTH];
    char lifespan_str[100];             // string where lifespan will be stored
    
    
    sprintf(lifespan_str, "%ld", lifespan);
    
    sprintf(log_msg, "%8ld ; %7d ; %4c   ; %10ld ; %6s ; %s\n", 
            clock() - start, 
            vehicle.info.vehicle_id, 
            vehicle.controller_fifo_name[strlen(vehicle.controller_fifo_name) - 1],
            vehicle.info.parking_time,
            lifespan_str,
            status
           );
    
    return Log(fp_logger, log_msg);
}