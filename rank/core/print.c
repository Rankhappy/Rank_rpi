#include "uart.h"

#define va_start(v,l)   __builtin_va_start(v,l)
#define va_end(v)       __builtin_va_end(v)
#define va_arg(v,l)     __builtin_va_arg(v,l)
#define va_copy(d,s)    __builtin_va_copy(d,s)

typedef __builtin_va_list va_list;

#define putchar(c) uart_putc(c, g_console_base)

static uint32_t g_console_base;

static void print_char(char c)
{
	if(c >= 0 && c <= 9)
	{
		c += 0x30;
	}
	else if(c >= 0xa && c <= 0xf)
	{
		c = c + 'a' - 0xa;
	}
	
	putchar(c);
}

static void print_int(int value, int width)
{
	char c;
	
	if(value == 0)
	{
		while(width-- > 0)
		{
			print_char('0');
		}
		return;
	}
	
	c = value%10;
	
	print_int(value/10, width-1);
	
	print_char(c);
}

static void print_hex(unsigned int value, int width)
{
	char c;
	
	if(value == 0)
	{
		while(width-- > 0)
		{
			print_char('0');
		}
		return;
	}
	
	c = value&0xf;
	
	print_hex(value>>4, width-1);
	
	print_char(c);	
}

static void print_str(char *str)
{
	char *p = str;
	
	while(*p)
	{
		putchar(*p++);
	}
}

static void vprintf(const char *fmt, va_list ap)
{
	char *p = (char *)fmt;
	int width  = -1;
	
	while(*p)
	{
		if(*p =='\n')
		{
			putchar('\r');
			putchar('\n');
			p++;
			continue;
		}
		if(*p == '%' || width >= 0)
		{
			p++;
			if(*p == 'd')
			{
				int value = va_arg(ap, int);
				if(width == -1)
				{
					width = 1;
				}
				print_int(value, width);
				width = -1;
			}
			else if(*p == 's')
			{
				char *str = va_arg(ap, char *);
				print_str(str);
				width = -1;
			}
			else if(*p == 'c')
			{
				char c = va_arg(ap, int);
				width--;
				while(width--)
				{
					putchar(' ');
				}
				width = -1;
				putchar(c);
			}
			else if(*p == 'x')
			{
				unsigned int xvalue = va_arg(ap, int);
				if(width == -1)
				{
					width = 1;
				}
				print_hex(xvalue, width);
				width = -1;
			}
			else
			{
				while(*p >= '0' && *p <= '9')
				{
					if(width == -1)
					{
						width = (*p-0x30);
					}
					else
					{
						width = width*10 + (*p-0x30);
					}
					p++;
				}
				if(width >= 0)
				{
					p--;
					continue;
				}
				putchar(*p);
			}
			p++;
		}
		else
		{
			putchar(*p);
			p++;
		}
	}
}

void printf(const char *fmt, ...)
{
	va_list ap;
	va_start(ap, fmt);
		
	vprintf(fmt, ap);
	
	va_end(ap);
}

void print_init(uint32_t base)
{
	g_console_base = base;
}

