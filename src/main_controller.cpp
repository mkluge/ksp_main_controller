#undef TEST
#undef PRINT_DEBUG

#ifndef TEST
#include "Arduino.h"
#include "ArduinoJson.h"
#include "AnalogInput.h"
#include "ConsoleSetup.h"
#include "PCF8574.h"
#include "Wire.h"
#include "LedControl.h"
#include "ksp_display_defines.h"
#include "mikemap.h"
//#include <ArduinoUnit.h>

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
 4 kn�pfe mitte: 0(4-7)
 licht 6(0-7) und 7(0-7)

 5 links: oben 1(0) 1(3-7); 3 statt 0??;

 */


#define NUM_ANALOG_BUTTONS 7
#define NUM_KEY_CHIPS 5
#define NUM_LIGHT_CHIPS 2

#define READ_BUFFER_SIZE 400
char read_buffer[READ_BUFFER_SIZE];
unsigned int read_buffer_offset = 0;
int empty_buffer_size = 0;
bool have_handshake = false;
bool stage_enabled = false;
bool message_complete = false;

#define NO_PIN 9
#define NO_CHIP 9
#define NO_KEY -1

#define DISPLAY_UPDATE_MILLISECONDS 10000

LedControl led_top(5, 7, 6, 1);
LedControl led_bottom(8, 10, 9, 1);

// double check used pins
// 5: 0 1 2 3 4 5 6 7 - FULL
// 4: 0 1 2 3 4 5 6 7 - FULL
// 3: 0 1 2 3 4 6 7
// 2: 3 4 5 6 7
// 1: 0 1 2 3 4 5 6 7 - FULL

int action_group_buttons[10] = {
	BUTTON_ACTION_1, BUTTON_ACTION_2, BUTTON_ACTION_3, BUTTON_ACTION_4,
	BUTTON_ACTION_5, BUTTON_ACTION_6, BUTTON_ACTION_7, BUTTON_ACTION_8,
	BUTTON_ACTION_9, BUTTON_ACTION_10
};

AnalogInput analog_inputs[] = {
	AnalogInput( KSP_INPUT_YAW, A5, true),
	AnalogInput( KSP_INPUT_PITCH, A6, true),
	AnalogInput( KSP_INPUT_ROLL, A7, true),
	AnalogInput( KSP_INPUT_XTRANS, A2, true),
	AnalogInput( KSP_INPUT_YTRANS, A3, true),
	AnalogInput( KSP_INPUT_ZTRANS, A1, true),
	AnalogInput( KSP_INPUT_THRUST, A0, false)
};

PCF8574 key_chips[] = {
	PCF8574(PCF_BASE_ADDRESS + 0),
	PCF8574(PCF_BASE_ADDRESS + 1),
	PCF8574(PCF_BASE_ADDRESS + 2),
	PCF8574(PCF_BASE_ADDRESS + 3),
	PCF8574(PCF_BASE_ADDRESS + 4)
};

PCF8574 light_chips[] = {
	PCF8574(PCF_BASE_ADDRESS + 5),
	PCF8574(PCF_BASE_ADDRESS + 6)
};

#define KC1(kpin) *chip=0; *pin=kpin; return true;
#define KC2(kpin) *chip=1; *pin=kpin; return true;
#define KC3(kpin) *chip=2; *pin=kpin; return true;
#define KC4(kpin) *chip=3; *pin=kpin; return true;
#define KC5(kpin) *chip=4; *pin=kpin; return true;
#define LC1(lpin) *chip=0; *pin=lpin; return true;
#define LC2(lpin) *chip=1; *pin=lpin; return true;

// some button indizes for easier handling
#define STAGE_BUTTON_INDEX 0
#define RCS_BUTTON_INDEX 1
#define SAS_BUTTON_INDEX 2
#define GEAR_BUTTON_INDEX 3
//#define LIGHT_BUTTON_INDEX 6
//#define BRAKES_BUTTON_INDEX 7
bool interrupt_seen = false;

void dieError(int code);
void reset_serial_buffer();
void sendToSlave(const JsonDocument& message);

/* memorizes pressed buttons until stuff can be sent to master */
MikeMap updates;
/* memorizes data that will be send to display */
MikeMap display_updates;

int freeRam () {
  extern int __heap_start, *__brkval;
  int v;
  return (int) &v - (__brkval == 0 ? (int) &__heap_start : (int) __brkval);
}

