// Fuzz netdevsim lifecycle from user space using libFuzzer (pure C).
// Drives: create -> (mutated ops) -> delete (optionally double delete).
// Mutates IP/MTU/MAC/order; runs two threads with slight skew to tickle races.
//
// Build: clang -O2 -g -fsanitize=fuzzer,address -std=c11 fuzz_nsim.c -o fuzz_nsim
// Run:   sudo ./fuzz_nsim -runs=0

#define _GNU_SOURCE
#include <stdint.h>
#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <glob.h>
#include <pthread.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>

static inline bool write_sysfs(const char *path, const char *buf, size_t len) {
  int fd = open(path, O_WRONLY | O_CLOEXEC);
  if (fd < 0) return false;
  ssize_t n = write(fd, buf, len);
  close(fd);
  return n == (ssize_t)len;
}

static bool nsim_new(int id, int ports) {
  char b[64];
  int n = snprintf(b, sizeof(b), "%d %d", id, ports);
  return write_sysfs("/sys/bus/netdevsim/new_device", b, (size_t)n);
}
static bool nsim_del(int id) {
  char b[32];
  int n = snprintf(b, sizeof(b), "%d", id);
  return write_sysfs("/sys/bus/netdevsim/del_device", b, (size_t)n);
}

static int sh(const char *cmd) { return system(cmd); }

static void first_nsim_if(char *out, size_t outsz) {
  glob_t g = {0};
  out[0] = 0;
  if (glob("/sys/devices/netdevsim*/net/*", 0, NULL, &g) == 0 && g.gl_pathc > 0) {
    const char *p = g.gl_pathv[0];
    const char *slash = strrchr(p, '/');
    if (slash && slash[1]) snprintf(out, outsz, "%s", slash + 1);
  }
  globfree(&g);
}

