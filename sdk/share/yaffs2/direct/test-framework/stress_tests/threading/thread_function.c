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

#include "thread_function.h"



int number_of_tests=2;


void * init(int threadId, const char *baseDir, int bovver_type)
{
	struct bovver_context *bc = malloc(sizeof(struct bovver_context));

	if(bc){
		memset(bc,0,sizeof(*bc));
		bc->threadId = threadId;
		strncpy(bc->baseDir,baseDir,200);
		bc->bovverType = bovver_type;
		bc->opMax = 99;
		printf("bovver_init %d \"%s\"\n",threadId,baseDir);
	}
	return (void *)bc;
}


typedef struct test {
	void (*p_function)(void *);	/*pointer to test function*/
	/*char pass_message[50]; will not need a pass message*/
	char *name_of_test;	/*pointer to fail message, needs to include name of test*/
}test_template;


test_template test_list[]={
	//{dummy_test,dummy_test_clean,"dummy_test"},
	{test_a, "test_a"},
	{test_b, "test_b"}
};

int thread_function(void *thread_id_ptr)
{
	int thread_id = (int) thread_id_ptr;
	int test_id=0;
	unsigned int y=0;
	printf("Starting thread %d, id %d\n", pthread_self(),thread_id);
	void *x=init(thread_id,"/yaffs2/",0);
	while(1){
		y++;
		//set_counter(thread_id,y);
		test_id=(rand()%(number_of_tests-1));
		test_list[test_id].p_function(x);
		//printf("thread: %d. ran test: %d\n",thread_id,y);
		//printf("counter before %d\n",get_counter(thread_id));
		set_counter(thread_id,y);
		//printf("counter after setting %d\n",get_counter(thread_id));
	}
	//select random file name from a list.
	//run a random function on the file.
	return 1;
}
