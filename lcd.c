#include <avr/io.h>

#define F_CPU 8E6  // 8 MHz
#include <util/delay.h>

#include <stdarg.h>
#include "lcd.h"

#define LCD_RS 4
#define LCD_RW 5
#define LCD_E 6

#define DATAPORT PORTA
#define DATADDR DDRA
#define CMDPORT PORTD
#define CMDDDR DDRD

#define LCD_SET(bit) {CMDPORT |= (1 << (bit));}
#define LCD_CLR(bit) {CMDPORT &= ~(1 << (bit));}

// define common LCD functions
#define LCD_CLEAR 0x01
#define LCD_INIT_BIGTXT 0x3c
#define LCD_INIT_SMALLTXT 0x38
#define LCD_ENTRYMODE 0x06
#define LCD_SHIFT_LEFT 0x18
#define LCD_SHIFT_RIGHT 0x1c
#define LCD_SCREEN_OFF 0x08
#define LCD_SCREEN_ON 0x0c
#define LCD_CURSOR_LEFT 0x10
#define LCD_CURSOR_RIGHT 0x14
#define LCD_CURSOR_OFF 0x0c
#define LCD_CURSOR_BLINK 0x0d
#define LCD_CURSOR_UNDERSCORE 0x0e
#define LCD_CURSOR_BLINKSCORE 0x0f

#define sleep(tms) {_delay_ms(tms);}

void lcd_init(void)
{
	sleep(10);

	DATADDR = 0xff;
	CMDDDR |= (1 << LCD_RS) | (1 << LCD_RW) | (1 << LCD_E);
		
	DATAPORT = 0;
	LCD_CLR(LCD_RS);
	LCD_CLR(LCD_RW);
	LCD_CLR(LCD_E);

	lcd_init_seq();
}

void wcommand(unsigned char c)
{
	DDRC = 0xff;
	LCD_CLR(LCD_RS);
	LCD_CLR(LCD_RW);
	DATAPORT = 0;
	sleep(1); //5
	LCD_SET(LCD_E);
	asm( "nop" "\n\t" :: );
	DATAPORT = c;
	sleep(5); //5
	LCD_CLR(LCD_E);
	sleep(5); //5
}

void lcd_wdata(unsigned char c)
{
	DDRC = 0xff;
	LCD_SET(LCD_RS);
	LCD_CLR(LCD_RW);
	DATAPORT = c;
	LCD_SET(LCD_E);
	sleep(4); //4
	LCD_CLR(LCD_E);
	sleep(1); //4
}

void lcd_go_line(char line)
{
	wcommand(0x80 | (0x40 * line));
}

void lcd_go_line_clear(char line)
{
	int i;
	lcd_go_line(line);
	for (i = 0; i < 16; i++) lcd_wdata(' ');
	lcd_go_line(line);
}

int lcd_print(const char *s)
{
	int r = 0;
	while (*s) {
		lcd_wdata(*s++);
		r++;
	}
	
	return r;
}

/* supports %s, %d, %%, and \n */
void lcd_printf(const char *fmt, ...)
{
	lcd_go_line(0);
	char l = 0, i = 0;

	// var args
	va_list ap;
	va_start(ap, fmt);
	
	while (*fmt) {		
		switch (*fmt) {
		case '%':
			switch (*++fmt) {
				case 's': i += lcd_print(va_arg(ap, char*));
					break;

				case 'd': i += lcd_print_int(va_arg(ap, int));
					break;

				case 'c': lcd_wdata(va_arg(ap, int));
					i++;
					break;

				case '\0': fmt--;
					break;

				default: lcd_wdata(*fmt);
					i++;
					break;
			}
			break;

		case '\n': while (i++ < 16) lcd_wdata(' '); // clear remainder of line
			lcd_go_line(++l);
			i = 0;
			break;
		
		default: lcd_wdata(*fmt);
			i++;
			break;
		}
		fmt++;
	}
	
	// clear remainder of line
	while (i++ < 16) lcd_wdata(' ');

	va_end(ap);
}

int lcd_print_int(signed int i)
{
	char string[7];
	char tempnum[5];
	char *pChar = &string[0];
	char j = 0, r;
	if (i < 0) {
		*pChar++ = '-';
		i = -i;
	}
	while (i || !j) {
		tempnum[(int)j] = i % 10;
		i /= 10;
		j++;
	}
	r = j;
	while (j--) *pChar++ = tempnum[(int)j] + '0';
	*pChar = 0;
	lcd_print(&string[0]);
	
	return r;
}

void lcd_print_hex(char n)
{
	unsigned char t;
	t = (n>>4)&0xf;
	lcd_wdata(t + (t < 10 ? '0' : 'a' - 10));
	t = n&0xf;
	lcd_wdata(t + (t < 10 ? '0' : 'a' - 10));
}

void lcd_init_seq(void)
{
	sleep(30); // wait at least 15 ms
	
	// eight bit interface
	wcommand(0x30);
	sleep(6);
	wcommand(0x30);
	sleep(6);
	wcommand(0x30);
	sleep(6);
	
	wcommand(LCD_INIT_BIGTXT);

	wcommand(LCD_SCREEN_OFF);
	
	wcommand(LCD_ENTRYMODE); // set entry mode
	wcommand(LCD_CURSOR_OFF); // cursor type and display on
	wcommand(LCD_CLEAR); // clear display
}

