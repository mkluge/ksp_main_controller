#undef ANALOG_TEST
#undef PRINT_DEBUG
#undef SERIAL_TEST
#undef KEY_TEST

#define NO_DISPLAYS

#ifdef ANALOG_TEST
#define TEST
#endif

#ifdef SERIAL_TEST
#define TEST
#endif

#ifdef KEY_TEST
#define TEST
#endif

#define SERIAL_PORT Serial

#include <Arduino.h>
#include <AnalogInput.h>
#include <LedControl.h>
#include "ConsoleSetup.h"
#include "PCF8574.h"
#include <Wire.h>
#include "ksp_display_defines.h"
#include "mikemap.h"

using namespace mikemap;

/*
 chip(pin)

 Schalter rechts: chip5 (4-7)
 15-er rechts: oben 5(1-3),
 6er mitte 5(0) 4(3-7)
 6er unten 4(0-2) ???

 grau/rot SDA
 viol/brn SCL
 blau INT (19)

 Led panel 0-9 : 2(3) 2(4-7) 3(4) 3(0-3)
 4 Knöpfe mitte: 0(4-7)
 Licht 6(0-7) und 7(0-7)

 5 links: oben 1(0) 1(3-7); 3 statt 0??;

 */

#define NUM_ANALOG_BUTTONS 7
#define NUM_KEY_CHIPS 5
#define NUM_LIGHT_CHIPS 2
#define NO_PIN 9
#define NO_CHIP 9
#define NO_KEY -1
#define BUFFER_SIZE 300
// some button indizes for easier handling
#define STAGE_BUTTON_INDEX 0
#define RCS_BUTTON_INDEX 1
#define SAS_BUTTON_INDEX 2
#define GEAR_BUTTON_INDEX 3
//#define LIGHT_BUTTON_INDEX 6
//#define BRAKES_BUTTON_INDEX 7

char buffer[BUFFER_SIZE];
unsigned int read_buffer_offset = 0;
int empty_buffer_size = 0;
bool have_handshake = false;
bool stage_enabled = false;
bool message_complete = false;
bool interrupt_seen = false;
char data_start[]="\"data\":[";
char cmd_start[]="\"cmd\":";

LedControl led_top(5, 7, 6, 1);
LedControl led_bottom(2, 4, 3, 1);

// double check used pins
// 5: 0 1 2 3 4 5 6 7 - FULL
// 4: 0 1 2 3 4 5 6 7 - FULL
// 3: 0 1 2 3 4   6 7
// 2:       3 4 5 6 7
// 1: 0 1 2 3 4 5 6 7 - FULL

/* memorizes pressed buttons until stuff can be sent to master */
MikeMap key_updates;
/* memorizes data that will be send to display */
MikeMap display_updates;
/* input from python to us */
MikeMap input_data;

int action_group_buttons[10] = {
	BUTTON_ACTION_1, BUTTON_ACTION_2, BUTTON_ACTION_3, BUTTON_ACTION_4,
	BUTTON_ACTION_5, BUTTON_ACTION_6, BUTTON_ACTION_7, BUTTON_ACTION_8,
	BUTTON_ACTION_9, BUTTON_ACTION_10};

AnalogInput ai1(KSP_INPUT_YAW, A13, true);
AnalogInput ai2(KSP_INPUT_PITCH, A14, true);
AnalogInput ai3(KSP_INPUT_ROLL, A15, true);
AnalogInput ai4(KSP_INPUT_XTRANS, A10, true);
AnalogInput ai5(KSP_INPUT_YTRANS, A11, true);
AnalogInput ai6(KSP_INPUT_ZTRANS, A9, true);
AnalogInput ai7(KSP_INPUT_THRUST, A8, false);

AnalogInput *analog_inputs[NUM_ANALOG_INPUTS] = {
	&ai1, &ai2, &ai3, &ai4, &ai5, &ai6, &ai7};

PCF8574 kc1(PCF_BASE_ADDRESS + 0);
PCF8574 kc2(PCF_BASE_ADDRESS + 1);
PCF8574 kc3(PCF_BASE_ADDRESS + 2);
PCF8574 kc4(PCF_BASE_ADDRESS + 3);
PCF8574 kc5(PCF_BASE_ADDRESS + 4);

