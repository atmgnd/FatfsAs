#include "ff.h"
#include <unistd.h>
#include <stdlib.h>
#include <util/delay.h>
#include <avr/pgmspace.h>
#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>

#include "diskio.h" // TODO REMOVE
/* Read a text file and display it */

#define LED_CONFIG	(DDRD |= (1<<6))
#define LED_ON		(PORTD &= ~(1<<6))
#define LED_OFF		(PORTD |= (1<<6))
#define CPU_PRESCALE(n)	(CLKPR = 0x80, CLKPR = (n))

ISR(TIMER0_COMPA_vect)
{
	disk_timerproc();	/* Drive timer procedure of low level disk I/O module */
}

int main(void)
{
	FIL fil;        /* File object */
// 	char line[100]; /* Line buffer */
	FRESULT fr;     /* FatFs return code */
	FATFS FatFs;   /* Work area (filesystem object) for logical drive */

	CPU_PRESCALE(0);
	LED_CONFIG;
	LED_ON;

	MCUCR = _BV(JTD); MCUCR = _BV(JTD);	/* Disable JTAG */

/* Start 100Hz system timer with TC0 */
	OCR0A = F_CPU / 1024 / 100 - 1;
	TCCR0A = _BV(WGM01);
	TCCR0B = 0b101;
	TIMSK0 = _BV(OCIE0A);

	sei();

	_delay_ms(5000); 

	/* Register work area to the default drive */
	fr = f_mount(&FatFs, "", 1);
	if (fr) {
		goto _hiber;
	}

	/* Open a text file */
	fr = f_open(&fil, "test.txt", FA_READ | FA_WRITE | FA_OPEN_APPEND);
	if (fr) {
		goto _hiber;
	}

	fr = f_puts("today is 2018\n", &fil);

	char buf[128];

	// Show disk status
	{
		uint32_t p2;

		if (disk_ioctl(DRV_MMC, GET_SECTOR_COUNT, &p2) == RES_OK) {
			f_printf(&fil, "Drive size: %lu sectors\n", p2);
		}
		if (disk_ioctl(DRV_MMC, GET_BLOCK_SIZE, &p2) == RES_OK) {
			f_printf(&fil, "Erase block: %lu sectors\n", p2);
		}
		if (disk_ioctl(DRV_MMC, ATA_GET_MODEL, buf) == RES_OK) {
			buf[40] = '\0'; f_printf(&fil, "Model: %s\n", buf);
		}
		if (disk_ioctl(DRV_MMC, ATA_GET_SN, buf) == RES_OK) {
			buf[20] = '\0'; f_printf(&fil, "S/N: %s\n", buf);
		}
	}

	f_close(&fil);

	fr = f_open(&fil, "mark.txt", FA_READ );
	if (fr) {
		goto _hiber;
	}

	unsigned int re;
	fr = f_read(&fil, buf, 16, &re);
	/* Close the file */
	f_close(&fil);

	f_mount(0, "", 0);
	for (uint8_t i = 1;;i++) {
		if (i & 2){
			LED_OFF;
		} else {
			LED_ON;
		}
		_delay_ms(3000);
	}

_hiber:

	for (;;) {
		_delay_ms(10000);
	}

	return 0;
}