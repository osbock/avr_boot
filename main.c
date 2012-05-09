/*-------------------------------------------------------------------------/
/  Stand-alone MMC boot loader  R0.01
/--------------------------------------------------------------------------/
/
/  Copyright (C) 2010, ChaN, all right reserved.
/
/ * This software is a free software and there is NO WARRANTY.
/ * No restriction on use. You can use, modify and redistribute it for
/   personal, non-profit or commercial products UNDER YOUR RESPONSIBILITY.
/ * Redistributions of source code must retain the above copyright notice.
/
/--------------------------------------------------------------------------/
/ Dec 6, 2010  R0.01  First release
/--------------------------------------------------------------------------/
/ This is a stand-alone MMC/SD boot loader for megaAVRs. It requires a 4KB
/ boot section for code, four GPIO pins for MMC/SD as shown in sch.jpg and
/ nothing else. To port the boot loader into your project, follow the
/ instruction sdescribed below.
/
/ 1. Setup the hardware. Attach a memory card socket to the any GPIO port
/    where you like. Select boot size at least 4KB for the boot loader with
/    BOOTSZ fuses and enable boot loader with BOOTRST fuse.
/
/ 2. Setup the software. Change the four port definitions in the asmfunc.S.
/    Change MCU_TARGET, BOOT_ADR and MCU_FREQ in the Makefile. The BOOT_ADR
/    is a BYTE address of boot section in the flash. Build the boot loader
/    and write it to the device with a programmer.
/
/ 3. Build the application program and output it in binary form instead of
/    hex format. Rename the file "app.bin" and put it into the memory card.
/
/ 4. Insert the card and turn the target power on. When the boot loader found
/    the application file, the file is written into the flash memory prior to
/    start the application program. On-board LED lights (if exist) during
/    the flash programming operation.
/
/-------------------------------------------------------------------------*/



#include <avr/io.h>
#include <avr/pgmspace.h>
#include <string.h>
#include "pff.h"


void flash_erase (DWORD);				/* Erase a flash page (asmfunc.S) */
void flash_write (DWORD, const BYTE*);	/* Program a flash page (asmfunc.S) */

FATFS Fatfs;				/* Petit-FatFs work area */
BYTE Buff[SPM_PAGESIZE];	/* Page data buffer */


static uint8_t
pagecmp(uint16_t addr, uint8_t *data)
{
	uint16_t i;

	for (i = 0; i < SPM_PAGESIZE; i++) {
		if (pgm_read_byte(addr++) != *data++)
			return 1;
	}

	return 0;
}


int main (void)
{

	DWORD fa;	/* Flash address */
	WORD br;	/* Bytes read */
	uint8_t i = 0;
	uint8_t ch = 0;

	pf_mount(&Fatfs);	/* Initialize file system */
	

	/* read board name from eeprom to Buff */
	while(i<13) {  //8+'.'+3
	#if defined(__AVR_ATmega168__)  || defined(__AVR_ATmega328P__)
		while(EECR & (1<<EEPE));
		EEAR = (uint16_t)(void *)E2END -i;
		EECR |= (1<<EERE);
		ch =EEDR;
	#else
		ch = eeprom_read_byte((void *)E2END - i);
	#endif
		if( ch == 0xFF) break;
		Buff[i] = ch;
		i++;
	}
	Buff[i] = '\0';

	ch=0;       /* Re-use the variable to save space, now we use it to check if EEPROM or hard-coded file name exists */
	if (i) {	/* File name found in EEPROM */
		if (pf_open(Buff) == FR_OK) { /* File opens normally */		
		ch=1;
		} 
	} 
	else { /* No EEPROM file name found, or the EEPROM file name could not be located on the SD card, so revert tp the hard-coded name */
	if (pf_open("app.bin") == FR_OK) ch=1;
	}
	
	
	if (ch) {	/* Open application file */
		for (fa = 0; fa < BOOT_ADR; fa += SPM_PAGESIZE) {	/* Update all application pages */
			memset(Buff, 0xFF, SPM_PAGESIZE);		/* Clear buffer */
			pf_read(Buff, SPM_PAGESIZE, &br);		/* Load a page data */
								
			if (br) {					/* Bytes Read > 0? */	
				for (i = br; i < SPM_PAGESIZE; i++)     /* Pad the remaining last page with 0xFF so that comparison goes OK */
					Buff[i] = 0xFF;
				if (pagecmp(fa, Buff)) {		/* Only flash if page is changed */
					flash_erase(fa);		/* Erase a page */
					flash_write(fa, Buff);		/* Write it if the data is available */				
				}
			}
		}
	}

	if (pgm_read_word(0) != 0xFFFF)		/* Start application if exist */
		((void(*)(void))0)();

	for (;;) ;	/* No application, Halt. */
}

