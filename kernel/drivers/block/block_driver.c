
#include "../../core/mmu.h"
#include "block_driver.h"
#include "../virtio_blk/virtio_blk.h"

#define BLOCK_READ "[block_read]"

static uint64_t GLOBAL_VIRTIO_ADDRESS[4] = {0};

static BlockData *GlobalBlockData = 0;
/*

currenlty our virtq desc is up to 12 in size

a single request (read/write) need 3 desc

in parallel we can only support 4 requests at time

- do we want to support more request in parallel? - we can increase the total size
- or can just make the other requests to wait untill, something is freed up - maybe lot of threads will be waiting?

-virtq supports batching, so along we have the memory, it is not an issue in the backend

- we might need some data structure ot implement syncing
*/

void init_virtio_backend(uint64_t drive_info_register)
{

#define VIRTIO_REG_RANGE 0xffff000086400000UL

#define VIRTQ_MAX_REQUESTS 24


    GlobalBlockData = (BlockData *)kmalloc(sizeof(BlockData));

    // init the data to zero
    GlobalBlockData->global_count = 0;
    for (int i =0; i < 12; i++) {
        GlobalBlockData->used_request_index[i] = 0;
    }

	virtio *virtio_obj = (virtio *)kmalloc( sizeof(virtio)); // address is hard code to this virtio reg

    GLOBAL_VIRTIO_ADDRESS[0] = (uint64_t)virtio_obj;
	// I think my pages are already 4k, even if i try to allocate something less, it would still alocate the full 4k, Maybe come back here if something goes wrong




	struct virtq_dec *virtq_desc_obj = (struct virtq_dec *)kmalloc( sizeof(struct virtq_dec) * VIRTQ_MAX_REQUESTS);
    GLOBAL_VIRTIO_ADDRESS[1] = (uint64_t)virtq_desc_obj;
	struct virtq_avail *virtq_avail_obj = (struct virtq_avail *)kmalloc( sizeof(struct virtq_avail) * VIRTQ_MAX_REQUESTS);
    GLOBAL_VIRTIO_ADDRESS[2] = (uint64_t)virtq_avail_obj;
	struct virtq_used *virtq_used_obj = (struct virtq_used *)kmalloc( sizeof(struct virtq_used) * VIRTQ_MAX_REQUESTS);
    GLOBAL_VIRTIO_ADDRESS[3] = (uint64_t)virtq_used_obj;

	// this is phycial address that we took from the parsed dtd

	// TODO:
	// qemu mimo will always have this address range mapped, but the backend will not bave anything that is why we need to chekcc if the device-id is there not
	// we just have the mimo starting address, we have to look into all device from this address with 0x200 as the offset and check if the device-id is not 0 and register all of them
	add_virtio_page(drive_info_register);


	uint32_t i = 0;

	uint64_t device_reg = 0;


	// we are only reading upto 32 devices?
	for (i =0; i <= 31; i++)
	{
		virtio temp = {
			.virtio_base = VIRTIO_REG_RANGE + (i * 0x200)
		};
	 	if (check_virtio_driver(&temp) == 1)
		{
			device_reg = VIRTIO_REG_RANGE + (i * 0x200);
			break;
		}
	}


	if (device_reg == 0)
	{
		uart_printf("no device found\n");
		return;
	}

	uart_printf("Device found at Index: %d\n", i);
	virtio_obj->virtio_base = device_reg;
	
	virtio_obj->virtual_virtq_desc = (uint64_t)virtq_desc_obj;
	virtio_obj->virtual_virtq_avail =(uint64_t) virtq_avail_obj;
	virtio_obj->virtual_virtq_used = (uint64_t)virtq_used_obj;
	virtio_obj->physical_virtq_desc = convert_to_physical((uint64_t)virtq_desc_obj);
	virtio_obj->physical_virtq_avail = convert_to_physical((uint64_t)virtq_avail_obj);
	virtio_obj->physical_virtq_used = convert_to_physical((uint64_t)virtq_used_obj);
	
	struct virtq *queue = (struct virtq *)kmalloc( sizeof(struct virtq));


	// we have the reg location mapped
	init_virtio_driver(virtio_obj, queue);

	uart_printf("init done for virtio driver\n");

}

