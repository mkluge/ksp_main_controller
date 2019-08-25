#include "PCF8574.h"
#include "Arduino.h"
#include <Wire.h>
#include <ArduinoUnit.h>

test(pcf8574)
{
	PCF8574 test_chip(PCF_BASE_ADDRESS);
	test_chip.selfTest();
}

PCF8574::PCF8574(uint8_t address) {
	chip_hw_address = address;
	last_data = 0;
	last_debounced_data = 0;
	last_error = 0;
	last_update = 0;
	input_mask = 0xff;
}

void PCF8574::selfTest()
{
	setInputMask(0xFF);
	this->write(0xFF);
	assertEqual( this->last_error, 0);
	assertEqual( this->testPin(1), false);
	assertEqual( this->getCurrentSignal(), 0);
	// simulate that the last thing we've seen is bit 2 set and all other are 0
	this->last_data = 0x04;
	this->last_debounced_data = 0x04;
	this->last_update = millis();
	assertEqual( this->updateState(), 0);
	delay(2*debounceDelay);
	assertEqual( this->updateState(), 0x04);
}

byte PCF8574::getCurrentSignal()
{
	Wire.requestFrom(chip_hw_address, 1);
	int current_data = Wire.read();
	return current_data;
}

void PCF8574::setInputMask( byte mask) {
	// clear this bit in the input mask
	input_mask = mask;
}

byte PCF8574::getInputMask() {
	return input_mask;
}

byte PCF8574::updateState() {
	byte current_data = getCurrentSignal();
	// make bits that are used as outputs to show up as zeros
	current_data &= input_mask;
	if( last_data != current_data )
	{
		// yes -> reset the timer
		last_update = millis();
	}
	else
	{
		if( millis()-last_update > debounceDelay &&
			last_data != last_debounced_data )
		{
			byte modified_bits = last_debounced_data ^ last_data;
			last_debounced_data = last_data;
			return modified_bits;
		}
	}
	last_data = current_data;
	return 0x00;
}

byte PCF8574::getValue() {
	return last_debounced_data;
}

void PCF8574::write(byte value) {
	Wire.beginTransmission(chip_hw_address);
	last_debounced_data = value;
	Wire.write(last_debounced_data);
	last_error = Wire.endTransmission();
}

bool PCF8574::testPin(short pin) {
	return (last_debounced_data & (1 << pin)) > 0;
}

void PCF8574::setPin(short pin, bool value) {
	byte new_data = this->getCurrentSignal();
	if (value == LOW) {
		new_data &= ~(1 << pin);
	} else {
		new_data |= (1 << pin);
	}
	// in this function you can only set pins to 0 that
	// are also 0 in the input mask, input pins have
	// always to stay high
	new_data |= input_mask;
	// remember the byte written as current data
	// but mask out the output bytes
	this->write(new_data);
	last_debounced_data = new_data & input_mask;
	this->updateState();
}

int PCF8574::lastError() {
	return last_error;
}

long PCF8574::lastUpdate() {
	return last_update;
}
