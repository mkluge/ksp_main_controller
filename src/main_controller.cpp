#include "Arduino.h"
#include "ArduinoJson.h"
#include "AnalogInput.h"
#include "ConsoleSetup.h"
#include "PCF8574.h"
#include "Wire.h"
#include "LedControl.h"
#include "ksp_display_defines.h"
#include "mikemap.h"
#include <ArduinoUnit.h>

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

//#define NOWIRE

#define READ_BUFFER_SIZE 400
char read_buffer[READ_BUFFER_SIZE];
unsigned int read_buffer_offset = 0;
int empty_buffer_size = 0;
bool have_handshake = false;
bool stage_enabled = false;
bool message_complete = false;
// first time this has to be true so that we can send
bool display_reply_complete = true;

#define LOOP_OVER(X) for( unsigned short index=0; index<X; index++)
#define GET_RID_OF( data, index) data.removeAt(index+1); data.removeAt(index);
#define NO_PIN 9
#define NO_CHIP 9
#define NO_KEY -1

LedControl led_top(5, 7, 6, 1);
LedControl led_bottom(8, 10, 9, 1);

AnalogInput ai1( KSP_INPUT_YAW, A5, true);
AnalogInput ai2( KSP_INPUT_PITCH, A6, true);
AnalogInput ai3( KSP_INPUT_ROLL, A7, true);
AnalogInput ai4( KSP_INPUT_XTRANS, A2, true);
AnalogInput ai5( KSP_INPUT_YTRANS, A3, true);
AnalogInput ai6( KSP_INPUT_ZTRANS, A1, true);
AnalogInput ai7( KSP_INPUT_THRUST, A0, false);

PCF8574 kc1(PCF_BASE_ADDRESS + 0);
PCF8574 kc2(PCF_BASE_ADDRESS + 1);
PCF8574 kc3(PCF_BASE_ADDRESS + 2);
PCF8574 kc4(PCF_BASE_ADDRESS + 3);
PCF8574 kc5(PCF_BASE_ADDRESS + 4);

PCF8574 lc1(PCF_BASE_ADDRESS + 5);
PCF8574 lc2(PCF_BASE_ADDRESS + 6);

#define KC1(cpin) *chip=&kc1; *pin=cpin; return true;
#define KC2(cpin) *chip=&kc2; *pin=cpin; return true;
#define KC3(cpin) *chip=&kc3; *pin=cpin; return true;
#define KC4(cpin) *chip=&kc4; *pin=cpin; return true;
#define KC5(cpin) *chip=&kc5; *pin=cpin; return true;
#define LC1(pin) *lchip=&lc1; *lpin=pin; return true;
#define LC2(pin) *lchip=&lc2; *lpin=pin; return true;

// double check used pins
// 5: 0 1 2 3 4 5 6 7 - FULL
// 4: 0 1 2 3 4 5 6 7 - FULL
// 3: 0 1 2 3 4 6 7
// 2: 3 4 5 6 7
// 1: 0 1 2 3 4 5 6 7 - FULL