int64_t find_free_index(void)
{
    for (int64_t i = 0; i < 12; i++)
    {
       if(GlobalBlockData->used_request_index[i] == 0)
       {
            uart_printf("Index %l is free for usage\n", i);
            // TODO: we need a mutex or something here, before we increment this, as someone else case also do it
            GlobalBlockData->global_count += 1;
            GlobalBlockData->used_request_index[i] = 1;
            return i * 3;
       }
    }

    return -1;
}

uint64_t block_read(uint32_t sector, char *data, uint64_t size)
{
    virtio * virtio_obj = (virtio *) GLOBAL_VIRTIO_ADDRESS[0];
    struct virtq_avail *virtq_avail_obj = (struct virtq_avail *) GLOBAL_VIRTIO_ADDRESS[2];
    struct virtq_used *virtq_used_obj = (struct virtq_used *) GLOBAL_VIRTIO_ADDRESS[3];

	uart_printf(BLOCK_READ ": Starting to read %d with size: %l\n", sector, size);

	struct virtio_blk_req *req = (struct virtio_blk_req *)kmalloc( sizeof(struct virtio_blk_req));

	req->type = VIRTIO_BLK_T_IN;
	req->sector = sector;
	req->reserved = 0; // TODO: is this need to be dynamic?

	virtio_mb();

	uart_printf(BLOCK_READ ": Prepared Request.");

    int64_t free_index = -1;
    while (free_index == -1)
    {
        free_index = find_free_index();
    }
	uint32_t i = (uint32_t)free_index;

	struct virtq_dec* desc1 = (struct virtq_dec *)(virtio_obj->virtual_virtq_desc + (sizeof(struct virtq_dec) * i++));
	struct virtq_dec* desc2 = (struct virtq_dec *)(virtio_obj->virtual_virtq_desc + (sizeof(struct virtq_dec) * i++));
	struct virtq_dec* desc3 = (struct virtq_dec *)(virtio_obj->virtual_virtq_desc + (sizeof(struct virtq_dec) * i++));

	uint8_t *status = (uint8_t *)kmalloc( sizeof(uint8_t));
	uart_printf("3\n");

	desc1->addr = kernal_to_user_space((uint64_t)req);
	desc1->len = sizeof(struct virtio_blk_req);
	desc1->flags = VIRTIO_DESC_F_NEXT;
	desc1->next = (uint16_t)free_index + 1;

	desc2->addr = kernal_to_user_space((uint64_t)data);
	desc2->len = 512;
	desc2->flags = VIRTIO_DESC_F_NEXT | VIRTIO_DESC_F_WRITE;
	desc2->next = (uint16_t)free_index + 2;

	desc3->addr = kernal_to_user_space((uint64_t)status);
	desc3->len = 1;
	desc3->flags =  VIRTIO_DESC_F_WRITE;
	// desc3->next = 0;

	uart_printf("4\n");

	virtio_mb();

	// we have to update the ring array with the index of the descriptor chain and then increment idx field, this way multiple chains can be batches together
	// virtq_avail_obj->idx = 0;
	virtq_avail_obj->flags = 1;
	virtio_mb();
	virtq_avail_obj->ring[free_index] = free_index;
	virtio_mb();
	virtq_avail_obj->idx += 1;
	uart_printf("5\n");
	uint32_t last_seen_idx = virtq_used_obj->idx;
	virtio_mb();
	write_u32(virtio_obj, VIRTIO_QueueNotify, 0);

	virtio_mb();

	uart_printf("6\n");
	uart_printf("\nstatus bit1: %d\n", *status);
	i = 0;
	virtio_mb();
	while (virtq_used_obj->idx == last_seen_idx) virtio_mb();
	uart_printf("new index: %d, freeindex: %d\n", virtq_used_obj->idx, free_index);
	uart_printf("used rring 0: %d\n", virtq_used_obj->ring[free_index].len);
	uart_printf("\nstatus bit2: %d\n", *status);

    GlobalBlockData->used_request_index[free_index] = 0;
    return virtq_used_obj->ring[free_index].len;
}


