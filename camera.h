
#ifndef CAMERA_H
#define CAMERA_H

#include <inttypes.h>

struct fatwrite_t;

// camera functions
void camera_init(void);
void camera_takephoto(const char * fname, struct fatwrite_t * fwrite);
void camera_txbyte(char c);
char camera_response(void);
void camera_rcv_cmd(unsigned char * cmdbuf);
unsigned char camera_rcv(int n);
void camera_snd_cmd(char cmd, char b3, char b4, char b5, char b6);
void camera_sleep(void);

#endif
