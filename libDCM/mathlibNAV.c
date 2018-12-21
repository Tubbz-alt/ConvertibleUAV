// This file is part of MatrixPilot.
//
//    http://code.google.com/p/gentlenav/
//
// Copyright 2009-2011 MatrixPilot Team
// See the AUTHORS.TXT file for a list of authors of MatrixPilot.
//
// MatrixPilot is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// MatrixPilot is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with MatrixPilot.  If not, see <http://www.gnu.org/licenses/>.


#include "libDCM_internal.h"
#include "../libUDB/heartbeat.h"
#include "../libSTM/libSTM.h"

//  math libraray

#define RADIANTOCIRCULAR 10430 

//  sine table for angles from zero to pi/2 with an increment of pi/128 radian.
//  sine values are multiplied by 2**14
const int16_t sintab[] = { 0,
	402,   804,   1205,  1606,  2006,  2404,  2801,   3196,  3590,  3981,
	4370,  4756,  5139,  5520,  5897,  6270,  6639,  7005,   7366,  7723,
	8076,  8423,  8765,  9102,  9434,  9760,  10080, 10394, 10702, 11003,
	11297, 11585, 11866, 12140, 12406, 12665, 12916, 13160, 13395, 13623,
	13842, 14053, 14256, 14449, 14635, 14811, 14978, 15137, 15286, 15426,
	15557, 15679, 15791, 15893, 15986, 16069, 16143, 16207, 16261, 16305,
	16340, 16364, 16379, 16384
};


int16_t sine(int8_t angle)
//  returns(2**14)*sine(angle), angle measured in units of pi/128 radians
{
	int16_t angle_int;
	angle_int = angle;
	if (angle_int >= 0)
	{
		if (angle_int > 64)
		{
			return (sintab[128-angle_int]);
		}
		else
		{
			return (sintab[angle_int]);
		}
	}
	else
	{
		angle_int = - angle_int;
		if (angle_int > 64)
		{
			return (-sintab[128-angle_int]);
		}
		else
		{
			return (-sintab[angle_int]);
		}
	}
}

int8_t arcsine(int16_t y)
// returns the inverse sine of y
// y is in Q2.14 format, 16384 is maximum value
// returned angle is a byte circular
{
	int8_t angle = 32;
	int8_t doubleangle = 64;
	int8_t step = 32;
	int8_t sign;
	if (y > 0)
	{
		sign = 1;
	}
	else
	{
		sign = - 1;
		y = - y;
	}
	if (y == 16384)
	{
		return sign*64;
	}
	while (step > 0)
	{
		angle = doubleangle>>1;
		if (y == sine(angle))
		{
			return sign*angle;
		}
		else if (y > ((sine(angle)+ sine(angle - 1))>>1))
		{
			doubleangle += step;
		}
		else
		{
			doubleangle -= step;
		}
		step = step>>1;
	}
	return sign*(doubleangle>>1);
}

int16_t cosine(int8_t angle)
{
	return (sine(angle+64));
}

void rotate_2D_vector_by_vector(int16_t vector[2], int16_t rotate[2])
{
//	rotate the vector by the implicit angle of rotate
//	vector[0] is x, vector[1] is y
//	rotate is RMAX*[ cosine(theta), sine(theta) ], theta is the desired rotation angle
//	upon exit, the vector [ x, y ] will be rotated by the angle theta.
//	theta is positive in the counter clockwise direction.
//	This routine can also be used to do a complex multiply, with 1/RMAX scaling,
//	and where vector and rotate are viewed as complex numbers
	int16_t newx, newy;
	union longww accum;
	accum.WW = ((__builtin_mulss(rotate[0], vector[0]) - __builtin_mulss(rotate[1], vector[1]))<<2);
	newx = accum._.W1;
	accum.WW = ((__builtin_mulss(rotate[1], vector[0]) + __builtin_mulss(rotate[0], vector[1]))<<2);
	newy = accum._.W1;
	vector[0] = newx;
	vector[1] = newy;	
}

void rotate_2D_long_vector_by_vector(int32_t vector[2], int16_t rotate[2])
{
//	same as rotate_2D_vector_by_vector, except the first vector is 32 bits
	int32_t newx, newy;
	newx = long_scale(vector[0], rotate[0]) - long_scale(vector[1], rotate[1]);
	newy = long_scale(vector[0], rotate[1]) + long_scale(vector[1], rotate[0]);
	vector[0] = newx;
	vector[1] = newy;
}