bool getPinForKey( int key, uint8_t *chip, uint8_t *pin)
{
	switch(key)
	{
		case BUTTON_SPEED_MODE: KC1(0)
		case BUTTON_THRUST_FULL: KC1(1)
		case BUTTON_THRUST_ZERO: KC1(2)
		case BUTTON_BREAKS: KC1(3)
		case BUTTON_STAGE: KC1(4)
		case BUTTON_RCS: KC1(5)
		case BUTTON_SAS: KC1(6)
		case BUTTON_GEAR: KC1(7)
		case BUTTON_SAS_MODE: KC2(0)
		case BUTTON_ACTION_1: KC2(3)
		case BUTTON_ACTION_4: KC2(4)
		case BUTTON_ACTION_5: KC2(5)
		case BUTTON_ACTION_3: KC2(6)
		case BUTTON_ACTION_2: KC2(7)
    case BUTTON_ACTION_7: KC3(0)
		case BUTTON_ACTION_8: KC3(1)
		case BUTTON_ACTION_9: KC3(2)
		case BUTTON_ACTION_10: KC3(3)
		case BUTTON_TEST: KC3(4)
		case BUTTON_IVA: KC3(5)
		case BUTTON_EVA: KC3(6)
		case BUTTON_ACTION_6: KC3(7)
		case BUTTON_STORE: KC4(0)
		case BUTTON_LOAD: KC4(1)
		case BUTTON_CAMERA: KC4(2)
		case BUTTON_SOLAR_OFF: KC4(2)
		case BUTTON_SOLAR_ON: KC4(3)
		case BUTTON_FUEL: KC4(5)
		case BUTTON_REACTION_WHEELS: KC4(6)
		case BUTTON_ENGINES_OFF: KC4(7)
		case BUTTON_ENGINES_ON: KC5(0)
		case BUTTON_ABORT: KC5(1)
		case BUTTON_CHUTES: KC5(2)
		case BUTTON_LIGHTS: KC5(3)
		case BUTTON_SWITCH_RIGHT: KC5(6)
		case BUTTON_SWITCH_LEFT: KC5(7)
	}
	return false;
}

signed int getKeyForChipPin( int key_chip, int current_bit)
{
	// 0 is kc1
	switch(key_chip)
	{
		case 0: switch(current_bit) {
			case 0: return BUTTON_SPEED_MODE;
			case 1: return BUTTON_THRUST_FULL;
			case 2: return BUTTON_THRUST_ZERO;
			case 3: return BUTTON_BREAKS;
			case 4: return BUTTON_STAGE;
			case 5: return BUTTON_RCS;
			case 6: return BUTTON_SAS;
			case 7: return BUTTON_GEAR;
		}
		case 1: switch(current_bit) {
			case 0: return BUTTON_SAS_MODE;
			case 3: return BUTTON_ACTION_1;
			case 4: return BUTTON_ACTION_4;
			case 5: return BUTTON_ACTION_5;
			case 6: return BUTTON_ACTION_3;
			case 7: return BUTTON_ACTION_2;
		}
		case 2: switch(current_bit) {
			case 0: return BUTTON_ACTION_7;
			case 1: return BUTTON_ACTION_8;
			case 2: return BUTTON_ACTION_9;
			case 3: return BUTTON_ACTION_10;
			case 4: return BUTTON_TEST;
			case 5: return BUTTON_IVA;
			case 6: return BUTTON_EVA;
			case 7: return BUTTON_ACTION_6;
		}
		case 3: switch(current_bit) {
			case 0: return BUTTON_STORE;
			case 1: return BUTTON_LOAD;
			case 2: return BUTTON_CAMERA;
			case 3: return BUTTON_SOLAR_OFF;
			case 4: return BUTTON_SOLAR_ON;
			case 5: return BUTTON_FUEL;
			case 6: return BUTTON_REACTION_WHEELS;
			case 7: return BUTTON_ENGINES_OFF;
		}
		case 4: switch(current_bit) {
			case 0: return BUTTON_ENGINES_ON;
			case 1: return BUTTON_ABORT;
			case 2: return BUTTON_CHUTES;
			case 3: return BUTTON_LIGHTS;
			case 6: return BUTTON_SWITCH_RIGHT;
			case 7: return BUTTON_SWITCH_LEFT;
		}
	}
	return NO_KEY;
}

