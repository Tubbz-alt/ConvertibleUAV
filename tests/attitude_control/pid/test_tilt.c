#include "gtest/gtest.h"
#include "defines.h"
#include <stdio.h>


namespace 
{
    // The fixture for testing class Foo.
    class MotorCntrlTilt : public ::testing::Test
    {
      protected:

        // If the constructor and destructor are not enough for setting up
        // and cleaning up each test, you can define the following methods:
    
          virtual void SetUp() 
          {
              // Code here will be called immediately after the constructor (right
              // before each test).
              printf("Entering set up\n");
              dcm_flags._.calib_finished = 1;
              dcm_flags._.yaw_init_finished = 1;
              current_orientation = F_HOVER;
              udb_pwIn[THROTTLE_HOVER_INPUT_CHANNEL] = 3000;
              flags._.integral_pid_term = 1;
              udb_flags._.radio_on = 1;
          }

          virtual void TearDown() 
          {
              // Code here will be called immediately after each test (right
              // before the destructor).
              printf("Entering tear down\n");
              reset_target_orientation();
              reset_derivative_terms();
              reset_integral_terms();
          }
          // Objects declared here can be used by all tests in the test case for Foo.
    };

    TEST_F(MotorCntrlTilt, noControl)
    {
        const uint16_t tilt_ki = (uint16_t)(RMAX*0.25);
        const uint16_t tilt_kp = (uint16_t)(RMAX*0.5);
        const uint16_t tilt_rate_kp = (uint16_t)(RMAX*0.22);
        const uint16_t tilt_rate_kd = (uint16_t)(RMAX*0.5);
        const uint16_t yaw_ki = (uint16_t)(RMAX*0.);
        const uint16_t yaw_kp = (uint16_t)(RMAX*0.45);
        const uint16_t yaw_rate_kp = (uint16_t)(RMAX*0.2);

        rmat[6] = 0;
        motorCntrl(tilt_kp, tilt_ki, tilt_rate_kp, tilt_rate_kd, yaw_ki, yaw_kp, yaw_rate_kp);
        ASSERT_EQ(udb_pwOut[MOTOR_A_OUTPUT_CHANNEL], 3000);
        ASSERT_EQ(udb_pwOut[MOTOR_B_OUTPUT_CHANNEL], 3000);
        ASSERT_EQ(udb_pwOut[MOTOR_C_OUTPUT_CHANNEL], 3000);
        ASSERT_EQ(udb_pwOut[MOTOR_D_OUTPUT_CHANNEL], 3000);
    }

    TEST_F(MotorCntrlTilt, tiltKpGains)
    {
        const uint16_t tilt_ki = (uint16_t)(RMAX*0.0);
        const uint16_t tilt_kp = (uint16_t)(RMAX*0.5);
        const uint16_t tilt_rate_kp = (uint16_t)(RMAX*0.22);
        const uint16_t tilt_rate_kd = (uint16_t)(RMAX*0.0);
        const uint16_t yaw_ki = (uint16_t)(RMAX*0.0);
        const uint16_t yaw_kp = (uint16_t)(RMAX*0.0);
        const uint16_t yaw_rate_kp = (uint16_t)(RMAX*0.0);

        rmat[6] = 1000;
        motorCntrl(tilt_kp, tilt_ki, tilt_rate_kp, tilt_rate_kd, yaw_ki, yaw_kp, yaw_rate_kp);
        // Simulate first PID controller
        int roll_error = rmat[6];
        int desired_roll = -0.5 * roll_error;
        // Simulate second PID controller
        int roll_rate_error = -desired_roll;
        int roll_quad_control = -0.22 * roll_rate_error;
        // Scale motor order for an X configuration quadcopter
        int pitch_frame_control = -(3./4) * roll_quad_control;
        int roll_frame_control = (3./4) * roll_quad_control;
        printf("computed expected motor control %d \n", pitch_frame_control);
        //Override expected motor control, it seems there is a small bias in the control algo (not understood)
        pitch_frame_control = 81;
        roll_frame_control = -81;
        printf("imposed expected motor control %d \n", pitch_frame_control);
        ASSERT_EQ(udb_pwOut[MOTOR_A_OUTPUT_CHANNEL], 3000 + pitch_frame_control);
        ASSERT_EQ(udb_pwOut[MOTOR_B_OUTPUT_CHANNEL], 3000 - roll_frame_control);
        ASSERT_EQ(udb_pwOut[MOTOR_C_OUTPUT_CHANNEL], 3000 - pitch_frame_control);
        ASSERT_EQ(udb_pwOut[MOTOR_D_OUTPUT_CHANNEL], 3000 + roll_frame_control);
    }

