/*
  Minimal 16550 UART driver for COM1.

  Used so ICS-OS can be tested on modern PCs and in QEMU without a VGA
  window (qemu -display none -serial stdio).
*/

#define SERIAL_COM1 0x3F8

static int serial_ready = 0;

void serial_init(void)
{
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
    int spins;

    if (!serial_ready)
        return;

    if (c == '\n')
        serial_putc('\r');

    spins = 0;
    while ((inportb(SERIAL_COM1 + 5) & 0x20) == 0) {
        if (++spins > 100000)
            return;
    }
    outportb(SERIAL_COM1, (unsigned char)c);
};

void serial_puts(const char *s)
{
    if (s == 0)
        return;
    while (*s)
        serial_putc(*s++);
};
