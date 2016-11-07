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

class LightButton {
public:
	LightButton( unsigned short section_id,
			PCF8574 *i2c_button_chip,
			unsigned short i2c_button_chip_pin,
			PCF8574 *i2c_light_chip_address = NULL,
			unsigned short i2c_light_chip_pin = 0);
	virtual ~LightButton();
	void setLight( bool enable);
	unsigned short getID() const;
	bool readState() const;

private:
	unsigned short section_id;
	PCF8574 *button_chip;
	unsigned short button_pin;
	PCF8574 *light_chip;
	unsigned short light_pin;
};

#endif /* LIGHTBUTTON_H_ */
