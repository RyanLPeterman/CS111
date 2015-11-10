#include <linux/version.h>
#include <linux/autoconf.h>
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/init.h>

#include <linux/sched.h>
#include <linux/kernel.h>  /* printk() */
#include <linux/errno.h>   /* error codes */
#include <linux/types.h>   /* size_t */
#include <linux/vmalloc.h>
#include <linux/genhd.h>
#include <linux/blkdev.h>
#include <linux/wait.h>
#include <linux/file.h>

#include <stdbool.h> 	   /* for bools */
#include "spinlock.h"
#include "osprd.h"

MODULE_LICENSE("Dual BSD/GPL");
MODULE_DESCRIPTION("CS 111 RAM Disk");
MODULE_AUTHOR("Ryan Peterman");

/* eprintk() prints messages to the console.
 * (If working on a real Linux machine, change KERN_NOTICE to KERN_ALERT or
 * KERN_EMERG so that you are sure to see the messages.  By default, the
 * kernel does not print all messages to the console.  Levels like KERN_ALERT
 * and KERN_EMERG will make sure that you will see messages.) */
#define eprintk(format, ...) printk(KERN_NOTICE format, ## __VA_ARGS__)

#define SECTOR_SIZE	512					// Size of an OSPRD Sector
#define F_OSPRD_LOCKED	0x80000  		// Flag to indicate that file is locked 
#define OSPRD_MAJOR	222					// Total Number of possible processes
#define NOSPRD 4 						// Total Number of possible devices

/* This module parameter controls how big the disk will be.
 * You can specify module parameters when you load the module,
 * as an argument to insmod: "insmod osprd.ko nsectors=4096" */
static int nsectors = 32;
module_param(nsectors, int, 0);

/* The internal representation of our device. */
typedef struct osprd_info {

	uint8_t *data;					// The data array. Its size is (nsectors * SECTOR_SIZE) bytes.
	osp_spinlock_t mutex;			// Mutex for synchronizing access to this block device
	unsigned ticket_head;			// Currently running ticket for the device lock
	unsigned ticket_tail;			// Next available ticket for the device lock
	wait_queue_head_t blockq;		// Wait queue for tasks blocked on the device lock

	// Added Fields
	unsigned num_read_locks;		// Contains number of read locks (can have mult for read)
	pid_t read_pids[OSPRD_MAJOR];	// Array containing all PID's of read lock holders
	bool is_write_lock;				// True if there is a write lock (bool since only one can write)
	pid_t write_pid;				// contains the one PID of the current write lock holder

	// The following elements are used internally; you don't need
	// to understand them.
	struct request_queue *queue;	// The device request queue.
	spinlock_t qlock;				// Used internally for mutual exclusion in the 'queue'.
	struct gendisk *gd;				// The generic disk.

} osprd_info_t;

// array containing devices
static osprd_info_t osprds[NOSPRD];

/*
 * file2osprd(filp)
 *   Given an open file, check whether that file corresponds to an OSP ramdisk.
 *   If so, return a pointer to the ramdisk's osprd_info_t.
 *   If not, return NULL.
 */
static osprd_info_t *file2osprd(struct file *filp);

/*
 * for_each_open_file(task, callback, user_data)
 *   Given a task, call the function 'callback' once for each of 'task's open
 *   files.  'callback' is called as 'callback(filp, user_data)'; 'filp' is
 *   the open file, and 'user_data' is copied from for_each_open_file's third
 *   argument.
 */
static void for_each_open_file(struct task_struct *task,
			       void (*callback)(struct file *filp,
						osprd_info_t *user_data),
			       osprd_info_t *user_data);

/*
 * osprd_process_request(d, req)
 *   Called when the user reads or writes a sector.
 *   Should perform the read or write, as appropriate.
 */
static void osprd_process_request(osprd_info_t *d, struct request *req)
{
	// keeps track of where and how much
	size_t offset, size;

	if (!blk_fs_request(req)) {
		end_request(req, 0);
		return;
	}

	// sector error checking
	if(req->sector < 0 || req->sector >= nsectors) {
		eprintk("Error: Invalid Sector requested (%lu)", (unsigned long) req->sector);
		end_request(req, 0);
	}

	// set the offset and amount we wish to use
	offset = req->sector * SECTOR_SIZE;
	size = req->current_nr_sectors * SECTOR_SIZE;

	// lock before accessing resources
	osp_spin_lock(&(d->mutex));

	// critical section of code
	// read request
	if(rq_data_dir(req) == READ) 
		memcpy(req->buffer, d->data + offset, size);
	// write request
	else if (rq_data_dir(req) == WRITE)
		memcpy(d->data + offset, req->buffer, size);

	// unlock mutex after critical code
	osp_spin_unlock(&(d->mutex));

	end_request(req, 1);
}

