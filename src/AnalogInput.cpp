/*
 * AnalogInput.cpp
 *
 *  Created on: 01.06.2016
 *      Author: mkluge
 */

#include "AnalogInput.h"

AnalogInput::AnalogInput( unsigned short section_id, unsigned short pin, bool get_diff) {
	this->section_id = section_id;
	this->pin = pin;
	this->last_value = 0;
	this->reference_value = 0;
	this->get_diff = get_diff;
}

AnalogInput::~AnalogInput() {
}

void AnalogInput::calibrate() {
	reference_value = 0;
	for (int l = 0; l < CHECK_LOOPS; l++)
	{
		reference_value += readValue();
	}
	reference_value /= CHECK_LOOPS;
	last_value = reference_value;
}

void AnalogInput::readInto(JsonArray& root, bool only_on_diff) {
	int val = readValue();

	if ( only_on_diff==false || abs(val-last_value)>20 )
	{
		if( get_diff )
		{
			root.add(section_id);
			root.add(val-reference_value);
		}
		else
		{
			root.add(section_id);
			root.add(val);
		}
		this->last_value = val;
	}
}

int AnalogInput::readValue() {
	return analogRead(pin);
}
