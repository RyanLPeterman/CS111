#ifndef OSP_SPINLOCK_H
#define OSP_SPINLOCK_H

#define CONFIG_OSP_SPINLOCK !(defined(CONFIG_SMP) || defined(CONFIG_PREEMPT))

#if CONFIG_OSP_SPINLOCK

#include <linux/kernel.h>	/* printk() */

typedef struct osp_spinlock {
	int lock;
} osp_spinlock_t;

static void osp_spin_lock_init(osp_spinlock_t *lock)
{
	lock->lock = 0;
}

static void osp_spin_lock(osp_spinlock_t *lock)
{
	if (lock->lock-- < 0)
	{
		printk(KERN_EMERG "spin_lock() on a locked lock! Run \"dmesg\" to see a stack trace.\n");
		dump_stack();
		if (current)
		{
			printk(KERN_EMERG "Killing your process because it would have deadlocked!\n");
			send_sig(SIGKILL, current, 0);
		}
	}
}

static void osp_spin_unlock(osp_spinlock_t *lock)
{
	if (++lock->lock > 0)
	{
		printk(KERN_EMERG "spin_unlock() on an unlocked lock!\n");
		dump_stack();
	}
}

#else

#define osp_spinlock_t		spinlock_t
#define osp_spin_lock_init	spin_lock_init
#define osp_spin_lock		spin_lock
#define osp_spin_unlock		spin_unlock

#endif

#endif /* OSP_SPINLOCK_H */
