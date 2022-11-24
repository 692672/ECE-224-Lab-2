/*

 * "Hello World" example.
 *
 * This example prints 'Hello from Nios II' to the STDOUT stream. It runs on
 * the Nios II 'standard', 'full_featured', 'fast', and 'low_cost' example
 * designs. It runs with or without the MicroC/OS-II RTOS and requires a STDOUT
 * device in your system's hardware.
 * The memory footprint of this hosted application is ~69 kbytes by default
 * using the standard reference design.
 *
 * For a reduced footprint version of this template, and an explanation of how
 * to reduce the memory footprint for a given application, see the
 * "small_hello_world" template.
 *
 */

/*=========================================================================*/
/*  Includes                                                               */
/*=========================================================================*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <string.h>

#include <system.h>
#include <sys/alt_alarm.h>
#include <io.h>

#include "fatfs.h"
#include "diskio.h"

#include "ff.h"
#include "monitor.h"
#include "uart.h"

#include "alt_types.h"

#include <altera_up_avalon_audio.h>
#include <altera_up_avalon_audio_and_video_config.h>
#include "altera_avalon_pio_regs.h"
#include "altera_avalon_timer_regs.h"
#include "sys/alt_irq.h"
#include "altera_avalon_timer.h"
/*=========================================================================*/
/*  DEFINE: All Structures and Common Constants                            */
/*=========================================================================*/

#define normalSpeed 0
#define halfSpeed 1
#define doubleSpeed 2
#define normalSpeedMono 3

/*=========================================================================*/
/*  DEFINE: Macros                                                         */
/*=========================================================================*/

#define PSTR(_a)  _a

/*=========================================================================*/
/*  DEFINE: Prototypes                                                     */
/*=========================================================================*/

/*=========================================================================*/
/*  DEFINE: Definition of all local Data                                   */
/*=========================================================================*/
static alt_alarm alarm;
static unsigned long Systick = 0;
static volatile unsigned short Timer; /* 1000Hz increment timer */

/*=========================================================================*/
/*  DEFINE: Definition of all local Procedures                             */
/*=========================================================================*/
//global variables
char filename[20][20];
unsigned long fileSize[20];
//
int flag = 0;
int length = 0;
int song_index = 0;
int playing = 0;
//pause = 0, play =1
int paused_play = 0;
int down_val = 0;
//for testing, im setting it to 0;
int stop_flag = 1;
int change_flag = 0;
int switch_val = 0;
int speed = 0;
int LCDcontrol = 0x30; //controls flags in LCDdisplay()
// 110000 => STOP
// 010000 => PAUSED
// 001000 => MONO
// 000100 => DOUBLE
// 000010 => HALF
// 000001 => NORMAL

// timer sends interrupt back
static void timer_ISR(void* context, alt_u32 id) {
	//reading the value of the button
	int button_pressed = IORD(BUTTON_PIO_BASE, 0);

	if (button_pressed == 7) {
		down_val = 7;
	}
	if (button_pressed == 15) {

		// previous
		if (down_val == 7) {
			change_flag = 1;
			song_index = ((length + song_index - 1) % length);
			LCDdisplay(song_index, LCDcontrol);
			//printf("Value: %d\n", stop_flag);
			down_val = 0;

		}
	}

	if (button_pressed == 11) {
				down_val = 11;
			}
	if (button_pressed == 15) {
		//stopped
		if (down_val == 11) {
			stop_flag = 1;
			LCDcontrol ^= 0x20;
			LCDdisplay(song_index, LCDcontrol);
			//printf("Value: %d\n", stop_flag);
			down_val = 0;
		}
	}

	if (button_pressed == 13) {
		down_val = 13;
	}
	if (button_pressed == 15) {
		//play and pause mode
		if (down_val == 13) {
			paused_play = !paused_play;
			if(stop_flag == 1){
				stop_flag = 0;
				paused_play = 1;
			}
			//if(paused_play == 0){
			LCDcontrol ^= 0x10;
			LCDdisplay(song_index, LCDcontrol);
			//}
			printf("Playing: %d\n", paused_play);
			down_val = 0;
		}
	}

	if (button_pressed == 14) {
			down_val = 14;
		}
	if (button_pressed == 15) {
		// next
		if (down_val == 14) {
			change_flag = 1;
			song_index = ((song_index + 1) % length);
			LCDdisplay(song_index, LCDcontrol);
			//printf("Value: %d\n", stop_flag);
			down_val = 0;
		}
	}

	IOWR(TIMER_0_BASE, 1, 0);
	IOWR(BUTTON_PIO_BASE, 3, 0x0);

	// Enable the interrupt

//	//PBACK
//	if (flag == 0 && IORD(BUTTON_PIO_BASE, 0) == 0x7) {
//		flag = 1;
//	}
//	//STOP
//	if (flag == 0 && IORD(BUTTON_PIO_BASE, 0) == 0xb) {
//		flag = 2;
//	}
//	//PLAY/PAUSE
//	if (flag == 0 && IORD(BUTTON_PIO_BASE, 0) == 0xd) {
//		flag = 3;
//	}
//	//NEXT SONG
//	if (flag == 0 && IORD(BUTTON_PIO_BASE, 0) == 0xe) {
//		flag = 4;
//	}

}

