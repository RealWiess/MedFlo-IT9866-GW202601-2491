#include <pthread.h>
#include "openrtos/FreeRTOS.h"
#include "openrtos/task.h"

#define TEST_STACK_SIZE 262140

extern int SDL_main(int argc, char *argv[]);

int main(void)
{
    pthread_t task;
    pthread_attr_t attr;

    pthread_attr_init(&attr);
    attr.stacksize = TEST_STACK_SIZE;

    pthread_create(&task, &attr, (void* (*)(void*))SDL_main, NULL);

    /* Now all the tasks have been started - start the scheduler. */
    vTaskStartScheduler();

    /* Should never reach here! */
    return 0;
}
