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


/* Global Variables */
static int numLugares;
static int tAbertura;
static int numLugaresOcupados = 0;
static pthread_mutex_t arrumador_lock = PTHREAD_MUTEX_INITIALIZER;


/* Functions */
void* controlador(void* arg);
void* arrumador(void* arg);
void notify_controllers(const char* message);


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
    
    
    int i;
    for (i = 0; i < NUM_CONTROLLERS; i++)
    {
        pthread_create(&threads[i], NULL, controlador, controller_name[i]);
    }
    
    sleep(tAbertura);
    notify_controllers(FINISH_STR);
    
    for (i = 0; i < NUM_CONTROLLERS; i++)
    {
        pthread_join(threads[i], NULL);
    }
    
    pthread_exit(0);
}




void* controlador(void* arg) {
    char* fifo_name;
    int fd, ret;
    pthread_t detach_thread;
    info_t* request_info;
    
    fifo_name = (char *) arg;
    
    if ( (fd = init_fifo(fifo_name)) == -1 )
        pthread_exit(NULL);
    
    request_info = (info_t *) malloc(sizeof(info_t));
    while (1) {
        
        ret = read(fd, request_info, sizeof(*request_info));
        
        if (ret > 0)
        {
            if (strcmp(request_info->vehicle_fifo_name, FINISH_STR) == 0)
                break;
            
            printf("%s\n", request_info->vehicle_fifo_name);
            
            pthread_create(&detach_thread, NULL, arrumador, request_info);
            request_info = (info_t *) malloc(sizeof(info_t));
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
    
    free(request_info);
    close(fd);
    unlink_fifo(fifo_name);
    pthread_exit(NULL);
}



void* arrumador(void* arg) {
    info_t info;
    int fd_vehicle;
    int accepted = 0;
    feedback_t request_feedback;
    
    pthread_detach(pthread_self());
    
    
    printf("%d\n", numLugaresOcupados);
    
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
        strcpy(request_feedback.msg, ACCEPTED_STR);
    }
    else {
        strcpy(request_feedback.msg, FULL_STR);
    }
    
    pthread_mutex_unlock(&arrumador_lock);
    /*********************************/
    
    write(fd_vehicle, &request_feedback, sizeof(request_feedback));
    
    if (accepted)
    {
        feedback_t exit_feedback;
        
        // Esperar
        wait_ticks(info.parking_time);
        
        // Validar saida (Zona critica)
        pthread_mutex_lock(&arrumador_lock);
        numLugaresOcupados--;
        pthread_mutex_unlock(&arrumador_lock);
        /*********************************/
        
        strcpy(exit_feedback.msg, EXITING_STR);
        write(fd_vehicle, &exit_feedback, sizeof(exit_feedback));
        
        //printf("%s - %s\n", info.vehicle_fifo_name, EXITING_STR);
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
    }
}