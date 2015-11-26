#define _BSD_EXTENSION
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <inttypes.h>
#include <sys/stat.h>
#include <string.h>
#include <assert.h>
#include <errno.h>
#include <sys/types.h>
#include <dirent.h>

#include "ospfs.h"
#include "md5.h"

/****************************************************************************
 * ospfsformat
 *
 *   Creates a OSPFS file system contained in a single file.
 *   The arguments determine the file system's contents.
 *
 ****************************************************************************/

#define nelem(x)	(sizeof(x) / sizeof((x)[0]))

int diskfd;
uint32_t nblocks;
uint32_t ninodes;
uint32_t nbitblock;
uint32_t nextb;
uint32_t nextinode;
int verbose = 0;
int link_contents = 0;

struct Hardlink {
	unsigned long osp_ino;
	uint32_t host_ino;
	unsigned char md5_digest[MD5_DIGEST_SIZE];
	struct Hardlink *next;
};

enum {
	BLOCK_SUPER,
	BLOCK_DIR,
	BLOCK_FILE,
	BLOCK_BITS,
	BLOCK_INODES
};

struct Block {
	uint32_t bno;
	uint32_t type;
	uint32_t busy;
	uint32_t used;
	union {
		uint8_t b[OSPFS_BLKSIZE];
		uint32_t u[OSPFS_BLKSIZE / 4];
		ospfs_inode_t ino[OSPFS_BLKINODES];
	} u;
};

struct Hardlink *hardlinks = NULL;

struct Block cache[16];

struct ospfs_super super;

// Return the osp ino for the given host ino
// Return 0 iff there is no mapping
uint32_t
get_hardlink(unsigned long host_ino, unsigned char *md5_digest)
{
	struct Hardlink *cur;
	for (cur = hardlinks; cur; cur = cur->next)
		if ((host_ino && cur->host_ino == host_ino)
		    || (link_contents && md5_digest
			&& memcmp(cur->md5_digest, md5_digest, MD5_DIGEST_SIZE) == 0))
			return cur->osp_ino;
	return 0;
}

// Add a new host->osp inode mapping to 'hardlinks'
void
add_hardlink(unsigned long host_ino, uint32_t osp_ino, unsigned char *md5_digest)
{
	struct Hardlink *prev_head = hardlinks;
	hardlinks = malloc(sizeof(*hardlinks));
	if (!hardlinks) {
		perror("malloc");
		abort();
	}
	hardlinks->host_ino = host_ino;
	hardlinks->osp_ino = osp_ino;
	if (link_contents && md5_digest)
		memcpy(hardlinks->md5_digest, md5_digest, MD5_DIGEST_SIZE);
	else
		memset(hardlinks->md5_digest, '\0', MD5_DIGEST_SIZE);
	hardlinks->next = prev_head;
}

ssize_t
readn(int f, void *av, size_t n)
{
	uint8_t *a;
	size_t t;

	a = av;
	t = 0;
	while (t < n) {
		size_t m = read(f, a + t, n - t);
		if (m <= 0) {
			if (t == 0)
				return m;
			break;
		}
		t += m;
	}
	return t;
}

// make little-endian
void
swizzle(uint32_t *x)
{
	uint32_t y;
	uint8_t *z;

	z = (uint8_t*) x;
	y = *x;
	z[0] = y & 0xFF;
	z[1] = (y >> 8) & 0xFF;
	z[2] = (y >> 16) & 0xFF;
	z[3] = (y >> 24) & 0xFF;
}

void
swizzleinode(struct ospfs_inode *inode)
{
	int i;

	if (inode->oi_nlink == 0)
		return;
	swizzle(&inode->oi_size);
	swizzle(&inode->oi_ftype);
	swizzle(&inode->oi_mode);
	swizzle(&inode->oi_nlink);
	for (i = 0; i < OSPFS_NDIRECT; i++)
		swizzle(&inode->oi_direct[i]);
	swizzle(&inode->oi_indirect);
	swizzle(&inode->oi_indirect2);
}