// This function is called when a /dev/osprdX file is opened.
// You aren't likely to need to change this.
static int osprd_open(struct inode *inode, struct file *filp)
{
	// Always set the O_SYNC flag. That way, we will get writes immediately
	// instead of waiting for them to get through write-back caches.
	filp->f_flags |= O_SYNC;
	return 0;
}

// This function is called when a /dev/osprdX file is finally closed.
// (If the file descriptor was dup2ed, this function is called only when the
// last copy is closed.)
static int osprd_close_last(struct inode *inode, struct file *filp)
{
	// if there is a file ptr
	if (filp) {

		int i; // for iter
		osprd_info_t *d = file2osprd(filp);

		// set flags based on bitwise & with constants
		bool is_opened_for_writing = ((filp->f_mode & FMODE_WRITE) != 0);
		bool is_locked = ((filp->f_flags & F_OSPRD_LOCKED) != 0);
		
		// obtain lock
		osp_spin_lock(&d->mutex);

		// if file is locked
		if (is_locked) {

			if (is_opened_for_writing) {
				// release lock
				d->is_write_lock = false;
				d->write_pid = -1;
			}
			// otherwise opened for reading
			else {
				// release one of the read locks
				d->num_read_locks--;

				// loop through all the available read processes
				for (i = 0; i < OSPRD_MAJOR; i++) {

					// release the current pid's lock
					if (d->read_pids[i] == current->pid) {
						d->read_pids[i] = -1;
						break;
					}
				}
			}
		}

		// bitwise xor with flags to unlock
		filp->f_flags ^= F_OSPRD_LOCKED;

		// release lock
		osp_spin_unlock(&d->mutex);

		// wakes up any blocked processes
		wake_up_all(&d->blockq);
	}

	return 0;
}


/*
 * osprd_ioctl(inode, filp, cmd, arg)
 *   Called to perform an ioctl on the named file.
 */
