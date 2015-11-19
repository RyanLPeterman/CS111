#include "schedos-kern.h"
#include "x86.h"
#include "lib.h"

/*****************************************************************************
 * schedos-kern
 *
 *   This is the schedos's kernel.
 *   It sets up process descriptors for the 4 applications, then runs
 *   them in some schedule.
 *
 *****************************************************************************/

// The program loader loads 4 processes, starting at PROC1_START, allocating
// 1 MB to each process.
// Each process's stack grows down from the top of its memory space.
// (But note that SchedOS processes, like MiniprocOS processes, are not fully
// isolated: any process could modify any part of memory.)

#define NPROCS		5
#define PROC1_START	0x200000
#define PROC_SIZE	0x100000

// +---------+-----------------------+--------+---------------------+---------/
// | Base    | Kernel         Kernel | Shared | App 0         App 0 | App 1
// | Memory  | Code + Data     Stack | Data   | Code + Data   Stack | Code ...
// +---------+-----------------------+--------+---------------------+---------/
// 0x0    0x100000               0x198000 0x200000              0x300000
//
// The program loader puts each application's starting instruction pointer
// at the very top of its stack.
//
// System-wide global variables shared among the kernel and the four
// applications are stored in memory from 0x198000 to 0x200000.  Currently
// there is just one variable there, 'cursorpos', which occupies the four
// bytes of memory 0x198000-0x198003.  You can add more variables by defining
// their addresses in schedos-symbols.ld; make sure they do not overlap!


// A process descriptor for each process.
// Note that proc_array[0] is never used.
// The first application process descriptor is proc_array[1].
static process_t proc_array[NPROCS];

// A pointer to the currently running process.
// This is kept up to date by the run() function, in mpos-x86.c.
process_t *current;

// The preferred scheduling algorithm.
int scheduling_algorithm;

// USE THESE VALUES FOR SETTING THE scheduling_algorithm VARIABLE.
#define ROUND_ROBIN   	 	0  	// the initial algorithm
#define STRICT_PRIORITY	 	2  	// strict priority scheduling (exercise 2)
#define SET_PRIORITY		41  // p_priority algorithm (exercise 4.a)
#define PROPORTIONAL_SHARE 	42  // p_share algorithm (exercise 4.b)
#define LOTTERY   			7  	// Exercise 7: Lottery Scheduler

/*****************************************************************************
 * lottery functions/members
 *
 *   random number generator to assign lottery tickets to the processes
 *
 *****************************************************************************/
// arbitrary max tickets per process
#define MAX_NUM_TICKETS 1000

// random number generator I found on stack overflow
unsigned short seed = 0xBD11;
unsigned bit;

// array to contain pids, will initialize after we know how many tickets
// each process was assigned
pid_t tickets[MAX_NUM_TICKETS * NPROCS] = {0};
unsigned total_num_tickets = 0;

// returns a pseudorandom number from 0 to MAX_INT
unsigned rand_num() {
	bit = ((seed >> 0) ^ (seed >>2) ^ (seed >> 3) ^ (seed >> 5)) & 1;
	return seed = (seed >> 1) | (bit << 15);
}

/*****************************************************************************
 * start
 *
 *   Initialize the hardware and process descriptors, then run
 *   the first process.
 *
 *****************************************************************************/

