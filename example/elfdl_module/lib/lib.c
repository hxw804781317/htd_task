#include "htd_task.h"
#include <stdio.h>


int lib_func(void)
{
    htd_print("hello world from dynamic library!\n");
    return 0;
}

int add_func(int a, int b)
{
    return (a + b);
}