void rotate_2D_vector_by_angle(int16_t vector[2], int8_t angle)
{
	//  rotate the vector by angle,
	//  where vector is [ x, y ], angle is in byte-circular scaling
	int16_t rotate[2];
	rotate[1] = sine(angle);
	rotate[0] = cosine(angle);
	rotate_2D_vector_by_vector(vector, rotate);
}

void rotate(struct relative2D *xy, int8_t angle)
{
	//  rotates xy by angle, measured in a counter clockwise sense.
	//  A mathematical angle of plus or minus pi is represented digitally as plus or minus 128.
	int16_t cosang, sinang, newx, newy;
	union longww accum;
	sinang = sine(angle);
	cosang = cosine(angle);
	accum.WW = ((__builtin_mulss(cosang, xy->x) - __builtin_mulss(sinang, xy->y))<<2);
	newx = accum._.W1;
	accum.WW = ((__builtin_mulss(sinang, xy->x) + __builtin_mulss(cosang, xy->y))<<2);
	newy = accum._.W1;
	xy->x = newx;
	xy->y = newy;
}

int8_t rect_to_polar(struct relative2D *xy)
{
	//  Convert from rectangular to polar coordinates using "CORDIC" arithmetic, which is basically
	//  a binary search for the angle.
	//  As a by product, the xy is rotated onto the x axis, so that y is driven to zero,
	//  and the magnitude of the vector winds up as the x component.

	int8_t theta = 0;
	int8_t delta_theta = 64;
	int8_t theta_rot;
	int8_t steps = 7;
	int16_t scaleShift;

	if (((xy-> x) < 255)  &&
		((xy-> x) > -255) &&
		((xy-> y) < 255)  &&
		((xy-> y) > -255)) 
	{
		scaleShift = 6;
		xy->x = (xy->x << 6);
		xy->y = (xy->y << 6);
	}
	else
	{
		scaleShift = 0;
	}

	while (steps > 0)
	{
		theta_rot = delta_theta;
		if (xy->y  > 0) theta_rot = -theta_rot;
		rotate(xy, theta_rot);
		theta += theta_rot;
		delta_theta = (delta_theta>>1);
		steps--;
	}
	if (xy->y > 0) theta--;

	xy->x = (xy->x >> scaleShift);
	xy->y = (xy->y >> scaleShift);

	return (-theta);
}

int16_t rect_to_polar16(struct relative2D *xy)
{
	//  Convert from rectangular to polar coordinates using "CORDIC" arithmetic, which is basically
	//  a binary search for the angle.
	//  As a by product, the xy is rotated onto the x axis, so that y is driven to zero,
	//  and the magnitude of the vector winds up as the x component.
	//  Returns a value as a 16 bit "circular" so that 180 degrees yields 2**15
	int16_t scaleShift;
	int16_t theta16;
	int8_t theta = 0;
	int8_t delta_theta = 64;
	int8_t theta_rot;
	int8_t steps = 7;

	if (((xy-> x) < 255)  &&
		((xy-> x) > -255) &&
		((xy-> y) < 255)  &&
		((xy-> y) > -255))
	{
		scaleShift = 6;
		xy->x = (xy->x << 6);
		xy->y = (xy->y << 6);
	}
	else
	{
		scaleShift = 0;
	}

	while (steps > 0)
	{
		theta_rot = delta_theta;
		if (xy->y  > 0) theta_rot = -theta_rot;
		rotate(xy, theta_rot);
		theta += theta_rot;
		delta_theta = (delta_theta>>1);
		steps--;
	}
	theta = -theta;
	theta16 = theta<<8;

	if (xy->x > 0)
	{
		theta16 += __builtin_divsd(__builtin_mulss(RADIANTOCIRCULAR, xy->y), xy->x);
	}

	xy->x = (xy->x >> scaleShift);
	xy->y = (xy->y >> scaleShift);

	return (theta16);
}

uint16_t sqrt_int(uint16_t sqr)
{
	// based on Heron's algorithm
	uint16_t binary_point = 0;
	uint16_t result = 255;

	int16_t iterations = 3;
	if (sqr == 0)
	{
		return 0;
	}
	while ((sqr & 0xC000) == 0) // shift left to get a 1 in the 2 MSbits
	{
		sqr = sqr*4;            // shift 2 bits
		binary_point ++;        // track half of the number of bits shifted
	}
	sqr = sqr/2;                // for convenience, Herons formula is result = (result + sqr/result) / 2
	while (iterations)
	{
		iterations --;
		result = result/2 + sqr/result;
	}
	result = result >> binary_point; // shift result right to account for shift left of sqr 
	return result;
}

