/*
 * LightButtons.cpp
 *
 *  Created on: 01.06.2016
 *      Author: mkluge
 */

#include "LightButton.h"

LightButton::LightButton( unsigned short section_id,
	PCF8574 *i2c_button_chip,
	unsigned short i2c_button_chip_pin,
	PCF8574 *i2c_light_chip,
	unsigned short i2c_light_chip_pin) {

  this->section_id = section_id;
	this->button_chip = i2c_button_chip;
	this->button_pin = i2c_button_chip_pin;

	this->light_chip = i2c_light_chip;
	this->light_pin = i2c_light_chip_pin;

	button_chip->setButtonForPin(button_pin, this);
}

LightButton::~LightButton() {
	button_chip->setButtonForPin(button_pin, NULL);
}

void LightButton::setLight(bool enable) {
	if( light_chip!=NULL)
	{
		light_chip->setPin(light_pin, enable);
	}
}

bool LightButton::readState() const {
	return button_chip->testPin(button_pin);
}

unsigned short LightButton::getID() const {
	return section_id;
}