static void ip_add(const char *ifn, uint8_t a, uint8_t b, uint8_t c, uint8_t d, uint8_t prefix) {
  char cmd[256];
  snprintf(cmd, sizeof(cmd), "ip addr add %u.%u.%u.%u/%u dev %s 2>/dev/null",
           a, b, c, d, prefix, ifn);
  sh(cmd);
}
static void ip_del(const char *ifn, uint8_t a, uint8_t b, uint8_t c, uint8_t d, uint8_t prefix) {
  char cmd[256];
  snprintf(cmd, sizeof(cmd), "ip addr del %u.%u.%u.%u/%u dev %s 2>/dev/null",
           a, b, c, d, prefix, ifn);
  sh(cmd);
}
static void link_set(const char *ifn, bool up) {
  char cmd[128];
  snprintf(cmd, sizeof(cmd), "ip link set %s %s 2>/dev/null", ifn, up ? "up" : "down");
  sh(cmd);
}
static void set_mtu(const char *ifn, int mtu) {
  char cmd[128];
  snprintf(cmd, sizeof(cmd), "ip link set %s mtu %d 2>/dev/null", ifn, mtu);
  sh(cmd);
}
static void change_mac(const char *ifn, const uint8_t mac[6]) {
  char cmd[256];
  snprintf(cmd, sizeof(cmd),
           "ip link set dev %s address %02x:%02x:%02x:%02x:%02x:%02x 2>/dev/null",
           ifn, mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
  sh(cmd);
}

enum Op : uint8_t {
  OP_CREATE     = 0,
  OP_ADDR_ADD   = 1,
  OP_ADDR_DEL   = 2,
  OP_LINK_UP    = 3,
  OP_LINK_DOWN  = 4,
  OP_DEL        = 5,
  OP_DOUBLE_DEL = 6,
  OP_CHG_MTU    = 7,
  OP_CHG_MAC    = 8,
};

typedef struct {
  uint8_t *bytes;
  size_t   len;
} Slice;

typedef struct {
  const Slice *seq;
  size_t       nseq;
  int          skew_ms;
} RunnerArgs;

static void do_op(const uint8_t *p, size_t n, int default_id) {
  if (n < 1) return;
  uint8_t op = p[0];
  int id = (n > 1 ? p[1] : (uint8_t)default_id) % 32;

  char ifn[64];
  switch (op) {
    case OP_CREATE:
      nsim_new(id, 1 + (n > 2 ? (p[2] % 4) : 0));
      break;
    case OP_ADDR_ADD: {
      first_nsim_if(ifn, sizeof(ifn));
      if (!ifn[0]) { nsim_new(id, 1); first_nsim_if(ifn, sizeof(ifn)); if (!ifn[0]) break; }
      uint8_t a = (n > 2 ? p[2] : 192), b = (n > 3 ? p[3] : 0),
              c = (n > 4 ? p[4] : 2),   d = (n > 5 ? p[5] : 1),
              pr = (n > 6 ? (p[6] % 30 + 1) : 24);
      ip_add(ifn, a, b, c, d, pr);
      break;
    }
    case OP_ADDR_DEL: {
      first_nsim_if(ifn, sizeof(ifn));
      if (!ifn[0]) break;
      uint8_t a = (n > 2 ? p[2] : 192), b = (n > 3 ? p[3] : 0),
              c = (n > 4 ? p[4] : 2),   d = (n > 5 ? p[5] : 1),
              pr = (n > 6 ? (p[6] % 30 + 1) : 24);
      ip_del(ifn, a, b, c, d, pr);
      break;
    }
    case OP_LINK_UP:
      first_nsim_if(ifn, sizeof(ifn));
      if (ifn[0]) link_set(ifn, true);
      break;
    case OP_LINK_DOWN:
      first_nsim_if(ifn, sizeof(ifn));
      if (ifn[0]) link_set(ifn, false);
      break;
    case OP_DEL:
      nsim_del(id);
      break;
    case OP_DOUBLE_DEL:
      nsim_del(id);
      nsim_del(id);
      break;
    case OP_CHG_MTU: {
      first_nsim_if(ifn, sizeof(ifn));
      if (!ifn[0]) break;
      int mtu = 500 + (n > 2 ? (p[2] * 16) : 1000);
      set_mtu(ifn, mtu);
      break;
    }
    case OP_CHG_MAC: {
      first_nsim_if(ifn, sizeof(ifn));
      if (!ifn[0]) break;
      uint8_t mac[6] = {0x02, 0, 0, 0, 0, 0}; // locally administered
      for (int i = 0; i < 5 && 3 + i < (int)n; i++) mac[i+1] = p[3+i];
      change_mac(ifn, mac);
      break;
    }
    default:
      break;
  }
}

static void *runner(void *argp) {
  RunnerArgs *a = (RunnerArgs *)argp;
  if (a->skew_ms < 0) a->skew_ms = 0;
  int default_id = 1;
  for (size_t i = 0; i < a->nseq; i++) {
    do_op(a->seq[i].bytes, a->seq[i].len, default_id);
    if (a->skew_ms) usleep(a->skew_ms * 1000);
  }
  return NULL;
}

int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size) {
  if (geteuid() != 0) return 0;         // needs root
  if (size == 0) return 0;

  size_t i = 0;
  uint8_t count = 1 + (data[i++] % 24); // limit per-input ops

  // Build a structure-aware sequence: force CREATE at start, DEL at end.
  // Parse remaining bytes into small op slices.
  Slice *seq = (Slice *)calloc((size_t)count + 3, sizeof(Slice));
  if (!seq) return 0;

  // Always CREATE first (device id derived from first byte)
  uint8_t head[3] = { OP_CREATE, (uint8_t)(data[0] % 32), 1 };
  seq[0].bytes = (uint8_t *)malloc(sizeof(head));
  memcpy(seq[0].bytes, head, sizeof(head));
  seq[0].len = sizeof(head);

  size_t idx = 1;
  while (i < size && idx < (size_t)count) {
    uint8_t op = data[i++];
    size_t left = size - i;
    size_t take = left > 8 ? 8 : left;      // bound per-op params
    seq[idx].bytes = (uint8_t *)malloc(2 + take);
    if (!seq[idx].bytes) break;
    seq[idx].bytes[0] = op;
    seq[idx].bytes[1] = (uint8_t)(data[0] % 32);  // keep same id for coherence
    memcpy(&seq[idx].bytes[2], &data[i], take);
    seq[idx].len = 2 + take;
    i += take;
    idx++;
  }

  // Always end with DEL; optionally DOUBLE_DEL to stress teardown
  uint8_t del[2] = { OP_DEL, (uint8_t)(data[0] % 32) };
  seq[idx].bytes = (uint8_t *)malloc(sizeof(del));
  memcpy(seq[idx].bytes, del, sizeof(del));
  seq[idx].len = sizeof(del);
  idx++;

  if ((data[0] & 1) == 1) {
    uint8_t d2[2] = { OP_DOUBLE_DEL, (uint8_t)(data[0] % 32) };
    seq[idx].bytes = (uint8_t *)malloc(sizeof(d2));
    memcpy(seq[idx].bytes, d2, sizeof(d2));
    seq[idx].len = sizeof(d2);
    idx++;
  }

  RunnerArgs a1 = { .seq = seq, .nseq = idx, .skew_ms = 0 };
  RunnerArgs a2 = { .seq = seq, .nseq = idx, .skew_ms = 1 };

  pthread_t t1, t2;
  pthread_create(&t1, NULL, runner, &a1);
  pthread_create(&t2, NULL, runner, &a2);
  pthread_join(t1, NULL);
  pthread_join(t2, NULL);

  // cleanup
  for (size_t k = 0; k < idx; k++) free(seq[k].bytes);
  free(seq);
  return 0;
}

