#include "Arduino.h"
#include "ArduinoJson.h"
#include "AnalogInput.h"
#include "ConsoleSetup.h"
#include "LightButton.h"
#include "PCF8574.h"
#include "Wire.h"
#include "LedControl.h"

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

#define READ_BUFFER_SIZE 300
char read_buffer[READ_BUFFER_SIZE];
unsigned int read_buffer_offset = 0;
int empty_buffer_size = 0;
bool have_handshake = false;
bool stage_enabled = false;
bool message_complete = false;
#define PCF_BASE_ADDRESS 0x38
#define LOOP_OVER(X) for( int index=0; index<X; index++)

LedControl led_top(5, 7, 6, 1);
LedControl led_bottom(8, 10, 9, 1);

AnalogInput ai1("yaw", A5, true);
AnalogInput ai2("pitch", A6, true);
AnalogInput ai3("roll", A7, true);
AnalogInput ai4("xtrans", A2, true);
AnalogInput ai5("ytrans", A3, true);
AnalogInput ai6("ztrans", A1, true);
AnalogInput ai7("thrust", A0, false);

PCF8574 kc1(PCF_BASE_ADDRESS + 0);
PCF8574 kc2(PCF_BASE_ADDRESS + 1);
PCF8574 kc3(PCF_BASE_ADDRESS + 2);
PCF8574 kc4(PCF_BASE_ADDRESS + 3);
PCF8574 kc5(PCF_BASE_ADDRESS + 4);

PCF8574 lc1(PCF_BASE_ADDRESS + 5);
PCF8574 lc2(PCF_BASE_ADDRESS + 6);

// main buttons
LightButton lb1("stage", &kc1, 4, &lc1, 0);
LightButton lb2("rcs", &kc1, 5, &lc1, 1);
LightButton lb3("sas", &kc1, 6, &lc2, 0);
LightButton lb4("gear", &kc1, 7, &lc2, 1);
LightButton lb5("switch_right", &kc5, 6);
LightButton lb6("switch_left", &kc5, 7);
LightButton lb7("lights", &kc2, 7, NULL, 0);
LightButton lb8("brakes", &kc1, 0, NULL, 0);

// Led panel 0-9 : 2(3) 2(4-7) 3(4) 3(0-3)
// licht 6(0-7) und 7(0-7)
LightButton lb9( "ag1", &kc2, 3, &lc2, 0);
LightButton lb10("ag2", &kc2, 4, &lc2, 1);
LightButton lb11("ag3", &kc2, 5, &lc2, 2);
LightButton lb12("ag4", &kc2, 6, &lc2, 3);
LightButton lb13("ag5", &kc2, 7, &lc2, 4);
LightButton lb14("ag6", &kc3, 4, &lc2, 5);
LightButton lb15("ag7", &kc3, 0, &lc2, 6);
LightButton lb16("ag8", &kc3, 1, &lc2, 7);
LightButton lb17("ag9", &kc3, 2, &lc1, 6);
LightButton lb18("ag10", &kc3, 3, &lc1, 7);
LightButton *action_group_buttons[10] = {
	&lb9, &lb10, &lb11, &lb12, &lb13, &lb14, &lb15, &lb16, &lb17, &lb18,
};

// all solar in out
LightButton lb19("solar1", &kc4, 4, NULL, 0);
LightButton lb20("solar0", &kc4, 3, NULL, 0);

#define NUM_ANALOG_BUTTONS 7
#define NUM_KEY_CHIPS 5
#define NUM_LED_CHIPS 2
#define NUM_LIGHT_BUTTONS 20

AnalogInput *analog_inputs[NUM_ANALOG_BUTTONS] = {
		&ai1, &ai2, &ai3, &ai4, &ai5, &ai6, &ai7
};
PCF8574 *key_chips[NUM_KEY_CHIPS] = {
		&kc1, &kc2, &kc3, &kc4, &kc5
};
PCF8574 *led_chips[NUM_LED_CHIPS] = {
		&lc1, &lc2
};
LightButton *buttons[NUM_LIGHT_BUTTONS] = {
	&lb1, &lb2, &lb3, &lb4, &lb5, &lb6, &lb7, &lb8, &lb9,
	&lb10, &lb11, &lb12, &lb13, &lb14, &lb15, &lb16, &lb17, &lb18, &lb19,
	&lb20
};