int osprd_ioctl(struct inode *inode, struct file *filp,
		unsigned int cmd, unsigned long arg)
{
	osprd_info_t *d = file2osprd(filp);

	int ret_val = 0;		// return value
	unsigned local_ticket;  // will store the current ticket we are servicing
	int i; 					// for iter

	// check the mode of the filp pointer
	bool is_opened_for_writing = (filp->f_mode & FMODE_WRITE) != 0;

	// Lock the ramdisk
	if (cmd == OSPRDIOCACQUIRE) {

		// 'ticket_tail' is the next ticket 
		// 'ticket_head' is the ticket currently being served. 
		// this allows us to service the requests in order of blocking

		// deadlock cases: check if process attempting to read/write to a device it holds

		// write deadlock case
		if (d->write_pid == current->pid)
			return -EDEADLK;
		// read deadlock case
		for (i = 0; i < OSPRD_MAJOR; i++) {
				if (d->read_pids[i] == current->pid)
					return -EDEADLK;
		}

		osp_spin_lock(&d->mutex);

		// store ticket to serve
		local_ticket = d->ticket_head;
		// increment ticket to be served
		d->ticket_head++;

		osp_spin_unlock(&d->mutex);

		// if file was open for file
		if (is_opened_for_writing) {
			// wait for read/write locks
			ret_val = wait_event_interruptible(d->blockq, d->ticket_tail == local_ticket && d->num_read_locks == 0 && !(d->is_write_lock));

			// if interrupted by signal
			if (ret_val == -ERESTARTSYS) {
				// set the ticket tail higher so it is not local_ticket
				if (local_ticket == d->ticket_tail)
					d->ticket_tail++;
				// otherwise set ticket_head back so we can service it again
				else
					d->ticket_head--;

				return ret_val;
			}

			// write lock acquired
			osp_spin_lock(&d->mutex);
			d->is_write_lock = true;
			// add current process to write lock holder
			d->write_pid = current->pid;
		}
		// file is opened for reading
		else {
			// wait for write lock
			ret_val = wait_event_interruptible(d->blockq, d->ticket_tail >= local_ticket && !(d->is_write_lock));

			// if interrupted by signal
			if (ret_val == -ERESTARTSYS) {
				// set the ticket tail higher so it is not local_ticket
				if (local_ticket == d->ticket_tail)
					d->ticket_tail++;
				// set ticket_head back so we can service it again
				else
					d->ticket_head--;

				return ret_val;
			}

			// read lock acquired
			osp_spin_lock(&d->mutex);
			d->num_read_locks++;
			
			// add current process to read lock holders
			for (i = 0; i < OSPRD_MAJOR; i++) {
				// look for available space in read_pids
				if (d->read_pids[i] == -1) {
					d->read_pids[i] = current->pid;
					break;
				}
			}
		}

		// process acquired a lock
		d->ticket_tail++;
		filp->f_flags |= F_OSPRD_LOCKED;

		osp_spin_unlock(&d->mutex);

		// correctly able to grant lock
		return 0;
	} 
	// Attempt to lock RAM Disk
	else if (cmd == OSPRDIOCTRYACQUIRE) {
	
		if (is_opened_for_writing) {
			// check for any read or write locks
			if (d->num_read_locks > 0 || d->is_write_lock)
				return -EBUSY;

			// acquire a write lock
			osp_spin_lock(&d->mutex);
			d->is_write_lock = true;

			// set the write lock holder to the current process
			d->write_pid = current->pid;
		}
		// otherwise its opened for reading
		else {
			// only check for write lock since multiple processes can read same file
			if (d->is_write_lock)
				return -EBUSY;

			// read lock acquired
			osp_spin_lock(&d->mutex);
			d->num_read_locks++;

			// add current process to read lock holders
			for (i = 0; i < OSPRD_MAJOR; i++) {
				// search for available space in read_pids
				if (d->read_pids[i] == -1) {
					d->read_pids[i] = current->pid;
					break;
				}
			}
		}

		// process acquired a lock
		d->ticket_head++;
		d->ticket_tail++;
		filp->f_flags |= F_OSPRD_LOCKED;
		osp_spin_unlock(&d->mutex);

		// lock granted successfully
		return 0;
	} 
	// Unlock the ramdisk
	else if (cmd == OSPRDIOCRELEASE) {

		// check flags to see if file is locked 
		bool is_locked = filp->f_flags & F_OSPRD_LOCKED;
		if (!is_locked)
			return -EINVAL;

		osp_spin_lock(&d->mutex);

		if (is_opened_for_writing) {
			// release write lock
			d->is_write_lock = false;
			d->write_pid = -1;
		}
		// otherwise opened for reading
		else {
			d->num_read_locks--;
			for (i = 0; i < OSPRD_MAJOR; i++) {
				// search for and release pid of current 
				if (d->read_pids[i] == current->pid) {
					d->read_pids[i] = -1;
					break;
				}
			}
		}

		// alter flags with xor to release lock
		filp->f_flags ^= F_OSPRD_LOCKED;
		osp_spin_unlock(&d->mutex);

		// wake up any blocked processes
		wake_up_all(&d->blockq);

		// successfully released ramdisk
		return 0;

	} else
		ret_val = -ENOTTY; // unknown command

	return ret_val;
}

// Initialize internal fields for an osprd_info_t.
static void osprd_setup(osprd_info_t *d)
{
	/* Initialize the wait queue. */
	init_waitqueue_head(&d->blockq);
	osp_spin_lock_init(&d->mutex);
	d->ticket_head = d->ticket_tail = 0;

	/* Added Code to initialize additional fields */
	d->num_read_locks = 0;
	d->is_write_lock = false;
}

/*****************************************************************************/
/*         THERE IS NO NEED TO UNDERSTAND ANY CODE BELOW THIS LINE!          */
/*                                                                           */
/*****************************************************************************/

// Process a list of requests for a osprd_info_t.
// Calls osprd_process_request for each element of the queue.
static void osprd_process_request_queue(request_queue_t *q)
{
	osprd_info_t *d = (osprd_info_t *) q->queuedata;
	struct request *req;

	while ((req = elv_next_request(q)) != NULL)
		osprd_process_request(d, req);
}

// Some particularly horrible stuff to get around some Linux issues:
// the Linux block device interface doesn't let a block device find out
// which file has been closed.  We need this information.

static struct file_operations osprd_blk_fops;
static int (*blkdev_release)(struct inode *, struct file *);

static int _osprd_release(struct inode *inode, struct file *filp)
{
	if (file2osprd(filp))
		osprd_close_last(inode, filp);
	return (*blkdev_release)(inode, filp);
}

static int _osprd_open(struct inode *inode, struct file *filp)
{
	if (!osprd_blk_fops.open) {
		memcpy(&osprd_blk_fops, filp->f_op, sizeof(osprd_blk_fops));
		blkdev_release = osprd_blk_fops.release;
		osprd_blk_fops.release = _osprd_release;
	}
	filp->f_op = &osprd_blk_fops;
	return osprd_open(inode, filp);
}

