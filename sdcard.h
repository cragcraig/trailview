#ifndef SDCARD_H
#define SDCARD_H

#include <inttypes.h>

// sd card functions
uint8_t mmc_init(void);
int mmc_readsector(uint32_t lba, uint8_t *buffer);
unsigned int mmc_writesector(uint32_t lba, uint8_t *buffer);


#endif
