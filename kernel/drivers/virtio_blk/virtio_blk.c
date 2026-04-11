#include "virtio_blk.h"

void write_u32(virtio *virtio_obj, uint32_t offset, uint32_t value)
{
    *(uint32_t *)(virtio_obj->virtio_base + offset) = value;
    virtio_mb();
}

uint32_t read_u32(virtio *virtio_obj,  uint32_t offset)
{
    return *(uint32_t *)(virtio_obj->virtio_base + offset);
}



void update_status(virtio *virtio_obj, uint32_t status)
{
    write_u32(virtio_obj, VIRTIO_STATUS, status);
}

uint32_t get_lower_half(uint64_t value)
{
    return (uint32_t) (value & 0x00000000ffffffff);
}

uint32_t get_upper_half(uint64_t value)
{
    uint64_t temp = (value & 0xffffffff00000000);

    return (uint32_t) (temp >> 32);
}


void init_virtio_registers(virtio *virtio_obj)
{
    // allocate memory for all three type and put them into the registers?
    // Allcoate memory for all three structs with our kmalloc and get the phsycial range for the allcoated VA, in out htat is just identity mapping, and pass the physical address to the virtio

    // write the low/upper part separtely in to hte registers


    // this value is not gettin updated properly

    // info virtio-queue-status /machine/peripheral-anon/device[1] 0


    uart_printf("setting desc with: %l\n", virtio_obj->physical_virtq_avail);
    write_u32(virtio_obj, VIRTIO_QueueDescLow, get_lower_half(virtio_obj->physical_virtq_desc));
    write_u32(virtio_obj, VIRTIO_QueueDescHigh, get_upper_half(virtio_obj->physical_virtq_desc));

    uart_printf("setting avail with: %l\n", virtio_obj->physical_virtq_avail);

    write_u32(virtio_obj, VIRTIO_QueueAvailLow, get_lower_half(virtio_obj->physical_virtq_avail));
    write_u32(virtio_obj, VIRTIO_QueueAvailHigh, get_upper_half(virtio_obj->physical_virtq_avail));


    write_u32(virtio_obj, VIRTIO_QueueUsedLow, get_lower_half(virtio_obj->physical_virtq_used));
    write_u32(virtio_obj, VIRTIO_QueueUsedHigh, get_upper_half(virtio_obj->physical_virtq_used));


}

void init_queue(virtio *virtio_obj, struct virtq *queue)
{
    /*
    Select the queue writing its index (first queue is 0) to QueueSel.
    Check if the queue is not already in use: read QueueReady, and expect a returned value of zero (0x0).
    Read maximum queue size (number of elements) from QueueNumMax. If the returned value is zero (0x0) the queue is not available.
    Allocate and zero the queue memory, making sure the memory is physically contiguous.
    Notify the device about the queue size by writing the size to QueueNum.
    Write physical addresses of the queue’s Descriptor Area, Driver Area and Device Area to (respectively) the QueueDescLow/QueueDescHigh, QueueDriverLow/QueueDriverHigh and QueueDeviceLow/QueueDeviceHigh register pairs.
    Write 0x1 to QueueReady.
    
    */


    // TODO: looks like we have to change the version of virtio, currently we are getting version 0x1, but all our implementation is for virtio 1.1 which is 0x2

    // maybe we have find the proper flags, or maybe find the porper device atall

    write_u32(virtio_obj, VIRTIO_QueueSel, 0);

    uint32_t ready_status = read_u32(virtio_obj, VIRTIO_QueueReady);
    uint32_t device_version = read_u32(virtio_obj, VIRTIO_VERSION);
    uint32_t device_id = read_u32(virtio_obj, VIRTIO_DEVICEID);
    uint32_t device_features = read_u32(virtio_obj, VIRTIO_DeviceFeatures);

    uart_printf("ready status: %d\n", ready_status);
    uart_printf("versopm: %d\n", device_version);
    uart_printf("device features: %d\n", device_features);
    uart_printf("device id: %d\n", device_id);

    if (ready_status != 0x0)
    {
        uart_printf("Queue 0 is not ready? \n");
        return;
    }
    uint32_t queue_max_size = read_u32(virtio_obj, VIRTIO_QueueNumMax);

    if (queue_max_size == 0x0)
    {
        uart_printf("Queue is not ready? because of max size? \n");
        return;
    }

    uart_printf("queue max size: %d\n", queue_max_size);

    write_u32(virtio_obj, VIRTIO_QueueNum, 12); // maybbe we have to writeh overall size, or just the count? or maybe we have to write whatever we get out the NumMax read?


    init_virtio_registers(virtio_obj);



    write_u32(virtio_obj, VIRTIO_QueueReady, 0x1);


}



uint32_t check_virtio_driver(virtio *virtio_obj)
{
    uint32_t magic_value = read_u32(virtio_obj, VIRTIO_MAGIC_VALUE);
    uint32_t device_version = read_u32(virtio_obj, VIRTIO_VERSION);
    uint32_t device_id = read_u32(virtio_obj, VIRTIO_DEVICEID);

    uart_printf("%l: magic: %d, version: %d, id: %d\n", virtio_obj->virtio_base, magic_value,device_version, device_id);
    if (magic_value == 0x74726976 && device_version == 0x2 && device_id != 0)
    {
        uart_printf("%l, this address is vaild\n", virtio_obj->virtio_base);
        return 1;
    }
    return 0;
}
void init_virtio_driver(virtio *virtio_obj, struct virtq *queue)
{
    update_status(virtio_obj, VIRTIO_STATUS_RESET); 
    update_status(virtio_obj, VIRTIO_STATUS_ACK); 
    update_status(virtio_obj, VIRTIO_STATUS_ACK | VIRTIO_STATUS_DRIVER); 

    uart_printf("current status %d\n", read_u32(virtio_obj, VIRTIO_STATUS));

    // i am reading the lower half of the feature, but problably have to use sel to set the upper half and read that value too
    // uint32_t device_features = read_u32(virtio_obj, VIRTIO_DeviceFeatures);

    // we read the features from the device reg and write features that we want into the driver features reg
    // we select the upper half by write 1 into featuresel
    // and then we directly write 32-63 bits

    // we are setting 33rd bit, is it enough for basic read/write block operations?
    write_u32(virtio_obj, VIRTIO_DriverFeaturesSel, 1);
    write_u32(virtio_obj, VIRTIO_DriverFeatures, 1 << 1);


    update_status(virtio_obj, VIRTIO_STATUS_ACK | VIRTIO_STATUS_DRIVER | VIRTIO_STATUS_FEATURES_OK); 

    uart_printf("current status %d\n", read_u32(virtio_obj, VIRTIO_STATUS));
    // do we have to re-read the feature bit here?

    // set the value for the queue

    // prepare a request to write to block 100
    // prepare all descriptor table value



    update_status(virtio_obj, VIRTIO_STATUS_ACK | VIRTIO_STATUS_DRIVER | VIRTIO_STATUS_FEATURES_OK | VIRTIO_STATUS_DRIVER_OK ); 
    uart_printf("current status %d\n", read_u32(virtio_obj, VIRTIO_STATUS));


    init_queue(virtio_obj, queue);
}
