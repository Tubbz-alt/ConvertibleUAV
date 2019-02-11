#include "gtest/gtest.h"
#include "defines.h"
#include <stdio.h>

namespace 
{
    // The fixture for testing class Foo.
    class MotorCntrlYaw : public ::testing::Test
    {
      protected:

        // If the constructor and destructor are not enough for setting up
        // and cleaning up each test, you can define the following methods:
    
        virtual void SetUp() 
        {
            // Code here will be called immediately after the constructor (right
            // before each test).
            printf("Entering set up\n");

            //Set-up corresponds to quadricopter motor control in the following conditions:
            //UAV is in hover flight mode,
            //manual control mode,
            //near ground (so integral gains are deactivated),
            //throttle is above the minimum value such that motor control is activated
            //Only roll axis is tested (TODO: we could test each axis within this fixture)

            rmat[0] = RMAX;
            rmat[1] = 0;
            rmat[2] = 0;
            rmat[3] = 0;
            rmat[4] = RMAX;
            rmat[5] = 0;
            rmat[6] = 0;
            rmat[7] = 0;
            rmat[8] = RMAX;
            dcm_flags._.calib_finished = 1;
            manual_to_auto_ramp = RMAX;
            yaw_control_ramp = RMAX;
            flags._.pitch_feedback = 0;
            current_orientation = F_HOVER;
            flags._.mag_failure = 0;
            flags._.invalid_mag_reading = 0;
            flags._.is_close_to_ground = 1;
            yaw_control = 0;
            roll_control = 0;
            pitch_control = 0;
            flags._.engines_off = 0;
            current_flight_phase = F_MANUAL_TAKE_OFF;
            throttle_hover_control = 0;
            udb_flags._.radio_on = 1;
            udb_pwIn[THROTTLE_HOVER_INPUT_CHANNEL] = 2000 + 1000;
        }

        virtual void TearDown() 
        {
          // Code here will be called immediately after each test (right
          // before the destructor).
          printf("Entering tear down\n");
          reset_derivative_terms();
          reset_target_orientation();
          rmat[6] = 0;
          rmat[7] = 0;
          rmat[8] = RMAX;
          udb_pwIn[RUDDER_INPUT_CHANNEL] = 0;
          udb_pwIn[AILERON_INPUT_CHANNEL] = 0;
          udb_pwIn[ELEVATOR_INPUT_CHANNEL] = 0;
        }
        // Objects declared here can be used by all tests in the test case for Foo.
    };

    TEST_F(MotorCntrlPID, ComputesCorrectRollGains)
    {
        rmat[6] = 1000;
        motorCntrl();
        ASSERT_EQ(udb_pwOut[MOTOR_A_OUTPUT_CHANNEL], 3267);
        ASSERT_EQ(udb_pwOut[MOTOR_B_OUTPUT_CHANNEL], 3267);
        ASSERT_EQ(udb_pwOut[MOTOR_C_OUTPUT_CHANNEL], 2733);
        ASSERT_EQ(udb_pwOut[MOTOR_D_OUTPUT_CHANNEL], 2733);
    }

    TEST_F(MotorCntrlPID, ComputesCorrectPitchGains)
    {
        rmat[7] = -1000;
        motorCntrl();
        ASSERT_EQ(udb_pwOut[MOTOR_A_OUTPUT_CHANNEL], 2733);
        ASSERT_EQ(udb_pwOut[MOTOR_B_OUTPUT_CHANNEL], 3267);
        ASSERT_EQ(udb_pwOut[MOTOR_C_OUTPUT_CHANNEL], 3267);
        ASSERT_EQ(udb_pwOut[MOTOR_D_OUTPUT_CHANNEL], 2733);
    }

    TEST_F(MotorCntrlPID, ComputesCorrectYawGains)
    {
        udb_pwIn[RUDDER_INPUT_CHANNEL] = 1000;
        motorCntrl();
        ASSERT_EQ(udb_pwOut[MOTOR_A_OUTPUT_CHANNEL], 2956);
        ASSERT_EQ(udb_pwOut[MOTOR_B_OUTPUT_CHANNEL], 3044);
        ASSERT_EQ(udb_pwOut[MOTOR_C_OUTPUT_CHANNEL], 2956);
        ASSERT_EQ(udb_pwOut[MOTOR_D_OUTPUT_CHANNEL], 3044);
    }

