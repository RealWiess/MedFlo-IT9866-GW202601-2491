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

/*
 * message_buffer.c contains code for a message buffer .
 */

#include "message_buffer.h"

void append_to_buffer(buffer *p_Buffer, char *message,char message_level,char print){
	/*wrapper function for add_to_buffer_root_function*/
	add_to_buffer_root_function(p_Buffer,message, message_level,APPEND_MESSAGE,print);
}

void add_to_buffer(buffer *p_Buffer, char *message,char message_level,char print){
	/*wrapper function for add_to_buffer_root_function*/
	add_to_buffer_root_function(p_Buffer,message, message_level,DO_NOT_APPEND_MESSAGE,print);
}

void add_int_to_buffer(buffer *p_Buffer, int num,char message_level,char print){
	char message[20];
	sprintf(message, "%d",num);
	add_to_buffer_root_function(p_Buffer,message, message_level,DO_NOT_APPEND_MESSAGE,print);
}

void append_int_to_buffer(buffer *p_Buffer, int num,char message_level,char print){
	char message[20];
	sprintf(message, "%d",num);
	add_to_buffer_root_function(p_Buffer,message, message_level,APPEND_MESSAGE,print);
}


void add_to_buffer_root_function(buffer *p_Buffer, char *message,char message_level,char append,char print){
	FILE *log_handle;
	
	if (append==APPEND_MESSAGE){	/* append current message*/
		strncat(p_Buffer->message[p_Buffer->head],message,BUFFER_MESSAGE_LENGTH);
	}
	else {

		/*move the head up one. the head always points at the last written data*/
		p_Buffer->head++;

		/*printf("p_Buffer->tail=%d\n",p_Buffer->tail);*/
		/*printf("p_Buffer->head=%d\n",p_Buffer->head);*/
		if (p_Buffer->head >=BUFFER_SIZE-1) {
			/*printf("buffer overflow\n");*/
			p_Buffer->head -= (BUFFER_SIZE-1);	/*wrap the head around the buffer*/
			/*printf("new p_Buffer->head=%d\n",p_Buffer->head);*/
		}	
		/*if the buffer is full then delete last entry by moving the tail*/
		if (p_Buffer->head==p_Buffer->tail){
			/*printf("moving buffer tail from %d to ",p_Buffer->tail);*/
			p_Buffer->tail++;	
			if (p_Buffer->tail >=BUFFER_SIZE) p_Buffer->tail -= BUFFER_SIZE;/*wrap the tail around the buffer*/
			/*printf("%d\n",p_Buffer->tail);*/

		}

		p_Buffer->message_level[p_Buffer->head]=message_level; /*copy the message level*/
		strncpy(p_Buffer->message[p_Buffer->head],message,BUFFER_MESSAGE_LENGTH); /*copy the message*/
		/*printf("copied %s into p_Buffer->message[p_Buffer->head]: %s\n",message,p_Buffer->message[p_Buffer->head]);
		printf("extra %s\n",p_Buffer->message[p_Buffer->head]);*/
	}
	if ((p_Buffer->message_level[p_Buffer->head]<=DEBUG_LEVEL)&& (print==PRINT)){
		/*printf("printing buffer 1\n");
		// the print buffer function is not working this is just a quick fix		 
		print_buffer(p_Buffer,1); //if the debug level is higher enough then print the new message  
		*/
		printf("%s\n",p_Buffer->message[p_Buffer->head]);
		log_handle=fopen(LOG_FILE,"a");
		if (log_handle!=NULL){
			fputs(p_Buffer->message[p_Buffer->head],log_handle);
			fputs("\n",log_handle);
			fclose(log_handle);
		}
	}
}



void print_buffer(buffer *p_Buffer, int number_of_messages_to_print){	
	int x=0;
	int i=0;
	printf("print buffer\n");
	printf("buffer head:%d\n",p_Buffer->head);
	printf("buffer tail:%d\n",p_Buffer->tail);

	if (number_of_messages_to_print==PRINT_ALL) number_of_messages_to_print=BUFFER_SIZE;
//	printf("number_of_messages_to_print=%d\n",number_of_messages_to_print);
	for (i=0,x=0; (x>=p_Buffer->tail) && (i<number_of_messages_to_print); i++, x--){
//		printf("printing buffer\n");
//		printf("x:%d\n",x);
		if (x<0) x = BUFFER_SIZE-1;		/*wrap x around buffer*/
		printf("%s\n",p_Buffer->message[p_Buffer->head]);
		printf("printed buffer\n");
	}

}



