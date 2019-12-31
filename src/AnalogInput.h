/*
 * AnalogInput.h
 *
 *  Created on: 01.06.2016
 *      Author: mkluge
 */

#ifndef ANALOGINPUT_H_
#define ANALOGINPUT_H_

#include "Arduino.h"
#include "mikemap.h"

#define CHECK_LOOPS 100
using namespace mikemap;

class AnalogInput {
public:
	AnalogInput( unsigned short section_id, unsigned short pin, bool get_diff);
	virtual ~AnalogInput();

	void calibrate();
	void readInto( MikeMap *m, bool only_on_diff );
	int readValue();

private:

	unsigned short pin;
	long reference_value;
	int last_value;
	unsigned short section_id;
	bool get_diff;
};

#endif /* ANALOGINPUT_H_ */
