/* libraries used
 *  MPR 121
 *  Set FL studio midi input port to loop midi port
 *  Connections
 *  A0 - A3 Glove FSR's
 *  IRQ to D3
 *  A5 SCL to MPR121
 *  A4 SDA to MPR121
 *  D2  Serial to 1053, THIS HAS TO BE CONNECTED TO RX Pin
 *  Note : Touching any Capacitive sensor changes pitch.
 */
#include "mpr121.h"
#include <Wire.h>

int irqpin = 3; // Digital 2
boolean touchStates[12]; //to keep track of the previous touch states

// Analog pin 0
int  readPinZero = 0;
int  prevZero = 0;
 
// Analog pine 1
int  readPinOne = 0;
int  prevOne = 0;
 
  //Analog pin 2
int  readPinTwo = 0;
int  prevTwo = 0;

 // Analog pin 3
 int readPinThree = 0;
 int prevThree = 0;

void setup(){
 pinMode(irqpin, INPUT);
 digitalWrite(irqpin, HIGH); //enable pullup resistor
 
 Serial.begin(57600); //set baud rate same as hairless midi.
 Wire.begin();

 mpr121_setup();
}

void loop(){
 readTouchInputs(); // read capacitive inputs
 //writeOutputs();

 // Controller 0
// --------
readPinZero = analogRead (0) /8; // limit analog reading to 127
if  (readPinZero != prevZero ) { 
MIDI_message (176, 65, readPinZero); // 176 = midi control change message. effect = 65.check midi cc table for effects.
 // Delay (100); // For debugging
}
prevZero = readPinZero; 


// Controller 1
// --------
readPinOne = analogRead (1) /8 ;
if (readPinOne != prevOne)
{
 
MIDI_message (176, 101, readPinOne); // midi cc message. effect = 101. velocity = pin one reading.
 // Delay (100);
}
 prevOne = readPinOne;


// Controller 2
// --------
readPinTwo = analogRead (2) /8;
if (readPinTwo != prevTwo)
{
  
MIDI_message (176, 77, readPinTwo); // midi cc message. effect = 77.
// Delay (100);
}

// Controller 3

 readPinThree = analogRead(3) /8;
 if (readPinThree != prevThree)
 {
  MIDI_message (176, 95, readPinThree); //midi cc message
 }
 prevThree = readPinThree;

}

void  MIDI_message (unsigned char  MESSAGE, unsigned char  data1, unsigned char  data2) // send function of the MIDI message ==> see table
{
 Serial.write (MESSAGE); // send the message byte to the serial port
 Serial.write (data1); // send the data byte 1 of the serial port
 Serial.write (data2); // send the data byte 2 on the serial port
}

void readTouchInputs(){
 if(!checkInterrupt()){
  
  //read the touch state from the MPR121
  Wire.requestFrom(0x5A,2); 
  
  byte LSB = Wire.read();
  byte MSB = Wire.read();
  
  uint16_t touched = ((MSB << 8) | LSB); //16bits that make up the touch states

  
  for (int i=0; i < 12; i++){ // Check what electrodes were pressed
   if(touched & (1<<i)){
   
    if(touchStates[i] == 0){
     //pin i was just touched
     Serial.print("pin ");
     Serial.print(i);
     Serial.println(" was just touched");
     MIDI_message(224, 100, 100); // 224 = midi pitch bend. velocity = 100
    }

    
    
    else if(touchStates[i] == 1){
     //pin i is still being touched
     MIDI_message (224, 100, 100);
    } 
   
    touchStates[i] = 1;   
   }else{
    if(touchStates[i] == 1){
     Serial.print("pin ");
     Serial.print(i);
     Serial.println(" is no longer being touched");
     MIDI_message(224, 100, 0); //midi pitch bend 

     //pin i is no longer being touched
   }
    
    touchStates[i] = 0;
   }
  
  }
  
 }
}




void mpr121_setup(void){

 set_register(0x5A, ELE_CFG, 0x00); 
 
 // Section A - Controls filtering when data is > baseline.
 set_register(0x5A, MHD_R, 0x01);
 set_register(0x5A, NHD_R, 0x01);
 set_register(0x5A, NCL_R, 0x00);
 set_register(0x5A, FDL_R, 0x00);

 // Section B - Controls filtering when data is < baseline.
 set_register(0x5A, MHD_F, 0x01);
 set_register(0x5A, NHD_F, 0x01);
 set_register(0x5A, NCL_F, 0xFF);
 set_register(0x5A, FDL_F, 0x02);
 
 // Section C - Sets touch and release thresholds for each electrode
 set_register(0x5A, ELE0_T, TOU_THRESH);
 set_register(0x5A, ELE0_R, REL_THRESH);

 set_register(0x5A, ELE1_T, TOU_THRESH);
 set_register(0x5A, ELE1_R, REL_THRESH);
 
 set_register(0x5A, ELE2_T, TOU_THRESH);
 set_register(0x5A, ELE2_R, REL_THRESH);
 
 set_register(0x5A, ELE3_T, TOU_THRESH);
 set_register(0x5A, ELE3_R, REL_THRESH);
 
 set_register(0x5A, ELE4_T, TOU_THRESH);
 set_register(0x5A, ELE4_R, REL_THRESH);
 
 set_register(0x5A, ELE5_T, TOU_THRESH);
 set_register(0x5A, ELE5_R, REL_THRESH);
 
 set_register(0x5A, ELE6_T, TOU_THRESH);
 set_register(0x5A, ELE6_R, REL_THRESH);
 
 set_register(0x5A, ELE7_T, TOU_THRESH);
 set_register(0x5A, ELE7_R, REL_THRESH);
 
 set_register(0x5A, ELE8_T, TOU_THRESH);
 set_register(0x5A, ELE8_R, REL_THRESH);
 
 set_register(0x5A, ELE9_T, TOU_THRESH);
 set_register(0x5A, ELE9_R, REL_THRESH);
 
 set_register(0x5A, ELE10_T, TOU_THRESH);
 set_register(0x5A, ELE10_R, REL_THRESH);
 
 set_register(0x5A, ELE11_T, TOU_THRESH);
 set_register(0x5A, ELE11_R, REL_THRESH);
 
 // Section D
 // Set the Filter Configuration
 // Set ESI2
 set_register(0x5A, FIL_CFG, 0x04);
 
 // Section E
 // Electrode Configuration
 // Set ELE_CFG to 0x00 to return to standby mode
 set_register(0x5A, ELE_CFG, 0x0C); // Enables all 12 Electrodes
 
 
 // Section F
 // Enable Auto Config and auto Reconfig
 /*set_register(0x5A, ATO_CFG0, 0x0B);
 set_register(0x5A, ATO_CFGU, 0xC9); // USL = (Vdd-0.7)/vdd*256 = 0xC9 @3.3V  set_register(0x5A, ATO_CFGL, 0x82); // LSL = 0.65*USL = 0x82 @3.3V
 set_register(0x5A, ATO_CFGT, 0xB5);*/ // Target = 0.9*USL = 0xB5 @3.3V
 
 set_register(0x5A, ELE_CFG, 0x0C);
 
}


boolean checkInterrupt(void){
 return digitalRead(irqpin);
}


void set_register(int address, unsigned char r, unsigned char v){
  Wire.beginTransmission(address);
  Wire.write(r);
  Wire.write(v);
  Wire.endTransmission();
}

