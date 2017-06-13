#include "uart.h"
#include "type.h"

#define GPFSEL1 0x3f200004

static inline void writel(uint32_t val, uint32_t addr)
{
	*(volatile uint32_t *)addr = val;
}

static inline uint32_t readl(uint32_t addr)
{
	return *(volatile uint32_t *)addr;
}

void uart_flush(uint32_t base)
{
	while (!(readl(base + UART_FR) & UART_FR_TXFE));
}

void uart_init(uint32_t base, uint32_t uart_clk, uint32_t baud_rate)
{
	writel(0, base + UART_RSR_ECR);

	writel(0, base + UART_CR);

	uint32_t ra=readl(GPFSEL1);
    ra&=~(7<<12); //gpio14
    ra|=4<<12;    //alt0
    ra&=~(7<<15); //gpio15
    ra|=4<<15;    //alt0
    writel(GPFSEL1,ra);

	if (baud_rate) {
		uint32_t divisor = (uart_clk * 4) / baud_rate;

		writel(divisor >> 6, base + UART_IBRD);
		writel(divisor & 0x3f, base + UART_FBRD);
	}

	writel(UART_LCRH_WLEN_8 /*| UART_LCRH_FEN*/, base + UART_LCR_H);

	writel(UART_IMSC_RXIM | UART_IMSC_RTIM, base + UART_IMSC);
	
	writel(UART_CR_UARTEN | UART_CR_TXE | UART_CR_RXE, base + UART_CR);

	uart_flush(base);
}

void uart_putc(int8_t ch, uint32_t base)
{
	while (readl(base + UART_FR) & UART_FR_TXFF);

	writel(ch, base + UART_DR);
}

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

void hexstrings (unsigned int d)
{
    //unsigned int ra;
    unsigned int rb;
    unsigned int rc;

    rb=32;
    while(1)
    {
        rb-=4;
        rc=(d>>rb)&0xF;
        if(rc>9) rc+=0x37; else rc+=0x30;
        uart_putc(rc, UART0_BASE_PADDR);
        if(rb==0) break;
    }
    uart_putc(0x20, UART0_BASE_PADDR);
}
//------------------------------------------------------------------------

void hexstring (unsigned int d)
{
    hexstrings(d);
	uart_putc('\r', UART0_BASE_PADDR);
	uart_putc('\n', UART0_BASE_PADDR);
}


