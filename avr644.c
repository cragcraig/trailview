// Craig Harrison
// 640 x 400 px LCD Driver
// Source for AVR2

#include <avr/io.h>
#include <inttypes.h>

#include "convert.h"
#include "serial.h"
#include "sdcard.h"
#include "fat32.h"
#include "lcd.h"

int main(void)
{

	uint32_t i;
	char c;
	char s1[64], s2[64];
	
	uint8_t bufb[512];
	
	init_serial();

	lcd_init();
	
	lcd_printf("Hello World! \n n: %d", 152);

	// init card
	send_str("\ninit mmc card: ");
	c = mmc_init();
	if (c) {
		send_str("failed (");
		send_int(c);
		send_str(")\n");
	} else {
		send_str("success\n");
		
		init_partition(0);
		
		c = 'h';
		
		while (1) {
		
			switch (c) {
				case 'l' :
					ls();
					break;
				
				case 'c' :
					send_str("dir: ");
					receive_str(s1);
					cd(s1);
					break;
					
				case 'r' :
					send_str("rename file: ");
					receive_str(s1);
					send_str("new name: ");
					receive_str(s2);
					rn(s1, s2);
					break;
					
				case 'd' :
					send_str("delete file: ");
					receive_str(s1);
					del(s1);
					break;
					
				case 'p' :
					send_str("file: ");
					receive_str(s1);
					cat(s1);
					break;

				case 't' :
					send_str("name: ");
					receive_str(s1);
					touch(s1);
					break;
					
				case 's' :
					send_str("sector: 0x");
					i = receive_hex();
					if (mmc_readsector(i, bufb)) {
						send_str("error reading sector\n");
					} else {
						dump_sector(bufb);
						send_char('\n');
					}
					break;
				
				case 'h' :
				default :
					send_str("h - help\nl - dir listing\nc - change dir\nd - delete file\np - print file contents\nt - create empty file\ns - dump sector\n");
					break;
			
			}
			
			// prompt
			send_str("> ");
			c = receive_char();
			send_char('\n');
		}
		
	}
	
	while (1) ;
	
	return 0;
}

