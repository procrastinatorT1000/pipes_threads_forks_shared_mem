//============================================================================
// Name        : pipe.cpp
// Author      : slb-rs
// Version     :
// Copyright   : Your copyright notice
// Description : Hello World in C++, Ansi-style
//============================================================================

#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <fcntl.h>

#define  SHR_MEM_NOT_READY  0
#define  SHR_MEM_FILLED     1
#define  SHR_MEM_TAKEN      -1

typedef struct
{
	int operationStatus;	/* SHR_MEM_NOT_READY, SHR_MEM_FILLED or SHR_MEM_TAKEN */
	long sqrVal;
}SQR_SHR_MEM_OBJ;

#define SHARED_MEMORY_OBJECT_NAME "my_shared_memory"
#define SHARED_MEMORY_OBJECT_SIZE sizeof(SQR_SHR_MEM_OBJ)

int main()
{
	int data_processed;
	int file_pipes[2];
	const char some_data[] = "123";
	char buffer[BUFSIZ + 1];
	memset(buffer, 0, sizeof(buffer));

	if (pipe(file_pipes) == 0)
	{

	 pid_t pid = -1;

	 pid = fork();

	 switch(pid)
	 {
		 case 0:	/* child */
		 {
			 /** Process B */
			 printf("CHILD 1\n");

			 int shm = 0;
			 SQR_SHR_MEM_OBJ *pShrMemObj = NULL;

			 if ( (shm = shm_open(SHARED_MEMORY_OBJECT_NAME, O_CREAT|O_RDWR, S_IRWXO|S_IRWXG|S_IRWXU)) == -1 )
			 {
				perror("shm_open");
				return 1;
			 }

			 if ( ftruncate(shm, SHARED_MEMORY_OBJECT_SIZE+1) == -1 )
			 {
				perror("ftruncate");
				return 1;
			 }

			 if ( (pShrMemObj = (SQR_SHR_MEM_OBJ *) mmap(0, SHARED_MEMORY_OBJECT_SIZE+1,
					 	 PROT_WRITE|PROT_READ, MAP_SHARED, shm, 0)) == (void*)-1 )
			 {
				 perror("mmap");
				 return 1;
			 }

			 memset(pShrMemObj, 0, SHARED_MEMORY_OBJECT_SIZE);

			 pid_t pid1 = -1;

			 pid1 = fork();

			 switch(pid1)
			 {
				 case 0:
					 /** Process C */
					 printf("Child 2\n");

					 while(1)
					 {
						 if(pShrMemObj->operationStatus == SHR_MEM_FILLED)
						 {
							 printf("Val GET with ShrMEM %d\n", pShrMemObj->sqrVal);
							 pShrMemObj->operationStatus = SHR_MEM_TAKEN;
						 }
						 sleep(1);
					 }

					 _exit(0);
					 break;
				 case -1:
					 perror("FORK error!\n");
					 break;
				 default:
					 for(int i = 0; i < 100; i++)
					 {
						  data_processed = read(file_pipes[0], buffer, BUFSIZ);
						  printf("Read %d bytes: %s\n", data_processed, buffer);



						  if(pShrMemObj->operationStatus == SHR_MEM_TAKEN ||
								  pShrMemObj->operationStatus == SHR_MEM_NOT_READY)
						  {
							  printf("Val SENT with ShrMEM %d\n", i);
							  pShrMemObj->sqrVal = i;
							  pShrMemObj->operationStatus = SHR_MEM_FILLED;
						  }
						  sleep(2);
					 }

					munmap(pShrMemObj, SHARED_MEMORY_OBJECT_SIZE+1);
					close(shm);
					shm_unlink(SHARED_MEMORY_OBJECT_NAME);

					break;
			 }



			 break;
		 }
		 case -1:	/* error fork */
		 {
			 printf("Fork ERROR\n");
			 break;
		 }
		 default:
		 {
			 /** Process A */
			 printf("PARENT\n");

			 for(int i = 0; i < 20; i++)
			 {
				  data_processed = write(file_pipes[1], some_data, strlen(some_data));
				  printf("Wrote %d bytes\n", data_processed);
				  sleep(1);
			 }
			 break;
		 }
	 }

	 exit(EXIT_SUCCESS);
	}

 exit(EXIT_FAILURE);
}