PCF8574 *key_chips[NUM_KEY_CHIPS] = {
	&kc1, &kc2, &kc3, &kc4, &kc5};

PCF8574 lc1(PCF_BASE_ADDRESS + 5);
PCF8574 lc2(PCF_BASE_ADDRESS + 6);

PCF8574 *light_chips[NUM_LIGHT_CHIPS] = {
	&lc1, &lc2};

#define KC1(kpin) \
	*chip = 0;    \
	*pin = kpin;  \
	return true;
#define KC2(kpin) \
	*chip = 1;    \
	*pin = kpin;  \
	return true;
#define KC3(kpin) \
	*chip = 2;    \
	*pin = kpin;  \
	return true;
#define KC4(kpin) \
	*chip = 3;    \
	*pin = kpin;  \
	return true;
#define KC5(kpin) \
	*chip = 4;    \
	*pin = kpin;  \
	return true;
#define LC1(lpin) \
	*chip = 0;    \
	*pin = lpin;  \
	return true;
#define LC2(lpin) \
	*chip = 1;    \
	*pin = lpin;  \
	return true;

namespace {

void dieError(int code);
void reset_serial_buffer();
void read_console_updates(MikeMap *updates);
bool isSwitchEnabled(int key);

/*
int freeRam()
{
       extern int __heap_start, *__brkval;
       int v;
       return (int)&v - (__brkval == 0 ? (int)&__heap_start : (int)__brkval);
}

#define RAM if (freeRam()<500 ) dieError(22);
//#define RAM min_freeRam = freeRam();
//#define RAM { }
*/

bool getPinForKey(int key, uint8_t *chip, uint8_t *pin)
{
	switch (key)
	{
	case BUTTON_SPEED_MODE:
		KC1(0)
	case BUTTON_THRUST_FULL:
		KC1(1)
	case BUTTON_THRUST_ZERO:
		KC1(2)
	case BUTTON_BREAKS:
		KC1(3)
	case BUTTON_STAGE:
		KC1(4)
	case BUTTON_RCS:
		KC1(5)
	case BUTTON_SAS:
		KC1(6)
	case BUTTON_GEAR:
		KC1(7)
	case BUTTON_SAS_MODE:
		KC2(0)
	case BUTTON_ACTION_1:
		KC2(3)
	case BUTTON_ACTION_4:
		KC2(4)
	case BUTTON_ACTION_5:
		KC2(5)
	case BUTTON_ACTION_3:
		KC2(6)
	case BUTTON_ACTION_2:
		KC2(7)
	case BUTTON_ACTION_7:
		KC3(0)
	case BUTTON_ACTION_8:
		KC3(1)
	case BUTTON_ACTION_9:
		KC3(2)
	case BUTTON_ACTION_10:
		KC3(3)
	case BUTTON_TEST:
		KC3(4)
	case BUTTON_IVA:
		KC3(5)
	case BUTTON_EVA:
		KC3(6)
	case BUTTON_ACTION_6:
		KC3(7)
	case BUTTON_STORE:
		KC4(0)
	case BUTTON_LOAD:
		KC4(1)
	case BUTTON_CAMERA:
		KC4(2)
	case BUTTON_SOLAR_OFF:
		KC4(2)
	case BUTTON_SOLAR_ON:
		KC4(3)
	case BUTTON_FUEL:
		KC4(5)
	case BUTTON_REACTION_WHEELS:
		KC4(6)
	case BUTTON_ENGINES_OFF:
		KC4(7)
	case BUTTON_ENGINES_ON:
		KC5(0)
	case BUTTON_ABORT:
		KC5(1)
	case BUTTON_CHUTES:
		KC5(2)
	case BUTTON_LIGHTS:
		KC5(3)
	case BUTTON_SWITCH_RIGHT:
		KC5(6)
	case BUTTON_SWITCH_LEFT:
		KC5(7)
	}
	return false;
}

signed int getKeyForChipPin(int key_chip, int current_bit)
{
	// 0 is kc1
	switch (key_chip)
	{
	case 0:
		switch (current_bit)
		{
		case 0:
			return BUTTON_SPEED_MODE;
		case 1:
			return BUTTON_THRUST_FULL;
		case 2:
			return BUTTON_THRUST_ZERO;
		case 3:
			return BUTTON_BREAKS;
		case 4:
			return BUTTON_STAGE;
		case 5:
			return BUTTON_RCS;
		case 6:
			return BUTTON_SAS;
		case 7:
			return BUTTON_GEAR;
		}
	case 1:
		switch (current_bit)
		{
		case 0:
			return BUTTON_SAS_MODE;
		case 3:
			return BUTTON_ACTION_1;
		case 4:
			return BUTTON_ACTION_4;
		case 5:
			return BUTTON_ACTION_5;
		case 6:
			return BUTTON_ACTION_3;
		case 7:
			return BUTTON_ACTION_2;
		}
	case 2:
		switch (current_bit)
		{
		case 0:
			return BUTTON_ACTION_7;
		case 1:
			return BUTTON_ACTION_8;
		case 2:
			return BUTTON_ACTION_9;
		case 3:
			return BUTTON_ACTION_10;
		case 4:
			return BUTTON_TEST;
		case 5:
			return BUTTON_IVA;
		case 6:
			return BUTTON_EVA;
		case 7:
			return BUTTON_ACTION_6;
		}
	case 3:
		switch (current_bit)
		{
		case 0:
			return BUTTON_STORE;
		case 1:
			return BUTTON_LOAD;
		case 2:
			return BUTTON_CAMERA;
		case 3:
			return BUTTON_SOLAR_OFF;
		case 4:
			return BUTTON_SOLAR_ON;
		case 5:
			return BUTTON_FUEL;
		case 6:
			return BUTTON_REACTION_WHEELS;
		case 7:
			return BUTTON_ENGINES_OFF;
		}
	case 4:
		switch (current_bit)
		{
		case 0:
			return BUTTON_ENGINES_ON;
		case 1:
			return BUTTON_ABORT;
		case 2:
			return BUTTON_CHUTES;
		case 3:
			return BUTTON_LIGHTS;
		case 6:
			return BUTTON_SWITCH_RIGHT;
		case 7:
			return BUTTON_SWITCH_LEFT;
		}
	}
	return NO_KEY;
}

bool getLightFromKey(int key, uint8_t *chip, uint8_t *pin)
{
	switch (key)
	{
	case BUTTON_STAGE:
		LC1(0)
	case BUTTON_RCS:
		LC1(1)
	case BUTTON_ACTION_1:
		LC1(2)
	case BUTTON_ACTION_2:
		LC1(4)
	case BUTTON_ACTION_3:
		LC1(5)
	case BUTTON_ACTION_4:
		LC2(6)
	case BUTTON_ACTION_5:
		LC1(7)
	case BUTTON_SAS:
		LC2(0)
	case BUTTON_GEAR:
		LC2(1)
	case BUTTON_ACTION_6:
		LC2(2)
	case BUTTON_ACTION_7:
		LC2(4)
	case BUTTON_ACTION_8:
		LC2(5)
	case BUTTON_ACTION_9:
		LC2(6)
	case BUTTON_ACTION_10:
		LC2(7)
	}
	return false;
}

void setupLC(LedControl &lc, int intensity)
{
	lc.shutdown(0, false);		   // turn off power saving, enables display
	lc.setIntensity(0, intensity); // sets brightness (0~15 possible values)
	lc.clearDisplay(0);			   // clear screen
}

void print_led(LedControl &target, long val)
{
	int digit = 0;
	bool negative = (val >= 0) ? false : true;
	val = abs(val);
	while (val > 0 && digit < 8)
	{
		int last_digit = val % 10;
		val = val / 10;
		target.setDigit(0, digit, (byte)last_digit, false);
		digit++;
	}
	if (negative && digit < 8)
	{
		target.setChar(0, digit, '-', false);
		digit++;
	}
	while (digit < 8)
	{
		target.setChar(0, digit, ' ', false);
		digit++;
	}
}

void print_led(LedControl &target, const char *str)
{
	int len = strlen(str);
	int digit = 0;
	while (digit < 8 && len > 0)
	{
		target.setChar(0, digit, str[len - 1], false);
		len--;
		digit++;
	}
}

/**
 * checks all buttons and if anyone changed its state, adds the new state
 * of the botton to the json object
 */
void testAllButtons(MikeMap *updates)
{
	// update chips
	int chip_index = 0;

	for (PCF8574 *pcf8754 : key_chips)
	{
		byte changed_bits = pcf8754->updateState();
		if (changed_bits != 0x00)
		{
			// test all bits and update the json for each bit set
			int current_bit = 0;
			while (changed_bits != 0)
			{
				if (changed_bits & 1)
				{
					int key = getKeyForChipPin(chip_index, current_bit);
					if (key != NO_KEY)
					{
						// low active inputs
						short new_state = (pcf8754->testPin(current_bit) == false) ? 1 : 0;
						// set only on keydown
						if (new_state == 1)
						{
							updates->set(key, new_state);
							// remember buttons that trigger the display controller directly
							if (key == BUTTON_NEXT_LEFT_TFT_MODE)
							{
								display_updates.set(BUTTON_NEXT_LEFT_TFT_MODE, 1);
							}
						}
					}
				}
				current_bit++;
				changed_bits >>= 1;
			}
		}
		chip_index++;
	}
}

bool isSwitchEnabled(int key)
{
	uint8_t chip;
	uint8_t pin;

	if (!getPinForKey(key, &chip, &pin))
		return false;

	// low active
	return (key_chips[chip]->testPin(pin) == false);
}

void reset_serial_buffer()
{
	memset(buffer, 0, BUFFER_SIZE);
	read_buffer_offset = 0;
	message_complete = false;
}

int serial_read_until(char delimiter)
{
	int bytes_read = 0;
	while (1)
	{
		if (SERIAL_PORT.available())
		{
			char inByte = (char)SERIAL_PORT.read();
			bytes_read++;
			if (inByte == delimiter)
			{
				break;
			}
			if (read_buffer_offset < (BUFFER_SIZE - 2))
			{
				buffer[read_buffer_offset] = (char)inByte;
				read_buffer_offset++;
			}
			else
			{
				dieError(99);
			}
		}
	}
	buffer[read_buffer_offset] = 0;
	message_complete = true;
	return bytes_read;
}

void check_serial_port()
{
	if (message_complete == true)
	{
		// not supposed to happen, that we have a complete
		// message on board an not yet processed and another
		// message arrives
		dieError(88);
		return;
	}
	// nothing is waiting, so just leave ...
	if (!SERIAL_PORT.available())
	{
		return;
	}
	// if transmission has started, read until the delimiter
	serial_read_until('+');
	// send the serial ACK
	SERIAL_PORT.print("OK");
}

void dieError(int code)
{
	print_led(led_top, "Error:");
	print_led(led_bottom, code);
	while (44)
		;
}

#ifndef NO_DISPLAYS
// warning: this thing just sends, it does not care about
// the protocoll (the 77 coming back as "ready to receive")
void sendToSlave(const JsonDocument &message)
{

	char buf[DISPLAY_WIRE_BUFFER_SIZE];
	int len = serializeJson(message, buf);
	buf[len] = '\n';
	len++;
	//	print_led(&led_bottom, len);
	if (len >= DISPLAY_WIRE_BUFFER_SIZE)
	{
		dieError(45);
	}
	char *ptr = buf;
	// need to send in 32 byte chunks
	//	print_led(&led_top, len);
	while (len > 0)
	{
		int send_len = (len > 32) ? 32 : len;
		Wire.beginTransmission(DISPLAY_I2C_ADDRESS);
		Wire.write(ptr, send_len);
		Wire.endTransmission(true);
		len -= send_len;
		ptr += send_len;
	}
	// request the reply
	Wire.requestFrom(DISPLAY_I2C_ADDRESS, 1);
}
#endif

void setLightPin(int key, bool state)
{
	uint8_t chip;
	uint8_t pin;
	if (getLightFromKey(key, &chip, &pin) == true)
	{
		light_chips[chip]->setPin(pin, state);
	}
}

void check_action_groups_enabled(MikeMap *data)
{
	if ( data->has(INFO_ACTION_GROUPS))
	{
		int status = data->get(INFO_ACTION_GROUPS);
		int mask = 1;
		for (int bit = 0; bit < 10; bit++)
		{
			setLightPin(action_group_buttons[bit], status & mask);
			mask = mask * 2;
		}
	}
}

void check_button_enabled(MikeMap *data, unsigned short key)
{
	if ( data->has(key) )
	{
		bool state = data->get(key) == 1 ? true : false;
		setLightPin(key, state);
	}
}

void update_console(MikeMap *data)
{
	//	static unsigned long last_display_update = 0;

	read_console_updates(&key_updates);
	check_button_enabled(data, BUTTON_RCS);
	check_button_enabled(data, BUTTON_SAS);
	check_button_enabled(data, BUTTON_GEAR);
	//	check_button_enabled( rj, BUTTON_LIGHTS, LIGHT_BUTTON_INDEX);
	//	check_button_enabled( rj, BUTTON_BREAKS, BRAKES_BUTTON_INDEX);
	check_action_groups_enabled(data);

	read_console_updates(&key_updates);
	if( data->has(INFO_SPEED) )
		print_led(led_top, data->get(INFO_SPEED));
	if( data->has(INFO_HEIGHT) )
		print_led(led_bottom, data->get(INFO_HEIGHT));
	read_console_updates(&key_updates);

#ifndef NO_DISPLAYS
	// put stuff into display_updates
	for (const auto &key : display_keys)
	{
		index = check_for_key(data, key);
		if (index != KEY_NOT_FOUND)
		{
			display_updates.set(data[index], data[index + 1]);
		}
	}

	// send stuff to slave, if the slave is ok with this
	// it is OK, if we have a byte on the wire waiting
	if (Wire.available() > 0 && display_updates.get_len() > 0)
	{
		byte b = Wire.read();
		if (b != 77)
		{
			dieError(88);
		}
		// OK, send
		for (unsigned int i = 0; i < display_updates.get_len(); i++)
		{
			MAP_KEY_TYPE k;
			MAP_VALUE_TYPE v;
			display_updates.get_at(i, &k, &v);
			data.add(k);
			data.add(v);
			RAM;
			
		}
		display_updates.clear();
		DynamicJsonDocument root(DISPLAY_WIRE_BUFFER_SIZE);
		root["data"] = data;
		sendToSlave(root);
		RAM;
	}
#endif
}

void read_console_updates(MikeMap *updates)
{
	for (AnalogInput *ai : analog_inputs)
	{
		ai->readInto(updates, false);
	}
	testAllButtons(updates);
	// there are two special buttons :)
	// the two switches on the right top

	bool value = isSwitchEnabled(BUTTON_SWITCH_RIGHT);
	key_chips[4]->setPin(4, value);
	light_chips[0]->setPin(0, value);
	stage_enabled = value;

	key_chips[4]->setPin(5, isSwitchEnabled(BUTTON_SWITCH_LEFT));

	// let "stage" only pass if staging is enabled
	if (updates->get(BUTTON_STAGE) && stage_enabled == false)
	{
		updates->del(BUTTON_STAGE);
	}
}

} // end of namespace

