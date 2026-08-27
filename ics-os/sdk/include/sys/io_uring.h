#ifndef _SYS_IO_URING_H
#define _SYS_IO_URING_H

#include <stdint.h>

#define IORING_OP_NOP     0
#define IORING_OP_READV   1
#define IORING_OP_WRITEV  2
#define IORING_OP_FSYNC   3
#define IORING_OP_OPENAT  18
#define IORING_OP_CLOSE   19
#define IORING_OP_READ    22
#define IORING_OP_WRITE   23

#define IORING_ENTER_GETEVENTS 1

struct io_uring_sqe {
   uint8_t opcode;
   uint8_t flags;
   uint16_t ioprio;
   int32_t fd;
   uint64_t off;
   uint64_t addr;
   uint32_t len;
   uint32_t rw_flags;
   uint64_t user_data;
   uint16_t buf_index;
   uint16_t personality;
   uint32_t splice_fd_in;
   uint64_t addr3;
   uint64_t pad2;
};

struct io_uring_cqe {
   uint64_t user_data;
   int32_t res;
   uint32_t flags;
};

struct io_sqring_offsets {
   uint32_t head, tail, ring_mask, ring_entries, flags, dropped, array, resv1;
   uint64_t user_addr;
};

struct io_cqring_offsets {
   uint32_t head, tail, ring_mask, ring_entries, overflow, cqes, flags, resv1;
   uint64_t user_addr;
};

struct io_uring_params {
   uint32_t sq_entries;
   uint32_t cq_entries;
   uint32_t flags;
   uint32_t sq_thread_cpu;
   uint32_t sq_thread_idle;
   uint32_t features;
   uint32_t wq_fd;
   uint32_t resv[3];
   struct io_sqring_offsets sq_off;
   struct io_cqring_offsets cq_off;
};

struct io_uring {
   int ring_fd;
   unsigned sq_entries;
   unsigned cq_entries;
   uint32_t *sq_head;
   uint32_t *sq_tail;
   uint32_t *sq_mask;
   uint32_t *sq_array;
   struct io_uring_sqe *sqes;
   uint32_t *cq_head;
   uint32_t *cq_tail;
   uint32_t *cq_mask;
   struct io_uring_cqe *cqes;
};

int io_uring_setup(unsigned entries, struct io_uring_params *p);
int io_uring_enter(int fd, unsigned to_submit, unsigned min_complete, unsigned flags);
int io_uring_queue_init(unsigned entries, struct io_uring *ring, unsigned flags);
void io_uring_queue_exit(struct io_uring *ring);
struct io_uring_sqe *io_uring_get_sqe(struct io_uring *ring);
int io_uring_submit(struct io_uring *ring);
int io_uring_submit_and_wait(struct io_uring *ring, unsigned wait_nr);
int io_uring_wait_cqe(struct io_uring *ring, struct io_uring_cqe **cqe);
void io_uring_cqe_seen(struct io_uring *ring, struct io_uring_cqe *cqe);

#endif
