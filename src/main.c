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

extern void *bankArray[5];
const char *tomba = "Tomba World";

IDirectFB *dfb = NULL;
IDirectFBSurface *psurface = NULL;
IDirectFBSurface *ssurface = NULL;
IDirectFBFont *font_16 = NULL;

int keep_running = 1;

void int_handler(int dummy) {
	keep_running = 0;
}

int main(int argc,char *argv[]) {

	int s_width, s_height;
	int font_h;

	DFBSurfaceDescription psdes;
	DFBFontDescription fdes;
	DFBRectangle srect;

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

	if(DirectFBInit(&argc, &argv) != DFB_OK ||DirectFBCreate(&dfb) != DFB_OK)
		exit(EXIT_FAILURE);
	dfb->SetCooperativeLevel( dfb, DFSCL_FULLSCREEN );
		/*Initialize primary surface*/
		psdes.flags	= DSDESC_CAPS | DSDESC_PIXELFORMAT;
		psdes.caps	= DSCAPS_PRIMARY | DSCAPS_DOUBLE | DSCAPS_DEPTH;
		psdes.pixelformat = DSPF_RGB16;
		if(dfb->CreateSurface(dfb, &psdes, &psurface) != DFB_OK)
			exit(EXIT_FAILURE);

		/*Initialize fonts*/
		fdes.flags	= DFDESC_HEIGHT;
		fdes.height	= 16;
		if(dfb->CreateFont(dfb, "Roboto-Bold.ttf", &fdes, &font_16)!= DFB_OK)
				  exit(EXIT_FAILURE);

		/*Setup main bakground*/
		psurface->GetSize(psurface, &s_width, &s_height);
		psurface->SetColor(psurface, 0x00, 0x00, 0xFF, 0xFF);
		psurface->FillRectangle(psurface, 0, 0, s_width, s_height);
		psurface->DrawRectangle(psurface, 0, 0, s_width, s_height);

		/*Simple sub header with different color*/
		font_16->GetHeight(font_16, &font_h);
		srect.x = 0;
		srect.y = 0;
		srect.w = s_width;
		srect.h = font_h + 2;
		psurface->GetSubSurface(psurface, &srect, &ssurface);
		ssurface->SetColor(ssurface, 0xff, 0x40, 0x00, 0xFF);
		ssurface->FillRectangle(ssurface, 0, 0, s_width, font_h + 2);
		ssurface->DrawRectangle(ssurface, 0, 0, s_width, font_h + 2);

		/*write header in sub surface*/
		ssurface->SetFont(ssurface, font_16);
		ssurface->SetColor(ssurface, 0xFF, 0xFF, 0xFF, 0xFF);
		ssurface->DrawString(ssurface, tomba, -1, s_width/2, 0,	DSTF_TOPCENTER);

		psurface->Flip(psurface, NULL, DSFLIP_NONE);

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
				psurface->GetSubSurface(psurface, &srect, &ssurface);
				ssurface->SetColor(ssurface, 0xff, 0x40, 0x00, 0xFF);
				ssurface->FillRectangle(ssurface, 0, 0, s_width, font_h + 2);
				ssurface->DrawRectangle(ssurface, 0, 0, s_width, font_h + 2);

				/*write header in sub surface*/
				ssurface->SetFont(ssurface, font_16);
				ssurface->SetColor(ssurface, 0xFF, 0xFF, 0xFF, 0xFF);
				ssurface->DrawString(ssurface, currentBank->names[currentBank->index], -1, s_width/2, 0,	DSTF_TOPCENTER);
				//psurface->Flip(psurface, NULL, DSFLIP_NONE);
				ssurface->Flip(ssurface,NULL,DSFLIP_NONE);
				sendProgramChange(fd_uart, currentBank->ID, currentBank->index);
				usleep(sleepTime);
			}
		}
	}

	close(fd_uart);
	close(fd_spi);
	ssurface->Release(ssurface);
	font_16->Release(font_16);
	psurface->Release(psurface);
	dfb->Release(dfb);

	return 0;
}

