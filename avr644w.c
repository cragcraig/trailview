// Craig Harrison
// 640 x 400 px LCD Driver
// Source for AVR2

#include <avr/io.h>
#include <compat/ina90.h>
#include <inttypes.h>

#include "convert.h"

// serial functions
void init_serial(void);
void send_int(unsigned int n);
void send_hex(unsigned int n);
void send_str(const char * s);
inline void send_char(char c);
inline char receive_char(void);
int receive_int(void);
int receive_hex(void);

// sd card functions
uint8_t mmc_init(void);
int mmc_readsector(uint32_t lba, uint8_t *buffer);
unsigned int mmc_writesector(uint32_t lba, uint8_t *buffer);

int main(void)
{

	unsigned int i;
	char c;
	
	uint8_t buf[512], bufb[512];
	
	init_serial();
	
	// init card
	send_str("\ninit mmc card: ");
	c = mmc_init();
	if (c) {
		send_str("failed (");
		send_int(c);
		send_str(")\n");
	} else {
		send_str("success\n");
	
		for (i=0; i<512; i++)
			buf[i] = i;
		
		for (i=0; i<512; i++)
			bufb[i] = 'a';
		
		if (c = mmc_writesector(6, buf)) {
			send_str("error writing sector (");
			send_hex(c);
			send_str(")\n");
		}
		
		
		while (1) {
			send_str("sector to read?\n> ");
			i = receive_int();
		
			if (mmc_readsector(i, bufb))
				send_str("error reading sector\n");
		
			for (i=0; i<15; i++) {
				send_hex(bufb[i]);
				send_char('\n');
			}
		
			send_char('\n');
		}
		
	}
	
	while (1) ;
	
	return 0;
}

void init_serial(void)
{
	UBRR0H = 0;
	UBRR0L = 51; // 19.2k BAUD FOR 8MHZ SYSTEM CLOCK
	UCSR0C = (1<<USBS0)|(3<<UCSZ00);  // 8 BIT NO PARITY 2 STOP
	UCSR0B = (1<<RXEN0)|(1<<TXEN0); // ENABLE TX AND RX ALSO 8 BIT
}

