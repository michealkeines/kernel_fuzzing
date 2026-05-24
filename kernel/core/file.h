
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


/*
- user space will only pass fd as a number not as file struct, inside the kerenel, we use the number to get the file struct from the process's fd table

that also mean a process has to hold fd table

- lets start by saying our process will only have max 12 fd, so we havve keep our size small as of now

- once we have resptvice file_struct, and know the current positoin wihtin the file and how manu bytes we want read from that position

- we use the inodes data table to get all the block that are used by this inode

in extents, the logical block will be 0 -> n

the physical block will be anything is free

eg:

extent 1 : 0 -> 4 : 456 -> 450
extent 2 : 5 -> 10 : 600 -> 610


the inode will have this extent table infor + the number of blocks alocated

based onthe current index + number of bytes we want, we cna calculate the number of blocks we want to read from

we need an read_iter -> this will class the filesystem specific iter function

within that we get the inode of the file_struct

we get i_block of the file struct, ext_header, if we parse this we ill know how max ext values are there, we may have to go through them all

then finally with in the ext, we can find the logical block and the phsyical block and the count



*/

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


/*





Example: A process calls read(fd, buf, 512) to read 512 bytes starting at file offset 5000 from a file on an ext4 filesystem. 

System Call & File Descriptor Lookup: The read system call is made. The VFS uses fd to find the struct file in the process's file descriptor table. 
Get Offset: The VFS reads the current file position from struct file->f_pos. In this case, f_pos is 5000. 
Dispatch to Filesystem: The VFS checks struct file->f_op->read_iter and calls the ext4_file_read_iter function. 
Get Inode: The ext4_file_read_iter function gets the struct inode for the file via file->f_mapping->host. 
Parse Extent Tree Header: The function accesses the i_block field of the struct ext4_inode. The first 12 bytes are interpreted as struct ext4_extent_header.
eh_magic is 0xF30A (valid).
eh_entries is 3 (three extent entries are valid).
eh_max is 4 (can hold four entries).
eh_depth is 0 (this is a leaf node; the extents are stored directly in the inode).
Search Extents: The function scans the three struct ext4_extent entries that follow the header.
Extent 1: ee_block=0, ee_len=1, ee_start=100. This maps logical blocks 0 to 0. Our target offset (5000) is greater than 4095 (1 block * 4096 bytes), so it doesn't match.
Extent 2: ee_block=1, ee_len=1, ee_start=500. This maps logical blocks 1 to 1 (bytes 4096 to 8191). Our target offset (5000) falls within this range (4096 <= 5000 < 8192).
Calculate Physical Block: The function has found the correct extent.
The logical block number for offset 5000 is 5000 / 4096 = 1.
The offset within that logical block is 5000 % 4096 = 904 bytes.
The physical block number is ee_start (500) + (logical_block - ee_block) = 500 + (1 - 1) = 500. 
The VFS now knows to read from physical block 500 on the disk, starting at byte 904 of that block


*/