bool getLightFromKey( int key, uint8_t *chip, uint8_t *pin)
{
	switch(key)
	{
		case BUTTON_STAGE: LC1(0)
		case BUTTON_RCS: LC1(1)
		case BUTTON_ACTION_1: LC1(2)
		case BUTTON_ACTION_2: LC1(4)
		case BUTTON_ACTION_3: LC1(5)
		case BUTTON_ACTION_4: LC2(6)
		case BUTTON_ACTION_5: LC1(7)
		case BUTTON_SAS: LC2(0)
		case BUTTON_GEAR: LC2(1)
		case BUTTON_ACTION_6: LC2(2)
		case BUTTON_ACTION_7: LC2(4)
		case BUTTON_ACTION_8: LC2(5)
		case BUTTON_ACTION_9: LC2(6)
		case BUTTON_ACTION_10: LC2(7)
	}
	return false;
}

signed int check_for_key( JsonArray data, short key)
{
	for( size_t index=0; index<data.size(); index+=2)
	{
		if( data[index]==key )
		{
			return index;
		}
	}
	return KEY_NOT_FOUND;
}

void setupLC(LedControl &lc, int intensity) {
	lc.shutdown(0, false); // turn off power saving, enables display
	lc.setIntensity(0, intensity); // sets brightness (0~15 possible values)
	lc.clearDisplay(0); // clear screen
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
		target.setDigit(0, digit, (byte) last_digit, false);
		digit++;
	}
	if (negative && digit < 8) {
		target.setChar(0, digit, '-', false);
		digit++;
	}
	while (digit < 8) {
		target.setChar(0, digit, ' ', false);
		digit++;
	}
}

void print_led(LedControl &target, const char *str) {
	int len = strlen(str);
	int digit = 0;
	while (digit < 8 && len > 0) {
		target.setChar(0, digit, str[len - 1], false);
		len--;
		digit++;
	}
}

/**
 * checks all buttons and if anyone changed its state, adds the new state
 * of the botton to the json object
 */
void testAllButtons(MikeMap *updates) {
// update chips
	int index=0;
	for( auto pcf8754 : key_chips )
	{
		byte changed_bits=0x00;
		if ((changed_bits = pcf8754.updateState()) != 0x00) {
			// test all bits and update the json for each bit set
			int current_bit = 0;
			while (changed_bits != 0) {
				if (changed_bits & (0x01)) {
					int key = getKeyForChipPin( index, current_bit);
					if (key != NO_KEY)
					{
						// low active inputs
						updates->set( key,
												 (pcf8754.testPin(current_bit) == false) ? 1 : 0);
					  // remember buttons that trigger the display controller directly
						if( key==BUTTON_NEXT_LEFT_LCD_MODE &&
					      pcf8754.testPin(current_bit) == false) // low active
						{
							display_updates.set( BUTTON_NEXT_LEFT_LCD_MODE, 1);
						}
					}
					else
					{
						updates->set( 401+index, current_bit);
					}
				}
				current_bit++;
				changed_bits >>= 1;
			}
		}
		index++;
	}
}

void setup() {

	Wire.begin();

#ifdef PRINT_DEBUG
	Test::run();
#endif
	updates.clear();
	display_updates.clear();
	setupLC(led_top, 15);
	setupLC(led_bottom, 3);
	print_led(led_top, 88888888);
	print_led(led_bottom, 88888888);
	delay(1000);
	print_led(led_top, "        ");
	print_led(led_bottom, "        ");
	for( auto i : analog_inputs)
	{
		i.calibrate();
	}

	// to act as input, all outputs have to be on HIGH
	for( auto kc : key_chips)
	{
		kc.write(0xFF);
	}
	for( auto lc : light_chips)
	{
		lc.setInputMask( 0x00 );
		lc.write(0xff);
	}

	delay(1000);
	print_led(led_top, "        ");
	print_led(led_bottom, "        ");
	// first 4 chips have all pins as inputs
	key_chips[0].setInputMask( 0xff );
	key_chips[1].setInputMask( 0xff );
	key_chips[2].setInputMask( 0xff );
	key_chips[3].setInputMask( 0xff );
	for( auto lc : light_chips)
	{
		lc.write(0x00);
	}

	// set input mask for chip 4, all inputs except
	// unset bits 4 and 5 for the two leds
	byte kc5_mask = 0xff;
	kc5_mask &= ~(1<<4);
	kc5_mask &= ~(1<<5);
	key_chips[4].setInputMask( kc5_mask );
	// turn off the two leds
	// LED rechts
	key_chips[4].setPin(4, 0);
	// LED links
	key_chips[4].setPin(5, 0);

	// i2c Bus input??
	pinMode(19, INPUT);
	// wait for the i2c slave to initialize
	delay(100);

  // send init to display
	StaticJsonDocument<DISPLAY_WIRE_BUFFER_SIZE> root;
	root["chk"]=1;
	sendToSlave(root);

	Serial.begin(115200);
	empty_buffer_size = Serial.availableForWrite();
	print_led( led_bottom, "- - -");

#ifdef PRINT_DEBUG
	Serial.println(F("setup ende"));
#endif
}