// The device operations structure.
static struct block_device_operations osprd_ops = {
	.owner = THIS_MODULE,
	.open = _osprd_open,
	// .release = osprd_release, // we must call our own release
	.ioctl = osprd_ioctl
};

// Given an open file, check whether that file corresponds to an OSP ramdisk.
// If so, return a pointer to the ramdisk's osprd_info_t.
// If not, return NULL.
static osprd_info_t *file2osprd(struct file *filp)
{
	if (filp) {
		struct inode *ino = filp->f_dentry->d_inode;
		if (ino->i_bdev
		    && ino->i_bdev->bd_disk
		    && ino->i_bdev->bd_disk->major == OSPRD_MAJOR
		    && ino->i_bdev->bd_disk->fops == &osprd_ops)
			return (osprd_info_t *) ino->i_bdev->bd_disk->private_data;
	}
	return NULL;
}

// Call the function 'callback' with data 'user_data' for each of 'task's
// open files.
static void for_each_open_file(struct task_struct *task,
		  void (*callback)(struct file *filp, osprd_info_t *user_data),
		  osprd_info_t *user_data)
{
	int fd;
	task_lock(task);
	spin_lock(&task->files->file_lock);
	{
#if LINUX_VERSION_CODE <= KERNEL_VERSION(2, 6, 13)
		struct files_struct *f = task->files;
#else
		struct fdtable *f = task->files->fdt;
#endif
		for (fd = 0; fd < f->max_fds; fd++)
			if (f->fd[fd])
				(*callback)(f->fd[fd], user_data);
	}
	spin_unlock(&task->files->file_lock);
	task_unlock(task);
}

// Destroy a osprd_info_t.
static void cleanup_device(osprd_info_t *d)
{
	wake_up_all(&d->blockq);
	if (d->gd) {
		del_gendisk(d->gd);
		put_disk(d->gd);
	}
	if (d->queue)
		blk_cleanup_queue(d->queue);
	if (d->data)
		vfree(d->data);
}

// Initialize a osprd_info_t.
static int setup_device(osprd_info_t *d, int which)
{
	memset(d, 0, sizeof(osprd_info_t));

	/* Get memory to store the actual block data. */
	if (!(d->data = vmalloc(nsectors * SECTOR_SIZE)))
		return -1;
	memset(d->data, 0, nsectors * SECTOR_SIZE);

	/* Set up the I/O queue. */
	spin_lock_init(&d->qlock);
	if (!(d->queue = blk_init_queue(osprd_process_request_queue, &d->qlock)))
		return -1;
	blk_queue_hardsect_size(d->queue, SECTOR_SIZE);
	d->queue->queuedata = d;

	/* The gendisk structure. */
	if (!(d->gd = alloc_disk(1)))
		return -1;
	d->gd->major = OSPRD_MAJOR;
	d->gd->first_minor = which;
	d->gd->fops = &osprd_ops;
	d->gd->queue = d->queue;
	d->gd->private_data = d;
	snprintf(d->gd->disk_name, 32, "osprd%c", which + 'a');
	set_capacity(d->gd, nsectors);
	add_disk(d->gd);

	/* Call the setup function. */
	osprd_setup(d);

	return 0;
}

static void osprd_exit(void);

// The kernel calls this function when the module is loaded.
// It initializes the 4 osprd block devices.
static int __init osprd_init(void)
{
	int i, r;

	// shut up the compiler
	(void) for_each_open_file;
#ifndef osp_spin_lock
	(void) osp_spin_lock;
	(void) osp_spin_unlock;
#endif

	/* Register the block device name. */
	if (register_blkdev(OSPRD_MAJOR, "osprd") < 0) {
		printk(KERN_WARNING "osprd: unable to get major number\n");
		return -EBUSY;
	}

	/* Initialize the device structures. */
	for (i = r = 0; i < NOSPRD; i++)
		if (setup_device(&osprds[i], i) < 0)
			r = -EINVAL;

	if (r < 0) {
		printk(KERN_EMERG "osprd: can't set up device structures\n");
		osprd_exit();
		return -EBUSY;
	} else
		return 0;
}

// The kernel calls this function to unload the osprd module.
// It destroys the osprd devices.
static void osprd_exit(void)
{
	int i;
	for (i = 0; i < NOSPRD; i++)
		cleanup_device(&osprds[i]);
	unregister_blkdev(OSPRD_MAJOR, "osprd");
}


// Tell Linux to call those functions at init and exit time.
module_init(osprd_init);
module_exit(osprd_exit);