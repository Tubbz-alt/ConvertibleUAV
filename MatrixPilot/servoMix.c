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


#include "defines.h"
#include "../libUDB/heartbeat.h"

int16_t aileronbgain;
int16_t elevatorbgain;
int16_t rudderbgain;
const float invdeltaservofilter = (float)(80.);
float roll_filtered_flt=0.;
float pitch_filtered_flt=0.;
float yaw_filtered_flt=0.;

int16_t compute_pot_order(int16_t pot_order, int16_t order_min, int16_t order_max)
{
    int16_t tmp1 = pot_order - 2000;
	tmp1=limit_value(tmp1, 0, 2000);
	int32_t tmp2 = __builtin_mulss(order_max - order_min, tmp1);
	return (int16_t)(tmp2/2000)+order_min;
}

//void applyManoeuvres(void)
////update controls with current_manoeuvreValues. if manoeuvreValue = -RMAX, the manoeuvre is disabled.
//{
//    if (flags._.manoeuvre == 1)
//    {
//        if (current_manoeuvreValues[THROTTLE_OUTPUT_CHANNEL] > -RMAX) throttle_control = current_manoeuvreValues[THROTTLE_OUTPUT_CHANNEL];
//        if (current_manoeuvreValues[ELEVATOR_OUTPUT_CHANNEL] > -RMAX) pitch_control = current_manoeuvreValues[ELEVATOR_OUTPUT_CHANNEL];
//        if (current_manoeuvreValues[AILERON_OUTPUT_CHANNEL] > -RMAX) roll_control = current_manoeuvreValues[AILERON_OUTPUT_CHANNEL];
//        if (current_manoeuvreValues[RUDDER_OUTPUT_CHANNEL] > -RMAX) yaw_control = current_manoeuvreValues[RUDDER_OUTPUT_CHANNEL];
//    }
//}

//	Perform control based on the airframe type.
//	Use the radio to determine the baseline pulse widths if the radio is on.
//	Otherwise, use the trim pulse width measured during power up.
//
//	Mix computed roll and pitch controls into the output channels for the compiled airframe type.