void
swizzledirentry(struct ospfs_direntry *direntry)
{
	if (direntry->od_name[0] == 0)
		return;
	swizzle(&direntry->od_ino);
}

void
swizzleblock(struct Block *b)
{
	int i;
	struct ospfs_super *s;
	struct ospfs_direntry *od;

	switch (b->type) {
	case BLOCK_SUPER:
		s = (struct ospfs_super*) &b->u;
		swizzle(&s->os_magic);
		swizzle(&s->os_nblocks);
		swizzle(&s->os_ninodes);
		swizzle(&s->os_firstinob);
		break;
	case BLOCK_DIR:
		for (i = 0; i < OSPFS_BLKSIZE; i += OSPFS_DIRENTRY_SIZE) {
			od = (struct ospfs_direntry*) (b->u.b + i);
			swizzledirentry(od);
		}
		break;
	case BLOCK_BITS:
		for (i = 0; i < OSPFS_BLKSIZE / 4; i++)
			swizzle(&b->u.u[i]);
		break;
	case BLOCK_INODES:
		for (i = 0; i < OSPFS_BLKINODES; i++)
			swizzleinode(&b->u.ino[i]);
		break;
	}
}

void
flushb(struct Block *b)
{
	swizzleblock(b);
	if (lseek(diskfd, b->bno * OSPFS_BLKSIZE, 0) < 0
	    || write(diskfd, &b->u, OSPFS_BLKSIZE) != OSPFS_BLKSIZE) {
		perror("flushb");
		fprintf(stderr, "\n");
		abort();
	}
	swizzleblock(b);
}

struct Block*
getblk(uint32_t bno, int clr, uint32_t type)
{
	int i, least;
	static int t = 1;
	struct Block *b;

	if (bno >= nblocks) {
		fprintf(stderr, "attempt to access past end of disk bno=%d\n", bno);
		abort();
	}

	least = -1;
	for (i = 0; i < nelem(cache); i++) {
		if (cache[i].bno == bno) {
			b = &cache[i];
			goto out;
		}
		if (!cache[i].busy
		    && (least == -1 || cache[i].used < cache[least].used))
			least = i;
	}

	if (least == -1) {
		fprintf(stderr, "panic: block cache full\n");
		abort();
	}

	b = &cache[least];
	if (b->used)
		flushb(b);

	if (lseek(diskfd, bno*OSPFS_BLKSIZE, 0) < 0
	    || readn(diskfd, &b->u, OSPFS_BLKSIZE) != OSPFS_BLKSIZE) {
		fprintf(stderr, "read block %d: ", bno);
		perror("");
		fprintf(stderr, "\n");
		abort();
	}
	b->bno = bno;
	if (!clr)
		swizzleblock(b);
	b->busy = 0;
	b->type = type;

out:
	if (clr)
		memset(&b->u, 0, sizeof(b->u));
	b->used = ++t;
	b->busy++;
	/* it is important to reset b->type in case we reuse a block for a
	 * different purpose while it is still in the cache - this can happen
	 * for example if a file ends exactly on a block boundary */
	assert(b->type == type || b->busy == 1);
	b->type = type;
	return b;
}

void
putblk(struct Block *b)
{
	b->busy--;
}

