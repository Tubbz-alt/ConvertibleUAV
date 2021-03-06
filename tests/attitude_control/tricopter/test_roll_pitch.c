#include "gtest/gtest.h"
#include "defines.h"
#include <stdio.h>
#include <math.h>


namespace
{
    // The fixture for testing class Foo.
    class TricopterRollPitchControl : public ::testing::Test
    {
      protected:

          // If the constructor and destructor are not enough for setting up
          // and cleaning up each test, you can define the following methods:

          //tricopter geometry
          const float beta_eq = BETA_EQ_DEG * M_PI / 180;

          // PID gains
          const uint16_t tilt_ki = (uint16_t)(RMAX*0.0);
          const uint16_t tilt_kp = (uint16_t)(RMAX*TILT_KP);
          const uint16_t tilt_rate_kp = (uint16_t)(RMAX*TILT_RATE_KP);
          const uint16_t tilt_rate_kd = (uint16_t)(RMAX*0.0);
          const uint16_t yaw_ki = (uint16_t)(RMAX*YAW_KI);
          const uint16_t yaw_kp = (uint16_t)(RMAX*YAW_KP);
          const uint16_t yaw_rate_kp = (uint16_t)(RMAX*YAW_RATE_KP);

          virtual void SetUp()
          {
              // Code here will be called immediately after the constructor (right
              // before each test).
              printf("Entering set up\n");
              dcm_flags._.calib_finished = 1;
              dcm_flags._.yaw_init_finished = 1;
              current_orientation = F_HOVER;
              udb_pwIn[THROTTLE_HOVER_INPUT_CHANNEL] = 3000;
              udb_pwIn[INPUT_CHANNEL_AUX1] = 3000;
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

    TEST_F(TricopterRollPitchControl, noControl)
    {
        rmat[1] = 0;
        rmat[6] = 0;
        rmat[7] = 0;
        udb_pwIn[THROTTLE_HOVER_INPUT_CHANNEL] = 3000;
        motorCntrl(tilt_kp, tilt_ki, tilt_rate_kp, tilt_rate_kd, yaw_ki, yaw_kp, yaw_rate_kp);

        const int offset_A = K_A * (3000 - 2000);
        const int offset_B = K_B * (3000 - 2000);
        printf("expected motor throttle A %d \n", offset_A);
        printf("expected motor throttle B %d \n", offset_B);
        printf("motor throttle A %d \n", udb_pwOut[MOTOR_A_OUTPUT_CHANNEL]);
        printf("motor throttle B %d \n", udb_pwOut[MOTOR_B_OUTPUT_CHANNEL]);
        ASSERT_NEAR(udb_pwOut[MOTOR_A_OUTPUT_CHANNEL], 2000 + offset_A, 1);
        ASSERT_NEAR(udb_pwOut[MOTOR_B_OUTPUT_CHANNEL], 2000 + offset_B, 1);
        ASSERT_NEAR(udb_pwOut[MOTOR_C_OUTPUT_CHANNEL], udb_pwOut[MOTOR_A_OUTPUT_CHANNEL], 1);
    }

    TEST_F(TricopterRollPitchControl, manualOffset)
    {
        rmat[1] = 0;
        rmat[6] = 0;
        rmat[7] = 0;
        udb_pwIn[THROTTLE_HOVER_INPUT_CHANNEL] = 2600;
        motorCntrl(tilt_kp, tilt_ki, tilt_rate_kp, tilt_rate_kd, yaw_ki, yaw_kp, yaw_rate_kp);

        const int offset_A = K_A * (udb_pwIn[THROTTLE_HOVER_INPUT_CHANNEL] - 2000);
        const int offset_B = K_B * (udb_pwIn[THROTTLE_HOVER_INPUT_CHANNEL] - 2000);
        printf("expected motor throttle A %d \n", offset_A);
        printf("expected motor throttle B %d \n", offset_B);
        printf("motor throttle A %d \n", udb_pwOut[MOTOR_A_OUTPUT_CHANNEL]);
        printf("motor throttle B %d \n", udb_pwOut[MOTOR_B_OUTPUT_CHANNEL]);
        ASSERT_NEAR(udb_pwOut[MOTOR_A_OUTPUT_CHANNEL], 2000 + offset_A, 1);
        ASSERT_NEAR(udb_pwOut[MOTOR_B_OUTPUT_CHANNEL], 2000 + offset_B, 1);
    }

    TEST_F(TricopterRollPitchControl, maxManualThrottle)
    {
        rmat[1] = 0;
        rmat[6] = 0;
        rmat[7] = 0;
        udb_pwIn[THROTTLE_HOVER_INPUT_CHANNEL] = 4000;
        motorCntrl(tilt_kp, tilt_ki, tilt_rate_kp, tilt_rate_kd, yaw_ki, yaw_kp, yaw_rate_kp);
        printf("motor throttle A %d \n", udb_pwOut[MOTOR_A_OUTPUT_CHANNEL]);
        printf("motor throttle B %d \n", udb_pwOut[MOTOR_B_OUTPUT_CHANNEL]);
	ASSERT_NEAR(udb_pwOut[MOTOR_B_OUTPUT_CHANNEL] - 2000, int((udb_pwOut[MOTOR_A_OUTPUT_CHANNEL] - 2000) * K_B / K_A), 1);
	ASSERT_NEAR(udb_pwOut[MOTOR_C_OUTPUT_CHANNEL], udb_pwOut[MOTOR_A_OUTPUT_CHANNEL], 1);
    }

    TEST_F(TricopterRollPitchControl, rollKpGains)
    {
        rmat[1] = 0;
        rmat[6] = 1000;
        rmat[7] = 0;
        udb_pwIn[THROTTLE_HOVER_INPUT_CHANNEL] = 3000;
        const float k_roll = 0.707 * EQUIV_R / (R_A * SIN_ALPHA);
        motorCntrl(tilt_kp, tilt_ki, tilt_rate_kp, tilt_rate_kd, yaw_ki, yaw_kp, yaw_rate_kp);

        // roll control corresponding to this tricopter geometry and these rmat values
        // Simulate first PID controller
        const int roll_error = rmat[6];
        const int desired_roll = -TILT_KP * roll_error;
        // Simulate second PID controller
        const int roll_rate_error = -desired_roll;
        const int roll_quad_control = -TILT_RATE_KP * roll_rate_error;
        printf("roll quad control test %d \n", roll_quad_control);

        // Scale motor order for a tricopter
        const int offset_A = K_A * (3000 - 2000);
        const int offset_B = K_B * (3000 - 2000);
        const int offset_C = offset_A;
        const int control_A = -k_roll * roll_quad_control;
        const int control_B = 0.;
        const int control_C = k_roll * roll_quad_control;
        const int throttle_A = offset_A + control_A;
        const int throttle_B = offset_B + control_B;
        const int throttle_C = offset_C + control_C;
	printf("%d\n", control_A);
        printf("expected motor throttle A %d \n", throttle_A);
        printf("expected motor throttle B %d \n", throttle_B);
        printf("motor throttle A %d \n", udb_pwOut[MOTOR_A_OUTPUT_CHANNEL]);
        printf("motor throttle B %d \n", udb_pwOut[MOTOR_B_OUTPUT_CHANNEL]);
        ASSERT_NEAR(udb_pwOut[MOTOR_A_OUTPUT_CHANNEL], 2000 + throttle_A, 1);
        ASSERT_NEAR(udb_pwOut[MOTOR_B_OUTPUT_CHANNEL], 2000 + throttle_B, 1);
        ASSERT_NEAR(udb_pwOut[MOTOR_C_OUTPUT_CHANNEL], 2000 + throttle_C, 1);
    }

    TEST_F(TricopterRollPitchControl, pitchKpGainsBeta0)
    {
        rmat[1] = 0;
        udb_pwIn[INPUT_CHANNEL_AUX1] = 3000;
        rmat[6] = 0;
        rmat[7] = 1000;
        motorCntrl(tilt_kp, tilt_ki, tilt_rate_kp, tilt_rate_kd, yaw_ki, yaw_kp, yaw_rate_kp);

        const float k_pitch = 0.707 * EQUIV_R / (2 * R_A * COS_ALPHA + R_B);
        // Simulate first PID controller
        const int pitch_error = -rmat[7];
        const int desired_pitch = -0.5 * pitch_error;
        // Simulate second PID controller
        const int pitch_rate_error = -desired_pitch;
        const int pitch_quad_control = -0.22 * pitch_rate_error;

        // Scale motor order for a tricopter
        const int offset_A = K_A * (3000 - 2000);
        const int offset_B = K_B * (3000 - 2000);
        const int offset_C = offset_A;
        const int control_A = k_pitch * pitch_quad_control;
        const int control_B = -2 * control_A;
        const int control_C = k_pitch * pitch_quad_control;
        const int throttle_A = offset_A + control_A;
        const int throttle_B = offset_B + control_B;
        const int throttle_C = offset_C + control_C;

        ASSERT_NEAR(udb_pwOut[MOTOR_A_OUTPUT_CHANNEL], 2000 + throttle_A, 1);
        ASSERT_NEAR(udb_pwOut[MOTOR_B_OUTPUT_CHANNEL], 2000 + throttle_B, 1);
        ASSERT_NEAR(udb_pwOut[MOTOR_C_OUTPUT_CHANNEL], 2000 + throttle_C, 1);
    }

}  // namespace
