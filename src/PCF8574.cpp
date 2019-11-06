#include "PCF8574.h"
#include "Arduino.h"
#include <Wire.h>
#include <ArduinoUnit.h>

test(pcf8574)
{
	PCF8574 test_chip(PCF_BASE_ADDRESS);
	test_chip.selfTest();
}

PCF8574::PCF8574(int address) {
	this->chip_hw_address = address;
	this->last_data = 0;
	this->last_debounced_data = 0;
	this->last_error = 0;
	this->last_update = 0;
	this->input_mask = 0x00;
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
	Wire.requestFrom( this->chip_hw_address, 1);
	// wait for data
	while( !Wire.available() ) {};
	// read byte
	byte current_data = Wire.read();
	return current_data;
}

void PCF8574::setInputMask( byte mask) {
	// clear this bit in the input mask
	input_mask = mask;
}

byte PCF8574::getInputMask() {
	return input_mask;
}

// returns if pins have a stable change
// the bits set denote the pins that had a change
// to have a stable signal, we read the current signal
// and compare it with the last signal; if the new signal
// ist stable for a while, we use it as the new stable signal
// and return the difference to the old stable signal
byte PCF8574::updateState() {

	byte current_data = getCurrentSignal();
	// make bits that are used as outputs to show up as zeros
	if( last_data != current_data )
	{
		last_data = current_data;
		// reset the timer
		last_update = millis();
	}
	else
	{
		// is it a different value?
		if( last_data!=last_debounced_data ) {
			// stable for some time ?
			if( millis()-last_update >= debounceDelay )
			{
				// get the modified bits
				byte modified_bits = last_debounced_data ^ last_data;
				last_debounced_data = last_data;
				return modified_bits;
			}
		}
	}
	// 0 equals to "no modified bits"
	return 0;
}

byte PCF8574::getValue() {
	return last_debounced_data;
}

void PCF8574::write(byte value) {
	Wire.beginTransmission(chip_hw_address);
	last_debounced_data = value | input_mask;
	Wire.write( last_debounced_data );
	last_error = Wire.endTransmission();
}

bool PCF8574::testPin(short pin) {
	return (getCurrentSignal() & (1 << pin)) > 0;
}

void PCF8574::setPin(short pin, bool value) {

	byte new_data;

	if (value == false) {
		new_data = last_debounced_data & ~(1 << pin);
	} else {
		new_data = last_debounced_data | (1 << pin);
	}	
	new_data |= input_mask;
	this->write(new_data);
	// update the current state
	last_debounced_data = new_data;
	last_data = last_debounced_data;
	last_update = millis();

/*
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
*/
}

int PCF8574::lastError() {
	return last_error;
}

long PCF8574::lastUpdate() {
	return last_update;
}