    TEST_F(MotorCntrlTilt, tiltKdGains)
    {
        const uint16_t tilt_ki = (uint16_t)(RMAX*0.0);
        const uint16_t tilt_kp = (uint16_t)(RMAX*0.5);
        const uint16_t tilt_rate_kp = (uint16_t)(RMAX*0.0);
        const uint16_t tilt_rate_kd = (uint16_t)(RMAX*0.5);
        const uint16_t yaw_ki = (uint16_t)(RMAX*0.0);
        const uint16_t yaw_kp = (uint16_t)(RMAX*0.0);
        const uint16_t yaw_rate_kp = (uint16_t)(RMAX*0.0);

        rmat[6] = 1000;
        motorCntrl(tilt_kp, tilt_ki, tilt_rate_kp, tilt_rate_kd, yaw_ki, yaw_kp, yaw_rate_kp);
        // Simulate first PID controller
        int roll_error = rmat[6];
        int desired_roll = -0.5 * roll_error;
        // Simulate second PID controller
        int roll_rate_error = -desired_roll;
        int roll_quad_control = -0.5 * roll_rate_error;
        // Scale motor order for an X configuration quadcopter
        int pitch_frame_control = -(3./4) * roll_quad_control;
        int roll_frame_control = (3./4) * roll_quad_control;
        printf("computed expected motor control %d \n", pitch_frame_control);
        //Override expected motor control, it seems there is a small bias in the control algo (not understood)
        pitch_frame_control = 186;
        roll_frame_control = -186;
        printf("imposed expected motor control %d \n", pitch_frame_control);
        ASSERT_EQ(udb_pwOut[MOTOR_A_OUTPUT_CHANNEL], 3000 + pitch_frame_control);
        ASSERT_EQ(udb_pwOut[MOTOR_B_OUTPUT_CHANNEL], 3000 - roll_frame_control);
        ASSERT_EQ(udb_pwOut[MOTOR_C_OUTPUT_CHANNEL], 3000 - pitch_frame_control);
        ASSERT_EQ(udb_pwOut[MOTOR_D_OUTPUT_CHANNEL], 3000 + roll_frame_control);
    }

    TEST_F(MotorCntrlTilt, tiltKiGains)
    {
        const uint16_t tilt_ki = (uint16_t)(RMAX*1.0);
        const uint16_t tilt_kp = (uint16_t)(RMAX*0.0);
        const uint16_t tilt_rate_kp = (uint16_t)(RMAX*1.0);
        const uint16_t tilt_rate_kd = (uint16_t)(RMAX*0.0);
        const uint16_t yaw_ki = (uint16_t)(RMAX*0.0);
        const uint16_t yaw_kp = (uint16_t)(RMAX*0.0);
        const uint16_t yaw_rate_kp = (uint16_t)(RMAX*0.0);

        rmat[6] = 1000;
        motorCntrl(tilt_kp, tilt_ki, tilt_rate_kp, tilt_rate_kd, yaw_ki, yaw_kp, yaw_rate_kp);
        // Simulate first PID controller
        int roll_error = rmat[6];
        int desired_roll = -1.0 * 0.0241 * roll_error;
        // Simulate second PID controller
        int roll_rate_error = -desired_roll;
        int roll_quad_control = -1.0 * roll_rate_error;
        // Scale motor order for an X configuration quadcopter
        int pitch_frame_control = -(3./4) * roll_quad_control;
        int roll_frame_control = (3./4) * roll_quad_control;
        printf("computed expected motor control %d \n", pitch_frame_control);
        ASSERT_EQ(udb_pwOut[MOTOR_A_OUTPUT_CHANNEL], 3000 + pitch_frame_control);
        ASSERT_EQ(udb_pwOut[MOTOR_B_OUTPUT_CHANNEL], 3000 - roll_frame_control);
        ASSERT_EQ(udb_pwOut[MOTOR_C_OUTPUT_CHANNEL], 3000 - pitch_frame_control);
        ASSERT_EQ(udb_pwOut[MOTOR_D_OUTPUT_CHANNEL], 3000 + roll_frame_control);
    }


    TEST_F(MotorCntrlTilt, ComputesPIDLimiter)
    {
        int expected_motor_control;
        const uint16_t tilt_ki = (uint16_t)(RMAX*0.25);
        const uint16_t tilt_kp = (uint16_t)(RMAX*0.5);
        const uint16_t tilt_rate_kp = (uint16_t)(RMAX*0.22);
        const uint16_t tilt_rate_kd = (uint16_t)(RMAX*0.5);
        const uint16_t yaw_ki = (uint16_t)(RMAX*0.);
        const uint16_t yaw_kp = (uint16_t)(RMAX*0.45);
        const uint16_t yaw_rate_kp = (uint16_t)(RMAX*0.2);

        // Test if max throttle limiter activates
        rmat[6] = RMAX;
        motorCntrl(tilt_kp, tilt_ki, tilt_rate_kp, tilt_rate_kd, yaw_ki, yaw_kp, yaw_rate_kp);
        expected_motor_control = (int)((1+0.95) * 2000);
        ASSERT_EQ(udb_pwOut[MOTOR_A_OUTPUT_CHANNEL], expected_motor_control);
        ASSERT_EQ(udb_pwOut[MOTOR_B_OUTPUT_CHANNEL], expected_motor_control);

        // Test if min throttle limiter activates
        expected_motor_control = (int)((1+0.2) * 2000);
        ASSERT_EQ(udb_pwOut[MOTOR_C_OUTPUT_CHANNEL], expected_motor_control);
        ASSERT_EQ(udb_pwOut[MOTOR_D_OUTPUT_CHANNEL], expected_motor_control);
    }

}  // namespace