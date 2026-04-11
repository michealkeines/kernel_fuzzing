#include <stdint.h>
extern void uart_printf(const char* fmt, ...);
#define DTB_BASE 0x40000000
#define VIRT_DTB_BASE 0xffff000085400000UL // if we ever reach this through our dynamic allocation, we will have to move this aroudn

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
    // we moving the byte order 4 bytes are read here
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



void get_drive_info(uint64_t *result)
{
    DTB *header = (DTB *) VIRT_DTB_BASE;

    uint32_t off_dt_struct = __builtin_bswap32(header->off_dt_struct);
    uint32_t off_dt_string = __builtin_bswap32(header->off_dt_strings);


    uint8_t *dt_struct = (uint8_t *) (VIRT_DTB_BASE + off_dt_struct);
    uint8_t *dt_strings = (uint8_t *) (VIRT_DTB_BASE + off_dt_string);

    uint8_t *p = dt_struct;

    uint8_t inside_virtio = 0;

    for (;;)
    {
        uint32_t token = read_b32(p);
        p += 4;
        
        
        uint64_t total = 0;

        uart_printf("current otken: %d\n", token);
        switch (token)
        {
        case TOKEN_BEGIN_NODE:
        {
            char *name = (char *) p;

            uart_printf("token begin name: %s\n", name);
            if (name[0] == '\0') 
            {
                inside_virtio = 0;
            }
            else if (name[0] == 'v' && name[1] == 'i' && name[2] == 'r' )
            {
                inside_virtio = 1;
            }
            else
            {
                inside_virtio = 0;
            }

            while (*p != '\0') p++;

            if (*p == '\0') p++;

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

            if (inside_virtio && string[0] == 'c' && string[1] == 'o' && string[2] == 'm')
            {
                uart_printf("inside compatble: %d\n", len);
                // uint32_t string_offset = read_b32(p);

                char *value = (char *)p;
                uart_printf("inside compatble string: %s\n", value);
                // p += len;
            }
            
		    uart_printf("current string: %s\n", string);
            if (inside_virtio && string[0] == 'r' && string[2] == 'g')
            {
                // let's divide the the len by 16, as len is the number of bytes, 4 32 bit entries = 16 bytes
                
		        uart_printf("virtio reg: %s\n", string);
                if (len <= 0) break;

                uint32_t max = len / 16;
                

                // for (uint32_t i = 0; i < max; i++)
                // {
                uint32_t add2 = read_b32(p);
                p += 4;
                uint32_t add1 = read_b32(p);
                p += 4;
                uint32_t size2 = read_b32(p);
                p += 4;
                uint32_t size1 = read_b32(p);
                p -= 12;



                uint64_t add = (((uint64_t)add2 << 32) | (uint64_t)add1);
                uint64_t size = (((uint64_t)size2 << 32) | (uint64_t)size1);
                //     total += size;
                // }
                // read_here(total);

                result[0] = add;
                result[1] = size;
                //after reading first reg, we return
                return;
            }
            else if (inside_virtio && string[0] == 'i' && string[1] == 'n' && string[2] == 't')
            {
                if (len <= 0) break;

		        uart_printf("virtio interrupts: %s, %d\n", string, len);
                // uint32_t max = len / 16;

                // for (uint32_t i = 0; i < max; i++)
                // {
                uint32_t num2 = 0;
                uint32_t num1 = read_b32(p + 4);
                // p += 4;
                // for (int i = 0; i < 12; ++i){
                // uart_printf("interrupt numner: %d => %d\n", i, p[i]);
                // }
                // p += 3;
                uint32_t type2 = 0;
                uint32_t type1 = read_b32(p + 0);
                // // p += 4;
                
                uint32_t edge2 = 0;
                uint32_t edge1 = read_b32(p + 8);
                // // p += 4;

                uint64_t edge = (((uint64_t)edge2 << 32) | (uint64_t)edge1);
                uint64_t type = (((uint64_t)type2 << 32) | (uint64_t)type1);
                uint64_t num = (((uint64_t)num2 << 32) | (uint64_t)num1);
                // // //     total += size;
                // // // }
                // // // read_here(total);

                result[2] = type;
                result[3] = num;
                result[4] = edge;

                // there can be multiple mimo, we are returing afer we found the first
                // return;
            }
            p += len;
            while ((uint64_t)p & 3) p++;
            break;
        }

        case TOKEN_END_NODE:
            inside_virtio = 0;
            // p += 4;
            break;

        case TOKEN_NOP:
            // p += 4;
            break;

        case TOKEN_END:
            // p += 4;
            return;
            // default
        default:
            p += 4;
        uart_printf("here1\n");
            break;
        }
    }

    
}