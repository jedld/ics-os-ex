#ifndef ICSOS_SERIAL_H
#define ICSOS_SERIAL_H

void serial_init(void);
void serial_putc(char c);
void serial_puts(const char *s);
int serial_getc(void);

#endif
