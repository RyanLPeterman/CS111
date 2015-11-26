#ifndef OSPFS_H
#define OSPFS_H
// OSPFS Constants and Structure Definitions

/*****************************************************************************
 * BLOCKS
 *
 *   The OSPFS format divides disk data into a series of blocks,
 *   where each block contains 'OSPFS_BLKSIZE' bytes (1024 bytes)
 *   and 'OSPFS_BLKBITSIZE' bits.
 *
 *****************************************************************************/
#define OSPFS_BLKSIZE_BITS  10
#define OSPFS_BLKSIZE       (1 << OSPFS_BLKSIZE_BITS) /* == 1024 */
#define OSPFS_BLKBITSIZE    (OSPFS_BLKSIZE * 8)


/*****************************************************************************
 * FILE SYSTEM LAYOUT
 *
 *   The OSPFS is laid out as follows.
 *   0. BOOT SECTOR.  As in most real file systems, the disk's first block
 *      is reserved for possible boot loaders and partition tables.
 *   1. SUPERBLOCK.  There is exactly one, in the disk's second block.
 *      It contains a possible boot sector and information about the OSPFS:
 *      the total number of blocks and inodes on the disk.
 *   2. FREE BLOCK BITMAP.  Located immediately after the superblock.
 *      There is one bit for each block.  If that bit is set to 1,
 *      the block is free.  (The superblock, free block bitmap, and inode
 *      blocks are never free.)
 *   3. INODE BLOCKS.  Located immediately after the free block bitmap.
 *      An "inode" holds a file's metadata: its size, its type, and the
 *      numbers of the data blocks that contain its data.
 *      (The file's name, however, is stored elsewhere.)
 *      Each file and directory on the disk corresponds to an inode.
 *      All inodes are stored in the inode blocks.
 *   4. The rest of the disk consists of DATA BLOCKS.
 *      Each data block belongs to a normal file or to a directory.
 *      Directory data blocks consist of sequences of directory entry
 *      structures, which refer to inodes.
 *      Indirect blocks, which are sets of other block pointers, are also
 *      stored here.
 *
 *   |<--------------------------    N blocks    --------------------------->|
 *   |                                                                       |
 *   +------+-------+------------+-------------+-----------------------------+
 *   | boot | super | free block |    inode    |           data              |
 *   | stuff| block |   bitmap   |    blocks   |           blocks            |
 *   +------+-------+------------+-------------+-----------------------------+
 * Block 0      1      2 to X-1      X to Y-1             Y to N-1
 *                    (enough to    (enough to
 *                   hold N bits)  hold M inodes)
 *
 *   where X equals the superblock's "s_firstinob" member.
 *
 *****************************************************************************/

// OSPFS's superblock.
#define OSPFS_MAGIC 0x013101AE  // Related vaguely to '\11\1!'

#define OSPFS_FREEMAP_BLK  2  	// First block in free block bitmap

typedef struct ospfs_super {
	uint32_t os_magic;     		// Magic number: OSPFS_MAGIC
	uint32_t os_nblocks;   		// Number of blocks on disk
	uint32_t os_ninodes;   		// Number of inodes on disk
	uint32_t os_firstinob; 		// First inode block
} ospfs_super_t;


/*****************************************************************************
 * INODES
 *
 *   Inodes are represented by 'struct ospfs_inode'.
 *   This structure is 64 bytes long, so 16 inodes fit in an inode block.
 *
 *   Each inode stores the block numbers of the blocks that contain that
 *   file's data.  If the file is less than 10KB big, the block pointers are
 *   stored directly in the inode, using "direct" block pointers.
 *   Larger files use the "indirect block" as well.  This is a block that
 *   contains not data, but more block pointers.  Still larger files also
 *   use the "doubly indirect block", which is a block that contains pointers
 *   to more INDIRECT blocks.
 *
 *   Inode number 0 is illegal, and inode number 1 is reserved for the root
 *   directory.
 *
 *****************************************************************************/
#define OSPFS_INODESIZE		64
#define OSPFS_BLKINODES		(OSPFS_BLKSIZE / OSPFS_INODESIZE)

