#include "controls.h"

const double Rad2Deg = 180.0 / M_PI;
const double Deg2Rad = M_PI / 180.0;

static double joyangle(int x,int y);

static __useconds_t delayTimes[8] = {1000000,700000,500000,300000,150000,70000,50000,25000};
/**
 * Returns bank index based on x and y joystick coordinates
 */
unsigned char getBankIndex(int x,int y){
	static int dir = 0;
	static double oldangle = 0,angle;
	x -= 128;
	y = -(y - 128);

	angle = joyangle(x,y);

	if(angle > oldangle){
		printf("Increase\n");
		dir = 1;
		oldangle = angle;
	}
	if(dir == 1 && angle == 0)
		oldangle = 0;
	if(angle < oldangle){
		printf("Decrease\n");
		dir = -1;
		oldangle = angle;
	}

	if(dir == -1 && angle == 0)
		oldangle = 360;
	printf("angle: %f oldangle: %f\n",angle,oldangle);

	if(x > -10 && x < 10 && y > -10 && y < 10)
		oldangle = 0;

	return 0;
}

/**
 * Return sleep time in us depending on input parameter, which is either x or y
 * joystick axis value
 */
__useconds_t calculateSleepTime(int value){

	value = abs(value);
	if(value == 128)
		value = 127;


	return delayTimes[value / 16];
}

/**
 * Translate joystick inputs to zero-centered coordinates
 */
void translateJoystick(int xin,int yin,int *px,int *py){
	*px = xin - 128;
	*py = -(yin - 128);
}

/*
 * Calculates angle in radians between two points and x-axis.
 */
static double joyangle(int x,int y)
{
	double result;

	//printf("%d %d\n",x,y);
	result = atan2(y,x) * Rad2Deg;
	if(result <= 0)
		return -result;
	return 360.00 - result;
}