void
opendisk(const char *name)
{
	int i, r;
	uint32_t ninodeblock;
	struct stat s;
	struct Block *b;
	struct ospfs_inode *oi;

	if ((diskfd = open(name, O_RDWR | O_CREAT, 0666)) < 0) {
		fprintf(stderr, "open %s: ", name);
		perror("");
		fprintf(stderr, "\n");
		abort();
	}

	if ((r = ftruncate(diskfd, 0)) < 0
	    || (r = ftruncate(diskfd, nblocks * OSPFS_BLKSIZE)) < 0) {
		fprintf(stderr, "truncate %s: ", name);
		perror("");
		abort();
	}

	nbitblock = (nblocks + OSPFS_BLKBITSIZE - 1) / OSPFS_BLKBITSIZE;
	for (i = 0; i < nbitblock; i++){
		b = getblk(OSPFS_FREEMAP_BLK + i, 0, BLOCK_BITS);
		memset(&b->u.b, 0xFF, OSPFS_BLKSIZE);
		putblk(b);
	}

	ninodeblock = (ninodes + OSPFS_BLKINODES - 1) / OSPFS_BLKINODES;
	for (i = 0; i < ninodeblock; i++) {
		b = getblk(OSPFS_FREEMAP_BLK + nbitblock + i, 1, BLOCK_INODES);
		putblk(b);
	}

	nextb = OSPFS_FREEMAP_BLK + nbitblock + ninodeblock;
	nextinode = 0;

	super.os_magic = OSPFS_MAGIC;
	super.os_nblocks = nblocks;
	super.os_ninodes = ninodes;
	super.os_firstinob = OSPFS_FREEMAP_BLK + nbitblock;
	if (verbose)
		fprintf(stderr, "superblock, free block bitmap %d, first inode block %d, first data block %d\n", OSPFS_FREEMAP_BLK, super.os_firstinob, nextb);
}

void
storeblk(struct ospfs_inode *ino, struct Block *b, int nblk, int indent)
{
	if (nblk < OSPFS_NDIRECT)
		ino->oi_direct[nblk] = b->bno;
	else if (nblk < OSPFS_NDIRECT + OSPFS_NINDIRECT) {
		struct Block *bindir;
		if (ino->oi_indirect == 0) {
			bindir = getblk(nextb++, 1, BLOCK_BITS);
			ino->oi_indirect = bindir->bno;
			if (verbose)
				fprintf(stderr, "%*sindirect block %d\n", indent, "", nextb - 1);
		} else
			bindir = getblk(ino->oi_indirect, 0, BLOCK_BITS);
		bindir->u.u[nblk - OSPFS_NDIRECT] = b->bno;
		putblk(bindir);
	} else if (nblk < OSPFS_MAXFILEBLKS) {
		struct Block *bindir2;
		struct Block *bindir;
		if (ino->oi_indirect2 == 0) {
			bindir2 = getblk(nextb++, 1, BLOCK_BITS);
			ino->oi_indirect2 = bindir2->bno;
			if (verbose)
				fprintf(stderr, "%*sindirect2 block %d\n", indent, "", nextb - 1);
		} else
			bindir2 = getblk(ino->oi_indirect2, 0, BLOCK_BITS);
		// make nblk an offset from the first blk under indirect2
		nblk -= OSPFS_NDIRECT + OSPFS_NINDIRECT;
		if (bindir2->u.u[nblk / OSPFS_NINDIRECT] == 0) {
			bindir = getblk(nextb++, 1, BLOCK_BITS);
			bindir2->u.u[nblk / OSPFS_NINDIRECT] = bindir->bno;
			if (verbose)
				fprintf(stderr, "%*sindirect2-indirect block %d\n", indent, "", nextb - 1);
		} else
			bindir = getblk(bindir2->u.u[nblk / OSPFS_NINDIRECT], 0, BLOCK_BITS);
		bindir->u.u[nblk % OSPFS_NINDIRECT] = b->bno;
		putblk(bindir);
		putblk(bindir2);
	} else {
		fprintf(stderr, "file too large\n");
		abort();
	}
}

struct ospfs_inode *
allocinode(uint32_t *ino, struct Block **ib)
{
	if (nextinode == ninodes) {
		fprintf(stderr, "not enough inodes (exceeded %u inodes)\n", ninodes);
		abort();
	}

	*ino = nextinode++;
	*ib = getblk(super.os_firstinob + *ino / OSPFS_BLKINODES, 0, BLOCK_INODES);
	return &(*ib)->u.ino[*ino % OSPFS_BLKINODES];
}

