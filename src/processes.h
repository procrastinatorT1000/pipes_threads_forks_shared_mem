/*
 * pipe.h
 *
 *  Created on: Sep 6, 2017
 *      Author: roman
 */

#ifndef PROCESSES_H_
#define PROCESSES_H_

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

#define SHARED_MEMORY_OBJECT_NAME "sqr_value_container"
#define SHARED_MEMORY_OBJECT_SIZE sizeof(SQR_SHR_MEM_OBJ)

typedef struct
{
	int operationStatus;	/* SHR_MEM_NOT_READY, SHR_MEM_FILLED or SHR_MEM_TAKEN */
	long long sqrVal;
}SQR_SHR_MEM_OBJ;

void terminateMainProcHandl (int sig);
int waitForProcessFinishing(pid_t pid);
void processA(int writePD);
int processB(int readPD);
int processC(SQR_SHR_MEM_OBJ *pShrMem);

#endif /* PROCESSES_H_ */
