#include <termios.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/signal.h>
#include <sys/types.h>
#include <stdlib.h>
#include <string.h>
#include <direct/clock.h>
#include <direct/debug.h>
#include <direct/util.h>
#include <directfb.h>
#include "main.h"
#include "midi.h"
#include "controls.h"
#include "fbgraphics.h"

extern void *bankArray[5];
int main2( int argc, char *argv[] );

int keep_running = 1;

void int_handler(int dummy) {
	keep_running = 0;
}

int main(int argc,char *argv[]) {

	int fd_uart, fd_spi; /* file descriptors for UART-midi and spimega */
	unsigned char inputdata[6]; /* 6 inputs from atmega */
	unsigned char byte, numOfBytes; /* for UART-Midi communication */
	struct bank *currentBank = bankArray[bankA]; /* Bank A selected initially */

	__useconds_t sleepTime = 1000000;
	int joyx = 0, joyy = 0; /* track joystick position: x,y coordinates zero-centered */
	BOOL joychanged = FALSE; /* set to true whenever joystick moves */

	signal(SIGINT, int_handler);

	fd_uart = open("/dev/ttyS2", O_RDWR | O_NONBLOCK);
	if (fd_uart < 0) {
		perror("/dev/ttyS2");
		exit(-1);
	}
	fd_spi = open("/dev/spimega328", O_RDONLY);
	if (fd_spi < 0) {
		perror("/dev/spimega328");
		exit(-1);
	}

	main2(argc,argv);
	midiInit();	// very important: MIDI_WAIT

	sendProgramChange(fd_uart, currentBank->ID, 0); /* bank A program 1: Grand Piano */


	while (keep_running) {

		//fbg_flip(fbg);
		if (read(fd_uart, &byte, 1) == 1) {
			if (readMidiMessage(byte, &numOfBytes) == TRUE && joychanged == FALSE) {
				sendMidiMessage(fd_uart, numOfBytes);
			}
		}

		if (read(fd_spi, inputdata, 6) > 0) {
			translateJoystick(inputdata[JOYX], inputdata[JOYY], &joyx, &joyy);
			sleepTime = calculateSleepTime(joyx);
			joychanged = TRUE;
		} else {
			if (joyx > 5 && currentBank->index < 127)
				currentBank->index++;
			else if (joyx < -5 && currentBank->index > 0)
				currentBank->index--;
			else
				joychanged = FALSE;

			if (joychanged == TRUE) {
				printf("%d: %s\n", currentBank->index + 1,currentBank->names[currentBank->index]);
				sendProgramChange(fd_uart, currentBank->ID, currentBank->index);
				usleep(sleepTime);
			}
		}
	}

	close(fd_uart);
	close(fd_spi);

	return 0;
}

