#include <stdio.h>
#include <pthread.h>
#include "openrtos/FreeRTOS.h"
#include "openrtos/task.h"

#define TEST_STACK_SIZE 262140

extern void* MainFunc(void* arg);

int main(void)
{
    printf("\n>>> main_openrtos: entry <<<\n");
    fflush(stdout);

    pthread_t task;
    pthread_attr_t attr;

    pthread_attr_init(&attr);
    attr.stacksize = TEST_STACK_SIZE;

    printf(">>> main_openrtos: creating MainFunc thread...\n");
    fflush(stdout);

    pthread_create(&task, &attr, MainFunc, NULL);

    printf(">>> main_openrtos: starting scheduler...\n");
    fflush(stdout);

    /* Now all the tasks have been started - start the scheduler. */
    vTaskStartScheduler();

    /* Should never reach here! */
    printf(">>> main_openrtos: SCHEDULER RETURNED! <<<\n");
    return 0;
}
