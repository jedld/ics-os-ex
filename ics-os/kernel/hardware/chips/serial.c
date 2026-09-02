/*
  Minimal 16550 UART driver for COM1.

  Used so ICS-OS can be tested on modern PCs and in QEMU without a VGA
  window (qemu -display none -serial stdio).
*/

#include "../../cpu/spinlock.h"

#define SERIAL_COM1 0x3F8

static int serial_ready = 0;
static spinlock_t serial_lock;
static volatile int serial_owner = -1;

typedef struct serial_guard {
    spin_irq_flags_t flags;
    int locked;
} serial_guard;

extern unsigned int lapic_get_id(void);
extern volatile unsigned int *lapic_mmio;

static serial_guard serial_guard_acquire(void)
{
    serial_guard guard;
    int cpu = lapic_mmio ? (int)lapic_get_id() : 0;
    unsigned spins = 0;

    __asm__ __volatile__("pushfq; popq %0; cli"
                         : "=r"(guard.flags) : : "memory");
    guard.locked = 0;
    if (serial_lock.locked && serial_owner == cpu)
        return guard;
    while (!__sync_bool_compare_and_swap(&serial_lock.locked, 0, 1)) {
        if (++spins > 100000)
            return guard;
        __asm__ __volatile__("pause");
    }
    serial_owner = cpu;
    guard.locked = 1;
    return guard;
}

static void serial_guard_release(serial_guard guard)
{
    if (guard.locked) {
        serial_owner = -1;
        spin_unlock(&serial_lock);
    }
    if (guard.flags & (1ULL << 9))
        __asm__ __volatile__("sti" : : : "memory");
}

static void serial_putc_raw(char c)
{
    int spins = 0;

    while ((inportb(SERIAL_COM1 + 5) & 0x20) == 0) {
        if (++spins > 100000)
            return;
    }
    outportb(SERIAL_COM1, (unsigned char)c);
}

void serial_init(void)
{
    spin_init(&serial_lock);
    serial_owner = -1;
    outportb(SERIAL_COM1 + 1, 0x00);    /* disable UART interrupts */
    outportb(SERIAL_COM1 + 3, 0x80);    /* enable DLAB */
    outportb(SERIAL_COM1 + 0, 0x01);    /* 115200 baud */
    outportb(SERIAL_COM1 + 1, 0x00);
    outportb(SERIAL_COM1 + 3, 0x03);    /* 8N1 */
    outportb(SERIAL_COM1 + 2, 0xC7);    /* enable FIFO */
    outportb(SERIAL_COM1 + 4, 0x0B);    /* IRQs enabled, RTS/DSR set */
    serial_ready = 1;
};

void serial_putc(char c)
{
    serial_guard guard;

    if (!serial_ready)
        return;

    guard = serial_guard_acquire();
    if (c == '\n')
        serial_putc_raw('\r');
    serial_putc_raw(c);
    serial_guard_release(guard);
};

void serial_puts(const char *s)
{
    serial_guard guard;

    if (s == 0)
        return;
    if (!serial_ready)
        return;
    guard = serial_guard_acquire();
    while (*s) {
        if (*s == '\n')
            serial_putc_raw('\r');
        serial_putc_raw(*s++);
    }
    serial_guard_release(guard);
};

int serial_getc(void)
{
    if (!serial_ready)
        return -1;
    if ((inportb(SERIAL_COM1 + 5) & 1) == 0)
        return -1;
    return (int)inportb(SERIAL_COM1);
};
