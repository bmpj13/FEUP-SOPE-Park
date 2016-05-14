#include "utils.h"


static int ERROR_MSG_LEN = 200;


/*
 * strtol man page:
 * 
 * "Since strtol() can legitimately return 0, LONG_MAX, or LONG_MIN
 *      on both success and failure,
 *      the calling program should set errno to 0 before the call, and then
 *      determine if an error occurred by checking whether errno has a
 *      nonzero value after the call."
 */
int convert_str_to_int(char* str, int* num) {
    
    int i = 0;
    while (str[i] != '\0')
    {
        if (str[i] >= '1' && str[i] <= '9')
            break;
        
        i++;
    }
    
    if (i == strlen(str))
        return -1;
    
    errno = 0;
    *num = strtol(str, NULL, 10);
    if (errno != 0)
        return -1;
    
    return 0;
}


/* Waits 'wait_time' ticks */
void wait_ticks(clock_t wait_time) {
    clock_t start, diff;
    
    start = clock();
    diff = clock();
    while ((diff - start) < wait_time)
        diff = clock();
}




/* Creates and opens FIFO */
int init_fifo(char* fifo_name, int oflags) {
    int fd;
    char msg[ERROR_MSG_LEN];
    
    if (mkfifo(fifo_name, S_IWUSR | S_IRUSR) == -1)
    {
        sprintf(msg, "FIFO %s creation failed", fifo_name);
        perror(msg);
        return -1;
    }
    
    if ( (fd = open(fifo_name, oflags)) == -1 )
    {
        sprintf(msg, "FIFO %s opening failed", fifo_name);
        perror(msg);
        unlink_fifo(fifo_name);
        return -1;
    }
    
    return fd;
}



/* Handles unlink */
int unlink_fifo(char* fifo_name) {
    char msg[ERROR_MSG_LEN];
    
    if (unlink(fifo_name) == -1)
    {
        sprintf(msg, "FIFO %s unlink failed", fifo_name);
        perror(msg);
        return -1;
    }
    
    return 0;
}




int Log(FILE* fp, const char* message) {
    return fprintf(fp, "%s", message);
}