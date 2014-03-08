/* Trailview GPS logger system
 * Craig Harrison & Zach Norrison
 * 12/13/2009
 */
 
#include <stdio.h>
#include <string.h>
#include <inttypes.h>
#include <stdlib.h>
#include "gps.h"
#include "serialgps.h"
#include "lcd.h"
#include "fat32.h"
#include "sdcard.h"
#include "camera.h"

void init_logtoggle(void);
#define CHECK_LOGTOGGLE() (PIND&0x80)

int main (int argc, char* argv[])
{
	struct gps_location gl1 , gl2;
	struct gps_displacement gd;
	struct fatwrite_t fout;
	char logging_state = 0;
	char flag_reset = 0;

	gps_init_serial();
	lcd_init();
	camera_init();
	camera_sleep();
	lcd_printf("sd card:\nconnecting");
	char rt = mmc_init();
	if (rt) {
		lcd_printf("sd card: error\n");
		while (1) ;
	}
	
	init_partition(0);
	init_logtoggle();
	lcd_printf("GPS ...");

	gps_disable_unwanted();

	char in[64];
	int i, img_counter = 0;
	char c = 0;
	char loading_map[] = {'-', '\\', '|', '/'};
	const char * fpic;

	while (1) {

		// wait until valid location
		do {
			receive_str(in);
			gps_log_data(in , &gl1);
			lcd_printf("GPS Fixing %c\n", loading_map[(c++)&0x3]);
		} while (gl1.status != 'A');
	
		// got fix
		lcd_printf("Acquired Fix");

		// compute displacement
		while (1) {
			// read in gps data
			receive_str(in);
			if (flag_reset) {
				// reset waypoint
				gps_log_data(in, &gl1);
				flag_reset = 0;
			}
			i = gps_log_data(in , &gl2);
			
			// end log
			if (logging_state && !CHECK_LOGTOGGLE()) {
					logging_state = 0;
					lcd_printf("log: finishing..\n");
					log_end(&fout);
			}
		
			// check if we have a fix
			if (gl2.status != 'A') {
				lcd_printf("Lost GPS Fix %c\n", loading_map[(c++)&0x3]);
				continue;
			}
		
			// compute and display gps data
			gps_calc_disp(&gl1, &gl2, &gd);
			lcd_printf("I: %d\xb2 F: %d\xb2\nMg: %dm Sp: %d",
				(int)gd.initial_bearing,
				(int)gd.final_bearing,
				(int)gd.magnitude,
				(int)(1.15*gl2.sog + 0.5));
			
			// start / update logging
			if (logging_state) {
				// add to log
				fpic = gps_gen_name(img_counter++);
				camera_init();
				camera_takephoto(fpic, &fout);
				camera_sleep();
				log_add(&fout, &gl2, &gd, fpic);
			} else if (CHECK_LOGTOGGLE()) {
				// start logging
				logging_state = 1;
				flag_reset = 1;
				img_counter = 0;
				log_start(&fout);
			}
		}

	}
	
	return 0;
}

/* initializes the log-toggle switch */
void init_logtoggle(void)
{
	DDRD &= ~0x80;
}

