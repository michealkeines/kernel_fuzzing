# checklist

- confirm if we have cleared the interrupt mask in kmain
- check if GIC is enabled in distributor/cpu interface, and we have enabled the correct bit for ppi
- check if Timer is enabled for virtual driver
- check if we have inited all handler in the vector.s