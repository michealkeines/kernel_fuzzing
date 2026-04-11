#ifndef ___VIRTIOBLK___
#define ___VIRTIOBLK___

#include <stdint.h>

extern void uart_printf(const char* fmt, ...);

// https://docs.oasis-open.org/virtio/virtio/v1.1/csprd01/virtio-v1.1-csprd01.html

#define VIRTIO_MAGIC_VALUE 0x000
#define VIRTIO_VERSION     0x004
#define VIRTIO_DEVICEID    0x008
#define VIRTIO_STATUS      0x070
#define VIRTIO_QueueSel    0x030
#define VIRTIO_QueueNumMax 0x034
#define VIRTIO_QueueNum    0x038
#define VIRTIO_QueueReady    0x044
#define VIRTIO_QueueAlign    0x044
#define VIRTIO_QueueNotify 0x050
#define VIRTIO_QueueNotify 0x050
#define VIRTIO_QueueNotify 0x050
#define VIRTIO_QueueDescLow 0x080
#define VIRTIO_QueueDescHigh 0x084
#define VIRTIO_QueueAvailLow 0x090
#define VIRTIO_QueueAvailHigh 0x094
#define VIRTIO_QueueUsedLow 0x0a0
#define VIRTIO_QueueUsedHigh 0x0a4
#define VIRTIO_DeviceFeatures 0x010
#define VIRTIO_DeviceFeaturesSel 0x014
#define VIRTIO_DriverFeatures 0x020
#define VIRTIO_DriverFeaturesSel 0x024


// FLAGS

#define VIRTIO_STATUS_ACK   1
#define VIRTIO_STATUS_RESET 0
#define VIRTIO_STATUS_DRIVER 2
#define VIRTIO_STATUS_FAILED 128
#define VIRTIO_STATUS_DRIVER_OK 4
#define VIRTIO_STATUS_FEATURES_OK 8
#define VIRTIO_STATUS_DEVICES_NEEDS_RESET 64

/*

flow 

- i prepare the request with the block number and the read/write flag
- then i update the descriptor table with the pointers to the req, data, status
- we chaing all the descriptors and get the index and put the into aval.ring
- we notify the queu, with queueNotify
- we can pool the used ring for the process output

Prepare request: 

Set type = VIRTIO_BLK_T_OUT, sector = 100, status byte.
Update descriptor table:

Desc 0: Point to request (read-only).
Desc 1: Point to 1 KiB data buffer (read-only).
Desc 2: Point to status byte (write-only).
Chain them using next fields.

Add to available ring: 

Write index of first descriptor (e.g., 0) to avail.ring[avail.idx % N], increment avail.idx. 


Notify device: 
Write queue index to QueueNotify.


Poll used ring: 

After interrupt or polling, check used.ring[used.idx % N]:
Match id to your descriptor chain.
Verify len and status == VIRTIO_BLK_S_OK

TODO:

- create all needs structs

*/


#define VIRTIO_BLK_T_IN 0 
#define VIRTIO_BLK_T_OUT 1
#define VIRTIO_BLK_T_FLUSH 4


#define VIRTIO_DESC_F_WRITE 2
#define VIRTIO_DESC_F_NEXT 1

/*

 Virtqueue Descriptors (in guest memory)
┌───────────────────────┐
│       Descriptor 0    │
├───────────────────────┤
│ addr: 0x8000_0000    │ ← Points to request header
│ len:  16              │ ← sizeof(virtio_blk_req_header)
│ flags: VIRTQ_DESC_F_NEXT │ → Chain to next
│ next:  1               │
└───────────────────────┘

┌───────────────────────┐
│       Descriptor 1    │
├───────────────────────┤
│ addr: 0x8000_1000    │ ← Points to data buffer
│ len:  512             │ ← One sector
│ flags: VIRTQ_DESC_F_WRITE │ ← Device writes data here (for read)
│        | VIRTQ_DESC_F_NEXT
│ next:  2               │
└───────────────────────┘

┌───────────────────────┐
│       Descriptor 2    │
├───────────────────────┤
│ addr: 0x8000_2000    │ ← Points to status byte
│ len:  1               │
│ flags: VIRTQ_DESC_F_WRITE │ ← Device writes status here
│ next:  (ignored)      │
└───────────────────────┘  


TODO:

- create the request with the type, reserver, sector
- get the physical address of the request
- create a desc with this address and chained a next flag

- create the data with some value
- get the physical address of the buffer
- create a desc wit hthis address and chained as next and write flags


so we ahve increment the desc base address + the bytes used to go to next desc and write the info, as all desc of static size

we have put the desc starting index into avail ring

 and then notify the register QueueNotify
*/
static inline void virtio_mb(void)
{
	__asm__ volatile ("dmb sy"::: "memory");
}



struct __attribute__((packed)) virtio_blk_req { 
    uint32_t type; 
    uint32_t reserved; 
    uint64_t sector; 
    uint8_t data[0][512]; 
    uint8_t status; 
};

struct __attribute__((packed)) virtq_dec {
    uint64_t addr;
    uint32_t len;
    uint16_t flags;
    uint16_t next;
};

struct __attribute__((packed)) virtq_avail {
    uint16_t flags;
    uint16_t idx;
    uint16_t ring[12];
    uint16_t used_event;
};
struct __attribute__((packed)) virtq_used_elem {
    uint32_t id;
    uint32_t len;
};
struct __attribute__((packed)) virtq_used {
    uint16_t flags;
    uint16_t idx;
    struct virtq_used_elem ring[]; 
};
struct __attribute__((packed)) virtq {
    unsigned int num;
    struct virtq_dec      *desc;
    struct virtq_avail    *avail;
    struct virtq_used     *used;
};

typedef struct __attribute__((packed)) _virtio {
    uint64_t virtio_base;
    uint64_t virtual_virtq_desc;
    uint64_t virtual_virtq_avail;
    uint64_t virtual_virtq_used;
    uint64_t physical_virtq_desc;
    uint64_t physical_virtq_avail;
    uint64_t physical_virtq_used;
} virtio;




void set_virtio_base(virtio *virtio_obj);
void virtio_status_update(virtio *virtio_obj);
void init_virtio_driver(virtio *virtio_obj, struct virtq *queue);
void update_status(virtio *virtio_obj, uint32_t status);


uint32_t check_virtio_driver(virtio *virtio_obj);

uint32_t read_u32(virtio *virtio_obj,  uint32_t offset);
void write_u32(virtio *virtio_obj, uint32_t offset, uint32_t value);

#endif // ___VIRTIOBLK___