void send_str(const char * s)
{
	while (*s != '\0') send_char(*s++);
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

void send_char(char c)
{
	while ((UCSR0A&(1<<UDRE0)) == 0); // wait until empty
	UDR0 = c;
}

char receive_char(void)
{
	char c;
	while ((UCSR0A&(1<<RXC0)) == 0);  // wait for char
	c = UDR0;
	send_char(c);
	return c;
}

char receive_char_noecho(void)
{
	while ((UCSR0A&(1<<RXC0)) == 0);  // wait for char
	return UDR0;
}

/* ------------- SD CARD LOW LEVEL ACCESS ------------- */

#define MMC_CS 4
#define MMC_MOSI 5
#define MMC_MISO 6
#define MMC_SCK 7
#define CS_ASSERT {PORTB &= ~(1 << MMC_CS);}
#define CS_DEASSERT {PORTB |= (1 << MMC_CS);}

uint8_t spi_byte(uint8_t byte)
{
	SPDR = byte;
	while(!(SPSR & (1<<SPIF))) ;
	return SPDR;
}

/** Send a command to the MMC/SD card.
	\param command	Command to send
	\param px	Command parameter 1
	\param py	Command parameter 2
*/
void mmc_send_command(uint8_t command, uint32_t param)
{
	union u32convert r;
	r.value = param;

	CS_ASSERT;

	spi_byte(0xff);			// dummy byte

	spi_byte(command | 0x40);

	spi_byte(r.bytes.byte1);	// 1st byte of param
	spi_byte(r.bytes.byte2);	// 2nd byte of param

	spi_byte(r.bytes.byte2);	// 3rd byte of param
	spi_byte(r.bytes.byte4);	// 4th byte of param

	spi_byte(0x95);			// correct CRC for first command in SPI          
							// after that CRC is ignored, so no problem with 
							// always sending 0x95                           
	spi_byte(0xff);			// ignore return byte
}


/** Get Token.
	Wait for and return a non-ff token from the MMC/SD card
	\return The received token or 0xFF if timeout
*/
uint8_t mmc_get(void)
{
	uint16_t i = 0xffff;
	uint8_t b = 0xff;

	while ((b == 0xff) && (--i)) 
	{
		b = spi_byte(0xff);
	}
	return b;

}

/** Get Datatoken.
	Wait for and return a data token from the MMC/SD card
	\return The received token or 0xFF if timeout
*/
uint8_t mmc_datatoken(void)
{
	uint16_t i = 0xffff;
	uint8_t b = 0xff;

	while ((b != 0xfe) && (--i)) 
	{
		b = spi_byte(0xff);
	}
	return b;
}


/** Finish Clocking and Release card.
	Send 10 clocks to the MMC/SD card
 	and release the CS line 
*/
void mmc_clock_and_release(void)
{
	uint8_t i;

	// SD cards require at least 8 final clocks
	for(i=0;i<10;i++)
		spi_byte(0xff);	

    CS_DEASSERT;
}

#define GO_IDLE_STATE            0
#define SEND_OP_COND             1
#define SEND_CSD                 9
#define STOP_TRANSMISSION        12
#define SEND_STATUS              13
#define SET_BLOCK_LEN            16
#define READ_SINGLE_BLOCK        17
#define READ_MULTIPLE_BLOCKS     18
#define WRITE_SINGLE_BLOCK       24
#define WRITE_MULTIPLE_BLOCKS    25
#define ERASE_BLOCK_START_ADDR   32
#define ERASE_BLOCK_END_ADDR     33
#define ERASE_SELECTED_BLOCKS    38
#define CRC_ON_OFF               59

/** Read MMC/SD sector.
 	Read a single 512 byte sector from the MMC/SD card
	\param lba	Logical sectornumber to read
	\param buffer	Pointer to buffer for received data
	\return 0 on success, -1 on error
*/
int mmc_readsector(uint32_t lba, uint8_t *buffer)
{
	uint16_t i;

	// send read command and logical sector address
	mmc_send_command(READ_SINGLE_BLOCK, lba<<9);

	if (mmc_datatoken() != 0xfe)	// watch for start of block token
	{
	    mmc_clock_and_release();	// cleanup and	
   		return -1;					// return error code
	}

	for (i=0;i<512;i++)				// read sector data
    	*buffer++ = spi_byte(0xff);

	spi_byte(0xff);					// ignore dummy checksum
	spi_byte(0xff);					// ignore dummy checksum

    mmc_clock_and_release();		// cleanup

	return 0;						// return success		
}




unsigned int mmc_writesector(uint32_t lba, uint8_t *buffer)
{
	uint16_t i;
	uint8_t r;
	
	CS_ASSERT;

	// send read command and logical sector address
	mmc_send_command(WRITE_SINGLE_BLOCK, lba<<9);
	
	mmc_datatoken(); // get response

	spi_byte(0xfe); // send start block token

	for (i=0;i<512;i++)				// read sector data
    	spi_byte(*buffer++);

	spi_byte(0xff);					// ignore dummy checksum
	spi_byte(0xff);					// ignore dummy checksum

    r = spi_byte(0xff);
    
    // check for error
    if ((r & 0x1f) != 0x05) return r;
    
    // wait for SD card to complete writing and become idle
    i = 0xffff;
    while (!spi_byte(0xff) && --i) ; // wait for card to finish writing
    
    mmc_clock_and_release();		// cleanup

	if (!i) return -1;				// timeout error

	return 0;						// return success
}





/** Init MMC/SD card.
	Initialize I/O ports for the MMC/SD interface and 
	send init commands to the MMC/SD card
	\return 0 on success, other values on error 
*/
uint8_t mmc_init(void)
{
	int i;

	// setup I/O ports 

	PORTB &= ~((1 << MMC_SCK) | (1 << MMC_MOSI));	// low bits
	PORTB |= (1 << MMC_MISO);						// high bits
	DDRB  |= (1<<MMC_SCK) | (1<<MMC_MOSI);			// direction


	PORTB |= (1 << MMC_CS);	// Initial level is high	
	DDRB  |= (1 << MMC_CS);	// Direction is output

	SPCR = (1<<MSTR)|(1<<SPE)|2;	// enable SPI interface
	SPSR = 0;					// set double speed

	for(i=0;i<10;i++)			// send 80 clocks while card power stabilizes
		spi_byte(0xff);

	mmc_send_command(GO_IDLE_STATE,0);	// send CMD0 - reset card

	if (mmc_get() != 1)			// if no valid response code
	{
	   mmc_clock_and_release();
	   return 1;  				// card cannot be detected
	}

	//
	// send CMD1 until we get a 0 back, indicating card is done initializing 
	//
	i = 0xffff;						// max timeout
	while ((spi_byte(0xff) != 0) && (--i))	// wait for it
	{
	     mmc_send_command(SEND_OP_COND,0);	// send CMD1 - activate card init
	}
	
	mmc_send_command(SET_BLOCK_LEN, 512); //set block size to 512
	
	mmc_clock_and_release();		// clean up

	if (!i)	return 2;					// return error if we timed out above

	return 0;
}