uint16_t sqrt_long(uint32_t sqr)
{
	// based on Heron's algorithm
	uint16_t binary_point = 0;
	uint16_t result = 65535;    // need to start high and work down to avoid overflow in divud

	int16_t iterations = 3;     // thats all you need

	if (sqr < 65536)            // use the 16 bit square root
	{
		return sqrt_int((uint16_t) sqr);
	}
	while ((sqr & 0xC0000000) == 0) // shift left to get a 1 in the 2 MSbits
	{
		sqr = sqr << 2;
		binary_point ++;        // track half of the number of bits shifted
	}
	sqr = sqr >> 1;             // for convenience, Herons formula is result = (result + sqr/result) / 2
	while (iterations)
	{
		iterations --;
		result = result/2 + __builtin_divud(sqr, result);
	}
	result = result >> binary_point; // shift result right to account for shift left of sqr 
	return result;
}

uint16_t vector2_mag(int16_t x, int16_t y)
{
	uint32_t magsqr;
	magsqr = __builtin_mulss(x, x) + __builtin_mulss(y, y);
	return sqrt_long(magsqr);
}

uint16_t vector3_mag(int16_t x, int16_t y, int16_t z)
{
	uint32_t magsqr;
	magsqr = __builtin_mulss(x, x) + __builtin_mulss(y, y) + __builtin_mulss(z, z);
	return sqrt_long(magsqr);
}

uint16_t vector2_normalize(int16_t result[], int16_t input[])
{
	uint16_t magnitude;
	magnitude = vector2_mag(input[0], input[1]);
	if (magnitude > 0)
	{
		result[0] = __builtin_divsd(__builtin_mulss(RMAX, input[0]), magnitude);
		result[1] = __builtin_divsd(__builtin_mulss(RMAX, input[1]), magnitude);
	}
	else
	{
		result[0]=result[1]=0;
	}
	return magnitude;
}

uint16_t vector3_normalize(int16_t result[], int16_t input[])
{
	uint16_t magnitude;
	magnitude = vector3_mag(input[0], input[1], input[2]);
	if (magnitude > 0)
	{
		result[0] = __builtin_divsd(__builtin_mulss(RMAX, input[0]), magnitude);
		result[1] = __builtin_divsd(__builtin_mulss(RMAX, input[1]), magnitude);
		result[2] = __builtin_divsd(__builtin_mulss(RMAX, input[2]), magnitude);
	}
	else
	{
		result[0]=result[1]=result[2]=0;
	}
	return magnitude;
}

int32_t long_scale(int32_t arg1, int16_t arg2)
{
	// returns arg1*arg2/RMAX
	// usually this is used where arg2 is a Q14.2 fractional number
	int8_t sign_result = 1;
	int32_t product;
	union longww accum;
	union longww arg1ww;
	arg1ww.WW = arg1;
	if (arg1ww._.W1 < 0)
	{
		sign_result = - sign_result;
		arg1ww.WW = -arg1ww.WW;
	}
	if (arg2 < 0)
	{
		sign_result = - sign_result;
		arg2 = -arg2;
	}
	product =  __builtin_muluu(arg2, arg1ww._.W1);
	accum.WW = __builtin_muluu(arg2, arg1ww._.W0);
	accum._.W0 = accum._.W1;
	accum._.W1 = 0;
	product += accum.WW;
	product <<= 2;
	if (sign_result > 0)
	{
		return product;
	}
	else
	{
		return -product;
	}
}