// some button indizes for easier handling
#define STAGE_BUTTON 0
#define RCS_BUTTON 1
#define SAS_BUTTON 2
#define GEAR_BUTTON 3
#define LIGHT_BUTTON 6
#define BRAKES_BUTTON 7

bool interrupt_seen = false;

void wait_for_handshake();
void awakeSlave();
void dieError(int code);
void reset_serial_buffer();

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
void testAllButtons(JsonObject& root) {
// update chips
	LOOP_OVER(NUM_KEY_CHIPS) {
		PCF8574 *pcf8754 = key_chips[index];
		byte changed_bits=0x00;
		if ((changed_bits = pcf8754->updateState()) != 0x00) {
			// test all bits and update the json for each bit set
			int current_bit = 0;
			while (changed_bits != 0) {
				if (changed_bits & (0x01)) {
					LightButton *button = pcf8754->getButtonForPin(current_bit);
					if (button != NULL)
					{
						// low active inputs
						root[button->getName()] =
								(pcf8754->testPin(current_bit) == false) ? 1 : 0;
					}
				}
				current_bit++;
				changed_bits >>= 1;
			}
		}
	}
}

void setup() {

	Serial.begin(115200);
	Wire.begin();
	awakeSlave();

	setupLC(led_top, 15);
	setupLC(led_bottom, 3);

	LOOP_OVER(NUM_ANALOG_BUTTONS)
	{
		AnalogInput *i = analog_inputs[index];
		i->calibrate();
	}

	// to act as input, all outputs have to be on HIGH
	LOOP_OVER(NUM_KEY_CHIPS)
	{
		PCF8574 *key_chip = key_chips[index];
		key_chip->write(0xFF);
	}

	// test lamps
	LOOP_OVER(NUM_LED_CHIPS)
	{
		PCF8574 *led_chip = led_chips[index];
		led_chip->write(0xff);
	}
	print_led(&led_top, 88888888);
	print_led(&led_bottom, 88888888);
	delay(1000);
	LOOP_OVER(NUM_LED_CHIPS) {
		PCF8574 *led_chip = led_chips[index];
		led_chip->write(0x00);
	}
	print_led(&led_top, "        ");
	print_led(&led_bottom, "        ");
	// led chips have outputs only
	lc1.setInputMask( 0x00 );
	lc2.setInputMask( 0x00 );
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
	wait_for_handshake();
	reset_serial_buffer();
	// wait for the i2c slave to initialize
	delay(100);
	print_led(&led_top, "--");
	delay(100);
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
		bytes_read++;
		print_led(&led_top, bytes_read);
		char inByte = Serial.read();
		if (inByte==delimiter || bytes_read==max_bytes) {
			message_complete = true;
			return bytes_read;
		}
		if (read_buffer_offset < (READ_BUFFER_SIZE - 1)) {
			read_buffer[read_buffer_offset] = (char) inByte;
			read_buffer_offset++;
		} else {
			dieError(3);
		}
	}
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
	// first: read the number of bytes
	serial_read_until( ':', 10);
	int bytes_to_read=atoi(read_buffer);
	print_led(&led_bottom, bytes_to_read);
	reset_serial_buffer();

	// second: read so many bytes in 32 byte chunks
	while (bytes_to_read>0) {
		if( !Serial.available() )
		{
			continue;
		}
		int bytes_read = serial_read_until( '\n', 32);
		Serial.print("OK");
		Serial.flush();
		bytes_to_read -= bytes_read;
		if ( message_complete == true )
		{
			return;
		}
	}
}

