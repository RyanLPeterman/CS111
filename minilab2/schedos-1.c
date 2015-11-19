#include "schedos-app.h"
#include "x86sync.h"

/*****************************************************************************
 * schedos-1
 *
 *   This tiny application prints red "1"s to the console.
 *   It yields the CPU to the kernel after each "1" using the sys_yield()
 *   system call.  This lets the kernel (schedos-kern.c) pick another
 *   application to run, if it wants.
 *
 *   The other schedos-* processes simply #include this file after defining
 *   PRINTCHAR appropriately.
 *
 *****************************************************************************/

#ifndef PRINTCHAR
#define PRINTCHAR	('1' | 0x0C00)
#endif

#ifndef PRIORITY
#define PRIORITY 1
#endif 

// comment if you wish to switch locking mechanism
#define __EXERCISE_8__



void
start(void)
{
	int i;

	// set priority and share
	sys_priority(PRIORITY);
	sys_share(PRIORITY);

	for (i = 0; i < RUNCOUNT; i++) {

		// Exercise 6: Make system call to atomically print character
		#ifndef __EXERCISE_8__
		sys_printchar(PRINTCHAR);
		#endif

		// Exercise 8: Alternate Method for locking
		// using x86sync locking mechanism
		#ifdef __EXERCISE_8__
		while(atomic_swap(&write_lock, 1) != 0)
			continue;

		// Write characters to the console, yielding after each one.
		*cursorpos++ = PRINTCHAR;

		atomic_swap(&write_lock, 0);
		#endif

		sys_yield();
	}

	sys_exit(0);

	// Yield forever.
	while (1)
		sys_yield();
}
