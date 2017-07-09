/*
*Name: uart.h
*Author: Rank 
*Contact:<441552318@qq.com>
*/

#ifndef BOARD_H
#define BOARD_H
#include "type.h"

#define CPU_NUM 4

#define GPU_IO_PADDR        0x3e000000
#define GPU_IO_VADDR        0xee000000
#define LOCAL_IO_PADDR      0x40000000
#define LOCAL_IO_VADDR      0xf0000000
#define GPU_IO_SECTIONS     32	
#define LOCAL_IO_SECTIONS  	1

#define UART0_BASE_PADDR   0x3f201000
#define UART0_BASE_VADDR   0xef201000
#define UART_CLK           48000000
#define UART_DR		    0x00 /* data register */
#define UART_RSR_ECR	0x04 /* receive status or error clear */
/* reserved space */
#define UART_FR		0x18 /* flag register */
#define UART_ILPR	0x20 /* IrDA low-poer, not in use*/
#define UART_IBRD	0x24 /* integer baud register */
#define UART_FBRD	0x28 /* fractional baud register */
#define UART_LCR_H	0x2C /* line control register */
#define UART_CR		0x30 /* control register */
#define UART_IFLS	0x34 /* interrupt FIFO level select */
#define UART_IMSC	0x38 /* interrupt mask set/clear */
#define UART_RIS	0x3C /* raw interrupt register */
#define UART_MIS	0x40 /* masked interrupt register */
#define UART_ICR	0x44 /* interrupt clear register */
#define UART_DMACR	0x48 /* DMA control register */
/* flag register bits */
#define UART_FR_DCTS	(1 << 9)
#define UART_FR_RI	    (1 << 8)
#define UART_FR_TXFE	(1 << 7)
#define UART_FR_RXFF	(1 << 6)
#define UART_FR_TXFF	(1 << 5)
#define UART_FR_RXFE	(1 << 4)
#define UART_FR_BUSY	(1 << 3)
#define UART_FR_DCD	    (1 << 2)
#define UART_FR_DSR	    (1 << 1)
#define UART_FR_CTS	    (1 << 0)
/* transmit/receive line register bits */
#define UART_LCRH_SPS		(1 << 7)
#define UART_LCRH_WLEN_8	(3 << 5)
#define UART_LCRH_WLEN_7	(2 << 5)
#define UART_LCRH_WLEN_6	(1 << 5)
#define UART_LCRH_WLEN_5	(0 << 5)
#define UART_LCRH_FEN		(1 << 4)
#define UART_LCRH_STP2		(1 << 3)
#define UART_LCRH_EPS		(1 << 2)
#define UART_LCRH_PEN		(1 << 1)
#define UART_LCRH_BRK		(1 << 0)
/* control register bits */
#define UART_CR_CTSEN		(1 << 15)
#define UART_CR_RTSEN		(1 << 14)
#define UART_CR_RTS		    (1 << 11)
#define UART_CR_DTR		    (1 << 10)
#define UART_CR_RXE		    (1 << 9)
#define UART_CR_TXE		    (1 << 8)
#define UART_CR_LPE		    (1 << 7)
#define UART_CR_UARTEN		(1 << 0)
#define UART_IMSC_RTIM		(1 << 6)
#define UART_IMSC_RXIM		(1 << 4)

#define GPU_TIMER_BASE     0xef003000
#define GPU_TIMER_CS       0x00 /*status&control*/
#define GPU_TIMER_CLO      0x04 /*lower 32bits counter*/
#define GPU_TIMER_CHI      0x08 /*higher 32bits counter*/
#define GPU_TIMER_C0       0x0C /*compare channle 0*/
#define GPU_TIMER_C1       0x10 /*compare channle 1*/
#define GPU_TIMER_C2       0x14 /*compare channle 2*/
#define GPU_TIMER_C3       0x18 /*compare channle 3*/

#define INTERRUPT_BASE     0xef00b000
#define IRQ_BASIC_PENDING  0x200 /*IRQ basic pending*/
#define IRQ_PENDING1       0x204 /*IRQ GPU 0-31 pending, read-only*/
#define IRQ_PENDING2       0x208 /*IRQ GPU 32-63 pending, read-only*/
#define FIQ_CONTROL        0x20c /*FIQ control, only one source at one time*/
#define IRQ_ENABLE1        0x210 /*IRQ GPU 0-31 enable*/
#define IRQ_ENABLE2        0x214 /*IRQ GPU 32-63 enable*/
#define IRQ_BASIC_ENABLE   0x218 /*IRQ basic enable*/
#define IRQ_DISABLE1       0x21c /*IRQ GPU 0-31 disable*/
#define IRQ_DISABLE2       0x220 /*IRQ GPU 32-63 disable*/
#define IRQ_BASIC_DISABLE  0x224 /*IRQ basic disable*/

#define GPU_TIMER_IRQ 0

enum
{
	TIMER_CHANNEL0 = 0,
	TIMER_CHANNEL1,
	TIMER_CHANNEL2,
	TIMER_CHANNEL3,
	GPU_TIMER_CHANNELS
};

#define GPFSEL1 0x3f200004

static inline void writel(uint32_t val, uint32_t addr)
{
	*(volatile uint32_t *)addr = val;
}

static inline uint32_t readl(uint32_t addr)
{
	return *(volatile uint32_t *)addr;
}

#endif

