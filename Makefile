CC       = g++ -std=c++11 -v
INCPATH  = -I. -I/usr/include/gtest/include -ITools/HILSIM_XPlane -ITools/MatrixPilot-SIL -IlibUDB -IlibDCM -IMatrixPilot
LIBS     = -lm -lgtest -lpthread -L /usr/include/gtest/lib
RM_FILE  = rm -f

OBJECTS_DIR = ./


MPSIL_TEST_TARGET  = matrixpilot-test.out

MP_HEADERS = MatrixPilot/options.h MatrixPilot/waypoints.h MatrixPilot/flightplan-logo.h

MPSIL_OBJECTS = \
 \
./Tools/MatrixPilot-SIL/SIL-udb.o \
./Tools/MatrixPilot-SIL/SIL-ui-mp-term.o \
./Tools/MatrixPilot-SIL/SIL-serial.o \
./Tools/MatrixPilot-SIL/SIL-dsp.o \
./Tools/MatrixPilot-SIL/SIL-eeprom.o \
./Tools/MatrixPilot-SIL/SIL-events.o \
./Tools/HILSIM_XPlane/UDBSocketUnix.o \
 \
./libUDB/barometerAltitude.o \
./libUDB/lidarAltitude.o \
./libUDB/sonarAltitude.o \
 \
./libDCM/deadReckoning.o \
./libDCM/estWind.o \
./libDCM/estYawDrift.o \
./libDCM/gpsParseCommon.o \
./libDCM/gpsParseMTEK.o \
./libDCM/gpsParseSTD.o \
./libDCM/gpsParseUBX.o \
./libDCM/libDCM.o \
./libDCM/mathlibNAV.o \
./libDCM/rmat.o \
 \
./MatrixPilot/airspeedCntrl.o \
./MatrixPilot/altitudeCntrl.o \
./MatrixPilot/altitudeCntrlVariable.o \
./MatrixPilot/behavior.o \
./MatrixPilot/cameraCntrl.o \
./MatrixPilot/config_tests.o \
./MatrixPilot/data_services.o \
./MatrixPilot/data_storage.o \
./MatrixPilot/euler_angles.o \
./MatrixPilot/flightplan-logo.o \
./MatrixPilot/flightplan-waypoints.o \
./MatrixPilot/MAVLink.o \
./MatrixPilot/mode_switch.o \
./MatrixPilot/mp_osd.o \
./MatrixPilot/navigate.o \
./MatrixPilot/nv_memory_table.o \
./MatrixPilot/parameter_table.o \
./MatrixPilot/pitchCntrl.o \
./MatrixPilot/rollCntrl.o \
./MatrixPilot/servoMix.o \
./MatrixPilot/servoPrepare.o \
./MatrixPilot/states.o \
./MatrixPilot/telemetry.o \
./MatrixPilot/yawCntrl.o \
./MatrixPilot/sonarCntrl.o \
./MatrixPilot/motorCntrl.o

MPSIL_TEST_OBJECTS = \
./tests/test_main.o \
./tests/attitude_control/tricopter/test_motor_tilt.o \
./tests/attitude_control/tricopter/test_roll_pitch.o \
./tests/attitude_control/tricopter/test_yaw.o \
./tests/control_mode/test_control_mode.o \
./tests/attitude_control/test_tx_linear_control.o \
./tests/math/test_scaling.o

MPCAT_TARGET   = silcat

MPCAT_OBJECTS  = ./Tools/HILSIM_XPlane/UDBSocketUnix.o ./Tools/MatrixPilot-SIL/SILcat.o


first: all

%.o: %.c $(MP_HEADERS)
	$(CC) -c $(CFLAGS) $(INCPATH) -o $@ $<

test: $(MPSIL_TEST_TARGET)


$(MPSIL_TEST_TARGET): $(MPSIL_OBJECTS) $(MPSIL_TEST_OBJECTS)
	$(CC) -o $(MPSIL_TEST_TARGET) $(LFLAGS) $(MPSIL_OBJECTS) $(MPSIL_TEST_OBJECTS) $(LIBS)

clean:
	-$(RM_FILE) $(MPSIL_OBJECTS) $(MPCAT_OBJECTS) $(MPSIL_TEST_OBJECTS)
	-$(RM_FILE) *~ core *.core
	find \( -name '*.gcda' -o -name '*.gcno' \) -exec rm -f {} +
