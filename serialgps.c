#include <avr/io.h>
#include "serialgps.h"
#include "gps.h"

#define BAUD 103

void gps_init_serial(void)
{
	unsigned int baud = BAUD;
	UBRR0H = (unsigned char)(baud>>8);
	UBRR0L = (unsigned char)baud; 		//BAUD setting set to BAUD
	UCSR0B = (1<<RXEN0)|(1<<TXEN0);		// ENABLE TX AND RX ALSO 8 BIT
	UCSR0C = (3<<UCSZ00);	// 8 BIT NO PARITY 1 STOP
}

void send_gps(const char * s)
{
	char c = gps_calcchecksum(s);
	while (*s != '*' && *s != '\0') send_char(*s++);
	send_char('*');
	send_hexbyte(c);
	send_str("\r\n");
}

void send_str(const char * s)
{
	while (*s) send_char(*s++);
}
/*
void send_nstr(const char * s, int len)
{
	while (*s != '\0' && len--) send_char(*s++);
}

void send_int(unsigned int n)
{
	char buf[12];
	buf[11] = '\0';
	int i = 10;
	do {
		buf[i] = (n % 10) + '0';
		n /= 10;
		i--;
	} while (n && i >= 0);
	send_str(buf + i + 1);
}

int receive_int(void)
{
	char c;
	int r = 0;
	while ((c = receive_char()) != '\n')
		if (c >= '0' && c <= '9') r = r * 10 + c - '0';
	return r;
}

void send_hex(unsigned int n)
{
	unsigned char t;
	char buf[12];
	buf[11] = '\0';
	int i = 10;
	do {
		t = n & 0xf;
		buf[i] = t + (t < 10 ? '0' : 'a' - 10);
		n >>= 4;
		i--;
	} while (n && i >= 0);
	send_str(buf + i + 1);
}

void send_long(uint32_t n)
{
	unsigned char t;
	char buf[12];
	buf[11] = '\0';
	int i = 10;
	do {
		t = n & 0xf;
		buf[i] = t + (t < 10 ? '0' : 'a' - 10);
		n >>= 4;
		i--;
	} while (n && i >= 0);
	send_str(buf + i + 1);
}
*/
void send_hexbyte(unsigned char n)
{
	unsigned char t;
	t = (n>>4)&0xf;
	send_char(t + (t < 10 ? '0' : 'a' - 10));
	t = n&0xf;
	send_char(t + (t < 10 ? '0' : 'a' - 10));
}
/*
void dump_sector(const unsigned char * p)
{
	int i;
	for (i=0; i<512; i++) {
		send_hexbyte(p[i]);
		if ((i&0xf) == 0xf) send_char('\n');
		else send_char(' ');
	}
}

int receive_hex(void)
{
	char c;
	int r = 0;
	while ((c = receive_char()) != '\n') {
		if (c >= '0' && c <= '9') r = (r << 4) | (c - '0');
		else if (c >= 'a' && c <= 'f') r = (r << 4) | (c - 'a' + 10);
	}
	return r;
}

uint32_t receive_long(void)
{
	char c;
	uint32_t r = 0;
	while ((c = receive_char()) != '\n') {
		if (c >= '0' && c <= '9') r = (r << 4) | (c - '0');
		else if (c >= 'a' && c <= 'f') r = (r << 4) | (c - 'a' + 10);
	}
	return r;
}
*/
void receive_str(char * buf)
{
	char c;
	do {
		c = receive_char();	//wait till string starts
	} while (c != '$');
	
	while (1) {
		*buf++ = c;
		c = receive_char();
		if (c == '*') break;			//watch for end
	}
	*buf = '\0';					//NULL terminate that string
}

void send_char(char c)
{
	while ((UCSR0A&(1<<UDRE0)) == 0);		//wait until empty
	UDR0 = c;					//put c into output location
}

char receive_char(void)
{
	char c;
	while ((UCSR0A&(1<<RXC0)) == 0);		//wait for char
	c = UDR0;					//get c from input location
	return c;
}

char receive_char_noecho(void)
{
	while ((UCSR0A&(1<<RXC0)) == 0);  // wait for char
	return UDR0;
}
