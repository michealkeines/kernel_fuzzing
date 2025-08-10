+---------------------+
|  libFuzzer Input #1 |
+---------------------+
          |
          v
  [ user space harness ]
          |
          v
  syscalls to kernel (netdevsim ops)
          |
          v
  [ KCOV enabled in thread ]
          |
          v
  Kernel executes:
    netdevsim_new_device()
    register_netdevice()
    inetdev_event()
          |
          v
  KCOV buffer fills with PCs:
    area[0] = 3
    area[1] = 0xffffffff81012345  <-- netdevsim_new_device start
    area[2] = 0xffffffff81012678  <-- register_netdevice middle
    area[3] = 0xffffffff81012abc  <-- inetdev_event handler
          |
          v
  Harness loop:
    for each PC in area[1..n]:
        __sanitizer_cov_trace_pc_indir(pc)
          |
          v
  libFuzzer coverage map updated
          |
          v
libFuzzer says:
   "Hey! New PC address seen!"
   -> Keep this input in corpus
   -> Mutate it to try to find more new PCs
