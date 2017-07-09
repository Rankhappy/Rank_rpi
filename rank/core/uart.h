/*
*Name: uart.h
*Author: Rank 
*Contact:<441552318@qq.com>
*/

#ifndef UART_H
#define UART_H

#include "type.h"

extern void uart_flush(uint32_t);
extern void uart_init(uint32_t, uint32_t, uint32_t);
extern void uart_putc(int8_t, uint32_t);

#endif
