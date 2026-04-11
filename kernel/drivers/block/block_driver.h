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



uint64_t block_read(char **buff);
uint64_t block_write(char **buff, uint64_t size);



#endif // ___BLOCK_DRIVER__