// button pressed
static void button_ISR(void* context, alt_u32 id) {
	// Start timer/enables ITO
	IOWR(TIMER_0_BASE, 1, 0b101);

	// Disable the interrupt (prevents duplicates)
	IOWR(BUTTON_PIO_BASE, 3, 0x0);

}

/***************************************************************************/
/*  TimerFunction                                                          */
/*                                                                         */
/*  This timer function will provide a 10ms timer and                      */
/*  call ffs_DiskIOTimerproc.                                              */
/*                                                                         */
/*  In    : none                                                           */
/*  Out   : none                                                           */
/*  Return: none                                                           */
/***************************************************************************/
static alt_u32 TimerFunction(void *context) {
	static unsigned short wTimer10ms = 0;

	(void) context;

	Systick++;
	wTimer10ms++;
	Timer++; /* Performance counter for this module */

	if (wTimer10ms == 10) {
		wTimer10ms = 0;
		ffs_DiskIOTimerproc(); /* Drive timer procedure of low level disk I/O module */
	}

	return (1);
} /* TimerFunction */

/***************************************************************************/
/*  IoInit                                                                 */
/*                                                                         */
/*  Init the hardware like GPIO, UART, and more...                         */
/*                                                                         */
/*  In    : none                                                           */
/*  Out   : none                                                           */
/*  Return: none                                                           */
/***************************************************************************/
static void IoInit(void) {
	uart0_init(115200);

	/* Init diskio interface */
	ffs_DiskIOInit();

	//SetHighSpeed();

	/* Init timer system */
	alt_alarm_start(&alarm, 1, &TimerFunction, NULL);

} /* IoInit */

/*=========================================================================*/
/*  DEFINE: All code exported                                              */
/*=========================================================================*/

uint32_t acc_size; /* Work register for fs command */
uint16_t acc_files, acc_dirs;
FILINFO Finfo;
#if _USE_LFN
char Lfname[512];
#endif

char Line[256]; /* Console input buffer */

FATFS Fatfs[_VOLUMES]; /* File system object for each logical drive */
FIL File1, File2; /* File objects */
DIR Dir; /* Directory object */
uint8_t Buff[1024] __attribute__ ((aligned(4))); /* Working buffer */

int fifospace;
char *ptr, *ptr2;
long p1, p2, p3;
uint8_t res, b1, drv = 0;
uint16_t w1;
uint32_t s1, s2, cnt, blen = sizeof(Buff);
static const uint8_t ft[] = { 0, 12, 16, 32 };
uint32_t ofs = 0, sect = 0, blk[2];
FATFS *fs; /* Pointer to file system object */
int song_index;

alt_up_audio_dev * audio_dev;
/* used for audio record/playback */
unsigned int l_buf;
unsigned int r_buf;

static FRESULT scan_files(char *path) {
	DIR dirs;
	FRESULT res;
	uint8_t i;
	char *fn;

	if ((res = f_opendir(&dirs, path)) == FR_OK) {
		i = (uint8_t) strlen(path);
		while (((res = f_readdir(&dirs, &Finfo)) == FR_OK) && Finfo.fname[0]) {
			if (_FS_RPATH && Finfo.fname[0] == '.')
				continue;
#if _USE_LFN
			fn = *Finfo.lfname ? Finfo.lfname : Finfo.fname;
#else
			fn = Finfo.fname;
#endif
			if (Finfo.fattrib & AM_DIR) {
				acc_dirs++;
				*(path + i) = '/';
				strcpy(path + i + 1, fn);
				res = scan_files(path);
				*(path + i) = '\0';
				if (res != FR_OK)
					break;
			} else {
				//      xprintf("%s/%s\n", path, fn);
				acc_files++;
				acc_size += Finfo.fsize;
			}
		}
	}

	return res;
}

//                put_rc(f_mount((uint8_t) p1, &Fatfs[p1]));

