/*
 * LightButton.h
 *
 *  Created on: 01.06.2016
 *      Author: mkluge
 */

#ifndef LIGHTBUTTON_H_
#define LIGHTBUTTON_H_

#include "Arduino.h"
#include "PCF8574.h"
#ifndef MAX_NAME_LENGTH
#define MAX_NAME_LENGTH 20
#endif

class LightButton {
public:
	LightButton( const char *button_name,
			PCF8574 *i2c_button_chip,
			int i2c_button_chip_pin,
			PCF8574 *i2c_light_chip_address = NULL,
			int i2c_light_chip_pin = 0);
	virtual ~LightButton();
	void setLight( bool enable);
	String getName() const;
	bool readState() const;

private:
	char name[MAX_NAME_LENGTH];
	PCF8574 *button_chip;
	int button_pin;
	PCF8574 *light_chip;
	int light_pin;
};

#endif /* LIGHTBUTTON_H_ */
