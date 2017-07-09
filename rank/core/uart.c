/*
*Name: uart.c
*Author: Rank 
*Contact:<441552318@qq.com>
*/

/*
* PL011 Uart.
*/

#include "uart.h"
#include "type.h"
#include "board.h"

void uart_flush(uint32_t base)
{
	while(!(readl(base + UART_FR) & UART_FR_TXFE));
}

void uart_init(uint32_t base, uint32_t uart_clk, uint32_t baud_rate)
{
	uint32_t gpsel1;

	writel(0, base + UART_RSR_ECR);

	writel(0, base + UART_CR);

	/*gpio select for uart0*/
	gpsel1 = readl(GPFSEL1);
	gpsel1 &= ~(7<<12);
	gpsel1 |= 4<<12;
	gpsel1 &= ~(7<<15);
	gpsel1 |= 4<<15;
	writel(GPFSEL1, gpsel1);

	if (baud_rate) {
		uint32_t divisor = (uart_clk * 4) / baud_rate;

		writel(divisor >> 6, base + UART_IBRD);
		writel(divisor & 0x3f, base + UART_FBRD);
	}

	writel(UART_LCRH_WLEN_8 /*| UART_LCRH_FEN*/, base + UART_LCR_H); //without fifo.

	writel(UART_IMSC_RXIM | UART_IMSC_RTIM, base + UART_IMSC);
	
	writel(UART_CR_UARTEN | UART_CR_TXE | UART_CR_RXE, base + UART_CR);

	uart_flush(base);
}

void uart_putc(int8_t ch, uint32_t base)
{
	while (readl(base + UART_FR) & UART_FR_TXFF);

	writel(ch, base + UART_DR);
}

#if 0 //for booting debug. if needed, can enable.
void uart_test(void)
{
	uart_init(UART0_BASE_PADDR, UART_CLK, 115200);
	uart_putc('R', UART0_BASE_PADDR);
	uart_putc('a', UART0_BASE_PADDR);
	uart_putc('n', UART0_BASE_PADDR);
	uart_putc('k', UART0_BASE_PADDR);
	uart_putc('\r', UART0_BASE_PADDR);
	uart_putc('\n', UART0_BASE_PADDR);
}
#endif

#if 0 //for booting debug. if needed, can enable.
void _print_rigister (uint32_t r)
{
	unsigned int rb;
	unsigned int rc;

	rb = 32;
	for(;;)
	{
        	rb -= 4;
       		rc = (r >> rb) & 0xF;
        	if(rc > 9) rc += 0x37;
        	else rc+=0x30;
        	uart_putc(rc, UART0_BASE_PADDR);
        	if(rb==0) break;
	}
    
	uart_putc(0x20, UART0_BASE_PADDR);
}
void print_rigister (uint32_t r)
{
	_print_rigister(r);
	uart_putc('\r', UART0_BASE_PADDR);
	uart_putc('\n', UART0_BASE_PADDR);
}

void vprint_rigister (uint32_t r)
{
	_print_rigister(r);
	uart_putc('\r', UART0_BASE_VADDR);
	uart_putc('\n', UART0_BASE_VADDR);
}

#endif