void servoMix(void)
{
	int32_t temp;
	int16_t pwManual[NUM_INPUTS+1];

	// If radio is off, use udb_pwTrim values instead of the udb_pwIn values
	for (temp = 0; temp <= NUM_INPUTS; temp++)
		if (udb_flags._.radio_on)
			pwManual[temp] = udb_pwIn[temp];
		else
			pwManual[temp] = udb_pwTrim[temp];

	// Apply boosts if in a stabilized mode
	if (udb_flags._.radio_on && flags._.pitch_feedback)
	{
        //apply full rate in hovering mode
        if (canStabilizeHover() && current_orientation == F_HOVER)
        {
             aileronbgain = 8;
             elevatorbgain = 8;
             rudderbgain = 8;
        }
        else
        {
             aileronbgain = (int16_t)(8.0*AILERON_BOOST);
             elevatorbgain = (int16_t)(8.0*ELEVATOR_BOOST);
             rudderbgain = (int16_t)(8.0*RUDDER_BOOST);
        }

		pwManual[AILERON_INPUT_CHANNEL] += ((pwManual[AILERON_INPUT_CHANNEL] - udb_pwTrim[AILERON_INPUT_CHANNEL]) * aileronbgain) >> 3;
		pwManual[ELEVATOR_INPUT_CHANNEL] += ((pwManual[ELEVATOR_INPUT_CHANNEL] - udb_pwTrim[ELEVATOR_INPUT_CHANNEL]) * elevatorbgain) >> 3;
		pwManual[RUDDER_INPUT_CHANNEL] += ((pwManual[RUDDER_INPUT_CHANNEL] - udb_pwTrim[RUDDER_INPUT_CHANNEL]) * rudderbgain) >> 3;
	}

	// Standard airplane airframe
	// Mix roll_control into ailerons
	// Mix pitch_control into elevators
	// Mix yaw control and waggle into rudder
#if (AIRFRAME_TYPE == AIRFRAME_STANDARD)

		temp = pwManual[AILERON_INPUT_CHANNEL] + REVERSE_IF_NEEDED(AILERON_CHANNEL_REVERSED, roll_control + waggle);
        temp = exponential_filter(temp, &roll_filtered_flt, invdeltaservofilter);
        udb_pwOut[AILERON_OUTPUT_CHANNEL] = udb_servo_pulsesat(temp + REVERSE_IF_NEEDED(FLAP_CHANNEL_REVERSED, flap_control));
		udb_pwOut[AILERON_SECONDARY_OUTPUT_CHANNEL] = 3000 +
			REVERSE_IF_NEEDED(AILERON_SECONDARY_CHANNEL_REVERSED, udb_servo_pulsesat(temp-REVERSE_IF_NEEDED(FLAP_CHANNEL_REVERSED, flap_control)) - 3000);

		temp = pwManual[ELEVATOR_INPUT_CHANNEL] + REVERSE_IF_NEEDED(ELEVATOR_CHANNEL_REVERSED, pitch_control);
        temp = exponential_filter(temp, &pitch_filtered_flt, invdeltaservofilter);
		udb_pwOut[ELEVATOR_OUTPUT_CHANNEL] = udb_servo_pulsesat(temp);

		temp = pwManual[RUDDER_INPUT_CHANNEL] + REVERSE_IF_NEEDED(RUDDER_CHANNEL_REVERSED, yaw_control - waggle);
        temp = exponential_filter(temp, &yaw_filtered_flt, invdeltaservofilter);
		udb_pwOut[RUDDER_OUTPUT_CHANNEL] = udb_servo_pulsesat(temp);

		if (pwManual[THROTTLE_INPUT_CHANNEL] == 0)
		{
			udb_pwOut[THROTTLE_OUTPUT_CHANNEL] = 0;
		}
		else
		{
			temp = pwManual[THROTTLE_INPUT_CHANNEL] + REVERSE_IF_NEEDED(THROTTLE_CHANNEL_REVERSED, throttle_control);
			udb_pwOut[THROTTLE_OUTPUT_CHANNEL] = udb_servo_pulsesat(temp);
		}
#endif

	// V-Tail airplane airframe
	// Mix roll_control and waggle into ailerons
	// Mix pitch_control and yaw_control into both elevator and rudder
#if (AIRFRAME_TYPE == AIRFRAME_VTAIL)
		int32_t vtail_yaw_control = REVERSE_IF_NEEDED(ELEVON_VTAIL_SURFACES_REVERSED, yaw_control);

		temp = pwManual[AILERON_INPUT_CHANNEL] + REVERSE_IF_NEEDED(AILERON_CHANNEL_REVERSED, roll_control + waggle);
		udb_pwOut[AILERON_OUTPUT_CHANNEL] = udb_servo_pulsesat(temp);
		
		//	Reverse the polarity of the secondary aileron if necessary
		udb_pwOut[AILERON_SECONDARY_OUTPUT_CHANNEL] = 3000 +
			REVERSE_IF_NEEDED(AILERON_SECONDARY_CHANNEL_REVERSED, udb_pwOut[AILERON_OUTPUT_CHANNEL] - 3000);

		temp = pwManual[ELEVATOR_INPUT_CHANNEL] +
			REVERSE_IF_NEEDED(ELEVATOR_CHANNEL_REVERSED, pitch_control + vtail_yaw_control);
		udb_pwOut[ELEVATOR_OUTPUT_CHANNEL] = udb_servo_pulsesat(temp);

		temp = pwManual[RUDDER_INPUT_CHANNEL] +
			REVERSE_IF_NEEDED(RUDDER_CHANNEL_REVERSED, pitch_control - vtail_yaw_control);
		udb_pwOut[RUDDER_OUTPUT_CHANNEL] = udb_servo_pulsesat(temp);

		if (pwManual[THROTTLE_INPUT_CHANNEL] == 0)
		{
			udb_pwOut[THROTTLE_OUTPUT_CHANNEL] = 0;
		}
		else
		{
			temp = pwManual[THROTTLE_INPUT_CHANNEL] + REVERSE_IF_NEEDED(THROTTLE_CHANNEL_REVERSED, throttle_control);
			udb_pwOut[THROTTLE_OUTPUT_CHANNEL] = udb_servo_pulsesat(temp);
		}
#endif

	// Delta-Wing airplane airframe
	// Mix roll_control, pitch_control, and waggle into aileron and elevator
	// Mix rudder_control into  rudder
#if (AIRFRAME_TYPE == AIRFRAME_DELTA)
		int32_t delta_roll_control = REVERSE_IF_NEEDED(ELEVON_VTAIL_SURFACES_REVERSED, roll_control);

		temp = pwManual[AILERON_INPUT_CHANNEL] +
			REVERSE_IF_NEEDED(AILERON_CHANNEL_REVERSED, -delta_roll_control + pitch_control - waggle);
		udb_pwOut[AILERON_OUTPUT_CHANNEL] = udb_servo_pulsesat(temp);

		temp = pwManual[ELEVATOR_INPUT_CHANNEL] +
			REVERSE_IF_NEEDED(ELEVATOR_CHANNEL_REVERSED, delta_roll_control + pitch_control + waggle);
		udb_pwOut[ELEVATOR_OUTPUT_CHANNEL] = udb_servo_pulsesat(temp);

		temp = pwManual[RUDDER_INPUT_CHANNEL] + REVERSE_IF_NEEDED(RUDDER_CHANNEL_REVERSED, yaw_control);
		udb_pwOut[RUDDER_OUTPUT_CHANNEL] =  udb_servo_pulsesat(temp);
		
		if (pwManual[THROTTLE_INPUT_CHANNEL] == 0)
		{
			udb_pwOut[THROTTLE_OUTPUT_CHANNEL] = 0;
		}
		else
		{
			temp = pwManual[THROTTLE_INPUT_CHANNEL] + REVERSE_IF_NEEDED(THROTTLE_CHANNEL_REVERSED, throttle_control);
			udb_pwOut[THROTTLE_OUTPUT_CHANNEL] = udb_servo_pulsesat(temp);
		}
#endif

	// Helicopter airframe
	// Mix half of roll_control and half of pitch_control into aileron channels
	// Mix full pitch_control into elevator
	// Ignore waggle for now
#if (AIRFRAME_TYPE == AIRFRAME_HELI)
		temp = pwManual[AILERON_INPUT_CHANNEL] +
			REVERSE_IF_NEEDED(AILERON_CHANNEL_REVERSED, roll_control/2 + pitch_control/2);
		udb_pwOut[AILERON_OUTPUT_CHANNEL] = udb_servo_pulsesat(temp);

		temp = pwManual[ELEVATOR_INPUT_CHANNEL] + REVERSE_IF_NEEDED(ELEVATOR_CHANNEL_REVERSED, pitch_control);
		udb_pwOut[ELEVATOR_OUTPUT_CHANNEL] = udb_servo_pulsesat(temp);

		temp = pwManual[AILERON_SECONDARY_OUTPUT_CHANNEL] + 
			REVERSE_IF_NEEDED(AILERON_SECONDARY_CHANNEL_REVERSED, -roll_control/2 + pitch_control/2);
		udb_pwOut[AILERON_SECONDARY_OUTPUT_CHANNEL] = temp;

		temp = pwManual[RUDDER_INPUT_CHANNEL] /*+ REVERSE_IF_NEEDED(RUDDER_CHANNEL_REVERSED, yaw_control)*/;
		udb_pwOut[RUDDER_OUTPUT_CHANNEL] = udb_servo_pulsesat(temp);

		if (pwManual[THROTTLE_INPUT_CHANNEL] == 0)
		{
			udb_pwOut[THROTTLE_OUTPUT_CHANNEL] = 0;
		}
		else
		{
			temp = pwManual[THROTTLE_INPUT_CHANNEL] + REVERSE_IF_NEEDED(THROTTLE_CHANNEL_REVERSED, throttle_control);
			udb_pwOut[THROTTLE_OUTPUT_CHANNEL] = udb_servo_pulsesat(temp);
		}
#endif

		udb_pwOut[PASSTHROUGH_A_OUTPUT_CHANNEL] = udb_servo_pulsesat(pwManual[PASSTHROUGH_A_INPUT_CHANNEL]);
		udb_pwOut[PASSTHROUGH_B_OUTPUT_CHANNEL] = udb_servo_pulsesat(pwManual[PASSTHROUGH_B_INPUT_CHANNEL]);
		udb_pwOut[PASSTHROUGH_C_OUTPUT_CHANNEL] = udb_servo_pulsesat(pwManual[PASSTHROUGH_C_INPUT_CHANNEL]);
		udb_pwOut[PASSTHROUGH_D_OUTPUT_CHANNEL] = udb_servo_pulsesat(pwManual[PASSTHROUGH_D_INPUT_CHANNEL]);
}

