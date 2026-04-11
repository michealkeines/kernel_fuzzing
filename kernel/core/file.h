
#ifndef ___FILE______
#define ___FILE______

#include <stdarg.h>
#include <stdint.h>


/*
vfs_lookup("/bin/ls")
    ↓
start at root inode - superblock define which tree of inode i am gonna look into
    ↓
lookup "bin" in root directory - lookup is funciton that is given by the diver of the filesystem
    ↓
lookup "ls" in bin directory
    ↓
 inode found
    ↓
with the inode create file sturct 

struct file {
    inode = ls_inode
    offset = 0
}

block layer block_read, block_write

    ↓
return the fd back


ext4 support

- 1024 bytes is padding and superblock starts right after that, read that and put inside a buffer (confirm this in reality with an example)
- read offset 0x18 which will give us the block size ie 2 ^ (10 + size we read)
- read offset 0x4 - block count
- read offset 0x20 - no of groups
- read offset 0x28 - inodes per group
- read offset 0x58 - inode size
- the second block in group 0 will be the group descriptor
- edge case if block size 1024, then group descriptor will be in block 2
- incase backup is enabled other groups will store the same info in the block 0
- data block bitmap is of size 1 block, so total number of blocks within in group will blocksize * 8, ie 8 bits per byte

- block group descriptor will have the infor about the block numbers within a group, that will have data block bitmap, inode bitmap, inode table
- inode table hold inodes metadata
- inode bitmap hold the allocated records for inodes within the inode table

 group = (inode_number - 1) / inodes_per_group

 index = (inode_number - 1) % inodes_per_group


- bytes interpretation

Mode	                    0x0	  16
Size	                    0x4	  32
Last access time	        0x8	  32
Last data modification time	0x10  32
Last inode change time	    0xc	  32



*/

#define MAX_FDS 8

 FDS_COUNTER = 2;

typedef struct _ops {
    uint64_t (*read)(File *, uint64_t);
    uint64_t (*write)(File *, uint64_t);
    uint64_t (*close)(File *);
} Operations;

typedef enum _file_types { DIR, FILE, SOCK, PIPE } file_types;

typedef struct _inode {
    file_types type;
    uint64_t id;
    void *data;
    uint64_t size;
} Inode;

typedef struct _file {
    uint64_t fd; // will not be shared accross process
    Inode inode;
    Operations ops;
    uint64_t offset;
} File;

void initilize_file(File *file)
{

}

uint64_t init_file(File *file, file_types type)
{
    uint64_t ret = 0;
    switch (type)
    {
        case DIR:
            ret = initilize_dir(file);
            break;
        case FILE:
            ret = initilize_file(file);
            break;
        case SOCK:
            ret = initilize_sock(file);
            break;
        default:
            uart_printf("unknown file type: %l\n", type);
            ret = 1;
            break;
    }

    return ret;
}

#endif // ___FILE______
