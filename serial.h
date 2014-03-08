
#ifndef SERIAL_H
#define SERIAL_H

#include <inttypes.h>

// serial functions
void init_serial(void);
void send_int(unsigned int n);
void send_hex(unsigned int n);
void send_hexbyte(unsigned char n);
void send_str(const char * s);
void send_nstr(const char * s, int len);
inline void send_char(char c);

void send_long(uint32_t n);
uint32_t receive_long(void);

inline char receive_char(void);
int receive_int(void);
int receive_hex(void);
void receive_str(char * buf);

void dump_sector(const unsigned char * p);

#endif
