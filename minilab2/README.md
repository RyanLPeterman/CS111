#Scheduler and Synchronization Lab

Name: Ryan Peterman
UCLA ID: 704269982

Specification is in this project folder

-------------------------------------------------------------------------------
##Response to Exercise 1:

	The name of the scheduling algorithm is: Round Robin
	This is because of the following lines of code:
		
		// this schedules the next one in a circular way (round robin)
		pid = (pid + 1) % NPROCS;

		// only runs if it is in the runnable state
		if (proc_array[pid].p_state == P_RUNNABLE)
			run(&proc_array[pid]);

	However, this is not truly round robin since this is not preemptive scheduling with
	a predefined timeslice. In this scheduler we must wait for the process to yield or exit.

-------------------------------------------------------------------------------
##Response to Exercise 2 (Coding Exercise):

	- schedos-kern.c - Added Strict Priority scheduler to schedule based on pid

-------------------------------------------------------------------------------
##Response to Exercise 3:

	Average turnaround time for scheduling_algorithm 0: 1278.5 milliseconds

		Proc 1 = 1277
		Proc 2 = 1278
		Proc 3 = 1279
		Proc 4 = 1280
		Total Turnaround Time = 5114 milliseconds

	Average turnaround time for scheduling_algorithm 1: 800 milliseconds

		Proc 1 = 320
		Proc 2 = 640
		Proc 3 = 960
		Proc 4 = 1280
		Total Turnaround Time = 3200 milliseconds

	Average wait time for scheduling_algorithm 0: 1.5 milliseconds

		Proc 1 = 0
		Proc 2 = 1
		Proc 3 = 2
		Proc 4 = 3
		Total Wait = 6 milliseconds

	Average wait time for scheduling_algorithm 1: 480 milliseconds

		Proc 1 = 0
		Proc 2 = 320
		Proc 3 = 640
		Proc 4 = 960
		Total Wait Time = 1920 milliseconds

-------------------------------------------------------------------------------
##Response to Exercise 4:

	4A Changes: 

		- schedos-kern.c - Added SET_PRIORITY case and filled out interrupt handler for INT_SYS_PRIORITY
		- schedos-app.h - Added a system call sys_priority
		- schedos-1.c - Used in system call in applications program 
		- schedos-kern.h - Added p_priority field to process descriptor

	Pseudocode:

		while there are processes to run
			find the highest priority process and save that priority

			increment the pid to keep switching between pid of same number

			if a process is runnable and it has the priority that we know is currently the largest
				run it

		Note: if you set priority in the process's code, then for the first 4 characters,
		the processes will run as round robin since they initially have the same priority at 0.

	4B Changes:

		- schedos-kern.c - Added PROPORTIONAL_SHARE case and filled out interrupt handler for INT_SYS_SHARE
		- schedos-app.h - Added a system call sys_share
		- schedoes-1.c - Used in system call in applications program 
		- schedos-kern.h - Added p_share field and p_run_count to keep track of share info


	Pseudocode:

		while there are processes to run
			if the current process is runnable
				if it has run as many times as its share
					set its run times to 0 so when we loop back we will run it again
				else
					run it and increment the run count

			at this portion of the code the current process used up its share therefore we 
			increment the pid to switch between processes

-------------------------------------------------------------------------------
##Response to Exercise 5:

	In the specification's picture, the interupt happens in the 4th process since the 20th 
	character should be a 4 however the scheduler preempted the process before it could print 
	out a 4 and therefore instead process 1 printed the 20th character (round robin)

-------------------------------------------------------------------------------
##Response to Exercise 6 (coding exercise):

Added new system call that atomically (no interrupts in kernel) prints a character to console

	- schedos.h - Added INT_SYS_PRINT 52
	- schedos-app.h - Added sys_print system call
	- schedos-kern.c - Filled out interrupt handler for system call
	- schedoes-1.c - Uses system call to print char instead of directly printing it

-------------------------------------------------------------------------------
##Reponse to Exercise 7:	

	- schedos-kern.c - Added a pseudorandom number generator that I
		found on google
		- in start we assign each process a random number of tickets then loop initialize a ticket 
		array with the pid's of the ticket holders
		- then in the scheduling algorithm we choose a random ticket
		- if the ticket holder is runnable then run it
		- check when we are done to break from the infinite loop

	- schedos-kern.h - Added p_num_tickets to process descriptor

-------------------------------------------------------------------------------
##Response to Exercise 8: 	
	- schedos.h - Added writelock external variable for use as a lock
	- schedos-symbols.ld - Added location of write lock into shared data section of memory
	- schedos-1.c - Calls atomic swap to lock down writing and incrementing curpos