void
start(void)
{
	int i,j, curr_ticket;

	// Set up hardware (schedos-x86.c)
	segments_init();
	interrupt_controller_init(0);
	console_clear();

	// Initialize process descriptors as empty
	memset(proc_array, 0, sizeof(proc_array));
	for (i = 0; i < NPROCS; i++) {
		proc_array[i].p_pid = i;
		proc_array[i].p_state = P_EMPTY;
	}

	// Set up process descriptors (the proc_array[])
	for (i = 1; i < NPROCS; i++) {
		process_t *proc = &proc_array[i];
		uint32_t stack_ptr = PROC1_START + i * PROC_SIZE;

		// Initialize the process descriptor
		special_registers_init(proc);

		// Set ESP
		proc->p_registers.reg_esp = stack_ptr;

		// Load process and set EIP, based on ELF image
		program_loader(i - 1, &proc->p_registers.reg_eip);

		// Mark the process as runnable!
		proc->p_state = P_RUNNABLE;

		// Added code here:
		// randomly assign number of tickets to proc
		proc->p_num_tickets = (rand_num() % MAX_NUM_TICKETS);
		// increment count of tickets
		total_num_tickets+=proc->p_num_tickets;

		// initialize priority
		proc->p_priority = 0;

		// initialize share and run times
		proc->p_share = 1;
		proc->p_run_count = 0;
	}

	// iterator in tickets
	curr_ticket = 0;

	// for each process
	for(i = 1; i < NPROCS; i++) {
		// for all their tickets
		for(j = 0; j < proc_array[i].p_num_tickets; j++) {
			// set pid for ticketholder
			tickets[curr_ticket] = i;
			// next ticket
			curr_ticket++;
		}
	}

	// Initialize the cursor-position shared variable to point to the
	// console's first character (the upper left).
	cursorpos = (uint16_t *) 0xB8000;

	// Initialize the scheduling algorithm.
	// USE THE FOLLOWING VALUES:
	//    0 = the initial algorithm
	//    2 = strict priority scheduling (exercise 2)
	//   41 = p_priority algorithm (exercise 4.a)
	//   42 = p_share algorithm (exercise 4.b)
	//    7 = lottery algorithm (exercise 8)
	scheduling_algorithm = 7;

	// Switch to the first process.
	run(&proc_array[1]);

	// Should never get here!
	while (1) {
		console_printf(cursorpos, 0x100, "\nWe should never get here!\n");
	}
}



/*****************************************************************************
 * interrupt
 *
 *   This is the weensy interrupt and system call handler.
 *   The current handler handles 4 different system calls (two of which
 *   do nothing), plus the clock interrupt.
 *
 *   Note that we will never receive clock interrupts while in the kernel.
 *
 *****************************************************************************/

void
interrupt(registers_t *reg)
{
	// Save the current process's register state
	// into its process descriptor
	current->p_registers = *reg;

	switch (reg->reg_intno) {

	case INT_SYS_YIELD:
		// The 'sys_yield' system call asks the kernel to schedule
		// the next process.
		schedule();

	case INT_SYS_EXIT:
		// 'sys_exit' exits the current process: it is marked as
		// non-runnable.
		// The application stored its exit status in the %eax register
		// before calling the system call.  The %eax register has
		// changed by now, but we can read the application's value
		// out of the 'reg' argument.
		// (This shows you how to transfer arguments to system calls!)
		current->p_state = P_ZOMBIE;
		current->p_exit_status = reg->reg_eax;
		schedule();

	// set priority then continue process that was running
	case INT_SYS_PRIORITY:
		current->p_priority = reg->reg_eax; 
		run(current);

	// set share then continue process that was running
	case INT_SYS_SHARE:
		current->p_share = reg->reg_eax;
		run(current);

	// print then continue process that was running
	case INT_SYS_PRINT:
		*cursorpos++ = reg->reg_eax;
		run(current);


	case INT_CLOCK:
		// A clock interrupt occurred (so an application exhausted its
		// time quantum).
		// Switch to the next runnable process.
		schedule();


	default:
		while (1) {
			console_printf(cursorpos, 0x100, "\nUnknown System Call/Interrupt\n");
		}
			/* do nothing */;

	}
}



/*****************************************************************************
 * schedule
 *
 *   This is the weensy process scheduler.
 *   It picks a runnable process, then context-switches to that process.
 *   If there are no runnable processes, it spins forever.
 *
 *   This function implements multiple scheduling algorithms, depending on
 *   the value of 'scheduling_algorithm'.  We've provided one; in the problem
 *   set you will provide at least one more.
 *
 *****************************************************************************/
