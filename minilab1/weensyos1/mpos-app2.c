#include "mpos-app.h"
#include "lib.h"

/*****************************************************************************
 * mpos-app2
 *
 *   This application as 1024 new processes and then waits for them to
 *   exit.  All processes print messages to the screen.
 *
 *****************************************************************************/

volatile int counter;

void run_child(void);

void
start(void)
{
	pid_t p;
	int status;

	counter = 0;

	while (counter < 1025) {
		int n_started = 0;

		// Start as many processes as possible, until we fail to start
		// a process or we have started 1025 processes total.
		while (counter + n_started < 1025) {
			p = sys_fork();
			// For debugging
			// app_printf("FIRST LOOP: %d \n", p);
			if (p == 0)
				run_child();
			else if (p > 0)
				n_started++;
			else
				break;
		}

		// If we could not start any new processes, give up!
		if (n_started == 0) {	
			break;
		}

		// We started at least one process, but then could not start
		// any more.
		// That means we ran out of room to start processes.
		// Retrieve old processes' exit status with sys_wait(),
		// to make room for new processes.
		for (p = 2; p < NPROCS; p++) {
			// For debugging:
			// app_printf("SECOND LOOP: %d \n", p);
			(void) sys_wait(p);
		}

	}

	sys_exit(0);
}

void
run_child(void)
{
	int input_counter = counter;
	pid_t pid = sys_getpid();

	counter++;		/* Note that all "processes" share an address
				   space, so this change to 'counter' will be
				   visible to all processes. */

	app_printf("Process %d lives, counter %d!\n",
		   pid, input_counter);

	// if the pid number is even #, kill the thread one number above it
	//if(pid%2 == 0) {
	//	sys_kill(pid + 1);
	//}

	sys_exit(input_counter);
}
