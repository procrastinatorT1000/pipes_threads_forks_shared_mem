//============================================================================
// Name        : pipe.cpp
// Author      : slb-rs
// Version     :
// Copyright   : Your copyright notice
// Description : Hello World in C++, Ansi-style
//============================================================================

#include <unistd.h>
#include <stdlib.h>
#include <iostream>
#include <string.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <signal.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <pthread.h>
#include <sstream>

#define  SHR_MEM_NOT_READY  0
#define  SHR_MEM_FILLED     1
#define  SHR_MEM_TAKEN      -1

#define SHARED_MEMORY_OBJECT_NAME "sqr_value_container"
#define SHARED_MEMORY_OBJECT_SIZE sizeof(SQR_SHR_MEM_OBJ)

typedef struct
{
	int operationStatus;	/* SHR_MEM_NOT_READY, SHR_MEM_FILLED or SHR_MEM_TAKEN */
	long long sqrVal;
}SQR_SHR_MEM_OBJ;

typedef struct
{
	int id;
	SQR_SHR_MEM_OBJ *pShrMem;
}THREAD_ARG;

int processC(SQR_SHR_MEM_OBJ *pShrMem);
int waitForProcessFinishing(pid_t pid);

/* When a SIGUSR1 signal arrives, set this variable. */
volatile sig_atomic_t terminateProcB = 0;
volatile sig_atomic_t terminateMainProc = 0;

void terminateProcBHandl (int sig)
{
	terminateProcB = 1;
}

void terminateMainProcHandl (int sig)
{
	terminateMainProc = 1;
}

/* Safety get integer form input */
int getIntFromIn()
{
    int x=0;

    while(true)
    {
        std::cout << "Enter an integer: " << std::endl;
        std::string s;
        if(!std::getline(std::cin, s))	/* check correct getting string */
        	break;	/* when program is going to terminate, getline()
        	returns 0, and puts '\0' to string, without handling of return
        	value process is going to infinite loop */

        std::stringstream stream(s);

        if(stream >> x)
        {
            break; /* Entered value is good */
        }
        else
        {
        	std::cout << "Invalid! Value should be >= -2147483648 and <= 2147483647"<< std::endl;
        }
    }
    return x;
}

/*
 * Process A, reads stdI and writes it to pipe
 *
 * writePD - is pipe descriptor for writing
 *
 * */
void processA(int writePD)
{
	size_t writenBlen = 0;
	int val;

	std::cout << "Process A started"<<std::endl;

	 while(!terminateMainProc)
	 {
		 val = getIntFromIn();
		writenBlen = write(writePD, &val, sizeof(val));
//		std::cout << "Wrote " << (int)writenBlen <<" bytes " << std::endl;
	 }

	 std::cout << "Got signal for terminating main process\n";

	return;
}

/*
 * Process B, reads data from A process with pipe
 * 			creates process C,
 * 			squares read data and send it to proc. C
 * 			with shared memory
 *
 * readPD - is pipe descriptor for reading
 *
 * */
int processB(int readPD)
{
		int shm = 0;
		SQR_SHR_MEM_OBJ *pShrMemObj = NULL;

		std::cout << "Process B started"<<std::endl;

		/* open shared memory */
		if ( (shm = shm_open(SHARED_MEMORY_OBJECT_NAME, O_CREAT|O_RDWR, S_IRWXO|S_IRWXG|S_IRWXU)) == -1 )
		{
			std::cerr << "shm_open";
			return 1;
		}

		if ( ftruncate(shm, SHARED_MEMORY_OBJECT_SIZE+1) == -1 )
		{
			std::cerr << "ftruncate";
			return 1;
		}

		if ( (pShrMemObj = (SQR_SHR_MEM_OBJ *) mmap(0, SHARED_MEMORY_OBJECT_SIZE+1,
			 PROT_WRITE|PROT_READ, MAP_SHARED, shm, 0)) == (void*)-1 )
		{
			std::cerr << "mmap\n";
			return 1;
		}

		memset(pShrMemObj, 0, SHARED_MEMORY_OBJECT_SIZE);

		pid_t pidCProc = -1;
		struct sigaction usr_action;
		sigset_t block_mask;
		/* Establish the signal handler for halting proc B */
		sigfillset(&block_mask);
		usr_action.sa_handler = terminateProcBHandl;
		usr_action.sa_mask = block_mask;
		usr_action.sa_flags = 0;
		sigaction(SIGUSR1, &usr_action, NULL);


	 pidCProc = fork();

	 switch(pidCProc)
	 {
		 case 0:
			 /** Process C */

			 processC(pShrMemObj);

			 _exit(0);
			 break;
		 case -1:
			 std::cerr << "FORK error!\n";
			 break;
		 default:
		 {
			int readBlen = 0;
			int readVal = 0;

			 while(!terminateProcB)
			 {
				  readBlen = read(readPD, &readVal, sizeof(readVal));
//				  std::cout << "Read " << readBlen << " bytes: "<< readVal << std::endl;

				  if(pShrMemObj->operationStatus == SHR_MEM_TAKEN ||
						  pShrMemObj->operationStatus == SHR_MEM_NOT_READY)
				  {
					  long long sqr = (long long) readVal;
					  sqr *= sqr;
					  pShrMemObj->sqrVal = sqr;
					  pShrMemObj->operationStatus = SHR_MEM_FILLED;
//					  std::cout << "Val SENT with ShrMEM " << pShrMemObj->sqrVal << std::endl;
				  }
			 }

			 std::cout << "Got terminating signal\n";

			 if(kill(pidCProc, SIGKILL))
				 std::cerr << "CProc kill Error\n";

			 waitForProcessFinishing(pidCProc);

			munmap(pShrMemObj, SHARED_MEMORY_OBJECT_SIZE+1);	/* close shared memory */
			close(shm);
			shm_unlink(SHARED_MEMORY_OBJECT_NAME);

			kill(getppid(), SIGUSR2);	/* Tell main process about finishing */

			break;
		 }
	 }


	return EXIT_SUCCESS;
}

