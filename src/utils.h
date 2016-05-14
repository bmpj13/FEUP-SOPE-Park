#ifndef UTILS_H
#define UTILS_H

#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#define LOG_LENGTH              1024

int convert_str_to_int(char* str, int* num);
void wait_ticks(clock_t wait_time);
int init_fifo(char* fifo_name, int oflags);
int unlink_fifo(char* fifo_name);
int Log(FILE* fp, const char* message);

#endif