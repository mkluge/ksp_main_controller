/*
 * ConsoleSetup.h
 *
 *  Created on: 05.06.2016
 *      Author: mkluge
 */

#ifndef CONSOLESETUP_H_
#define CONSOLESETUP_H_

#include "ksp_display_defines.h"

#define DISPLAY_I2C_ADDRESS 0x08
//#define MAIN_CONTROLLER_I2C_ADDRESS 0x09
#define KEY_NOT_FOUND -1

#define DISPLAY_WIRE_BUFFER_SIZE 300

char data_start[]="\"data\":[";
char cmd_start[]="\"cmd\":";
char display_start[]="\"disp\":[";
char disp_init[]="{\"chk\":1}\n";
char disp_data_start[]="{\"data\":[";

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

#endif /* CONSOLESETUP_H_ */
