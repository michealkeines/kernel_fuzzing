#ifndef ___BLOCK_DRIVER__
#define ___BLOCK_DRIVER__

#include <stdint.h>
/*

we have an image ext4fs/ext4fs.img

that as file test

we have to block_read, block_write

that is manully gonna read/write to this img in sector level

TODO:
before that parse DTB to get reg address, interrupt number


we have add the helper to get the reg, int


- check how linux does this



*/

// - global count - 12
// - used_requests_info - 24
// - used_requests_index - 12
/*
if index 0 is used that mean 0,1,2 is allocated - this be assumed

if index 1 is used that means 3,4,5 is allocated
*/
typedef struct _blockData {
    uint8_t global_count;
    uint8_t used_request_index[12];
} BlockData;


uint64_t block_read(uint32_t sector, char *buff, uint64_t size);
uint64_t block_write(uint32_t sector, char *buff, uint64_t size);

void init_virtio_backend(uint64_t drive_info_register);


#endif // ___BLOCK_DRIVER__