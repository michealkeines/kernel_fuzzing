


char *data = (char *)kmalloc( sizeof(512));
	i = 0;

	// for (i = 0; i < 512; i++) {
	// 	data[i] = 'K';
	// }
	// i = 11;
	// data[i++] = 'Y';
	// data[i++] = '#';
	// data[i++] = 'J';
	// data[i++] = '#';
	// data[i++] = '3';
	// data[i++] = 'E';

	i = 0;

	uart_printf("1\n");

	struct virtio_blk_req *req = (struct virtio_blk_req *)kmalloc( sizeof(struct virtio_blk_req));

	req->type = VIRTIO_BLK_T_IN;
	req->sector = 1000;
	req->reserved = 0;

	virtio_mb();

	uart_printf("2\n");

	i = 0;

	struct virtq_dec* desc1 = (struct virtq_dec *)(virtio_obj->virtual_virtq_desc + (sizeof(struct virtq_dec) * i++));
	struct virtq_dec* desc2 = (struct virtq_dec *)(virtio_obj->virtual_virtq_desc + (sizeof(struct virtq_dec) * i++));
	struct virtq_dec* desc3 = (struct virtq_dec *)(virtio_obj->virtual_virtq_desc + (sizeof(struct virtq_dec) * i++));

	uint8_t *status = (uint8_t *)kmalloc( sizeof(uint8_t));
	uart_printf("3\n");

	desc1->addr = kernal_to_user_space((uint64_t)req);
	desc1->len = sizeof(struct virtio_blk_req);
	desc1->flags = VIRTIO_DESC_F_NEXT;
	desc1->next = 1;

	desc2->addr = kernal_to_user_space((uint64_t)data);
	desc2->len = 512;
	desc2->flags = VIRTIO_DESC_F_NEXT | VIRTIO_DESC_F_WRITE;
	desc2->next = 2;

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
	virtq_avail_obj->ring[0] = 0;
	virtio_mb();
	virtq_avail_obj->idx = 1;
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
	uart_printf("new index: %d\n", virtq_used_obj->idx);
	uart_printf("used rring 0: %d\n", virtq_used_obj->ring[0].len);
	uart_printf("\nstatus bit2: %d\n", *status);
	for (i = 0; i < 512; i++) {

	uart_printf("%d", data[i]);
	}

