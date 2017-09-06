//============================================================================
// Name        : main.cpp
// Author      : Slobodchikov Roman S.
// Version     :
// Copyright   : Your copyright notice
// Description : Working with forks, threads, pipes, shared memory and signals
//  C++, Ansi-style
//============================================================================


#include "processes.h"

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


