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
			 printf("CHILD\n");
			 for(int i = 0; i < 1000000; i++)
			 {
			  data_processed = read(file_pipes[0], buffer, BUFSIZ);
			  printf("Read %d bytes: %s\n", data_processed, buffer);

			  sleep(2);
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
			 printf("PARENT\n");

			 for(int i = 0; i < 1000000; i++)
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
