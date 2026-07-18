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
 * Fuzzer to fuzz a file
 */

#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
                     
int main(int argc, char *argv[])
{
	int prob = 10000;
	int h;
	int flen = 0;
	int changesPerBuffer = 0;
	int b;
	char c;
	unsigned char buffer[1000000];
	int bufsize;
	int x;
	int i;
	int nbuffers;

	while((c = getopt(argc,argv,"p:")) != -1){
		switch(c){
			case 'p':
				prob = atoi(optarg);
				break;
		}
	}
	if(prob < 100){
		printf("-p value less than 100 is invalid\n");
		return 1;
	}
	
	if(optind >= argc){
		printf(" Needs a file name to fuzz\n");
		return 1;
	}
	
	h = open(argv[optind], O_RDWR);
	flen = lseek(h,0,SEEK_END);
	lseek(h,0,SEEK_SET);
	if(flen < 1){
		printf(" File is too short\n");
		return 1;
	}
	
	nbuffers = (flen + sizeof(buffer) - 1) / sizeof(buffer);

	changesPerBuffer = 1+ (sizeof(buffer) * 8) / prob;

	printf("Fuzzing file %s. Size %d, probablity 1/%d, changing %d bits in each of %d buffers\n",
		argv[optind],flen,prob,changesPerBuffer,nbuffers);

	srand(time(0));

	for(b = 0; b < nbuffers; b++){
		/* printf("buffer %d\n",b); */
		lseek(h,b * sizeof(buffer),SEEK_SET);
		bufsize = read(h,buffer,sizeof(buffer));
		for(i = 0; i < changesPerBuffer; i++){
			x = rand() % (sizeof(buffer) * 8);
			buffer[x >> 3] ^= (1 << (x & 7));
		}
		lseek(h,b * sizeof(buffer),SEEK_SET);
		write(h,buffer,bufsize);
	}
	close(h);
	return 0;
}
