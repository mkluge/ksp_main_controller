/*
 * AnalogInput.h
 *
 *  Created on: 01.06.2016
 *      Author: mkluge
 */

#ifndef ANALOGINPUT_H_
#define ANALOGINPUT_H_

#include "Arduino.h"
#include "ArduinoJson.h"

#define CHECK_LOOPS 100
#ifndef MAX_NAME_LENGTH
#define MAX_NAME_LENGTH 20
#endif

class AnalogInput {
public:
	AnalogInput( const char *json_section, int pin, bool get_diff);
	virtual ~AnalogInput();

	void calibrate();
	void readInto( JsonObject& root );

private:
	int readValue();

	int pin;
	long reference_value;
	int last_value;
	char name[MAX_NAME_LENGTH];
	bool get_diff;
};

#endif /* ANALOGINPUT_H_ */