void reset_serial_buffer() {
	memset(read_buffer, 0, READ_BUFFER_SIZE);
	read_buffer_offset = 0;
	message_complete=false;
}

int serial_read_until(char delimiter, int max_bytes)
{
	int bytes_read=0;
	while (1) {
		if( !Serial.available() )
		{
			continue;
		}
//		print_led(&led_top, bytes_read);
		char inByte = Serial.read();
		bytes_read++;
		if (read_buffer_offset < (READ_BUFFER_SIZE - 1)) {
			read_buffer[read_buffer_offset] = (char) inByte;
			read_buffer_offset++;
		} else {
			dieError(99);
		}
		// we are done if we have reached the delimiter
		if (inByte==delimiter) {
			message_complete = true;
			return bytes_read;
		}
		// or if we've read the max number of bytes
		if( bytes_read==max_bytes ) {
			return bytes_read;
		}
	}
}

// read the data into the buffer,
// if the current input buffer is not full

// called automatically when serial data is available
void check_serial_port() {
	if ( message_complete==true )
	{
		return;
	}
	// nothing is waiting, so just leave ...
	if( !Serial.available() )
	{
		return;
	}
	// first: read the number of bytes, schould'nt be nore than 10
	serial_read_until( ':', 10);
	int bytes_to_read=atoi(read_buffer);
//	print_led(&led_bottom, bytes_to_read);
	reset_serial_buffer();

	// second: read so many bytes in 32 byte chunks
	while (bytes_to_read>0) {
		int bytes_read = serial_read_until( '\n', 32);
		Serial.print(F("OK"));
		Serial.flush();
		bytes_to_read -= bytes_read;
		if ( message_complete == true )
		{
			return;
		}
	}
}

void dieError(int code) {
	print_led(led_top, 33333333);
	print_led(led_bottom, code);
	while(44);
}

void sendToSlave(const JsonDocument& message) {
	char buf[DISPLAY_WIRE_BUFFER_SIZE];
	int len=serializeJson( message, buf);
	buf[len] = '\n';
	len++;
//	print_led(&led_bottom, len);
	if( len>=DISPLAY_WIRE_BUFFER_SIZE )
	{
		dieError(45);
	}
	char *ptr=buf;
	// need to send in 32 byte chunks
//	print_led(&led_top, len);
	while( len>0 )
	{
		int send_len = (len>32) ? 32 : len;
		Wire.beginTransmission(DISPLAY_I2C_ADDRESS);
		Wire.write(ptr, send_len);
		Wire.endTransmission(true);
		len -= send_len;
		ptr += send_len;
	}
}

void setLightPin( int key, bool state)
{
	uint8_t chip;
	uint8_t pin;
	getLightFromKey( key, &chip, &pin);
	light_chips[chip].setPin( pin, state);
}

void check_action_groups_enabled(JsonArray data)
{
	auto index = check_for_key( data, INFO_ACTION_GROUPS);
	if ( index!=KEY_NOT_FOUND ) {
		int status=data[index+1];
		int mask=1;
		for( int bit=0; bit<10; bit++)
		{
			setLightPin( action_group_buttons[bit], status&mask);
			mask=mask*2;
		}
		data.remove(index+1);
		data.remove(index);
	}
}

void check_button_enabled(JsonArray data, unsigned short key) {
	auto index=check_for_key( data, key);
	if ( index!=KEY_NOT_FOUND) {
		int val = data[index+1];
		bool state = (val == 1) ? true : false;
		setLightPin( key, state);
		data.remove(index+1);
		data.remove(index);
	}
}

