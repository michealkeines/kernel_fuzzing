# https://syzkaller.appspot.com/bug?extid=8699aa1eeb85ffe5b398

its arm64


 ref_tracker_free+0x6df/0x7d0 lib/ref_tracker.c:244
 netdev_tracker_free include/linux/netdevice.h:4351 [inline]
 netdev_put include/linux/netdevice.h:4368 [inline]
 in_dev_finish_destroy+0x9b/0x190 net/ipv4/devinet.c:258
 in_dev_put include/linux/inetdevice.h:290 [inline]
 inetdev_destroy net/ipv4/devinet.c:338 [inline]
 inetdev_event+0x81c/0x15b0 net/ipv4/devinet.c:1656
 notifier_call_chain+0x1b6/0x3e0 kernel/notifier.c:85
 call_netdevice_notifiers_extack net/core/dev.c:2214 [inline]
 call_netdevice_notifiers net/core/dev.c:2228 [inline]


- started by copying .config from syskaller into linux root
- yes "" | make oldconfig (make everything to default)
- let the kernel build
- give the stacktrace to gpt and get some inital ideas of what it is
- use gpt to understand the actual use case of this code, why have reached this, what is point of reaching here and what higher levels apps may reach this code path
- ask gpt to bash/c that will do the valid code flow, and ask for places in stack flow where we can put debug logs and place those log messages

- in this case, we are working with netdevsim, an network interface simulating module, this is not enabled in normal kernal configs
- we can create and run and test network interfaces from userspace


clang -O2 -g -fsanitize=fuzzer,address -std=c11 fuzz_nsim.c -o fuzz_nsim