/*
// main buttons
LightButton lb1( BUTTON_STAGE, &kc1, 4, &lc1, 0); //OK
LightButton lb3( BUTTON_SAS, &kc1, 6, &lc2, 0); //OK
LightButton lb2( BUTTON_RCS, &kc1, 5, &lc1, 1); //OK
LightButton lb4( BUTTON_GEAR, &kc1, 7, &lc2, 1); //OK
LightButton lb5( BUTTON_SWITCH_RIGHT, &kc5, 6); //OK
LightButton lb6( BUTTON_SWITCH_LEFT, &kc5, 7); //OK

// Led panel 0-9 : 2(3) 2(4-7) 3(4) 3(0-3)
// licht 6(0-7) und 7(0-7)
LightButton  lb9( BUTTON_ACTION_1, &kc2, 3, &lc1, 2); //OK
LightButton lb10( BUTTON_ACTION_2, &kc2, 7, &lc1, 4); //OK
LightButton lb11( BUTTON_ACTION_3, &kc2, 6, &lc1, 5); //OK
LightButton lb12( BUTTON_ACTION_4, &kc2, 4, &lc2, 6); //OK
LightButton lb13( BUTTON_ACTION_5, &kc2, 5, &lc1, 7); //OK
LightButton lb14( BUTTON_ACTION_6, &kc3, 7, &lc2, 2); //OK
LightButton lb15( BUTTON_ACTION_7, &kc3, 0, &lc2, 4); //OK
LightButton lb16( BUTTON_ACTION_8, &kc3, 1, &lc2, 5); //OK
LightButton lb17( BUTTON_ACTION_9, &kc3, 2, &lc2, 6); //OK
LightButton lb18( BUTTON_ACTION_10, &kc3, 3, &lc2, 7); //OK
*/
int action_group_buttons[10] = {
	BUTTON_ACTION_1, BUTTON_ACTION_2, BUTTON_ACTION_3, BUTTON_ACTION_4,
	BUTTON_ACTION_5, BUTTON_ACTION_6, BUTTON_ACTION_7, BUTTON_ACTION_8,
	BUTTON_ACTION_9, BUTTON_ACTION_10
};
/*
// all solar in out
// 6er mitte 5(0) 4(3-7)
LightButton lb19( BUTTON_SOLAR_ON, &kc4, 4); //OK
LightButton lb20( BUTTON_SOLAR_OFF, &kc4, 3); //OK
LightButton lb21( BUTTON_FUEL, &kc4, 5); //OK
LightButton lb22( BUTTON_REACTION_WHEELS, &kc4, 6); //OK
LightButton lb23( BUTTON_ENGINES_ON, &kc5, 0); //OK
LightButton lb24( BUTTON_ENGINES_OFF, &kc4, 7); //OK

// 6er unten 4(0-2) ???
LightButton lb25( BUTTON_STORE, &kc4, 0); //OK
LightButton lb27( BUTTON_LOAD, &kc4, 1); //OK
LightButton lb29( BUTTON_CAMERA, &kc4, 2); //OK
LightButton lb26( BUTTON_TEST, &kc3, 4); //OK
LightButton lb28( BUTTON_IVA, &kc3, 5); // looks like it is defect
LightButton lb30( BUTTON_EVA, &kc3, 6); //OK

//15-er rechts: oben 5(1-3),
LightButton lb31( BUTTON_LIGHTS, &kc5, 3); //OK
LightButton lb32( BUTTON_CHUTES, &kc5, 2); //OK
LightButton lb33( BUTTON_ABORT, &kc5, 1); //OK

//5 links: oben 1(0) 1(3-7); 3 statt 0??;
LightButton lb34( BUTTON_SAS_MODE, &kc2, 0); //OK
LightButton lb38( BUTTON_SPEED_MODE, &kc1, 0); //OK
LightButton lb36( BUTTON_THRUST_FULL, &kc1, 1); //OK
LightButton lb37( BUTTON_THRUST_ZERO, &kc1, 2); //OK
LightButton lb35( BUTTON_BREAKS, &kc1, 3); //OK
*/
AnalogInput *analog_inputs[] = {
		&ai1, &ai2, &ai3, &ai4, &ai5, &ai6, &ai7
};
PCF8574 *key_chips[] = {
		&kc1, &kc2, &kc3, &kc4, &kc5
};
PCF8574 *light_chips[] = {
		&lc1, &lc2
};

#define NUM_ANALOG_BUTTONS 7
#define NUM_KEY_CHIPS 5
#define NUM_LIGHT_CHIPS 2

// some button indizes for easier handling
#define STAGE_BUTTON_INDEX 0
#define RCS_BUTTON_INDEX 1
#define SAS_BUTTON_INDEX 2
#define GEAR_BUTTON_INDEX 3
//#define LIGHT_BUTTON_INDEX 6
//#define BRAKES_BUTTON_INDEX 7
bool interrupt_seen = false;

void awakeSlave();
void dieError(int code);
void reset_serial_buffer();
// read the data into the buffer,
// if the current input buffer is not full
void receiveEvent(int how_many);