static
void put_rc(FRESULT rc) {
	const char *str =
			"OK\0" "DISK_ERR\0" "INT_ERR\0" "NOT_READY\0" "NO_FILE\0" "NO_PATH\0"
					"INVALID_NAME\0" "DENIED\0" "EXIST\0" "INVALID_OBJECT\0" "WRITE_PROTECTED\0"
					"INVALID_DRIVE\0" "NOT_ENABLED\0" "NO_FILE_SYSTEM\0" "MKFS_ABORTED\0" "TIMEOUT\0"
					"LOCKED\0" "NOT_ENOUGH_CORE\0" "TOO_MANY_OPEN_FILES\0";
	FRESULT i;

	for (i = 0; i != rc && *str; i++) {
		while (*str++)
			;
	}
	xprintf("rc=%u FR_%s\n", (uint32_t) rc, str);
}

static
void display_help(void) {
	xputs("dd <phy_drv#> [<sector>] - Dump sector\n"
			"di <phy_drv#> - Initialize disk\n"
			"ds <phy_drv#> - Show disk status\n"
			"bd <addr> - Dump R/W buffer\n"
			"be <addr> [<data>] ... - Edit R/W buffer\n"
			"br <phy_drv#> <sector> [<n>] - Read disk into R/W buffer\n"
			"bf <n> - Fill working buffer\n"
			"fc - Close a file\n"
			"fd <len> - Read and dump file from current fp\n"
			"fe - Seek file pointer\n"
			"fi <log drv#> - Force initialize the logical drive\n"
			"fl [<path>] - Directory listing\n"
			"fo <mode> <file> - Open a file\n"
			"fp -  (to be added by you) \n"
			"fr <len> - Read file\n"
			"fs [<path>] - Show logical drive status\n"
			"fz [<len>] - Get/Set transfer unit for fr/fw commands\n"
			"h view help (this)\n");
}

/*  Functions (isWav, Song Index and LCD Display)                          */
/*=========================================================================*/
int isWav(char* filename) {
	unsigned int sz = strlen(filename);
//	xprintf("BIAN: %s %d\n", filename, sz);
	if (sz < 4) {
		return 0;
	}
	if (filename[sz - 4] == '.' && filename[sz - 3] == 'W'
			&& filename[sz - 2] == 'A' && filename[sz - 1] == 'V') {
		return 1;
	}
	return 0;
}

// initialize the song player (disk initialize, open audio port, etc.)
void initPlayer() {
	// open the Audio port
	audio_dev = alt_up_audio_open_dev("/dev/Audio");
	if (audio_dev == NULL)
		alt_printf("Error: could not open audio device \n");
	else
		alt_printf("Opened audio device \n");

	IoInit();
	//di 0
	disk_initialize(0);
	//fi 0
	f_mount(0, &Fatfs[p1]);

//	while (*ptr == ' ')
//		ptr++;
	res = f_opendir(&Dir, 0);

	p1 = s1 = s2 = 0; // otherwise initialize the pointers and proceed.
	int index = 0;

	for (;;) {
		res = f_readdir(&Dir, &Finfo);
		if ((res != FR_OK) || !Finfo.fname[0])
			break;
		if (Finfo.fattrib & AM_DIR) {
			s2++;
		} else {
			s1++;
			p1 += Finfo.fsize;
		}

		if (isWav(&Finfo.fname[0])) {
			strcpy(&filename[index], &(Finfo.fname[0]));
			fileSize[index] = Finfo.fsize;
			printf("%s | %lu\n", filename[index], fileSize[index]);
			++index;
		}
	}

}

void GetInputString(char* entry, int size, FILE * stream) {
	int i;
	int ch = 0;

	for (i = 0; (ch != '\n') && (i < size);) {
		if ((ch = getc(stream)) != '\r') {
			entry[i] = ch;
			i++;
		}
	}
}

// display info on LCD board
// @param song_index: used to retrieve song name and index
// @param bottom: msg to display in lower row
void LCDdisplay(int song_index, int bottom) {
	FILE *lcd;
	static char ch;
	static char entry[4];

	lcd = fopen("/dev/lcd_display", "w");
	fprintf(lcd, "%c%s", 27, "[2J"); // wipe screen before anything else happens
	/* Write some simple text to the LCD. */
	if (lcd != NULL) {
		fprintf(lcd, "\n%d: %s\n", (song_index + 1), filename[song_index]);
		if((bottom & 0x30) == 0x30){
			fprintf(lcd, "STOPPED\n");
		}else if(bottom & 0x10){
			fprintf(lcd, "PAUSED\n");
		}else if(bottom & 0x8){
			fprintf(lcd, "PBACK-MONO\n");
		}else if(bottom & 0x4){
			fprintf(lcd, "PBACK-DBL SPD\n");
		}else if(bottom & 0x2){
			fprintf(lcd, "PBACK-HALF SPD\n");
		}else if(bottom & 0x1){
			fprintf(lcd, "PBACK-NORM SPD\n");
		}
		//fprintf(lcd, "\nThis is the LCD Display.\n");
		//fprintf(lcd, "If you can see this, your LCD is functional.\n");
	}

	fclose(lcd);

	return;
}