struct ospfs_direntry *
allocdirentry(struct ospfs_inode *dirino, const char *name, struct Block **dirb, int indent)
{
	struct ospfs_inode *ino;
	struct ospfs_direntry *od;
	int nblk, i, namelen = strlen(name);

	if (namelen > OSPFS_MAXNAMELEN)
		return 0;

	nblk = (int)((dirino->oi_size + OSPFS_BLKSIZE - 1) / OSPFS_BLKSIZE) - 1;
	if (nblk >= OSPFS_NDIRECT + OSPFS_NINDIRECT) {
		uint32_t nblk_off = nblk - OSPFS_NDIRECT - OSPFS_NINDIRECT;
		struct Block *bindir2 = getblk(dirino->oi_indirect2, 0, BLOCK_BITS);
		struct Block *bindir = getblk(bindir2->u.u[nblk_off / OSPFS_NINDIRECT], 0, BLOCK_BITS);
		*dirb = getblk(bindir->u.u[nblk_off % OSPFS_NINDIRECT], 0, BLOCK_DIR);
		putblk(bindir);
		putblk(bindir2);
	} else if (nblk >= OSPFS_NDIRECT) {
		struct Block *bindir = getblk(dirino->oi_indirect, 0, BLOCK_BITS);
		*dirb = getblk(bindir->u.u[nblk - OSPFS_NDIRECT], 0, BLOCK_DIR);
		putblk(bindir);
	} else if (nblk >= 0)
		*dirb = getblk(dirino->oi_direct[nblk], 0, BLOCK_DIR);
	else
		goto new_dirb;

	for (i = 0; i < OSPFS_BLKSIZE; i += OSPFS_DIRENTRY_SIZE) {
		od = (struct ospfs_direntry *) ((*dirb)->u.b + i);
		if (od->od_ino == 0)
			goto gotit;
	}

	putblk(*dirb);

new_dirb:
	*dirb = getblk(nextb++, 1, BLOCK_DIR);
	od = (struct ospfs_direntry *) (*dirb)->u.b;
	for (i = 0; i < OSPFS_BLKSIZE; i += OSPFS_DIRENTRY_SIZE) {
		od = (struct ospfs_direntry *) ((*dirb)->u.b + i);
		od->od_ino = 0;
	}
	storeblk(dirino, *dirb, ++nblk, indent);
	dirino->oi_size += OSPFS_BLKSIZE;
	assert((nblk + 1) * OSPFS_BLKSIZE == dirino->oi_size);
	
	od = (struct ospfs_direntry *) (*dirb)->u.b;
	
gotit:
	strcpy(od->od_name, name);
	return od;
}

void
writefile(struct ospfs_inode *dirino, const char *name, unsigned long host_ino, int indent, int mode)
{
	int fd;
	const char *last;
	struct ospfs_direntry *de;
	struct ospfs_inode *ino;
	int i, n, nblk, hardlink_ino;
	struct Block *dirb, *inob, *b, *bindir;
	unsigned char md5_digest[MD5_DIGEST_SIZE];

	if ((fd = open(name, O_RDONLY)) < 0) {
		fprintf(stderr, "open %s:", name);
		perror("");
		abort();
	}

	last = strrchr(name, '/');
	if (last)
		last++;
	else
		last = name;

	de = allocdirentry(dirino, last, &dirb, indent);

	if (link_contents) {
		unsigned char buf[BUFSIZ];
		ssize_t r;
		MD5_CONTEXT md5;
		md5_init(&md5);
		while (1) {
			r = read(fd, buf, BUFSIZ);
			if (r < 0 && r == EAGAIN)
				/* do nothing */;
			else if (r == 0)
				break;
			else if (r < 0) {
				perror("read");
				return;
			} else
				md5_update(&md5, buf, r);
		}
		md5_final(md5_digest, &md5);
		if (lseek(fd, 0, SEEK_SET) < 0) {
			perror("seek");
			return;
		}
	}

	if (host_ino || link_contents)
		hardlink_ino = get_hardlink(host_ino, md5_digest);
	else
		hardlink_ino = 0;

	if (!hardlink_ino) {
		ino = allocinode(&de->od_ino, &inob);
		ino->oi_nlink = 1;
		if (host_ino || link_contents)
			add_hardlink(host_ino, de->od_ino, md5_digest);
	} else {
		de->od_ino = hardlink_ino;
		inob = getblk(super.os_firstinob + hardlink_ino / OSPFS_BLKINODES, 0, BLOCK_INODES);
		ino = &inob->u.ino[hardlink_ino % OSPFS_BLKINODES];
		ino->oi_nlink++;

		if (verbose)
			fprintf(stderr, "%*s%s, directory block %d, inode %d [hardlink]\n", indent, "", last, dirb->bno, de->od_ino);
	}

	if (!hardlink_ino) {
		ino->oi_ftype = OSPFS_FTYPE_REG;
		ino->oi_mode = mode;
		if (verbose)
			fprintf(stderr, "%*s%s, directory block %d, inode %d\n", indent, "", last, dirb->bno, de->od_ino);

		n = 0;
		for (nblk = 0; ; nblk++) {
			b = getblk(nextb, 1, BLOCK_FILE);
			n = readn(fd, b->u.b, OSPFS_BLKSIZE);
			if (verbose)
				fprintf(stderr, "%*sdata block %d\n", indent, "", nextb);
			if (n < 0) {
				fprintf(stderr, "reading %s: ", name);
				perror("");
				abort();
			}
			if (n == 0) {
				putblk(b);
				break;
			}
			nextb++;
			storeblk(ino, b, nblk, indent);
			putblk(b);
			if (n < OSPFS_BLKSIZE)
				break;
		}
	
		ino->oi_size = nblk * OSPFS_BLKSIZE + n;
	}

	putblk(dirb);
	putblk(inob);
}

