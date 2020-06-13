#include <stdio.h>
#include <math.h>

const double Rad2Deg = 180.0 / M_PI;
const double Deg2Rad = M_PI / 180.0;

int main(void){

	printf("%f\n",atan2(1,-1) * Rad2Deg);
	return 0;

}