void sonarServoMix(void)
{
	int32_t temp;

    temp = REVERSE_IF_NEEDED(SONAR_PITCH_CHANNEL_REVERSED, 
		sonar_pitch_servo_pwm_delta);
	temp = cam_pitchServoLimit(temp);
	udb_pwOut[SONAR_PITCH_OUTPUT_CHANNEL] = udb_servo_pulsesat(temp + 3000);
}

void cameraServoMix(void)
{
	int32_t temp;
	int16_t pwManual[NUM_INPUTS+1];

	// If radio is off, use udb_pwTrim values instead of the udb_pwIn values
	for (temp = 0; temp <= NUM_INPUTS; temp++)
	{
		if (udb_flags._.radio_on)
			pwManual[temp] = udb_pwIn[temp];
		else
			pwManual[temp] = udb_pwTrim[temp];
	}

    temp = (pwManual[CAMERA_PITCH_INPUT_CHANNEL] - 3000) + REVERSE_IF_NEEDED(CAMERA_PITCH_CHANNEL_REVERSED, 
		cam_pitch_servo_pwm_delta);
	temp = cam_pitchServoLimit(temp);
	udb_pwOut[CAMERA_PITCH_OUTPUT_CHANNEL] = udb_servo_pulsesat(temp + 3000);

	temp = (pwManual[CAMERA_YAW_INPUT_CHANNEL] - 3000) + REVERSE_IF_NEEDED(CAMERA_YAW_CHANNEL_REVERSED, 
		cam_yaw_servo_pwm_delta);
	temp = cam_yawServoLimit(temp);
	udb_pwOut[CAMERA_YAW_OUTPUT_CHANNEL] = udb_servo_pulsesat(temp + 3000);
}