/* main setup and loop function */
#ifndef TEST

void setup()
{
	Wire.begin();
#ifdef PRINT_DEBUG
	Test::run();
#endif
	key_updates.clear();
	display_updates.clear();
	input_data.clear();

	setupLC(led_top, 15);
	setupLC(led_bottom, 3);
	print_led(led_top, 88888888);
	print_led(led_bottom, 88888888);
	delay(1000);
	print_led(led_top, "        ");
	print_led(led_bottom, "        ");
	for (const auto &i : analog_inputs)
	{
		i->calibrate();
	}

	// to act as input, all outputs have to be on HIGH
	for (const auto &kc : key_chips)
	{
		kc->write(0xFF);
	}
	for (const auto &lc : light_chips)
	{
		lc->setInputMask(0x00);
		lc->write(0xff);
	}

	delay(1000);
	print_led(led_top, "        ");
	print_led(led_bottom, "        ");
	// first 4 chips have all pins as inputs
	key_chips[0]->setInputMask(0xff);
	key_chips[1]->setInputMask(0xff);
	key_chips[2]->setInputMask(0xff);
	key_chips[3]->setInputMask(0xff);
	for (PCF8574 *lc : light_chips)
	{
		lc->write(0x00);
	}

	// set input mask for chip 4, all inputs except
	// unset bits 4 and 5 for the two leds
	byte kc5_mask = 0xff;
	kc5_mask &= ~(1 << 4);
	kc5_mask &= ~(1 << 5);
	key_chips[4]->setInputMask(kc5_mask);
	// turn off the two leds
	// LED rechts
	key_chips[4]->setPin(4, 0);
	// LED links
	key_chips[4]->setPin(5, 0);

	// i2c Bus input??
	pinMode(19, INPUT);
	// wait for the i2c slave to initialize
	delay(100);

#ifndef NO_DISPLAYS
	// send init to display
	// this should also give us the initial
	// reply from the display controller to get the thing going
//	DynamicJsonDocument root(DISPLAY_WIRE_BUFFER_SIZE);
	root["chk"] = 1;
	sendToSlave(root);
#endif

	SERIAL_PORT.begin(115200);
	empty_buffer_size = SERIAL_PORT.availableForWrite();
	print_led(led_bottom, "- - -");

#ifdef PRINT_DEBUG
	SERIAL_PORT.println(F("setup ende"));
#endif
}

