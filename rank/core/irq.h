/*
*Name: irq.h
*Author: Rank 
*Contact:<441552318@qq.com>
*/

#ifndef IRQ_H
#define IRQ_H

#include "type.h"

typedef void (*irq_handle_t)(void);

extern void irq_init(void);
extern void enable_irq(int);
extern void disable_irq(int);
extern int irq_register(int, irq_handle_t);
extern void irq_dispatch(void);

#endif