void
addsymlink(struct ospfs_inode *dirino, const char *name, const char *linkbuf, unsigned long host_ino, int indent)
{
	const char *last;
	struct ospfs_direntry *de;
	struct ospfs_symlink_inode *sino;
	int i, n, r, nblk, hardlink_ino;
	struct Block *dirb, *inob;

	last = strrchr(name, '/');
	if (last)
		last++;
	else
		last = name;

	de = allocdirentry(dirino, last, &dirb, indent);

	if (host_ino)
		hardlink_ino = get_hardlink(host_ino, 0);
	else
		hardlink_ino = 0;

	if (!hardlink_ino) {
		sino = (struct ospfs_symlink_inode *) allocinode(&de->od_ino, &inob);
		sino->oi_nlink = 1;
		if (host_ino)
			add_hardlink(host_ino, de->od_ino, 0);
	} else {
		de->od_ino = hardlink_ino;
		inob = getblk(super.os_firstinob + hardlink_ino / OSPFS_BLKINODES, 0, BLOCK_INODES);
		sino = (struct ospfs_symlink_inode *) &inob->u.ino[hardlink_ino % OSPFS_BLKINODES];
		sino->oi_nlink++;

		if (verbose)
			fprintf(stderr, "%*s%s, directory block %d, inode %d [hardlink]\n", indent, "", last, dirb->bno, de->od_ino);
	}

	if (!hardlink_ino) {
		sino->oi_ftype = OSPFS_FTYPE_SYMLINK;
		if (verbose)
			fprintf(stderr, "%*s%s, directory block %d, inode %d\n", indent, "", last, dirb->bno, de->od_ino);

		strcpy(sino->oi_symlink, linkbuf);
		sino->oi_size = strlen(sino->oi_symlink);
	}

	putblk(dirb);
	putblk(inob);
}

void
writesymlink(struct ospfs_inode *dirino, const char *name, unsigned long host_ino, int indent)
{
	char linkbuf[OSPFS_MAXSYMLINKLEN + 1];
	ssize_t linklen;

	if ((linklen = readlink(name, linkbuf, OSPFS_MAXSYMLINKLEN + 1)) == -1) {
		fprintf(stderr, "readlink %s:", name);
		perror("");
		abort();
	} else if (linklen > OSPFS_MAXSYMLINKLEN) {
		fprintf(stderr, "readlink %s: symlink name too long, ignored\n", name);
		return;
	}

	linkbuf[linklen] = '\0';
	addsymlink(dirino, name, linkbuf, host_ino, indent);
}