/* memorizes pressed buttons until stuff can be sent to master */
MikeMap updates;
/* memorizes data that will be send to display */
MikeMap display_updates;

extern uint8_t _end;
extern uint8_t __stack;
uint16_t freemem=1;

void StackPaint(void) __attribute__ ((naked)) __attribute__ ((section (".init1")));
void StackPaint(void)
{
#if 0
    uint8_t *p = &_end;

    while(p <= &__stack)
    {
        *p = STACK_CANARY;
        p++;
    }
#else
    __asm volatile ("    ldi r30,lo8(_end)\n"
                    "    ldi r31,hi8(_end)\n"
                    "    ldi r24,lo8(0xc5)\n" // STACK_CANARY = 0xc5
                    "    ldi r25,hi8(__stack)\n"
                    "    rjmp .cmp\n"
                    ".loop:\n"
                    "    st Z+,r24\n"
                    ".cmp:\n"
                    "    cpi r30,lo8(__stack)\n"
                    "    cpc r31,r25\n"
                    "    brlo .loop\n"
                    "    breq .loop"::);
#endif
}

bool getPinForKey( int key, PCF8574 **chip, byte *pin)
{
	switch(key)
	{
		case BUTTON_SPEED_MODE: KC1(0)
		case BUTTON_THRUST_FULL: KC1(1)
		case BUTTON_THRUST_ZERO: KC1(2)
		case BUTTON_BREAKS: KC1(3)
		case BUTTON_STAGE: KC1(4)
		case BUTTON_SAS: KC1(5)
		case BUTTON_RCS: KC1(6)
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
			case 5: return BUTTON_SAS;
			case 6: return BUTTON_RCS;
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

bool getLightFromKey( int key, PCF8574 **lchip, byte *lpin)
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

uint16_t StackCount(void)
{
	const uint8_t *p = &_end;
	uint16_t       c = 0;

	while(*p == 0xc5 && p <= &__stack)
	{
		p++;
		c++;
	}
	return c;
}

signed int check_for_key( JsonArray &data, short key)
{
	for( unsigned int index=0; index<data.size(); index+=2)
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

void print_led(LedControl *target, long val) {

	int digit = 0;
	bool negative = (val >= 0) ? false : true;
	val = abs(val);
	while (val > 0 && digit < 8)
	{
		int last_digit = val % 10;
		val = val / 10;
		target->setDigit(0, digit, (byte) last_digit, false);
		digit++;
	}
	if (negative && digit < 8) {
		target->setChar(0, digit, '-', false);
		digit++;
	}
	while (digit < 8) {
		target->setChar(0, digit, ' ', false);
		digit++;
	}

}

void print_led(LedControl *target, const char *str) {
	int len = strlen(str);
	int digit = 0;
	while (digit < 8 && len > 0) {
		target->setChar(0, digit, str[len - 1], false);
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
	LOOP_OVER(NUM_KEY_CHIPS) {
		PCF8574 *pcf8754 = key_chips[index];
		byte changed_bits=0x00;
		if ((changed_bits = pcf8754->updateState()) != 0x00) {
			// test all bits and update the json for each bit set
			int current_bit = 0;
			while (changed_bits != 0) {
				if (changed_bits & (0x01)) {
					int key = getKeyForChipPin( index, current_bit);
					if (key != NO_KEY)
					{
						// low active inputs
						updates->set( key,
												 (pcf8754->testPin(current_bit) == false) ? 1 : 0);
					  // remember buttons that trigger the display controller directly
						if( key==BUTTON_NEXT_LEFT_LCD_MODE &&
					      pcf8754->testPin(current_bit) == false) // low active
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
	}
}

void setup() {
	updates.clear();
	display_updates.clear();
	delay(5000);
	Serial.begin(115200);
	Serial.println(F("tests startup"));
	Test::run();
	Serial.println(F("setup startup"));
#ifndef NOWIRE
	Wire.onReceive(receiveEvent);
	Wire.begin(MAIN_CONTROLLER_I2C_ADDRESS);
#endif
Serial.println(F("setup ende"));
  return;

	setupLC(led_top, 15);
	setupLC(led_bottom, 3);
	print_led(&led_top, 88888888);
	print_led(&led_bottom, 88888888);
	delay(1000);
	print_led(&led_top, "        ");
	print_led(&led_bottom, "        ");

	setupLC(led_top, 15);
	setupLC(led_bottom, 3);

	LOOP_OVER(NUM_ANALOG_BUTTONS)
	{
		AnalogInput *i = analog_inputs[index];
		i->calibrate();
	}
	// led chips have outputs only
	lc1.setInputMask( 0x00 );
	lc2.setInputMask( 0x00 );

	// to act as input, all outputs have to be on HIGH
	LOOP_OVER(NUM_KEY_CHIPS)
	{
		PCF8574 *key_chip = key_chips[index];
		key_chip->write(0xFF);
	}
	print_led(&led_top, 88888888);
	print_led(&led_bottom, 88888888);
	// test lamps
	LOOP_OVER(NUM_LIGHT_CHIPS)
	{
		PCF8574 *light_chip = light_chips[index];
		light_chip->write(0xff);
	}

	delay(1000);
	LOOP_OVER(NUM_LIGHT_CHIPS)
	{
		PCF8574 *light_chip = light_chips[index];
		light_chip->write(0x00);
	}
	print_led(&led_top, "        ");
	print_led(&led_bottom, "        ");
	// first 4 chips have all pins as inputs
	kc1.setInputMask( 0xff );
	kc2.setInputMask( 0xff );
	kc3.setInputMask( 0xff );
	kc4.setInputMask( 0xff );
	// set input mask for chip 4, all inputs except
	// unset bits 4 and 5 for the two leds
	byte kc5_mask = 0xff;
	kc5_mask &= ~(1<<4);
	kc5_mask &= ~(1<<5);
	kc5.setInputMask( kc5_mask );
	// turn off the two leds
	// LED rechts
	kc5.setPin(4, 0);
	// LED links
	kc5.setPin(5, 0);

	pinMode(19, INPUT);
	empty_buffer_size = Serial.availableForWrite();
	// wait for the i2c slave to initialize
	delay(100);
	Serial.println(F("setup ende"));
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
void receiveEvent(int how_many) {
	char buf[10];
	memset( buf, 0, 10);
#ifndef NOWIRE
	int buf_offset=0;
	while( Wire.available()>0 )
	{
		char inByte = Wire.read();
		if ( inByte == '\n' )
		{
			display_reply_complete = true;
			// dump if there is more one the wire
			return;
		}
		if( buf_offset<10 )
			buf[buf_offset++]=inByte;
	}
#endif
}

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
	print_led(&led_top, 33333333);
	print_led(&led_bottom, code);
}

void sendToSlave(JsonObject &message) {
	int len=message.measureLength();
	char buf[READ_BUFFER_SIZE];
	memset(buf, 0, READ_BUFFER_SIZE);
	message.printTo(buf, READ_BUFFER_SIZE);
	buf[len] = '\n';
	len++;
	char *ptr=buf;
	// need to send in 32 byte chunks
	while( len>0 )
	{
		int send_len = (len>32) ? 32 : len;
#ifndef NOWIRE
		Wire.beginTransmission(DISPLAY_I2C_ADDRESS);
		Wire.write(ptr, send_len);
		Wire.endTransmission();
#endif
		len -= send_len;
		ptr += send_len;
	}
}

void setLightPin( int key, bool state)
{
	PCF8574 *lchip;
	byte lpin;
	getLightFromKey( key, &lchip, &lpin);
	lchip->setPin( lpin, state);
}

void check_action_groups_enabled(JsonArray& rj)
{
	auto index = check_for_key( rj, INFO_ACTION_GROUPS);
	if ( index!=KEY_NOT_FOUND ) {
		int data=rj[index+1];
		int mask=1;
		for( int bit=0; bit<10; bit++)
		{
			setLightPin( action_group_buttons[bit], data&mask);
			mask=mask*2;
		}
		GET_RID_OF( rj, index);
	}
}

void check_button_enabled(JsonArray& rj, unsigned short key, unsigned short button_index) {
	auto index=check_for_key( rj, key);
	if ( index!=KEY_NOT_FOUND) {
		int val = rj[index+1];
		bool state = (val == 1) ? true : false;
		setLightPin( key, state);
		GET_RID_OF( rj, index);
	}
}

void update_console(JsonObject& obj)
{
	JsonArray& rj=obj["data"];
  check_button_enabled( rj, BUTTON_RCS, RCS_BUTTON_INDEX);
	check_button_enabled( rj, BUTTON_SAS, SAS_BUTTON_INDEX);
	check_button_enabled( rj, BUTTON_GEAR, GEAR_BUTTON_INDEX);
//	check_button_enabled( rj, BUTTON_LIGHTS, LIGHT_BUTTON_INDEX);
//	check_button_enabled( rj, BUTTON_BREAKS, BRAKES_BUTTON_INDEX);
	check_action_groups_enabled(rj);

	auto index = check_for_key( rj, INFO_SPEED);
	if ( index != KEY_NOT_FOUND ) {
	  print_led( &led_top, (long) rj[index+1]);
		GET_RID_OF( rj, index);
	}
	index = check_for_key( rj, INFO_HEIGHT);
	if ( index != KEY_NOT_FOUND ) {
	  print_led(&led_bottom, (long) rj[index+1]);
	  GET_RID_OF( rj, index);
	}

	// wenn noch lang genug -> display controller
	if (rj.size() > 0 || display_updates.get_len()>0) {
		// can we send or do we have to store?
		if( display_reply_complete==true )
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
		} else {
			// no, just store stuff
			for( unsigned index=0; index<rj.size(); index+=2)
			{
				display_updates.set( rj[index], rj[index+1]);
			}
		}
	}
}

void read_console_updates(MikeMap *updates)
{
	LOOP_OVER(NUM_ANALOG_BUTTONS)
	{
		AnalogInput *i = analog_inputs[index];
		i->readInto( updates, false);
	}
	testAllButtons(updates);
	// there are two special buttons :)
	// the two switches on the right top
	if ( updates->has(BUTTON_SWITCH_RIGHT))
	{
		bool value = (updates->get(BUTTON_SWITCH_RIGHT)==1) ? true : false;
		key_chips[4]->setPin(4, value);
		light_chips[0]->setPin(0, value);
		stage_enabled = value;
	}
	if ( updates->has(BUTTON_SWITCH_LEFT))
	{
		kc5.setPin( 5, (updates->get(BUTTON_SWITCH_LEFT)) ? true : false);
	}
	// let "stage" only pass if staging is enabled
	if ( updates->get(BUTTON_STAGE) && stage_enabled==false)
	{
		updates->del(BUTTON_STAGE);
	}
}

void loop()
{
	char buf[100];
	sprintf( buf, "S: %d", StackCount());
	Serial.println(buf);
//	while(1) {};
	reset_serial_buffer();
	check_serial_port();
	if (message_complete == true) {
		StaticJsonBuffer<READ_BUFFER_SIZE> readBuffer;
		JsonObject& rj = readBuffer.parseObject(read_buffer);
//		freemem = StackCount();

		if (!rj.success()) {
			dieError(2);
		} else {
			if( !rj.containsKey("cmd") )
			{
				dieError(7);
			}
			if( rj["cmd"] == CMD_GET_UPDATES )
			{
				read_console_updates( &updates );
				StaticJsonBuffer<READ_BUFFER_SIZE> writeBuffer;
				JsonObject& root = writeBuffer.createObject();
				JsonArray& data = root.createNestedArray("data");
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
				root.printTo(Serial);
				Serial.print('\n');
				Serial.flush();
			}
			else if( rj["cmd"]== CMD_UPDATE_CONSOLE )
			{
				update_console( rj );
			}
			else if( rj["cmd"]== CMD_INIT )
			{
				print_led(&led_top, "        ");
				print_led(&led_bottom, "        ");
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

		print_led(&led_top, "        ");
		print_led(&led_bottom, "        ");

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