void wait_for_handshake() {
	reset_serial_buffer();
	bool dot_on = true;
	while (true) {
		check_serial_port();
		if (message_complete == false) {
			if (dot_on == true) {
				print_led(&led_top, "   .");
				print_led(&led_bottom, "    ");
			} else {
				print_led(&led_top, "    ");
				print_led(&led_bottom, "   .");
			}
			dot_on = !dot_on;
			delay(100);
		} else {
			DynamicJsonBuffer sjb;
			JsonObject& rj = sjb.parseObject(read_buffer);
			// the only way to get this thing going
			if (rj.success() && rj["start"] == 2016) {
				have_handshake = true;
				return;
			}
			reset_serial_buffer();
		}
	}
}

void dieError(int code) {
//	print_led(&led_top, "EEEEEEEE");
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
		Wire.beginTransmission(SLAVE_HW_ADDRESS);
		Wire.write(ptr, send_len);
		Wire.endTransmission();
		len -= send_len;
		ptr += send_len;
	}
}

void check_action_groups_enabled(JsonObject& rj)
{
	if (rj.containsKey("ag_state")) {
		int data=rj["ag_state"];
		int mask=1;
		for( int bit=0; bit<10; bit++)
		{
			if ( data & mask )
			{
				action_group_buttons[bit]->setLight(true);
			}
			else
			{
				action_group_buttons[bit]->setLight(false);
			}
			mask=mask*2;
		}
	}
}

void check_button_enabled(JsonObject& rj, const char *key, int button_index) {
	if (rj.containsKey(key)) {
		int val = rj[key];
		bool state = (val == 1) ? true : false;
		buttons[button_index]->setLight(state);
		rj.remove(key);
	}
}

void check_for_command() {
	if (message_complete == true) {
		DynamicJsonBuffer readBuffer;
		JsonObject& rj = readBuffer.parseObject(read_buffer);

		// Lesen, was f�r hier dabei ist
		if (!rj.success()) {
			dieError(strlen(read_buffer));
		} else {
			check_button_enabled(rj, "rcs", RCS_BUTTON);
			check_button_enabled(rj, "sas", SAS_BUTTON);
			check_button_enabled(rj, "gear", GEAR_BUTTON);
			check_button_enabled(rj, "light", LIGHT_BUTTON);
			check_button_enabled(rj, "brakes", BRAKES_BUTTON);
			check_action_groups_enabled(rj);

			if (rj.containsKey("speed")) {
				print_led(&led_top, (long) rj["speed"]);
				rj.remove("speed");
			}
			if (rj.containsKey("height")) {
				print_led(&led_bottom, (long) rj["height"]);
				rj.remove("height");
			}

			// wenn noch lang genug -> display controller
			if (rj.size() > 0) {
				sendToSlave(rj);
			}
		}
		reset_serial_buffer();
	}
}

void awakeSlave()
{
	StaticJsonBuffer<READ_BUFFER_SIZE> buffer;
	JsonObject& rj = buffer.createObject();
	rj["start"] = 2016;
	sendToSlave(rj);
}

void loop()
{
	check_serial_port();
	StaticJsonBuffer<READ_BUFFER_SIZE> writeBuffer;
	JsonObject& root = writeBuffer.createObject();
//	if( message_complete == true )
//	{
//		root["chip"]=read_buffer;
//	}

	LOOP_OVER(NUM_ANALOG_BUTTONS)
	{
		AnalogInput *i = analog_inputs[index];
		i->readInto(root,false);
	}
	testAllButtons(root);
	// there are two special buttons :)
	// the two switches on the right top
	if (root.containsKey("switch_right")) {
		key_chips[4]->setPin(4, root["switch_right"]);
		led_chips[0]->setPin(0, root["switch_right"]);
		stage_enabled = root["switch_right"];
	}
	if (root.containsKey("switch_left")) {
		kc5.setPin( 5, root["switch_left"]);
	}
	// let "stage" only pass if staging is enabled
	if (root.containsKey("stage") && stage_enabled==false)
	{
		root.remove("stage");
	}

	// if we have data and can send (nothing is in the buffer)
	if (root.size() > 0 && (Serial.availableForWrite() == empty_buffer_size)) {
		root.printTo(Serial);
		Serial.print('\n');
		Serial.flush();
	}
	check_for_command();
}
