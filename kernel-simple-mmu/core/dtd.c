#include <stdint.h>

#define DTB_BASE 0x40000000

#define TOKEN_BEGIN_NODE 0x1
#define TOKEN_END_NODE   0x2
#define TOKEN_PROP  0x3
#define TOKEN_NOP   0x4
#define TOKEN_END   0x9

typedef struct __attribute__((packed)) fdt_header {
    uint32_t magic;            // 0xd00dfeed
    uint32_t totalsize;
    uint32_t off_dt_struct;    // OFFSET to structure block
    uint32_t off_dt_strings;   // OFFSET to strings block
    uint32_t off_mem_rsvmap;   // OFFSET to memory reserve array
    uint32_t version;
    uint32_t last_comp_version;
    uint32_t boot_cpuid_phys;
    uint32_t size_dt_strings;  
    uint32_t size_dt_struct;
} DTB;

static inline uint32_t read_b32(uint8_t *p)
{

    return (
        (uint32_t) (p[0] << 24) | 
        (uint32_t) (p[1] << 16) |
        (uint32_t) (p[2] << 8) |
        (uint32_t) (p[3] << 0)
    );
}

// static void read_here_32(uint32_t value)
// {
//     value = value + 1;
// }

// static void read_here(uint64_t value)
// {
//     value = value + 1;
// }

uint64_t get_total_ram()
{
    DTB *header = (DTB *) DTB_BASE;


    uint32_t off_dt_struct = __builtin_bswap32(header->off_dt_struct);
    uint32_t off_dt_string = __builtin_bswap32(header->off_dt_strings);


    uint8_t *dt_struct = (uint8_t *) (DTB_BASE + off_dt_struct);
    uint8_t *dt_strings = (uint8_t *) (DTB_BASE + off_dt_string);

    uint8_t *p = dt_struct;

    uint8_t inside_memory_node = 0;

    for (;;)
    {
        uint32_t token = read_b32(p);
        
        
        p += 4;
        uint64_t total = 0;

        switch (token)
        {
        case TOKEN_BEGIN_NODE:
        {
            char *name = (char *) p;

            if (name[0] == '\0') 
            {
                inside_memory_node = 0;
            }
            else if (name[0] == 'm' && name[1] == 'e' && name[5] == 'y' )
            {
                inside_memory_node = 1;
            }
            else
            {
                inside_memory_node = 0;
            }

            while (*p != '\0') p++;

            p++;

            while ((uint64_t)p & 3) p++;

            break;
        }

        case TOKEN_PROP:
        {
            uint32_t len = read_b32(p);

            p += 4;

            uint32_t string_offset = read_b32(p);

            p += 4;

            uint8_t *value = (uint8_t *)p;
            char *string = dt_strings + string_offset;

            if (inside_memory_node && string[0] == 'r' && string[2] == 'g')
            {
                // let's divide the the len by 16, as len is the number of bytes, 4 32 bit entries = 16 bytes
                
                if (len <= 0) return 0;

                uint32_t max = len / 16;
                

                for (uint32_t i = 0; i < max; i++)
                {
                    uint32_t add2 = read_b32(p);
                    p += 4;
                    uint32_t add1 = read_b32(p);
                    p += 4;
                    uint32_t size2 = read_b32(p);
                    p += 4;
                    uint32_t size1 = read_b32(p);
                    p += 4;

                    uint64_t add = (((uint64_t)add2 << 32) | (uint64_t)add1);
                    uint64_t size = (((uint64_t)size2 << 32) | (uint64_t)size1);
                    total += size;
                }
                // read_here(total);
                return total;
            }
            p += len;
            while ((uint64_t)p & 3) p++;
            break;
        }

        case TOKEN_END_NODE:
        inside_memory_node = 0;
        break;

        case TOKEN_NOP:
            break;

        case TOKEN_END:
            return total; 
        default:
            return total;
        }
    }

    
}