/*  Main Function									                       */
/*=========================================================================*/



int main() {

	IOWR(BUTTON_PIO_BASE, 3, 0x0); //clear the edge capture
	IOWR(BUTTON_PIO_BASE, 2, 0xf); // Enable button
	alt_irq_register(BUTTON_PIO_IRQ, (void *) 0, button_ISR); // Register the button ISR
	alt_irq_register(TIMER_0_IRQ, (void *) 0, timer_ISR); // Register the timer ISR
	IOWR(TIMER_0_BASE, 2, 100); // Set period of timer

	printf("Please, select a Player function to continue.\n");
	initPlayer();
	//int song_index = 0;

	// find total number of songs on SD card
	while (strlen(filename[song_index]) != 0) {
		++song_index;
	}

	length = song_index;
	song_index = 0;
	LCDdisplay(song_index, LCDcontrol);

	while (1) {
		change_flag = 0;
		(f_open(&File1, filename[song_index], 1));
		//printf("current song: %s | %d\n", filename[song_index],
		//		fileSize[song_index]);
		ofs = File1.fptr;
		p1 = fileSize[song_index];


		//changing playback speed
		switch_val = IORD(SWITCH_PIO_BASE, 0);
		if(switch_val == 0x3){ // normal
			speed = 4;
			LCDcontrol &= 0x30; // clear channel bits and maintain control bits
			LCDcontrol |= 0x1;
		}else if(switch_val == 0x2){ // half
			speed = 2;
			LCDcontrol &= 0x30; // clear channel bits and maintain control bits
			LCDcontrol |= 0x2;
		}else if(switch_val == 0x1){ // double
			speed = 8;
			LCDcontrol &= 0x30; // clear channel bits and maintain control bits
			LCDcontrol |= 0x4;
		}else if(switch_val == 0x0){ // mono
			LCDcontrol &= 0x30; // clear channel bits and maintain control bits
			LCDcontrol |= 0x8;
		}


		if (stop_flag == 0) {

			// play audio
			while (p1 > 0) {

				if(stop_flag == 1 || change_flag == 1){
					break;
				}

				if (paused_play == 0) {
					continue;
				}



				if (paused_play == 1) {
					LCDcontrol &= 0x1f; // audio is playing => clear STOP bit
					int i = 0;

					if ((uint32_t) p1 >= blen) {
						cnt = blen;
						p1 -= blen;
					} else {
						cnt = p1;
						p1 = 0;
					}
					res = f_read(&File1, Buff, cnt, &cnt);
					if (res != FR_OK) {
						put_rc(res);
						break;
					}
					if (!cnt)
						break;
					// play current audio buffer (MONO mode)
					if(switch_val == 0x0){
						for (; i < cnt; i += 4) {
							l_buf = (uint16_t) ((Buff[i + 3] << 8) | Buff[i + 2]);
							while ((alt_up_audio_write_fifo_space(audio_dev,
									ALT_UP_AUDIO_LEFT)) == 0) {
							}
							while ((alt_up_audio_write_fifo_space(audio_dev,
									ALT_UP_AUDIO_RIGHT)) == 0) {
							}
							alt_up_audio_write_fifo(audio_dev, &l_buf, 1,
									ALT_UP_AUDIO_LEFT);
							alt_up_audio_write_fifo(audio_dev, &l_buf, 1,
									ALT_UP_AUDIO_RIGHT);

						}
						ofs += cnt;
						continue;
					}
					// play current audio buffer
					for (; i < cnt; i += speed) {
							l_buf = (uint16_t) ((Buff[i + 3] << 8) | Buff[i + 2]);
							r_buf = (uint16_t) ((Buff[i + 1] << 8) | Buff[i]);
							while ((alt_up_audio_write_fifo_space(audio_dev,
									ALT_UP_AUDIO_LEFT)) == 0) {
							}
							while ((alt_up_audio_write_fifo_space(audio_dev,
									ALT_UP_AUDIO_RIGHT)) == 0) {
							}
							alt_up_audio_write_fifo(audio_dev, &l_buf, 1,
									ALT_UP_AUDIO_LEFT);
							alt_up_audio_write_fifo(audio_dev, &r_buf, 1,
									ALT_UP_AUDIO_RIGHT);

					}
					ofs += cnt;

				}

			}
			if(p1 <= 0){ // audio fully played/reached completion (if the STOP button was pressed mid execution, then this block is skipped)
				paused_play = 0;
				LCDcontrol ^= 0x10; // enable PAUSED bit
				LCDdisplay(song_index, LCDcontrol);
			}
			stop_flag = 0; // reset to start of while loop
		}

//		if(change_flag == 1){
//			continue;
//		}
//		stop_flag = 1; // reaching end of track
//		paused_play = 0;


	}
	return 0;

}
