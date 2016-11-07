#ifndef _PCF8574_H
#define _PCF8574_H

#include "Arduino.h"
#define PCF8574_LIB_VERSION "0.1.02"

class LightButton;
#define debounceDelay 50

class PCF8574 {
public:
	PCF8574(int address);

	// sets the pointer to the button that is connected to a pin
	void setButtonForPin(short pin, LightButton *button);
	// gets that pointer
	LightButton *getButtonForPin(short pin);
	// updates the internal state, including debouncing the input
	// returns a byte with all bits set to 1 that changed
	// if there was a stable state change
	byte updateState();
	// reads the current state from the pins
	byte getCurrentSignal();
	// reset state so that a new state change can be detected
	void resetState();
	// tests a pin (debounced)
	bool testPin(short pin);
	// gets the stored value (debounced)
	byte getValue();

	void write(byte value);
	void setPin(short pin, bool value);
	void setInputMask( byte mask);
	long lastUpdate();
	byte getInputMask();
	int lastError();

private:
	// the bits that are 0 are used as inputs
	byte input_mask;
	// the HW address on the I2C bus
	int chip_hw_address;
	// the last 8 bytes read
	byte last_data;
	// the last 8 bytes read, debounced
	byte last_debounced_data;
	// last error
	int last_error;
	// time of last read
	unsigned long last_update;
	// the pointer to the button for each pin
	LightButton *connected_buttons[8];
};

#endif