void loop()
{
	check_serial_port();
	if (message_complete == true)
	{
		char *cmd_ptr;
		read_console_updates(&key_updates);
		// get cmd _from string
		if( (cmd_ptr = strstr( buffer, cmd_start))==NULL )
		{
			dieError(2);
		}
		else
		{
			// jmp over
			cmd_ptr += 6;
			int command = atoi(cmd_ptr);
			if (command == CMD_GET_UPDATES)
			{
				read_console_updates(&key_updates);
				memset( buffer, 0, BUFFER_SIZE);
				strcpy( buffer, "{\"data\":[");
				int l = strlen(buffer);
				key_updates.to_string(&buffer[l]);
				l = strlen(buffer);
				strcpy( &buffer[l], "]}\n");
				key_updates.clear();
				read_console_updates(&key_updates);
				SERIAL_PORT.print(buffer);
				SERIAL_PORT.flush();
			}
			else if (command == CMD_UPDATE_CONSOLE)
			{
				char *data_ptr;
				if( (data_ptr=strstr( buffer, data_start))==NULL )
				{
					dieError(33);
				}
				input_data.from_string( buffer, data_start);
				update_console(&input_data);
			}
			else if (command == CMD_INIT)
			{
				print_led(led_top, "      - ");
				print_led(led_bottom, "      - ");
				have_handshake = true;
			}
			else
			{
				dieError(44);
			}
			
		}
		reset_serial_buffer();
	}
	// some funny blinking as long as we dont have a handshake
	if (!have_handshake)
	{
		delay(50);
		static int digit = 0;
		static int up_or_down = 0;

		print_led(led_top, "        ");
		print_led(led_bottom, "        ");

		if (up_or_down == 0)
		{
			led_top.setChar(0, digit, ' ', true);
		}
		else
		{
			led_bottom.setChar(0, digit, ' ', true);
		}

		digit++;
		if (digit == 8)
		{
			up_or_down = (up_or_down == 0) ? 1 : 0;
			digit = 0;
		}
	}
	else
	{
		// check for pressed buttons anyway and store them
		read_console_updates(&key_updates);
	}
}

