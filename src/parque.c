#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <time.h>
#include <errno.h>
#include <pthread.h>
#include <string.h>
#include <pthread.h>
#include "common.h"
#include "utils.h"

#define NUM_CONTROLLERS         4
#define LOGGER_NAME             "parque.log"

const char FINISH_STR[] = "ACABAR";

const char LOG_ACCEPTED_STR[] = "estacionamento";
const char LOG_FULL_STR[] = "cheio";
const char LOG_EXITING_STR[] = "saida";
const char LOG_CLOSED_STR[] = "encerrado";


/* Global Variables */
static clock_t start;
static int numLugares;
static int tAbertura;
static int numLugaresOcupados = 0;
static pthread_mutex_t arrumador_lock = PTHREAD_MUTEX_INITIALIZER;
static FILE* fp_logger;
static sem_t* sem;


/* Functions */
void* controlador(void* arg);
void* arrumador(void* arg);
void notify_controllers(const char* message);
int notify_pending_vehicles(int fd_controller);
FILE* init_logger(char* name);
int park_log(info_t info, const char* status);


int main(int argc, char** argv) {
    
    pthread_t threads[NUM_CONTROLLERS];
    
    if (argc != 3) 
    {
        fprintf(stderr, "Usage: parque <N_LUGARES> <T_ABERTURA>\n");
        exit(1);
    }
    
    
    if (convert_str_to_int(argv[1], &numLugares) != 0)
    {
        fprintf(stderr, "Invalid entry for N_LUGARES\n");
        exit(2);
    }
    
    if (convert_str_to_int(argv[2], &tAbertura) != 0)
    {
        fprintf(stderr, "Invalid entry for T_ABERTURA\n");
        exit(3);
    }
    
    if ( (fp_logger = init_logger(LOGGER_NAME)) == NULL )
    {
        fprintf(stderr, "Error opening %s\n", LOGGER_NAME);
        exit(4);
    }
    
    if ( (sem = init_sem(SEM_NAME)) == SEM_FAILED )
    {
        exit(5);
    }
    
    start = clock();
    
    int i;
    for (i = 0; i < NUM_CONTROLLERS; i++)
    {
        pthread_create(&threads[i], NULL, controlador, controller_name[i]);
    }
    
    sleep(tAbertura);
    
    /* Zona Critica */
    sem_wait(sem);
    
    notify_controllers(FINISH_STR);
    
    for (i = 0; i < NUM_CONTROLLERS; i++)
        pthread_join(threads[i], NULL);
    
    sem_post(sem);
    /***************/
    
    destroy_sem(sem, SEM_NAME);
    
    fclose(fp_logger);
    pthread_exit(0);
}




void* controlador(void* arg) {
    char* fifo_name;
    int fd, ret;
    pthread_t detach_thread;
    info_t* request_info;
    
    
    fifo_name = (char *) arg;
    
    if ( (fd = init_fifo(fifo_name, O_RDONLY)) == -1 )
        pthread_exit(NULL);
    
    
    while (1) {
        request_info = (info_t *) malloc(sizeof(info_t));
        
        ret = read(fd, request_info, sizeof(*request_info));
        
        if (ret > 0)
        {
            if (strcmp(request_info->vehicle_fifo_name, FINISH_STR) == 0)
                break;
            
            pthread_create(&detach_thread, NULL, arrumador, request_info);
        }
        else if (ret == -1)
        {
            perror("Error reading on controller");
            free(request_info);
            close(fd);
            unlink_fifo(fifo_name);
            return NULL;
        }
    }
    
    close(fd);
    unlink_fifo(fifo_name);
    pthread_exit(NULL);
}



void* arrumador(void* arg) {
    info_t info;
    int fd_vehicle;
    int accepted = 0;
    feedback_t feedback;
    
    
    pthread_detach(pthread_self());
    info = *(info_t *) arg;
    
    
    if ( (fd_vehicle = open(info.vehicle_fifo_name, O_WRONLY)) == -1 )
    {
        perror(strcat(info.vehicle_fifo_name, " FIFO opening failed on arrumador"));
        free(arg);
        return NULL;
    }
    
    // Validar entrada (Zona critica)
    pthread_mutex_lock(&arrumador_lock);
    
    if (numLugaresOcupados < numLugares)
    {
        accepted = 1;
        numLugaresOcupados++;
        strcpy(feedback.msg, ACCEPTED_STR);
        park_log(info, LOG_ACCEPTED_STR);
    }
    else {
        strcpy(feedback.msg, FULL_STR);
        park_log(info, LOG_FULL_STR);
    }
    
    pthread_mutex_unlock(&arrumador_lock);
    /*********************************/
    
    write(fd_vehicle, &feedback, sizeof(feedback));
    
    if (accepted)
    {
        feedback_t exit_feedback;
        
        // Esperar
        wait_ticks(info.parking_time);
        
        // Validar saida (Zona critica)
        pthread_mutex_lock(&arrumador_lock);
        
        numLugaresOcupados--;
        strcpy(exit_feedback.msg, EXITING_STR);
        park_log(info, LOG_EXITING_STR);
        
        pthread_mutex_unlock(&arrumador_lock);
        /*********************************/
        
        write(fd_vehicle, &exit_feedback, sizeof(exit_feedback));
    }
    
    
    free(arg);
    close(fd_vehicle);
    return NULL;
}





void notify_controllers(const char* message) {
    int fd;
    info_t info;
    
    strcpy(info.vehicle_fifo_name, message);
    
    int i;
    for (i = 0; i < NUM_CONTROLLERS; i++)
    {
        if ( (fd = open(controller_name[i], O_WRONLY)) == -1 )
        {
            perror("Error notifying controllers");
            return;
        }
        
        write(fd, &info, sizeof(info));
        close(fd);
    }
}


FILE* init_logger(char* name) {
    FILE* fp;
    
    if ( (fp = fopen(name, "w")) == NULL )
        return NULL;
    
    fprintf(fp, "t(ticks) ; nlug ; id_viat ; observ\n");
    
    return fp;
}



int park_log(info_t info, const char* status) {
    char log_msg[LOG_LENGTH];
    
    sprintf(log_msg, "%8ld ; %4d ; %7d ; %s\n",
            clock() - start,
            numLugaresOcupados, 
            info.vehicle_id, 
            status);
    
    return Log(fp_logger, log_msg);
}