void
writedirectory(struct ospfs_inode *parentdirino, char *name, int root, int indent, int mode)
{
	struct ospfs_inode *dirino;
	struct ospfs_direntry *dirod;
	int r;
	DIR *dir;
	struct dirent *ent;
	struct stat s;
	char pathbuf[PATH_MAX];
	int namelen;
	struct Block *dirb = NULL, *inob = NULL;

	if ((dir = opendir(name)) == NULL) {
		fprintf(stderr, "open %s:", name);
		perror("");
		abort();
	}

	if (!root) {
		const char *last = strrchr(name, '/');
		if (last)
			last++;
		else
			last = name;

		dirod = allocdirentry(parentdirino, last, &dirb, indent);
		dirino = allocinode(&dirod->od_ino, &inob);
		parentdirino->oi_nlink++;
		dirino->oi_ftype = OSPFS_FTYPE_DIR;
		dirino->oi_size = 0;
		dirino->oi_nlink = 1;
		dirino->oi_mode = mode;

		if (verbose)
			fprintf(stderr, "%*s%s, directory block %d, inode %d\n", indent, "", last, dirb->bno, dirod->od_ino);
	} else
		dirino = parentdirino;

	strcpy(pathbuf, name);
	namelen = strlen(pathbuf);
	if (pathbuf[namelen - 1] != '/') {
		pathbuf[namelen++] = '/';
		pathbuf[namelen] = 0;
	}

	while ((ent = readdir(dir)) != NULL) {
		int ent_namlen = strlen(ent->d_name);
		strcpy(pathbuf + namelen, ent->d_name);

		// don't depend on unreliable parts of the dirent structure
		if (lstat(pathbuf, &s) < 0)
			continue;
		
		if (S_ISREG(s.st_mode)) {
			unsigned long host_ino = (s.st_nlink > 1 ? s.st_ino : 0);
			writefile(dirino, pathbuf, host_ino, indent + 2, s.st_mode & 0777);
		} else if (S_ISDIR(s.st_mode)
			   && (ent_namlen > 1 || ent->d_name[0] != '.')
			   && (ent_namlen > 2 || ent->d_name[0] != '.' || ent->d_name[1] != '.')
			   && (ent_namlen > 3 || ent->d_name[0] != 'C' || ent->d_name[1] != 'V' || ent->d_name[2] != 'S')
			   && (ent_namlen > 4 || ent->d_name[0] != '.' || ent->d_name[1] != 's' || ent->d_name[2] != 'v' || ent->d_name[3] != 'n')
			   && (ent_namlen > 4 || ent->d_name[0] != '.' || ent->d_name[1] != 'g' || ent->d_name[2] != 'i' || ent->d_name[3] != 't'))
			writedirectory(dirino, pathbuf, 0, indent + 2, s.st_mode & 0777);
		else if (S_ISLNK(s.st_mode)) {
			unsigned long host_ino = (s.st_nlink > 1 ? s.st_ino : 0);
			writesymlink(dirino, pathbuf, host_ino, indent + 2);
		}
	}

	closedir(dir);
	if (dirb)
		putblk(dirb);
	if (inob)
		putblk(inob);
}

void
finishfs(void)
{
	int i;
	struct Block *b;

	// create free block bitmap
	for (i = 0; i < nextb; i++) {
		b = getblk(OSPFS_FREEMAP_BLK + i / OSPFS_BLKBITSIZE, 0, BLOCK_BITS);
		b->u.u[(i%OSPFS_BLKBITSIZE)/32] &= ~(1<<(i%32));
		putblk(b);
	}
	if (nblocks != nbitblock*OSPFS_BLKBITSIZE) {
		b = getblk(OSPFS_FREEMAP_BLK + nbitblock - 1, 0, BLOCK_BITS);
		for (i = nblocks % OSPFS_BLKBITSIZE; i < OSPFS_BLKBITSIZE; i++)
			b->u.u[i/32] &= ~(1<<(i%32));
		putblk(b);
	}

#if 0
	// create linked list of free blocks
	for (i = nextb; i < nblocks; i++) {
		b = getblk(i, 1, BLOCK_FILE);
		b->u.u[0] = (i + 1 < nblocks ? i + 1 : 0);
		putblk(b);
	}
	super.os_firstfree = (nextb < nblocks ? nextb : 0);
#endif
	
	// write superblock
	b = getblk(1, 1, BLOCK_SUPER);
	memmove(&b->u, &super, sizeof(struct ospfs_super));
	putblk(b);
}

