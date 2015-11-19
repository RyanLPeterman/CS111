#Managing Processes Lab

Specification Found in this Folder

-----------------------------------------------------------

##Excercise 1: 
Answer the following question: Say you replaced run(current) 
in the INT_SYS_GETPID clause with schedule(). The process that called 
sys_getpid() will eventually run again, picking  up its execution as if 
sys_getpid() had returned directly. When it does run will the sys_getpid()
call have returned the correct value?

	Yes. When the process eventually runs again, the sys_getpid()
	call will have the correct value in it since the %eax
	register gets changed and stored for the current process
	before the call to schedule() which will then run the process
	again.

##Exercise 2: 
Fill out do_fork() and copy_stack() functions in mpos-kern.c.

	do_fork pseudocode:
		loop through and find empty process descriptor
		if no open process descriptor 
			return -1

		copy over parent process descriptor into new process descriptor
		set pid, state, and the eax register
		copy stack over which also changes esp register

		return pid

	copy_stack pseudocode:
		stack top(dest and src) = bottom of stack + pid * size of a stack
		src stack bottom = stack pointer
		dest stack bottom = dest stack top - the size of src stack
		set dest stack pointer

		copy over contents of each stack using memcpy

##Exercise 3: 
Change the implementation of INT_SYS_WAIT in mpos-kern.c 
to use blocking instead of polling.

	Added a pointer in process declaration which points to
	the process parent process that is waiting for child

	interrupt() pseudocode:
		INT_SYS_WAIT case:

			else
				child waiting process is the parent which called sys_wait
				parent state = P_BLOCKED

		INT_SYS_EXIT case:

			if there is currently a waiting process for this process
				change the waiting process state to P_RUNNABLE
				set the waiting process eax register to what the child returned

##Exercise 4: 
Find and fix this bug (Clean up zombie processes).

	in interrupt:
		case INT_SYS_WAIT:
			if the child is a zombie
				set its state to empty and set parent's eax
				register to the exit status

		case INT_SYS_EXIT:
			if there is a waiting process
				reap the child process and pass exit status to 
				waiting process
				set parent to runnable again
			else become a zombie

##Exercise 5: 
Product a version of state with the following properties:
	1. The code uses only local variables.
	2. In a system with correct process isolation, the code would print "10"
	3. In MiniprocOS, the code would print "11"

	void start(void) {
		int x = 0;

		// volatile pointer
		// so thate compiler doesn't optimize the pointer
		int* volatile ptr_to_x = &x;

		pid_t p = sys_fork();
		if(p == 0) {
			ptr_to_x = &x;
			*ptr_to_x = 1;

			// with process isolation this does not affect parent x
			x = 1;
		}
		else if (p > 0)
			sys_wait(p);
		app_printf("%d", x);
		sys_exit(0);
	}

##Exercise 6: 
A big difference from threads is that we create a new
process by forking. New threads are created in a different way.
Introduce a new system call that creates a new process in a thread-
like way

	pid_t sys_newthread(void (*start_function)(void))pseudo code:
	added INT_SYS_NEWTHREAD in mpos.h
	in mpos-kern.c :
		INT_SYS_NEWTHREAD case:
			calls do_newthread and stores the result in parent eax

		do_newthread() :
			same as fork except don't copy stack and set
			eip to the start_function

	in mpos-app.h :
		added sys_newthread which looks the same as
		fork except we set the eip to start_function


##Exercise 7: 
Introduce a sys_kill(pid) system call by which one
process can make another process exit. Use this system call to alter
mpos-app2.c's run_child() function so that the even-numbered
processes kill off all odd-numbered processes(except thread 1)

added INT_SYS_KILL in mpos.h

	in mpos-kern.c:
		INT_SYS_KILL case:
			kills process pid passed in by setting it to empty
			and reaping its exit status

	in mpos-app.h:
		added sys_kill asm line similar to others except uses
		new INT_SYS_KILL directive

	in mpos-app2.c:
		in run_child:
			if pid is even and pid is not NPROCS
				kill the pid that is one above the current 
				(kills all odd processes)