uint64_t block_write(uint32_t sector, char *data, uint64_t size)
{
    virtio * virtio_obj = (virtio *) GLOBAL_VIRTIO_ADDRESS[0];
    struct virtq_avail *virtq_avail_obj = (struct virtq_avail *) GLOBAL_VIRTIO_ADDRESS[2];
    struct virtq_used *virtq_used_obj = (struct virtq_used *) GLOBAL_VIRTIO_ADDRESS[3];

	uart_printf(BLOCK_READ ": Starting to write %d with size: %l\n", sector, size);

	struct virtio_blk_req *req = (struct virtio_blk_req *)kmalloc( sizeof(struct virtio_blk_req));

	req->type = VIRTIO_BLK_T_OUT;
	req->sector = sector;
	req->reserved = 0; // TODO: is this need to be dynamic?

	virtio_mb();

	uart_printf(BLOCK_READ ": Prepared Request.");

    int64_t free_index = -1;
    while (free_index == -1)
    {
        free_index = find_free_index();
    }
	uint32_t i = (uint32_t)free_index;

	struct virtq_dec* desc1 = (struct virtq_dec *)(virtio_obj->virtual_virtq_desc + (sizeof(struct virtq_dec) * i++));
	struct virtq_dec* desc2 = (struct virtq_dec *)(virtio_obj->virtual_virtq_desc + (sizeof(struct virtq_dec) * i++));
	struct virtq_dec* desc3 = (struct virtq_dec *)(virtio_obj->virtual_virtq_desc + (sizeof(struct virtq_dec) * i++));

	uint8_t *status = (uint8_t *)kmalloc( sizeof(uint8_t));
	uart_printf("3\n");

	desc1->addr = kernal_to_user_space((uint64_t)req);
	desc1->len = sizeof(struct virtio_blk_req);
	desc1->flags = VIRTIO_DESC_F_NEXT;
	desc1->next = (uint16_t)free_index + 1;

	desc2->addr = kernal_to_user_space((uint64_t)data);
	desc2->len = 512;
	desc2->flags = VIRTIO_DESC_F_NEXT;
	desc2->next = (uint16_t)free_index + 2;

	desc3->addr = kernal_to_user_space((uint64_t)status);
	desc3->len = 1;
	desc3->flags =  VIRTIO_DESC_F_WRITE;
	// desc3->next = 0;

	uart_printf("4\n");

	virtio_mb();

	// we have to update the ring array with the index of the descriptor chain and then increment idx field, this way multiple chains can be batches together
	// virtq_avail_obj->idx = 0;
	virtq_avail_obj->flags = 1;
	virtio_mb();
	virtq_avail_obj->ring[free_index] = free_index;
	virtio_mb();
	virtq_avail_obj->idx += 1;
	uart_printf("5\n");
	uint32_t last_seen_idx = virtq_used_obj->idx;
	virtio_mb();
	write_u32(virtio_obj, VIRTIO_QueueNotify, 0);

	virtio_mb();

	uart_printf("6\n");
	uart_printf("\nstatus bit1: %d\n", *status);
	i = 0;
	virtio_mb();
	while (virtq_used_obj->idx == last_seen_idx) virtio_mb();
	uart_printf("new index: %d, freeindex: %d\n", virtq_used_obj->idx, free_index);
	uart_printf("used rring 0: %d\n", virtq_used_obj->ring[free_index].len);
	uart_printf("\nstatus bit2: %d\n", *status);

    GlobalBlockData->used_request_index[free_index] = 0;
    return virtq_used_obj->ring[free_index].len;
}
