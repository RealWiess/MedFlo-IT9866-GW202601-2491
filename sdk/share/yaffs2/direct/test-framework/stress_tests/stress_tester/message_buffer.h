/*
 * YAFFS: Yet Another Flash File System. A NAND-flash specific file
 * system.
 *
 * Copyright (C) 2002-2021 Aleph One Ltd.
 *
 * Created by Charles Manning <charles@aleph1.co.uk>
 *
 * This release of YAFFS is for commercial use by commercially licensed
 * customers only. It may be used under the terms of your agreement with
 * Aleph One Ltd. and is limited to use in products for which the
 * customer is licensee, unless the customer has received an unlimited
 * licence from Aleph One Ltd., in which case it may be used in any
 * product.
 * 
 * In the event that this code is used outside the terms of a current,
 * valid and paid agreement with Aleph One Ltd, the code is subject to
 * the GPL version3 license under which the open source code is
 * distributed.
 * 
 * This release was created on 2025-04-18.
 *
 */

#ifndef __message_buffer__
#define __message_buffer__

#include <stdio.h>
#include <string.h>
#define PRINT 1
#define NPRINT 0
#define APPEND_MESSAGE 1
#define DO_NOT_APPEND_MESSAGE 0		
#define PRINT_ALL -1			/*this is used to print all of the messages in a buffer*/
#define BUFFER_MESSAGE_LENGTH 60		/*number of char in message*/
#define BUFFER_SIZE 50			/*number of messages in buffer*/
#define MESSAGE_LEVEL_ERROR 0
#define MESSAGE_LEVEL_BASIC_TASKS 1

#define LOG_FILE "log.txt"
typedef struct buffer_template{
	char message[BUFFER_SIZE][BUFFER_MESSAGE_LENGTH];
	int head;
	int tail;
	char message_level[BUFFER_SIZE];
}buffer; 
#include "error_handler.h"		/*include this for the debug level*/


void print_buffer(buffer *p_Buffer,int number_of_messages_to_print);		/*print messages in the buffer*/ 
/*wrapper functions for add_to_buffer_root_function*/
void add_to_buffer(buffer *p_Buffer, char *message,char message_level,char print);
void append_to_buffer(buffer *p_Buffer, char *message,char message_level,char print);
void add_int_to_buffer(buffer *p_Buffer, int num,char message_level,char print);
void append_int_to_buffer(buffer *p_Buffer, int num,char message_level,char print);

void add_to_buffer_root_function(buffer *p_Buffer, char *message,char message_level,char append,char print);
#endif
