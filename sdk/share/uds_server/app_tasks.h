/**
 * @file        app_tasks.h
 *
 * @brief       Header for application tasks.
 *              The stack sizes, priorities and informational memory data are all defined here.
 *              The functions that create application tasks are prototyped here.
 */

#ifndef APP_TASKS_H_
#define APP_TASKS_H_

//This wraps creation of all tasks in a single function.
void InitApplicationTasks(void);

void CreateTaskCANPeriodic(void);

#endif /* APP_TASKS_H_ */
