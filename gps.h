#ifndef _GPS_H
#define _GPS_H

#include <inttypes.h>
#include "fat32.h"

struct gps_location {
	char time[16];		//time of GPS data query hhmmss.sss
	char status;		//A=valid V=invalid
	double lat;		//lattitude ddmm.mmmm
	char ns;		//'N'=north 'S'=south
	double lon;		//longintude dddmm.mmmm
	char ew;		//'E'=east 'W'=west
	double sog;		//speed over ground knots
	double cog;		//course over ground degrees 
	char date[16];		//ddmmyy
};

struct gps_displacement {
	double magnitude;
	double initial_bearing;
	double final_bearing;
	double speed;
	char iterations;
};

/* calculates a NMEA checksum */
char gps_calcchecksum(const char * s);

/*This function takes a data string
 *parses it and places it into the 
 *provided gps_location structure
 */
int gps_log_data(char * data, struct gps_location* loc);

/*This function takes the latitudes and longitudes
 *of two GPS locations and returns the displacement
 *using the Vincenty formula, accurate to .5 mm
 */
int gps_calc_disp(struct gps_location * gl1, struct gps_location * gl2, struct gps_displacement * gd);

/*This function converts a lattitude (ddmm.mmmm)
 *or longitude dddmm.mmmm to decimal degrees
 */
double dm_to_dd(double dm, char nsew);	//lat ddmm.mmmm and lon dddmm.mmmm to decimal degrees

/* disables the sending of GPS data we don't need (set rate to 0) */
void gps_disable_unwanted(void);

/* For logging KML data */
void log_start(struct fatwrite_t * fwrite);
void log_end(struct fatwrite_t * fwrite);
void log_add(struct fatwrite_t * fwrite, struct gps_location * gl, struct gps_displacement * gd, const char * img_name);
const char * gps_gen_name(unsigned int n);

#endif
