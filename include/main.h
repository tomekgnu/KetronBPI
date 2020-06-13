/*
 * main.h
 *
 *  Created on: Jun 6, 2020
 *      Author: nanker
 */

#ifndef MAIN_H_
#define MAIN_H_

#include <stdint.h>
/* Buttons: [00111100] [00100000] */
#define JOY_PRESS		0x20
#define BUTTON_1		0x10
#define BUTTON_2		0x08
#define BUTTON_3		0x04
#define BUTTON_5		0x20
#define POT0			0
#define POT1			1
#define	JOYX			2
#define JOYY			3
#define BUT0			4
#define BUT1			5

typedef enum{FALSE,TRUE} BOOL;
#define MIN(a,b) (((a)<(b))?(a):(b))
#define MAX(a,b) (((a)>(b))?(a):(b))

typedef struct {
	unsigned char family[2];
	unsigned char familyIndex[2];
}Preset;

typedef struct inputStates
{
	unsigned char button[4];
	unsigned char pot[2];
}inputStates;

// bank numbers
#define NUM_OF_BANKS		5
#define bankA				0
#define bankB				1
#define bankC				10
#define bankD				11
#define bankDrums			4

// sound family constants
#define NUM_OF_FAMILIES		13

#define PIANO				0
#define CHROMATIC			1
#define ORGAN				2
#define ACCORDION			3
#define GUITAR				4
#define STRINGS_CHOIR		5
#define BRASS				6
#define SAX_FLUTE			7
#define PAD					8
#define SYNTH				9
#define ETHNIC				10
#define BASS_SFX			11
#define DRUMS				12

struct bank {
	uint8_t ID;
	uint8_t index;	/* current index */
	char *names[128];
};

struct drumset {
	uint8_t ID;
	char *name;
};

struct drumbank {
	uint8_t ID;
	uint8_t index;
	struct drumset set[47];
};
#endif /* MAIN_H_ */
