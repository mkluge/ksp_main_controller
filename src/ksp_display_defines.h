#ifndef _HAVE_KSP_DISPLAY_DEFINES_
#define _HAVE_KSP_DISPLAY_DEFINES_

#define KSP_INPUT_YAW 1
#define KSP_INPUT_ROLL 2
#define KSP_INPUT_PITCH 3
#define KSP_INPUT_XTRANS 4
#define KSP_INPUT_YTRANS 5
#define KSP_INPUT_ZTRANS 6
#define KSP_INPUT_THRUST 7
#define BUTTON_STAGE 8
#define BUTTON_RCS 9
#define BUTTON_SAS 10
#define BUTTON_GEAR 11
#define BUTTON_SWITCH_RIGHT 12
#define BUTTON_SWITCH_LEFT 13
#define BUTTON_LIGHTS 14
#define BUTTON_BREAKS 15
#define BUTTON_ACTION_1 16
#define BUTTON_ACTION_2 17
#define BUTTON_ACTION_3 18
#define BUTTON_ACTION_4 19
#define BUTTON_ACTION_5 20
#define BUTTON_ACTION_6 21
#define BUTTON_ACTION_7 22
#define BUTTON_ACTION_8 23
#define BUTTON_ACTION_9 24
#define BUTTON_ACTION_10 25
#define BUTTON_SOLAR_ON 26
#define BUTTON_SOLAR_OFF 27
#define BUTTON_ENGINES_OFF 28
#define BUTTON_ENGINES_ON 29
#define BUTTON_ABORT 30
#define BUTTON_FUEL 31
#define BUTTON_REACTION_WHEELS 32
#define BUTTON_STORE 33
#define BUTTON_LOAD 34
#define BUTTON_CAMERA 35
#define BUTTON_TEST 36
#define BUTTON_EVA 37
#define BUTTON_IVA 38
#define BUTTON_SAS_MODE 39
#define BUTTON_SPEED_MODE 40
#define BUTTON_THRUST_FULL 41
#define BUTTON_THRUST_ZERO 42
#define BUTTON_CHUTES 43

#define INFO_HEIGHT 100
#define INFO_SPEED 101
#define INFO_ACTION_GROUPS 102
#define INFO_SURFACE_HEIGHT 103
#define INFO_SURFACE_TIME 104
#define INFO_APOAPSIS 105
#define INFO_APOAPSIS_TIME 106
#define INFO_PERIAPSIS 107
#define INFO_PERIAPSIS_TIME 108
#define INFO_PERCENTAGE_FUEL 112
#define INFO_PERCENTAGE_RCS 113
#define INFO_PERCENTAGE_BATTERY 114
#define INFO_PERCENTAGE_OXYGEN 115
#define BUTTON_NEXT_LEFT_TFT_MODE 36 // no extra button, currently the TEST button

#define CMD_GET_UPDATES 109
#define CMD_UPDATE_CONSOLE 110
#define CMD_INIT 111

unsigned int display_keys[8]={
	INFO_APOAPSIS_TIME,
	INFO_SURFACE_HEIGHT,
	INFO_SURFACE_TIME,
	INFO_PERCENTAGE_FUEL,
	INFO_PERCENTAGE_OXYGEN,
	INFO_PERCENTAGE_RCS,
	INFO_PERCENTAGE_BATTERY,
	BUTTON_NEXT_LEFT_TFT_MODE
};

#endif // _HAVE_KSP_DISPLAY_DEFINES_
