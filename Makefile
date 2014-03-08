CC=avr-gcc
#CFLAGS=-g -Os -Wall -mcall-prologues -mmcu=atmega644p -lm
CFLAGS=-Os -Wall -mcall-prologues -mmcu=atmega644p -Wl,-u,vfprintf -lprintf_flt -lm
OBJ2HEX=avr-objcopy
FILES=gpstest.c serialgps.c sdcard.c fat32.c lcd.c gps.c camera.c
#TARGET=avr644
TARGET=gps
ADFLAGS=-p m644p -c usbasp

.PHONY: fuses prog erase


prog:
	$(CC) $(FILES) -o $(TARGET) -O $(CFLAGS)
	$(OBJ2HEX) -R .eeprom -O ihex $(TARGET) $(TARGET).hex
#	$(OBJ2HEX) -j .eeprop -O ihex $(TARGET) $(TARGET).eeprom
	avrdude $(ADFLAGS) -V -F -U flash:w:$(TARGET).hex:i
#	avrdude $(ADFLAGS) -U eeprom:w:$(TARGET).eeprom:i

erase:
	avrdude $(ADFLAGS) -F -e
clean:
	rm -f *.hex *.obj *.o

fuses:
	avrdude $(ADFLAGS) -F -U lfuse:w:0xE2:m #http://www.engbedded.com/cgi-bin/fc.cgi 
	avrdude $(ADFLAGS) -F -U hfuse:w:0x99:m

# avrdude $(ADFLAGS) -F -U lfuse:w:0xE2:m # 8MHz
