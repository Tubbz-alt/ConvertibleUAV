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
#include "mode_switch.h"
#include "airspeedCntrl.h"

//	routines to drive the PWM pins for the servos,
//	assumes the use of the 16MHz crystal.

int16_t pitch_control, roll_control, yaw_control, throttle_control,
    throttle_hover_control, flap_control;
uint16_t wind_gain;
int16_t cam_pitch_servo_pwm_delta = 0;  // Change in PWM pulse value from
                                        // centred value (3000) to send to
                                        // camera pitch servo
int16_t cam_yaw_servo_pwm_delta = 0;  // Change in PWM pulse value from centred
                                      // value (3000) to send to camera yaw
                                      // servo
int16_t motor_tilt_servo_pwm_delta1 = 0;
int16_t motor_tilt_servo_pwm_delta2 = 0;

const uint16_t tilt_ki = (uint16_t)(RMAX * TILT_KI);
const uint16_t tilt_kp = (uint16_t)(RMAX * TILT_KP);
const uint16_t tilt_rate_kp = (uint16_t)(RMAX * TILT_RATE_KP);
const uint16_t tilt_rate_kd = (uint16_t)(RMAX * TILT_RATE_KD);
const uint16_t yaw_ki = (uint16_t)(RMAX * YAW_KI);
const uint16_t yaw_kp = (uint16_t)(RMAX * YAW_KP);
const uint16_t yaw_rate_kp = (uint16_t)(RMAX * YAW_RATE_KP);

void manualPassthrough(void);

void init_servoPrepare(void)  // initialize the PWM
{
  int16_t i;

#if (USE_NV_MEMORY == 1)
  if (udb_skip_flags.skip_radio_trim == 1) return;
#endif

  for (i = 0; i <= NUM_INPUTS; i++)
#if (FIXED_TRIMPOINT == 1)
    udb_pwTrim[i] = udb_pwIn[i] =
        ((i == THROTTLE_INPUT_CHANNEL) ? THROTTLE_TRIMPOINT
                                       : CHANNEL_TRIMPOINT);
#else
    udb_pwIn[i] = udb_pwTrim[i] = ((i == THROTTLE_INPUT_CHANNEL) ? 0 : 3000);
#endif

#if (FIXED_TRIMPOINT == 1)
  for (i = 0; i <= NUM_OUTPUTS; i++)
    udb_pwOut[i] =
        ((i == THROTTLE_OUTPUT_CHANNEL || i == MOTOR_A_OUTPUT_CHANNEL ||
          i == MOTOR_B_OUTPUT_CHANNEL || i == MOTOR_C_OUTPUT_CHANNEL ||
          i == MOTOR_D_OUTPUT_CHANNEL)
             ? THROTTLE_TRIMPOINT
             : CHANNEL_TRIMPOINT);
#else
  for (i = 0; i <= NUM_OUTPUTS; i++)
    udb_pwOut[i] =
        ((i == THROTTLE_OUTPUT_CHANNEL || i == MOTOR_A_OUTPUT_CHANNEL ||
          i == MOTOR_B_OUTPUT_CHANNEL || i == MOTOR_C_OUTPUT_CHANNEL ||
          i == MOTOR_D_OUTPUT_CHANNEL)
             ? 0
             : 3000);
#endif

  motorTiltInit();

#if (NORADIO == 1)
  udb_pwIn[MODE_SWITCH_INPUT_CHANNEL] = udb_pwTrim[MODE_SWITCH_INPUT_CHANNEL] =
      4000;
#endif
}

// Called at HEARTBEAT_HZ
void dcm_servo_callback_prepare_outputs(void) {
  if (dcm_flags._.calib_finished) {
    if (udb_heartbeat_counter % (HEARTBEAT_HZ / SERVO_HZ) == 0) {
      flight_mode_switch_2pos_poll();  // we always want this called at 40Hz

#if (DEADRECKONING == 1)
      process_flightplan();
#endif

#if (ALTITUDE_GAINS_VARIABLE == 1)
      airspeedCntrl();
#endif  // ALTITUDE_GAINS_VARIABLE

      updateBehavior();

      wind_gain = wind_gain_adjustment();

      // Manage motor pitch
      motorTiltCntrl();
      motorTiltServoMix1();
      motorTiltServoMix2();

      rollCntrl();
      yawCntrl();
      altitudeCntrl();
      pitchCntrl();

      servoMix();
    }

    // called at HEARTBEAT_HZ
    motorCntrl(tilt_kp, tilt_ki, tilt_rate_kp, tilt_rate_kd, yaw_ki, yaw_kp,
               yaw_rate_kp);

    if (udb_heartbeat_counter % (HEARTBEAT_HZ / SERVO_HZ) == 0) {
      updateTriggerAction();
    }
  } else {
    // otherwise, there is not anything to do
    manualPassthrough();  // Allow manual control while starting up
  }

  if (dcm_flags._.calib_finished)  // start telemetry after calibration
  {
#if (SERIAL_OUTPUT_FORMAT == SERIAL_MAVLINK)
    if (udb_heartbeat_counter % (HEARTBEAT_HZ / 40) == 0) {
      mavlink_output_40hz();
    }
#else
    // This is a simple check to send telemetry at 8hz
    //		if (udb_heartbeat_counter % (HEARTBEAT_HZ/40) == 0)
    //		{
    serial_output_8hz();
//		}
#endif
  }

#if (USE_OSD == 1)
  osd_run_step();
#endif
}

void manualPassthrough(void) {
  roll_control = pitch_control = yaw_control = throttle_control =
      throttle_hover_control = flap_control = 0;
  servoMix();
}
