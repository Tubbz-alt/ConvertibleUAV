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
#include "mode_switch.h"
#include "../libUDB/heartbeat.h"

#if (USE_CONFIGFILE == 1)
#include "config.h"
#include "redef.h"

uint16_t yawkdail;
uint16_t rollkp;
uint16_t rollkd;

#elif((SERIAL_OUTPUT_FORMAT == SERIAL_MAVLINK) || (GAINS_VARIABLE == 1))
uint16_t yawkdail = (uint16_t)(YAWKD_AILERON * SCALEGYRO * RMAX);
uint16_t rollkp = (uint16_t)(ROLLKP * RMAX);
uint16_t rollkd = (uint16_t)(ROLLKD * SCALEGYRO * RMAX);
#else
const uint16_t yawkdail = (uint16_t)(YAWKD_AILERON * SCALEGYRO * RMAX);
const uint16_t rollkp = (uint16_t)(ROLLKP * RMAX);
const uint16_t rollkd = (uint16_t)(ROLLKD * SCALEGYRO * RMAX);
#endif

#if (USE_CONFIGFILE == 1)
uint16_t hoverrollkp;
uint16_t hoverrollkd;
#endif

int16_t hovering_roll_order;

void normalRollCntrl(void);
void hoverRollCntrl(void);

#if (USE_CONFIGFILE == 1)
void init_rollCntrl(void) {
  yawkdail = (uint16_t)(YAWKD_AILERON * SCALEGYRO * RMAX);
  rollkp = (uint16_t)(ROLLKP * RMAX);
  rollkd = (uint16_t)(ROLLKD * SCALEGYRO * RMAX);

  hoverrollkp = (uint16_t)(HOVER_ROLLKP * RMAX);
  hoverrollkd = (uint16_t)(HOVER_ROLLKD * SCALEGYRO * RMAX);
}
#endif

void rollCntrl(void) {
#ifdef TestGains
  if (!flight_mode_switch_waypoints())
    flags._.GPS_steering = 0;  // turn off navigation
// flags._.pitch_feedback = 1;
#endif

  if (current_orientation == F_HOVER) {
    hoverRollCntrl();
  } else {
    normalRollCntrl();
  }
}

void normalRollCntrl(void) {
  union longww rollAccum = {0};
  union longww gyroRollFeedback;
  union longww gyroYawFeedback;

  fractional rmat6;
  fractional omegaAccum2;

  if (!canStabilizeInverted() || !desired_behavior._.inverted) {
    rmat6 = rmat[6];
    omegaAccum2 = omegaAccum[2];
  } else {
    rmat6 = -rmat[6];
    omegaAccum2 = -omegaAccum[2];
  }

  if (AILERON_NAVIGATION && flags._.GPS_steering) {
    rollAccum._.W1 = determine_navigation_deflection('a');
  }

  if (ROLL_STABILIZATION_AILERONS && flags._.pitch_feedback) {
    gyroRollFeedback.WW = __builtin_mulus(rollkd, omegaAccum[1]);
    rollAccum.WW += __builtin_mulsu(rmat6, rollkp);
  } else {
    gyroRollFeedback.WW = 0;
  }

  if (YAW_STABILIZATION_AILERON && flags._.pitch_feedback) {
    gyroYawFeedback.WW = __builtin_mulus(yawkdail, omegaAccum2);
  } else {
    gyroYawFeedback.WW = 0;
  }

  roll_control = (int32_t)rollAccum._.W1 - (int32_t)gyroRollFeedback._.W1 -
                 (int32_t)gyroYawFeedback._.W1;
  // Servo reversing is handled in servoMix.c
}

void hoverRollCntrl(void) { roll_control = 0.; }