void update_console(JsonArray data)
{
	static unsigned long last_display_update = 0;

  check_button_enabled( data, BUTTON_RCS);
	check_button_enabled( data, BUTTON_SAS);
	check_button_enabled( data, BUTTON_GEAR);
//	check_button_enabled( rj, BUTTON_LIGHTS, LIGHT_BUTTON_INDEX);
//	check_button_enabled( rj, BUTTON_BREAKS, BRAKES_BUTTON_INDEX);
	check_action_groups_enabled(data);

	auto index = check_for_key( data, INFO_SPEED);
	if ( index != KEY_NOT_FOUND ) {
	  print_led( led_top, (long) data[index+1]);
	}
	print_led( led_bottom, 4);
	index = check_for_key( data, INFO_HEIGHT);
	if ( index != KEY_NOT_FOUND ) {
	  print_led( led_bottom, (long) data[index+1]);
	}
	print_led( led_bottom, 5);

	// wenn noch lang genug -> display controller
	//	char pbuf[9];
	//	sprintf( pbuf, "%d-%d-%d", rj.size(), display_updates.get_len(), (display_reply_complete==true) ? 1 : 0);
	//	sprintf( pbuf, "%d", freeRam());
	//	pbuf[8]=0;
	//	print_led( &led_top, pbuf);
//	von kaputt, muss dann wieder ein : unsigned long elapsed_millies = millis() - last_display_update;
//  print_led( &led_top, elapsed_millies);

/*
kaputt!!

	if( (rj.size() > 0 || display_updates.get_len()>0) &&
      elapsed_millies>DISPLAY_UPDATE_MILLISECONDS) {
		// can we send or do we have to store?
		byte can_send = 1;
		if( can_send>0 )
		{
			// OK, send
			for( int i=0; i<display_updates.get_len(); i++)
			{
				int k;
				int v;
				display_updates.get_at( i, &k, &v);
				rj.add(k);
				rj.add(v);
			}
			obj["data"]=rj;
			display_updates.clear();
		  sendToSlave(obj);
			last_display_update = millis();
		} else {
			// no, just store stuff
			for( unsigned index=0; index<rj.size(); index+=2)
			{
				display_updates.set( rj[index], rj[index+1]);
			}
		}
	}
	*/
}

void read_console_updates(MikeMap *updates)
{
	for( auto ai : analog_inputs)
	{
		ai.readInto( updates, false);
	}
	testAllButtons(updates);
	// there are two special buttons :)
	// the two switches on the right top
	if ( updates->has(BUTTON_SWITCH_RIGHT))
	{
		bool value = (updates->get(BUTTON_SWITCH_RIGHT)==1) ? true : false;
		key_chips[4].setPin(4, value);
		light_chips[0].setPin(0, value);
		stage_enabled = value;
	}
	if ( updates->has(BUTTON_SWITCH_LEFT))
	{
		key_chips[4].setPin( 5, (updates->get(BUTTON_SWITCH_LEFT)) ? true : false);
	}
	// let "stage" only pass if staging is enabled
	if ( updates->get(BUTTON_STAGE) && stage_enabled==false)
	{
		updates->del(BUTTON_STAGE);
	}
}

void loop()
{
	reset_serial_buffer();
	check_serial_port();
	if (message_complete == true) {
		StaticJsonDocument<READ_BUFFER_SIZE> rj;
		DeserializationError error = deserializeJson( rj, read_buffer);
		if (error) {
			dieError(2);
		} else {
			if( !rj.containsKey("cmd") )
			{
				dieError(7);
			}
			if( rj["cmd"] == CMD_GET_UPDATES )
			{
				print_led( led_top, " - - - -");
				print_led( led_bottom, "      - ");
				read_console_updates( &updates );
				StaticJsonDocument<READ_BUFFER_SIZE> root;
				JsonArray data = root.createNestedArray("data");
				// read all updates and put them into the updates
				for( int i=0; i<updates.get_len(); i++)
				{
					int k;
					int v;
					updates.get_at( i, &k, &v);
					data.add(k);
					data.add(v);
				}
				updates.clear();
				serializeJson( root, Serial);
				Serial.print('\n');
				Serial.flush();
			}
			else if( rj["cmd"]== CMD_UPDATE_CONSOLE )
			{
				print_led( led_top, "   - - -");
				print_led( led_bottom, "      - ");
				update_console( rj["data"] );
			}
			else if( rj["cmd"]== CMD_INIT )
			{
				print_led( led_top, "     - -");
				print_led( led_bottom, "      - ");
				have_handshake = true;
			}
		}
	}
  // some funny blinking as long as we dont have a handshake
	if( !have_handshake )
	{
		delay(50);
		static int digit=0;
		static int up_or_down = 0;

		print_led( led_top, "        ");
		print_led( led_bottom, "        ");

		if( up_or_down== 0 )
		{
			led_top.setChar( 0, digit, ' ', true);
		}
		else
		{
			led_bottom.setChar( 0, digit, ' ', true);
		}

		digit++;
		if( digit==8 )
		{
			up_or_down = ( up_or_down==0 ) ? 1 : 0;
			digit = 0;
		}
	}
	else
	{
		// check for pressed buttons anyway and store them
		read_console_updates( &updates );
	}
}


#else
/****************************************
 *
 * Debug Setup and Loop
 *
 ****************************************/


#endif
