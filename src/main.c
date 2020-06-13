#include <termios.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/signal.h>
#include <sys/types.h>
#include <stdlib.h>
#include <string.h>
#include "main.h"
#include "midi.h"
#include "controls.h"

int com_serial;
int failcount;
extern void * bankArray[5];

int main() {
  int fduart,fdspi;
  unsigned char data[6];
  unsigned char byte,numOfBytes;
  struct bank *currentBank = bankArray[bankA];
  __useconds_t sleepTime = 1000000;
  int joyx = 0,joyy = 0;
  BOOL changed = FALSE;
  //struct drumsets *drumsetPtr = bankArray[bankDrums];

  fduart = open("/dev/ttyS2", O_RDWR | O_NONBLOCK);
  if (fduart <0) {perror("/dev/ttyS2"); exit(-1); }
  fdspi = open("/dev/spimega328",O_RDONLY);
  if (fdspi <0) {perror("/dev/spimega328"); exit(-1); }

  midiInit();	// very important: MIDI_WAIT

  /* family_data[family][sound][0] = bank;
   * family_data[family][sound][1] = program number
   */
  sendProgramChange(fduart,currentBank->ID,currentBank->index + 1);
  while (1){

	  if(read(fduart, &byte, 1) == 1){
		  if(readMidiMessage(byte,&numOfBytes) == TRUE && changed == FALSE){
			 sendMidiMessage(fduart,numOfBytes);
		  }
	  }

	  if(read(fdspi,data,6) > 0){
		  translateJoystick(data[JOYX],data[JOYY],&joyx,&joyy);
		  sleepTime = calculateSleepTime(joyx);
		  changed = TRUE;
		}
	  	//printf("Button %d\n",c);

	  else{
		  //printf("%d\n",currentBank->index);
		  if(joyx > 5 && currentBank->index < 127)
			  currentBank->index++;
		  else if(joyx < -5 && currentBank->index > 0)
			  currentBank->index--;
		  else changed = FALSE;
		  if(changed == TRUE){
			  printf("%d: %s\n",currentBank->index + 1,currentBank->names[currentBank->index]);
			  sendProgramChange(fduart,currentBank->ID,currentBank->index);
			  usleep(sleepTime);
		  }

	  }

  }

  close(fduart);

  return 0;
}


