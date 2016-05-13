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
#define MAX_REQUESTS            500


/* Global Variables */
static int numLugares;
static int tAbertura;
static int numLugaresOcupados = 0;
static pthread_mutex_t arrumador_lock = PTHREAD_MUTEX_INITIALIZER;


/* Functions */
void* controlador(void* arg);
void* arrumador(void* arg);


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
    
    for (i = 0; i < NUM_CONTROLLERS; i++)
    {
        pthread_join(threads[i], NULL);
    }
    
    pthread_exit(0);
}




void* controlador(void* arg) {
    clock_t start;
    char* fifo_name;
    int flags, fd;
    double timeElapsed = 0;
    pthread_t detach_thread;
    info_t* request_info;
    
    fifo_name = (char *) arg;
    
    if (mkfifo(fifo_name, S_IWUSR | S_IRUSR) == -1)
    {
        perror(strcat(fifo_name, " FIFO creation failed on controlador"));
        pthread_exit(NULL);
    }
    
    
    if ( (fd = open(fifo_name, O_RDWR)) == -1 )
    {
        perror(strcat(fifo_name, " FIFO opening failed on controlador"));
        pthread_exit(NULL);
    }
    
    
    /* Set FIFO to not block on read */
    flags = fcntl(fd, F_GETFL, 0);
    fcntl(fd, F_SETFL, flags | O_NONBLOCK);
    
    
    request_info = (info_t *) malloc(sizeof(info_t));
    start = clock();
    while (timeElapsed < tAbertura) {
        
        int ret;
        if ( (ret = read(fd, request_info, sizeof(*request_info))) > 0 )
        {
            pthread_create(&detach_thread, NULL, arrumador, request_info);
            request_info = (info_t *) malloc(sizeof(info_t));
        }
        else if (ret == -1 && errno != EAGAIN)          // if error is not caused by O_NONBLOCK 
        {
            perror("Error reading");
            free(request_info);
            close(fd);
            pthread_exit(NULL);
        } 
        
        timeElapsed = (double) (clock() - start) / CLOCKS_PER_SEC;
    }
    
    free(request_info);
    close(fd);
    
    if (unlink(fifo_name) == -1)
    {
        free(request_info);
        perror(strcat(fifo_name, " FIFO unlink failed on controlador"));
        pthread_exit(NULL);
    }
    
    pthread_exit(NULL);
}



void* arrumador(void* arg) {
    info_t info;
    int fd_vehicle;
    int accepted = 0;
    
    
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
        write(fd_vehicle, ACCEPTED_STR, sizeof(ACCEPTED_STR));
        
    }
    else {
        write(fd_vehicle, FULL_STR, sizeof(FULL_STR));
    }
    
    pthread_mutex_unlock(&arrumador_lock);
    /*********************************/
    
    if (accepted)
    {
        // Esperar
        wait_ticks(info.parking_time);
        
        // Validar saida (Zona critica)
        pthread_mutex_lock(&arrumador_lock);
        numLugaresOcupados--;
        write(fd_vehicle, EXITING_STR, sizeof(EXITING_STR));
        pthread_mutex_unlock(&arrumador_lock);
        /*********************************/
        
        //printf("%s - %s\n", info.vehicle_fifo_name, EXITING_STR);
    }
    
    
    free(arg);
    close(fd_vehicle);
    return NULL;
}