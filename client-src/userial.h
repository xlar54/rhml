#ifndef _USERIAL_H
#define _USERIAL_H

void us_putc(char c);

char us_getc();

void us_read(char* str, unsigned char len);

void us_printf(const char* format, ... );

void us_aprintf(const char* format, ... );

void us_init1200();

void us_init2400();

void us_close();

#endif