// Number of direct block pointers in 'struct ospfs_inode'.
#define OSPFS_NDIRECT		10
// Number of block pointers in an indirect block.
#define OSPFS_NINDIRECT		(OSPFS_BLKSIZE / 4)
// Maximum number of blocks in a file.
#define OSPFS_MAXFILEBLKS	\
	(OSPFS_NDIRECT					  /* direct blocks */ \
	 + OSPFS_NINDIRECT	    /* blocks pointed to by indirect block */ \
	 + OSPFS_NINDIRECT * OSPFS_NINDIRECT)   /* ... by indirect^2 block */
// Maximum file size.
#define OSPFS_MAXFILESIZE	(OSPFS_MAXFILEBLKS * OSPFS_BLKSIZE)

// File type constants for 'struct ospfs_inode's 'i_ftype' member.
#define OSPFS_FTYPE_REG		0  // Regular file
#define OSPFS_FTYPE_DIR		1  // Directory
#define OSPFS_FTYPE_SYMLINK	2  // Symbolic link

// Inode number for the root directory.
#define OSPFS_ROOT_INO		1

// OSPFS's inode structure.
typedef struct ospfs_inode {
	uint32_t oi_size;                   // File size
	uint32_t oi_ftype;                  // OSPFS_FTYPE_* constant
	uint32_t oi_nlink;                  // Link count (0 means free)
	uint32_t oi_mode;		    		// File permissions mode
	
	uint32_t oi_direct[OSPFS_NDIRECT];  // Direct block pointers
	uint32_t oi_indirect;               // Indirect block
	uint32_t oi_indirect2;		    	// Doubly indirect block
} ospfs_inode_t;


/*****************************************************************************
 * SYMBOLIC LINK INODES
 *
 *   Symbolic links are also stored in inodes.
 *   Unlike normal files, the "contents" of the symbolic link (that is, the
 *   destination file) is stored IN THE INODE AREA ITSELF, in an array of
 *   characters called "oi_symlink".  The inode's size equals the number of
 *   characters in oi_symlink.
 *
 *   For example, an inode representing a symbolic link pointing at file
 *   "hello.txt" might look like this:
 *
 *                         +------+------+------+------+
 *        oi_size ======>  |             9             |
 *                         +------+------+------+------+
 *        oi_ftype =====>  |    OSPFS_FTYPE_SYMLINK    |
 *                         +------+------+------+------+
 *        oi_nlink =====>  |             1             |
 *                         +------+------+------+------+
 *        oi_symlink ===>  |  'h' |  'e' |  'l' |  'l' |
 *                         +------+------+------+------+
 *                         |  'o' |  '.' |  't' |  'x' |
 *                         +------+------+------+------+
 *                         |  't' | '\0' | ........... |
 *                         +------+------+             |
 *                         | ......................... |
 *                         | .. 42 bytes of padding .. |
 *                         | ......................... |
 *                         +------+------+------+------+
 *
 *   We use a separate type of inode structure to represent this, namely
 *   'struct ospfs_symlink_inode'.
 *
 *****************************************************************************/
// Maximum length of a symbolic link.
#define OSPFS_MAXSYMLINKLEN	(OSPFS_INODESIZE - 13)

typedef struct ospfs_symlink_inode {
	uint32_t oi_size;		    // File size must be <= OSPFS_MAXSYMLINKLEN
	uint32_t oi_ftype;		    // == OSPFS_FTYPE_SYMLINK
	uint32_t oi_nlink;		    // Link count (0 means free)

	char oi_symlink[OSPFS_MAXSYMLINKLEN + 1]; // Destination file
} ospfs_symlink_inode_t;


/*****************************************************************************
 * DIRECTORY ENTRIES
 *
 *   Directory entries are represented by 'struct ospfs_direntry', which is
 *   a pair of an inode number and a C-style string representing the name.
 *
 *   If the inode number is 0, then the directory entry is EMPTY; it should
 *   be ignored on reads, and may be used to hold new files.
 *
 *   The whole structure is 128 bytes long, so the longest filename that can be
 *   stored is 123 bytes (128 bytes - 4 bytes for the inode - 1 byte for the
 *   terminating null character).
 *
 *****************************************************************************/

#define OSPFS_DIRENTRY_SIZE	128
#define OSPFS_MAXNAMELEN	(OSPFS_DIRENTRY_SIZE - 5)

typedef struct ospfs_direntry {
	uint32_t od_ino;						// Inode number
	char od_name[OSPFS_MAXNAMELEN + 1];		// File name
} ospfs_direntry_t;

#endif
