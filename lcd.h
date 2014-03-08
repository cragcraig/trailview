#ifndef LCD_H
#define LCD_H

void lcd_init(void);

/* supports %s, %d, %%, and \n */
void lcd_printf(const char *fmt, ...);

void lcd_wcommand(unsigned char c);

void lcd_wdata(unsigned char c);

void lcd_go_line(char line);

void lcd_go_line_clear(char line);

int lcd_print(const char *s);

int lcd_print_int(signed int i);

void lcd_print_hex(char n);

void lcd_init_seq(void);

#endif