#endif

//
// Test Setup for testing analog pins
//
#ifdef ANALOG_TEST

void setup()
{
	for (const auto &i : analog_inputs)
	
		i->calibrate();
	}

	SERIAL_PORT.begin(115200);
	empty_buffer_size = SERIAL_PORT.availableForWrite();
	setupLC(led_top, 15);
	setupLC(led_bottom, 3);
	print_led(led_top, "- - -");
	print_led(led_bottom, "- - -");
}

void loop()
{
	print_led(led_top, ai5.readValue());
	print_led(led_bottom, ai3.readValue());
}
#endif

#ifdef SERIAL_TEST

void setup()
{
	Serial.begin(115200);
	SERIAL_PORT.begin(115200);
}

void loop()
{
	// reply only when you receive data:
	if (Serial.available() > 0)
	{
		// read the incoming byte:
		Serial.read();

		// say what you got:
		Serial.print("1");
		Serial.flush();
	}
	if (SERIAL_PORT.available() > 0)
	{
		// read the incoming byte:
		SERIAL_PORT.read();

		// say what you got:
		SERIAL_PORT.print("2");
		SERIAL_PORT.flush();
	}
}
#endif

#ifdef KEY_TEST

void setup()
{
	Wire.begin();
	key_updates.clear();
	display_updates.clear();
	setupLC(led_top, 15);
	setupLC(led_bottom, 3);
	print_led(led_top, 88888888);
	print_led(led_bottom, 88888888);
	delay(200);
	print_led(led_top, "        ");
	print_led(led_bottom, "        ");
	// to act as input, all outputs have to be on HIGH
	for (const auto &kc : key_chips)
	{
		kc->write(0xFF);
	}
	for (const auto &lc : light_chips)
	{
		lc->setInputMask(0x00);
		lc->write(0xff);
	}
	// first 4 chips have all pins as inputs
	key_chips[0]->setInputMask(0xff);
	key_chips[1]->setInputMask(0xff);
	key_chips[2]->setInputMask(0xff);
	key_chips[3]->setInputMask(0xff);
	for (PCF8574 *lc : light_chips)
	{
		lc->write(0x00);
	}
	// set input mask for chip 4, all inputs except
	// unset bits 4 and 5 for the two leds
	byte kc5_mask = 0xff;
	kc5_mask &= ~(1 << 4);
	kc5_mask &= ~(1 << 5);
	key_chips[4]->setInputMask(kc5_mask);
	// turn off the two leds
	// LED rechts
	key_chips[4]->setPin(4, 0);
	// LED links
	key_chips[4]->setPin(5, 0);
}

