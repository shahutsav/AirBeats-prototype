#include <MIDI.h>

// FRAME MIDI (cccc = channel):
 
// | ================= =============== MESSAGE | ============ data1 ============ | ========= ========== data2 |
// | 1000 cccc = note off => 128 (10) | 0xxx xxxx: height notes | 0xxx xxxx: Velocity |
// | 1001 = cccc notes on => 129 (10) | 0xxx xxxx: height notes | 0xxx xxxx: Velocity |
// | 1110 cccc = pitch bend => 224 (10) | 0000 0000: Code | 0xxx xxxx: speed |
// | 1011 cccc = control change => 176 (10) | 0xxx xxxx: numero | 0xxx xxxx: value |
// ------------------------------------------------ --------------------------------------------------
// ========================
MIDI_CREATE_DEFAULT_INSTANCE();

int  valuePinZero = 0;
int  valuePinZero2 = 0;
 
// Analog pine ONE
int  valuePinOne = 0;
int  valuePinOne2 = 0;
 
/*// Analog pin TWO
int  valuePinTwo = 0;
int  valuePinTwo2 = 0; */
 
void  setup ()
{
Serial.begin (57600); // communication speed
}
 
void  loop ()
{
// Controller ZERO
// --------
valuePinZero = analogRead (0); //read the fsr value
int val = map(valuePinZero, 0, 1023, 0, 255);
if  (val > 0) { // if the value varies by more than 2 between reads ...
valuePinZero2 = val; //...on updates
Serial.println(val); 
MIDI_TX (176, 65, val); // and send message type (CHANNEL CONTROLLER 0, NUMBER 75 VALUE = read value) ==> see table
 // Delay (100); // For debugging
}

 
// Controller ONE
// --------
valuePinOne = analogRead (1) ;
int valOne = map(valuePinOne, 0, 1023, 0, 255);
if  (valOne > 0)
{
 
valuePinOne2 = valOne;
 
MIDI_TX (176, 67, valOne);
 // Delay (100);
}
 Serial.println(valOne);

// TWO Controller
// --------
/*valuePinTwo = analogRead (2);
int valTwo = map(valuePinTwo, 0, 1023, 0, 255);
if  (valTwo > 0)
{
 
valuePinTwo2 = valTwo;
 
MIDI_TX (176, 77, valTwo);
// Delay (100);
}
 */
}
 
void  MIDI_TX (unsigned char  MESSAGE, unsigned char  data1, unsigned char  data2) // send function of the MIDI message ==> see table
{
 Serial.write (MESSAGE); // send the message byte to the serial port
 Serial.write (data1); // send the data byte 1 of the serial port
 Serial.write (data2); // send the data byte 2 on the serial port
}