void * readValFromSharedMem(void *arg)
{
	THREAD_ARG *pThreadArg = (THREAD_ARG *) arg;
	SQR_SHR_MEM_OBJ *pShrMemObj = pThreadArg->pShrMem;

	while(1)
	{
		if(pShrMemObj->operationStatus == SHR_MEM_FILLED)
		{
			if(pShrMemObj->sqrVal == 100)
				kill(getppid(), SIGUSR1);

			std::cout << "Value = " << pShrMemObj->sqrVal<<std::endl;
			pShrMemObj->operationStatus = SHR_MEM_TAKEN;
		}
		sleep(1);	/* for low CPU load, without it this thread catch one CPU thread full */
	}

	return NULL;
}

void * showThatImAlive(void *arg)
{
	while(1)
	{
		std::cout << "I'm alive!\n";
		sleep(1);
	}

	return NULL;
}

int processC(SQR_SHR_MEM_OBJ *pShrMem)
{
	int id2, status;
	pthread_t thread1, thread2;
	THREAD_ARG thread1Arg;

	thread1Arg.id = 1;
	thread1Arg.pShrMem = pShrMem;

	std::cout << "Process C started"<<std::endl;

	status = pthread_create(&thread1, NULL, readValFromSharedMem, &thread1Arg);
	if (status != 0)
	{
		std::cerr << "Creating First thread Err\n";
		return EXIT_FAILURE;
	}

	id2 = 5;
	status = pthread_create(&thread2, NULL, showThatImAlive, &id2);
	if (status != 0)
	{
		std::cerr << "Creating Second thread Err\n";
		return EXIT_FAILURE;
	}

	status = pthread_join(thread1, NULL);
	if (status != 0)
	{
		std::cerr << "Wait for 1st thread\n";
		return EXIT_FAILURE;
	}
	else
	{
		std::cout << "1st thread fin\n";
	}

	status = pthread_join(thread2, NULL);
	if (status != 0)
	{
		std::cerr << "Wait for 2st thread\n";
		return EXIT_FAILURE;
	}
	else
	{
		std::cout << "2nd thread fin\n";
	}

	return EXIT_SUCCESS;
}

int waitForProcessFinishing(pid_t pid)
{
	 int wstatus;

	 do
	 {
		pid_t w = waitpid(pid, &wstatus, WUNTRACED | WCONTINUED);
		if (w == -1) {
			std::cerr << "waitpid\n";
			exit(EXIT_FAILURE);
		}

		if (WIFEXITED(wstatus)) {
			std::cout << "exited, status= " << WEXITSTATUS(wstatus) << std::endl;
		} else if (WIFSIGNALED(wstatus)) {
			std::cout << "killed by signal "<< WTERMSIG(wstatus) <<  std::endl;
		} else if (WIFSTOPPED(wstatus)) {
			std::cout << "stopped by signal "<< WSTOPSIG(wstatus) << std::endl;
		} else if (WIFCONTINUED(wstatus)) {
			std::cout << "continued\n";
		}
	} while (!WIFEXITED(wstatus) && !WIFSIGNALED(wstatus));

	 return wstatus;
}

int main()
{
	int file_pipes[2];

	if (pipe(file_pipes) == 0)
	{
		pid_t pidBProc = -1;

		struct sigaction usr_action;
		sigset_t block_mask;
		/* Establish the signal handler for halting proc A */
		sigfillset(&block_mask);
		usr_action.sa_handler = terminateMainProcHandl;
		usr_action.sa_mask = block_mask;
		usr_action.sa_flags = 0;
		sigaction(SIGUSR2, &usr_action, NULL);

		pidBProc = fork();

		switch(pidBProc)
		{
			case 0:	/* child */
			{
				/** Process B */

				close(file_pipes[1]);	/* close unused write end of pipe */

				processB(file_pipes[0]);	/* reads pipe, create process C*/

				close(file_pipes[0]);	/* close before finish */

				break;
			}
			case -1:	/* error fork */
			{
				std::cout << "Fork ERROR\n";
				break;
			}
			default:
			{
				/** Process A */

				close(file_pipes[0]);	/* close unused read end of pipe */

				processA(file_pipes[1]);	/* Reads stdI and writes pipe */

				waitForProcessFinishing(pidBProc);

				close(file_pipes[1]);	/* close before finish */

				break;
			}
		}
	 exit(EXIT_SUCCESS);
	}

 exit(EXIT_FAILURE);
}