void
flushdisk(void)
{
	int i;

	for (i = 0; i < nelem(cache); i++)
		if (cache[i].used)
			flushb(&cache[i]);
}

void
usage(void)
{
	fprintf(stderr, "Usage: ospfsformat [-c] [-l SRC:DST] fs.img NBLOCKS NINODES files...\n\
       ospfsformat [-c] [-l SRC:DST] fs.img NBLOCKS NINODES -r DIR\n\
  \"-c\" means treat files with identical contents as hard links.\n\
  \"-l SRC:DST\" means add a symbolic link from SRC to DST.\n");
	abort();
}

struct linkrecord {
	char *source;
	char *destination;
	struct linkrecord *next;
};

int
main(int argc, char **argv)
{
	int i;
	char *s;
	struct Block *rootinob;
	struct ospfs_inode *rootino;
	struct linkrecord *links = NULL;
	uint32_t rootinonumber;

	assert(sizeof(struct ospfs_inode) == OSPFS_INODESIZE);

    option:
	if (argc > 1 && strcmp(argv[1], "-V") == 0) {
		argc--, argv++, verbose = 1;
		goto option;
	}
	if (argc > 1 && strcmp(argv[1], "-c") == 0) {
		argc--, argv++, link_contents = 1;
		goto option;
	}
	if (argc > 1 && strcmp(argv[1], "-l") == 0) {
		struct linkrecord *nl;
		if (argc < 3 || strchr(argv[2], ':') == 0)
			usage();
		nl = malloc(sizeof(struct linkrecord));
		nl->source = argv[2];
		nl->destination = strchr(argv[2], ':') + 1;
		nl->destination[-1] = '\0';
		nl->next = links;
		links = nl;
		if (strchr(nl->destination, '/') != 0) {
			fprintf(stderr, "%s: I can't yet create symlinks that have '/' in them.\n", nl->source);
			usage();
		}
		argc -= 2, argv += 2;
		goto option;
	}

	if (argc < 4)
		usage();

	nblocks = strtol(argv[2], &s, 0);
	if (*s || s == argv[2] || nblocks < 2 || nblocks > 8192)
		usage();

	ninodes = strtol(argv[3], &s, 0);
	if (*s || s == argv[3] || ninodes < 2)
		usage();
	if (ninodes >= (nblocks - 2 - nblocks / OSPFS_BLKBITSIZE)) {
		fprintf(stderr, "Too many inodes, no room for data blocks!\n");
		usage();
	}

	opendisk(argv[1]);

	while (nextinode != OSPFS_ROOT_INO) {
		rootino = allocinode(&rootinonumber, &rootinob);
		rootino->oi_nlink = 1;
		putblk(rootinob);
	}
	rootino = allocinode(&rootinonumber, &rootinob);
	assert(rootinonumber == OSPFS_ROOT_INO);
	rootino->oi_ftype = OSPFS_FTYPE_DIR;
	rootino->oi_nlink = 1;
	rootino->oi_mode = 0777;
	if (strcmp(argv[4], "-r") == 0) {
		if (argc != 6)
			usage();
		writedirectory(rootino, argv[5], 1, 0, 0777);
	} else {
		for (i = 4; i < argc; i++)
			writefile(rootino, argv[i], 0, 0, 0666);
	}
	while (links) {
		struct linkrecord *l = links;
		addsymlink(rootino, l->destination, l->source, 0, 0);
		links = l->next;
		free(l);
	}
	putblk(rootinob);
	
	finishfs();
	flushdisk();
	exit(0);
	return 0;
}
