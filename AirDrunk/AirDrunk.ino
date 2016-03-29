/* 
   This program allows music interaction and composition via an arduino

   The circuit/Pin usage list:
   #  use
   * 2   Serial to 1053
   * 9   Reset 1053
   * A0  Tone changing

   Created: 2016

   https://github.com/YebaiZhao/AirBeats-prototype
*/


// #######    Import Libraries    ######
#include "mpr121.h"
#include <Wire.h>
#include <SoftwareSerial.h>

// #######    Constants    ######
#define VS1053_RX  2    // This is the pin that connects to the RX pin on VS1053
#define VS1053_RESET 9  // This is the pin that connects to the RESET pin on VS1053

#define VS1053_BANK_DEFAULT 0x00
#define VS1053_BANK_DRUMS1 0x78
#define VS1053_BANK_DRUMS2 0x7F
#define VS1053_BANK_MELODY 0x79

#define MIDI_NOTE_ON  0x90
#define MIDI_NOTE_OFF 0x80
#define MIDI_CHAN_MSG 0xB0
#define MIDI_CHAN_BANK 0x00
#define MIDI_CHAN_VOLUME 0x07
#define MIDI_CHAN_PROGRAM 0xC0

// #######    Variables    ######
int analogPin=A0;
int a0Input=0;
int midiInstr=1;
SoftwareSerial VS1053_MIDI(0, 2); 
int irqpin = 3;  // Digital 3 
boolean touchStates[12]; //to keep track of the 12 previous touch states

// #######    Functions    ######

/*
  Set up the pins and ensure that they are all working as
  expected with corresponding variables, modes and libraries. 
  Set up the channels for the MIDI control.
*/
void setup() {
	Serial.begin(9600);
	Serial.println("VS1053+MPR121 MIDI test");
	pinMode(irqpin, INPUT);
	pinMode(VS1053_RESET, OUTPUT);
	digitalWrite(irqpin, HIGH);  //enable pullup resistor

	digitalWrite(VS1053_RESET, LOW);
  	delay(10);
  	digitalWrite(VS1053_RESET, HIGH);
  	delay(10);                 //reset the VS1053

  	Wire.begin();
  	mpr121_setup();

  	VS1053_MIDI.begin(31250);  // MIDI uses a 'strange baud rate'
  	midiSetChannelBank(0, VS1053_BANK_MELODY);
  	midiSetInstrument(0, midiInstr);
  	midiSetChannelVolume(0, 63);
}

/*
  This method loops through each touch input and assigns the resistor inputs.
  It references methods: readTouchInputs() and readResistorInputs()
*/
void loop() {  
	readTouchInputs();
  	/*for (uint8_t i=60; i<69; i++) { //i is the note
    	midiNoteOn(0, i, 127);
    	delay(100);
    	midiNoteOff(0, i, 127);
  	}*/
   	readResistorInputs();
}


/*
  Read the resistor inputs and assign the corresponding instrument to be
  played in audio output.
*/
void readResistorInputs(){
  	a0Input=analogRead(analogPin)/8;
  	
  	if(midiInstr != a0Input;){
  		if (a0Input>0 && a0Input<129){
    		midiSetInstrument(0, a0Input);
    		Serial.println(" current instrument"+String(a0Input));
    		midiInstr = a0Input;
  		}else{
  			Serial.println("instrument selection wrong");
  		}
	}
}
  
/*
  Assign the instruments to a MIDI channel. Where the maximum number of 
  channels is 15.
*/
void midiSetInstrument(uint8_t chan, uint8_t inst) {
  if (chan > 15) return;
  inst --; // page 32 has instruments starting with 1 not 0 :(
  if (inst > 127) return;
  
  VS1053_MIDI.write(MIDI_CHAN_PROGRAM | chan);  
  VS1053_MIDI.write(inst);
}

/*
  Assign the volume for each MIDI channel, where the maximum volume 
  is 127.
*/
void midiSetChannelVolume(uint8_t chan, uint8_t vol) {
  if (chan > 15) return;
  if (vol > 127) return;
  
  VS1053_MIDI.write(MIDI_CHAN_MSG | chan);
  VS1053_MIDI.write(MIDI_CHAN_VOLUME);
  VS1053_MIDI.write(vol);
}

/*
  Assign the MIDI sounds for each channel referencing the sound
  library.
*/
void midiSetChannelBank(uint8_t chan, uint8_t bank) {
  if (chan > 15) return;
  if (bank > 127) return;
  
  VS1053_MIDI.write(MIDI_CHAN_MSG | chan);
  VS1053_MIDI.write((uint8_t)MIDI_CHAN_BANK);
  VS1053_MIDI.write(bank);
}

/*
  Map the velocity sensors to the corresponding channels
  in MIDI to turn on the note
*/
void midiNoteOn(uint8_t chan, uint8_t n, uint8_t vel) {
  if (chan > 15) return;
  if (n > 127) return;
  if (vel > 127) return;
  
  VS1053_MIDI.write(MIDI_NOTE_ON | chan);
  VS1053_MIDI.write(n);
  VS1053_MIDI.write(vel);
}

/*
  Map the velocity sensors to the corresponding channels
  in MIDI to turn off the note
*/
void midiNoteOff(uint8_t chan, uint8_t n, uint8_t vel) {
  if (chan > 15) return;
  if (n > 127) return;
  if (vel > 127) return;
  
  VS1053_MIDI.write(MIDI_NOTE_OFF | chan);
  VS1053_MIDI.write(n);
  VS1053_MIDI.write(vel);
}


// #######    Main Execution    ######

/*
  Main execution of the program to control the sensors and filtering
  for mpr121 capacitive touch sensor
*/
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
  
  // Section D - Set the Filter Configuration : Set ESI2
  set_register(0x5A, FIL_CFG, 0x04);
  
  // Section E - Electrode Configuration : Set ELE_CFG to 0x00 to return to standby mode
  set_register(0x5A, ELE_CFG, 0x0C);  // Enables all 12 Electrodes
  
  // Section F - Enable Auto Config and auto Reconfig
  set_register(0x5A, ATO_CFG0, 0x0B);
  set_register(0x5A, ATO_CFGU, 0xC9);  // USL = (Vdd-0.7)/vdd*256 = 0xC9 @3.3V   set_register(0x5A, ATO_CFGL, 0x82);  // LSL = 0.65*USL = 0x82 @3.3V
  set_register(0x5A, ATO_CFGT, 0xB5);  // Target = 0.9*USL = 0xB5 @3.3V
  
  set_register(0x5A, ELE_CFG, 0x0C);
  
}

void readTouchInputs(){
  if(!checkInterrupt()){
    
    //read the touch state from the MPR121
    Wire.requestFrom(0x5A,2); 
    
    byte LSB = Wire.read();
    byte MSB = Wire.read();
    
    uint16_t touched = ((MSB << 8) | LSB); //16bits that make up the touch states

    
    for (int i=0; i < 12; i++){  // Check what electrodes were pressed
      if(touched & (1<<i)){
      
        if(touchStates[i] == 0){
          //pin i was just touched
          	Serial.print("pin ");
          	Serial.print(i);
          	Serial.println(" was just touched");
        	midiNoteOn(0, i+59, 127);
        }else if(touchStates[i] == 1){
          //pin i is still being touched
        }  
      
        touchStates[i] = 1;      
      }else{
        if(touchStates[i] == 1){
          Serial.print("pin ");
          Serial.print(i);
          Serial.println(" is no longer being touched");
          midiNoteOff(0, i+59, 127);
          //pin i is no longer being touched
       }
        
        touchStates[i] = 0;
      }
    
    }
    
  }
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
