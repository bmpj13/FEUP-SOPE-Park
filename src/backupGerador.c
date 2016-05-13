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


#define RESPONSE_LEN              31



typedef struct {
    char controller_fifo_name[CONTROLLER_NAME_LEN];
    info_t info;
} vh_info_t;


int tGeracao;
int uRelogio;



void* veiculo(void* arg);


int main(int argc, char **argv)
{
    vh_info_t vh_info;
    int veicID = 1;
    pthread_t threads;
    
    
    strcpy(vh_info.controller_fifo_name, "/tmp/fifoN");
    vh_info.info.parking_time = 50000;
    vh_info.info.vehicle_id = veicID;
    strcpy(vh_info.info.vehicle_fifo_name, "/tmp/jorge");
    
    pthread_create(&threads, NULL, veiculo, &vh_info);
    //pthread_join(threads, NULL);

    
    /*
     *   clock_t start, end, auxStart, auxEnd;
     *   int entry;
     *   int time=0;
     *   int vehicle =0;
     *   int parked;
     *   int newVehicle;
     *   int auxTime=0;
     *   
     *   
     *   
     *   if(argc != 3)
     *   { 
     *       fprintf(stderr, "arguments invalid");
     *       exit(1);		
}

if(convert_str_to_int(argv[1], &tGeracao) == -1)
{
fprintf(stderr, "Error to Time Generator");
exit(2);			
}

if(convert_str_to_int(argv[2], &uRelogio)== -1)
{
fprinf(stdout, "Error to Unit Relog");
exit(3);		
}

start = clock();
while(time < tGeracao)
{
entry = srand(time(NULL)) % 4;            //0=N, 1 =S, 2=E, 3=W
parked = srand(time(NULL)) % 10 + 1;       //time in park
newVehicle = srand(time(NULL)) %10;       //probability

if(newVehicle >	7)
{
auxStart = clock();
while(auxTime < 2*uRelogio)
{
auxEnd = clock();
auxTime = (auxEnd - auxStart) / CLOCKS_PER_SEC;
}
auxTime = 0;
}
else if(newVehicle > 4)
{
auxStart = clock();
while(auxTime < uRelogio)
{
auxEnd = clock();
auxTime = (auxEnd - auxStart) / CLOCKS_PER_SEC;
}
auxTime=0;

}


//cria aqui um veiculo


newVehicle ++;

end = clock();
time = (end - start) / CLOCK_PER_SEC; 
}
*/
    
    pthread_exit(0);
}





void* veiculo(void* arg) {
    
    vh_info_t vh_info;
    int fd_vehicle, fd_controller;
    char response[RESPONSE_LEN];
    
    
    pthread_detach(pthread_self());
    
    vh_info = *(vh_info_t *) arg;
    
    
    if ( (fd_controller = open(vh_info.controller_fifo_name, O_WRONLY)) == -1 )
    {
        perror("Controller's FIFO opening failed (on Vehicle)");
        return NULL;
    }
    
    
    if (mkfifo(vh_info.info.vehicle_fifo_name, S_IWUSR | S_IRUSR) == -1)
    {
        perror("Vehicle's FIFO creation failed");
        unlink(vh_info.info.vehicle_fifo_name);
        close(fd_controller);
        return NULL;
    }
    
    if ( (fd_vehicle = open(vh_info.info.vehicle_fifo_name, O_RDWR)) == -1 )
    {
        perror("Vehicle's FIFO opening failed");
        unlink(vh_info.info.vehicle_fifo_name);
        close(fd_controller);
        return NULL;
    }
    
    
    if (write(fd_controller, &vh_info.info, sizeof(vh_info.info)) == -1)
    {
        perror("Error writing to controller");
        return NULL;
    }
    
    if (read(fd_vehicle, response, sizeof(response)) == -1)
    {
        perror("Error reading controller's response");
        return NULL;
    }
    
    
    printf("%s\n", response);
    
    
    if (strcmp(response, ACCEPTED_STR) == 0)
    {
        if (read(fd_vehicle, response, sizeof(response)) == -1)
        {
            perror("Error reading controller's response");
            return NULL;
        }
        
        printf("%s\n", response);
    }
    
    close(fd_vehicle);
    close(fd_controller);
    
    if (unlink(vh_info.info.vehicle_fifo_name) == -1)
    {
        perror("Vehicle's FIFO unlink failed");
        return NULL;
    }
    
    return NULL;
}