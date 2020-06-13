/*
 * input.h
 *
 *  Created on: Jun 12, 2020
 *      Author: nanker
 */

#ifndef CONTROLS_H_
#define CONTROLS_H_

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "main.h"

unsigned char getBankIndex(int x,int y);
void translateJoystick(int xin,int yin,int *px,int *py);
__useconds_t calculateSleepTime(int value);

#endif /* CONTROLS_H_ */
