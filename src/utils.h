#ifndef UTILS_H
#define UTILS_H

#include <errno.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>

int convert_str_to_int(char* str, int* num);
void wait_ticks(clock_t wait_time);

#endif