//void matrix22_add(int32_t *result, int32_t input1[], int32_t input2[])
//{
//    result[0]=input1[0]+input2[0];
//    result[1]=input1[1]+input2[1];
//    result[2]=input1[2]+input2[2];
//    result[3]=input1[3]+input2[3];
//}
//
//void matrix22_sub(int32_t *result, int32_t input1[], int32_t input2[])
//{
//    result[0]=input1[0]-input2[0];
//    result[1]=input1[1]-input2[1];
//    result[2]=input1[2]-input2[2];
//    result[3]=input1[3]-input2[3];
//}
//
//void matrix22_multiply(int32_t *result, int32_t input1[], int32_t input2[])
//{
//    result[0]=input1[0]*input2[0]+input1[1]*input2[2];
//    result[1]=input1[0]*input2[1]+input1[1]*input2[3];
//    result[2]=input1[2]*input2[0]+input1[3]*input2[2];
//    result[3]=input1[2]*input2[1]+input1[3]*input2[3];
//}
//
//void matrix22_vector_multiply(int32_t *result, int32_t input1[], int32_t input2[])
//{
//    result[0]=input1[0]*input2[0]+input1[1]*input2[1];
//    result[1]=input1[2]*input2[0]+input1[3]*input2[1];
//}
//
//void matrix22_transpose(int32_t *result, int32_t input[])
//{
//    result[0]=input[0];
//    result[1]=input[2];
//    result[2]=input[1];
//    result[3]=input[3];
//}

int16_t limit_value(int16_t value, int16_t limit_min, int16_t limit_max)
{
    int16_t limited_value=value;
    if (value < limit_min)
    {
        limited_value=limit_min;
    }
    if (value > limit_max)
    {
        limited_value=limit_max;
    }
    return limited_value;
}

int16_t compute_pi_block(int16_t input, int16_t target, uint16_t kp, uint16_t ki, int32_t *error_integral, 
                          int16_t heartbeat_hz, int32_t limitintegral, boolean integrate)
{
     int16_t output;
     int32_t tmp ;

     int16_t error=input-target;
     if (integrate)
     {
         *error_integral+=error;
     }
     if (*error_integral > limitintegral) *error_integral = limitintegral;
     if (*error_integral < -limitintegral) *error_integral = -limitintegral;

     tmp = -__builtin_mulsu(error, kp);
     tmp += -__builtin_mulsu((int16_t)((*error_integral)/heartbeat_hz), ki);
     output = tmp>>14;
     
     return output;
}

int16_t IIR_Filter(int32_t *filter, int16_t sample, int8_t delta_filter)
{	
	//TODO: use use alpha=delta_filter/heartbeat_hz instead of alpha=delta_filter
	int32_t local_sample = sample << 16;
	*filter = (local_sample * (256 - delta_filter) + (*filter) * delta_filter) / 256;
	return (int16_t)((*filter) >> 16);
}

int16_t exponential_filter(int16_t x, float *x_filtered, float invdeltafilter)
{	
    float dt = 1.0 / (float)(HEARTBEAT_HZ);
    *x_filtered = ((float)x * invdeltafilter + (float)(*x_filtered) * ((float)(HEARTBEAT_HZ) - invdeltafilter))*dt;
    return (int16_t)*x_filtered;
}

int32_t exponential_filter32(int32_t x, float *x_filtered, float invdeltafilter)
{
    float dt = 1.0 / (float)(HEARTBEAT_HZ);
    *x_filtered = ((float)x * invdeltafilter + (float)(*x_filtered) * ((float)(HEARTBEAT_HZ) - invdeltafilter))*dt;
    return (int32_t)*x_filtered;
}

const int16_t sga_prim_coefficients[][SGA_MAX_LENGTH]={
    {0, 0, 1, -8, 0, 8, -1, 0, 0},
    {0, 0, -2, -1, 0, 1, 2, 0, 0},
    {0, 22, -67, -58, 0, 58, 67, -22, 0},
    {0, -3, -2, -1, 0, 1, 2, 3, 0},
    {86, -142, -193, -126, 0, 126, 193, 142, -86},
    {-4, -3, -2, -1, 0, 1, 2, 3, 4},
};

const int16_t sga_prim_normalization[]= {12,10,252,28,1188,60};

int16_t history_SGA[SGA_LENGTH];

int16_t sga_prim(int16_t current_value)
{ 
    int64_t sum=0;
    int16_t i;

    for(i=1;i<SGA_LENGTH;i++)
    {
		history_SGA[i-1]=history_SGA[i];
    }
    history_SGA[SGA_LENGTH-1]=current_value;
    
    for(i=-SGA_MID;i<=SGA_MID;i++)
    {  
		sum+=history_SGA[i+SGA_MID]*sga_prim_coefficients[SGA_PRIM_INDEX][i+SGA_MAX_MID];
    }
    
    return (int16_t)(sum/sga_prim_normalization[SGA_PRIM_INDEX]);
}