int check_done() {

	pid_t iter;

	// check processes to see if they are all exited
	for(iter = 1; iter < NPROCS;iter++) {
		// still needs to run
		if(proc_array[iter].p_state == P_RUNNABLE || proc_array[iter].p_state == P_BLOCKED) {
			break;
		}
		// we looped through and all processes were not runnable or blocked
		if(iter == NPROCS - 1)
			return 1;
	}
	return 0;
}

void
schedule(void)
{
	pid_t pid = current->p_pid;
	pid_t iter;
	// starts at lowest possible priority
	unsigned int highest_priority = 0xffffffff;
	// bool to hold tell if done
	int is_done = 0;
	// will hold random ticket picked in lottery
	unsigned curr_ticket;
	
	switch (scheduling_algorithm) {
		case ROUND_ROBIN:
			while (!is_done) {
				pid = (pid + 1) % NPROCS;

				// Run the selected process, but skip
				// non-runnable processes.
				// Note that the 'run' function does not return.
				if (proc_array[pid].p_state == P_RUNNABLE)
					run(&proc_array[pid]);

				// check if all processes are done
				is_done = check_done();
			}
			break;

		case STRICT_PRIORITY:
			// consider pid's starting at the highest priority first
			for(pid = 1; pid < NPROCS; pid++) {

				// if its runnable then run it
				if(proc_array[pid].p_state == P_RUNNABLE) {
					run(&proc_array[pid]);
				}
			}
			break;

		case SET_PRIORITY:

			while(!is_done) {

				// Note: a lower number is associated with a higher priority here

				// iterate through processes
				for(iter = 1; iter < NPROCS; iter++) {
					// find a runnable thread with the highest priority
					if((proc_array[iter].p_state == P_RUNNABLE) && (proc_array[iter].p_priority < highest_priority))
						highest_priority = proc_array[iter].p_priority;
				}

				// increment pid so we switch if multiple processes have same highest priority
				pid = (pid + 1) % NPROCS;

				// if pid is runnable and it has the highest priority
				if((proc_array[pid].p_state == P_RUNNABLE) && (proc_array[pid].p_priority == highest_priority))
					run(&proc_array[pid]);

				// check processes to see if they are all done
				is_done = check_done();
			}

		case PROPORTIONAL_SHARE:

			while(!is_done) {

				// Note: High share here means we run it more often

				// if a process is runnable
				if(proc_array[pid].p_state == P_RUNNABLE) {

					// check if we have ran the process as many times as its share
					if(proc_array[pid].p_run_count == proc_array[pid].p_share) {
						proc_array[pid].p_run_count = 0;
					}
					// if we haven't, then run it and increment the counter
					else {
						proc_array[pid].p_run_count++;
						run(&proc_array[pid]);
					}
				}

				// switch to next process after current ran through all its share
				pid = (pid + 1) % NPROCS;

				// check processes to see if they are all done
				is_done = check_done();
			}
			break;

		case LOTTERY:
				while(!is_done) {
					// pick a random ticket from the array we initialized
					curr_ticket = (rand_num() % total_num_tickets);

					// if the process whose ticket we drew is runnable, then run it
					if(proc_array[tickets[curr_ticket]].p_state == P_RUNNABLE)
						run(&proc_array[tickets[curr_ticket]]);

					// check processes to see if they are all done
					is_done = check_done();
				}
			break;

		default:
			// If we get here, we are running an unknown scheduling algorithm.
			cursorpos = console_printf(cursorpos, 0x100, "\nUnknown scheduling algorithm %d\n", scheduling_algorithm);
			while (1); /* do nothing */
	}

	// if it ever gets here then we ran all runnable processes
	console_printf(cursorpos, 0x100, "\nExecution Complete!\n");

	// Jobs done!
	while(1);
}
