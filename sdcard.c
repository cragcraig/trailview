#include <avr/io.h>
#include "sdcard.h"
#include "convert.h"
#include "serial.h"

/* ------------- SD CARD LOW LEVEL ACCESS ------------- */

#define MMC_CS 4
#define MMC_MOSI 5
#define MMC_MISO 6
#define MMC_SCK 7
#define CS_ASSERT {PORTB &= ~(1 << MMC_CS);}
#define CS_DEASSERT {PORTB |= (1 << MMC_CS);}

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

/* communicates a byte over SPI */
uint8_t spi_byte(uint8_t byte)
{
	SPDR = byte;
	while(!(SPSR & (1<<SPIF))) ;
	return SPDR;
}

/* sends a command with parameters to the sd card */
void mmc_send_command(uint8_t command, uint32_t param)
{
	union u32convert r;
	r.value = param;

	CS_ASSERT;

	spi_byte(0xff);

	spi_byte(command | 0x40);	// send command

	spi_byte(r.bytes.byte4);	// send parameter, one byte at a time
	spi_byte(r.bytes.byte3);

	spi_byte(r.bytes.byte2);
	spi_byte(r.bytes.byte1);

	spi_byte(0x95);			// CRC for first command, after that ignore
	spi_byte(0xff);			// ignore return
}

/* waits until the sd card returns something other than busy */
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

/* waits until the sd card returns 0xfe */
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


/* final clocks and deassert SS line */
void mmc_release(void)
{
	uint8_t i;

	// at least 8 final clocks
	for(i=0;i<10;i++) spi_byte(0xff);	

	CS_DEASSERT;
}

/* reads a single 512 bytes sector from the SD card */
int mmc_readsector(uint32_t lba, uint8_t *buffer)
{
	uint16_t i;

	// send command and sector
	mmc_send_command(READ_SINGLE_BLOCK, lba<<9);

	if (mmc_datatoken() != 0xfe)	// wait for start of block token
	{
		 mmc_release();	// error
   		return -1;
	}

	for (i=0;i<512;i++)
		*buffer++ = spi_byte(0xff); // read data

	spi_byte(0xff);					// ignore checksum
	spi_byte(0xff);					// ignore checksum

	mmc_release();

	return 0;
}

/* write a single 512 byte sector to the SD card */
unsigned int mmc_writesector(uint32_t lba, uint8_t *buffer)
{
	uint16_t i;
	uint8_t r;
	
	CS_ASSERT;

	// send command and sector
	mmc_send_command(WRITE_SINGLE_BLOCK, lba<<9);
	
	mmc_datatoken();	// get response

	spi_byte(0xfe);	// send start block token

	for (i=0;i<512;i++)	// write sector data
    	spi_byte(*buffer++);

	spi_byte(0xff);	// ignore checksum
	spi_byte(0xff);	// ignore checksum

	r = spi_byte(0xff);
    
	// check for error
	if ((r & 0x1f) != 0x05) return r;
    
	// wait for SD card to complete writing and become idle
	i = 0xffff;
	while (!spi_byte(0xff) && --i) ;	// wait for card to finish writing
    
	mmc_release();	// cleanup

	if (!i) return -1;	// timeout error

	return 0;
}

/* Initialize a mmc/sd card */
uint8_t mmc_init(void)
{
	int i;

	// setup I/O ports 
	PORTB &= ~((1 << MMC_SCK) | (1 << MMC_MOSI));
	PORTB |= (1 << MMC_MISO);
	DDRB  |= (1<<MMC_SCK) | (1<<MMC_MOSI);


	PORTB |= (1 << MMC_CS);	
	DDRB  |= (1 << MMC_CS);

	SPCR = (1<<MSTR)|(1<<SPE)|2;	// enable SPI interface
	SPSR = 0;			// set/disable double speed

	for(i=0;i<10;i++)			// send 80 clocks
		spi_byte(0xff);

	mmc_send_command(GO_IDLE_STATE,0);	// reset card

	if (mmc_get() != 1)			// error if bad/no response code
	{
	   mmc_release();
	   return 1;
	}

	// wait for initialization to finish (gets a 0 back)
	i = 0xffff;
	while ((spi_byte(0xff) != 0) && (--i))
	{
	     mmc_send_command(SEND_OP_COND,0);	// init card
	}
	
	if (!i)	return 2; // timed out above
	
	mmc_send_command(SET_BLOCK_LEN, 512);	//set block size to 512
	
	mmc_release();
	
	// increase SPI clock to (Fosc/2)
	SPCR &= ~3;
	SPSR = 1;

	return 0;
}

