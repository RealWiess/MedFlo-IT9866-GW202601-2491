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
 * error_handler.c contains code for checking yaffs function calls for errors.
 */
#include "error_handler.h"
#include "shared.h"

typedef struct error_codes_template {
  int code;
  char * text;  
}error_entry;

const error_entry error_list[] = {
	{ ENOMEM , "ENOMEM" },
	{ EBUSY , "EBUSY"},
	{ ENODEV , "ENODEV"},
	{ EINVAL , "EINVAL"},
	{ EBADF , "EBADF"},
	{ EACCES , "EACCES"},
	{ EXDEV , "EXDEV" },
	{ ENOENT , "ENOENT"},
	{ ENOSPC , "ENOSPC"},
	{ ERANGE , "ERANGE"},
	{ ENODATA, "ENODATA"},
	{ ENOTEMPTY, "ENOTEMPTY"},
	{ ENAMETOOLONG,"ENAMETOOLONG"},
	{ ENOMEM , "ENOMEM"},
	{ EEXIST , "EEXIST"},
	{ ENOTDIR , "ENOTDIR"},
	{ EISDIR , "EISDIR"},
	{ 0, NULL }
};

char * error_to_str(int err)
{
	error_entry const *e = error_list;
	if (err < 0) 
		err = -err;
	while(e->code && e->text){
		if(err == e->code)
			return e->text;
		e++;
	}
	return "Unknown error code";
}

void yaffs_check_for_errors(char output, buffer *message_buffer,char error_message[],char success_message[]){
	int yaffs_error=-1;

	if (output==-1)
	{
		add_to_buffer(message_buffer, "\nerror##########",MESSAGE_LEVEL_ERROR,PRINT);
		add_to_buffer(message_buffer, error_message,MESSAGE_LEVEL_ERROR,PRINT);
		add_to_buffer(message_buffer, "error_code: ",MESSAGE_LEVEL_ERROR,NPRINT);
		yaffs_error=yaffs_get_error();
		append_int_to_buffer(message_buffer, yaffs_error,MESSAGE_LEVEL_ERROR,PRINT);

		add_to_buffer(message_buffer, error_to_str(yaffs_error),MESSAGE_LEVEL_ERROR,NPRINT);
		append_to_buffer(message_buffer, "\n\n",MESSAGE_LEVEL_ERROR,PRINT);	
		quit_program();
		//scanf("%c",dummy);	/*this line causes a segmentation fault. Need a better way of waiting for a key press*/
		//print_buffer(message_buffer,PRINT_ALL);
		
	}
	else{
		add_to_buffer(message_buffer, success_message,MESSAGE_LEVEL_BASIC_TASKS,PRINT);
	}		
}