    TEST_F(MotorCntrlPID, ComputesCorrectRollGains2)
    {
        rmat[6] = 1000;
        motorCntrl();
        ASSERT_EQ(udb_pwOut[MOTOR_A_OUTPUT_CHANNEL], 3267);
        ASSERT_EQ(udb_pwOut[MOTOR_B_OUTPUT_CHANNEL], 3267);
        ASSERT_EQ(udb_pwOut[MOTOR_C_OUTPUT_CHANNEL], 2733);
        ASSERT_EQ(udb_pwOut[MOTOR_D_OUTPUT_CHANNEL], 2733);
    }

    TEST_F(MotorCntrlPID, ComputesCorrectPitchGains2)
    {
        rmat[7] = -1000;
        motorCntrl();
        ASSERT_EQ(udb_pwOut[MOTOR_A_OUTPUT_CHANNEL], 2733);
        ASSERT_EQ(udb_pwOut[MOTOR_B_OUTPUT_CHANNEL], 3267);
        ASSERT_EQ(udb_pwOut[MOTOR_C_OUTPUT_CHANNEL], 3267);
        ASSERT_EQ(udb_pwOut[MOTOR_D_OUTPUT_CHANNEL], 2733);
    }

    TEST_F(MotorCntrlPID, ComputesCorrectYawGains2)
    {
        udb_pwIn[RUDDER_INPUT_CHANNEL] = 1000;
        motorCntrl();
        ASSERT_EQ(udb_pwOut[MOTOR_A_OUTPUT_CHANNEL], 2956);
        ASSERT_EQ(udb_pwOut[MOTOR_B_OUTPUT_CHANNEL], 3044);
        ASSERT_EQ(udb_pwOut[MOTOR_C_OUTPUT_CHANNEL], 2956);
        ASSERT_EQ(udb_pwOut[MOTOR_D_OUTPUT_CHANNEL], 3044);
    }

    TEST_F(MotorCntrlPID, ComputesCorrectCommandedRoll)
    {
        udb_pwIn[AILERON_INPUT_CHANNEL] = 166;
        motorCntrl();
        ASSERT_EQ(udb_pwOut[MOTOR_A_OUTPUT_CHANNEL], 3267);
        ASSERT_EQ(udb_pwOut[MOTOR_B_OUTPUT_CHANNEL], 3267);
        ASSERT_EQ(udb_pwOut[MOTOR_C_OUTPUT_CHANNEL], 2733);
        ASSERT_EQ(udb_pwOut[MOTOR_D_OUTPUT_CHANNEL], 2733);
    }

    TEST_F(MotorCntrlPID, ComputesCorrectCommandedPitch)
    {
        udb_pwIn[ELEVATOR_INPUT_CHANNEL] = 166;
        motorCntrl();
        ASSERT_EQ(udb_pwOut[MOTOR_A_OUTPUT_CHANNEL], 2733);
        ASSERT_EQ(udb_pwOut[MOTOR_B_OUTPUT_CHANNEL], 3267);
        ASSERT_EQ(udb_pwOut[MOTOR_C_OUTPUT_CHANNEL], 3267);
        ASSERT_EQ(udb_pwOut[MOTOR_D_OUTPUT_CHANNEL], 2733);
    }

    TEST_F(MotorCntrlPID, ComputesCorrectAutomaticRoll)
    {
        roll_control = 166;
        motorCntrl();
        ASSERT_EQ(udb_pwOut[MOTOR_A_OUTPUT_CHANNEL], 3267);
        ASSERT_EQ(udb_pwOut[MOTOR_B_OUTPUT_CHANNEL], 3267);
        ASSERT_EQ(udb_pwOut[MOTOR_C_OUTPUT_CHANNEL], 2733);
        ASSERT_EQ(udb_pwOut[MOTOR_D_OUTPUT_CHANNEL], 2733);
    }

    TEST_F(MotorCntrlPID, ComputesCorrectAutomaticPitch)
    {
        pitch_control = 166;
        motorCntrl();
        ASSERT_EQ(udb_pwOut[MOTOR_A_OUTPUT_CHANNEL], 3267);
        ASSERT_EQ(udb_pwOut[MOTOR_B_OUTPUT_CHANNEL], 2733);
        ASSERT_EQ(udb_pwOut[MOTOR_C_OUTPUT_CHANNEL], 2733);
        ASSERT_EQ(udb_pwOut[MOTOR_D_OUTPUT_CHANNEL], 3267);
    }

}  // namespace