void loop()
{
	MikeMap mm1, mm2;
/*
	while(1) {
		print_led( led_top, isSwitchEnabled(BUTTON_SWITCH_RIGHT) ? "1" : "0");
		print_led( led_bottom, kc5.getCurrentSignal());
		key_chips[4]->setPin(5, isSwitchEnabled(BUTTON_SWITCH_LEFT));
		key_chips[4]->setPin(4, isSwitchEnabled(BUTTON_SWITCH_RIGHT));
	}
*/
/*
	while(1) {
		print_led( led_top, (kc5.getCurrentSignal() & (1<<4)) ? "1" : "0");
//		print_led( led_bottom, isSwitchEnabled(BUTTON_SWITCH_RIGHT) ? "1" : "0");
		while(1) {};
//		key_chips[4]->write((1<<4));
		key_chips[4]->setPin(4, 1);
		key_chips[4]->setPin(5, 0);
		delay(500);

//		key_chips[4]->write((1<<5));
		key_chips[4]->setPin(5, 1);
		key_chips[4]->setPin(4, 0);
		delay(500);
	}
*/

	while(1) {
		testAllButtons(&mm1);
		print_led( led_top, isSwitchEnabled(BUTTON_SWITCH_LEFT) ? "1" : "0");
		print_led( led_bottom, isSwitchEnabled(BUTTON_SWITCH_RIGHT) ? "1" : "0");
		key_chips[4]->setPin(5, isSwitchEnabled(BUTTON_SWITCH_LEFT));
		key_chips[4]->setPin(4, isSwitchEnabled(BUTTON_SWITCH_RIGHT));
		testAllButtons(&mm2);
		delay(50);
		testAllButtons(&mm2);
		delay(50);
		testAllButtons(&mm2);
		delay(50);
		testAllButtons(&mm2);
		delay(50);
		testAllButtons(&mm2);
		delay(50);
		testAllButtons(&mm2);
		delay(50);
		testAllButtons(&mm2);
		delay(50);
		testAllButtons(&mm2);
		delay(50);
		testAllButtons(&mm2);
		delay(50);
		testAllButtons(&mm2);
		delay(50);
		testAllButtons(&mm2);
		delay(50);
		testAllButtons(&mm2);
		delay(50);
		testAllButtons(&mm2);
		delay(50);
		testAllButtons(&mm2);
		delay(50);
		testAllButtons(&mm2);
		delay(50);
		print_led( led_top, isSwitchEnabled(BUTTON_SWITCH_LEFT) ? "1" : "0");
		print_led( led_bottom, isSwitchEnabled(BUTTON_SWITCH_RIGHT) ? "1" : "0");
		key_chips[4]->setPin(5, isSwitchEnabled(BUTTON_SWITCH_LEFT));
		key_chips[4]->setPin(4, isSwitchEnabled(BUTTON_SWITCH_RIGHT));
		delay(100);
	}
}

#endif