#ifndef WEENSYOS_SCHEDOS_APP_H
#define WEENSYOS_SCHEDOS_APP_H
#include "schedos.h"

/*****************************************************************************
 * schedos-app.h
 *
 *   System call functions and constants used by SchedOS applications.
 *
 *****************************************************************************/


// The number of times each application should run
#define RUNCOUNT	320


/*****************************************************************************
 * sys_yield
 *
 *   Yield control of the CPU to the kernel, which will pick another
 *   process to run.  (It might run this process again, depending on the
 *   scheduling policy.)
 *
 *****************************************************************************/

static inline void
sys_yield(void)
{
	// We call a system call by causing an interrupt with the 'int'
	// instruction.  In weensyos, the type of system call is indicated
	// by the interrupt number -- here, INT_SYS_YIELD.
	asm volatile("int %0\n"
		     : : "i" (INT_SYS_YIELD)
		     : "cc", "memory");
}


/*****************************************************************************
 * sys_exit(status)
 *
 *   Exit the current process with exit status 'status'.
 *
 *****************************************************************************/

static inline void sys_exit(int status) __attribute__ ((noreturn));
static inline void
sys_exit(int status)
{
	// Different system call, different interrupt number (INT_SYS_EXIT).
	// This time, we also pass an argument to the system call.
	// We do this by loading the argument into a known register; then
	// the kernel can look up that register value to read the argument.
	// Here, the status is loaded into register %eax.
	// You can load other registers with similar syntax; specifically:
	//	"a" = %eax, "b" = %ebx, "c" = %ecx, "d" = %edx,
	//	"S" = %esi, "D" = %edi.
	asm volatile("int %0\n"
		     : : "i" (INT_SYS_EXIT),
		         "a" (status)
		     : "cc", "memory");
    loop: goto loop; // Convince GCC that function truly does not return.
}




/*****************************************************************************
 * sys_priority(unsigned int priority)
 *
 *  Calls Interrupt with value INT_SYS_PRIORITY in intno of the calling process.
 *	Puts the value passed into the system call in the eax register for the 
 *	interrupt handler to deal with.
 *
 *****************************************************************************/
static inline void
sys_priority(unsigned int priority)
{
	asm volatile("int %0\n"
		     : : "i" (INT_SYS_PRIORITY),
		     	 "a" (priority)
		     : "cc", "memory");
}


/*****************************************************************************
 * sys_share(unsigned int share)
 *
 *  Calls Interrupt with value INT_SYS_SHARE in intno of the calling process.
 *	Puts the value passed into the system call in the eax register for the 
 *	interrupt handler to deal with.
 *
 *****************************************************************************/
static inline void
sys_share(unsigned int share)
{
	asm volatile("int %0\n"
		     : : "i" (INT_SYS_SHARE),
		     	 "a" (share)
		     : "cc", "memory");
}

/*****************************************************************************
 * sys_printchar(unsigned int char)
 *
 *  Calls Interrupt with value INT_SYS_SHARE in intno of the calling process.
 *	Puts the value passed into the system call in the eax register for the 
 *	interrupt handler to deal with.
 *
 *****************************************************************************/
static inline void
sys_printchar(unsigned int to_print)
{
	asm volatile("int %0\n"
		     : : "i" (INT_SYS_PRINT),
		     	 "a" (to_print)
		     : "cc", "memory");
}

#endif