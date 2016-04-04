/*
AirBeats Project Code for Arduino Uno

V 0.3.4

Last edit: Yebai Zhao
--------------------
Pin uesage list. 
Even you don't see a wire coming out of a pin doesn't mean it is not used.
[#] [USEAGE]
D2  Serial to 1053
D3  IRQ to MPR121
D9  Reset 1053
D4  SD card reset

A4  SDA to MPR121
A5  SCL to MPR121
*/
#include "mpr121.h"
#include <Wire.h>
#include <SoftwareSerial.h>
#include <SPI.h>
#include <SD.h>

#define VS1053_RX  2 // This is the pin that connects to the RX pin on VS1053
#define CARDCS 4     // Card chip select pin

//MIDI Message settings
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
SoftwareSerial VS1053_MIDI(0, VS1053_RX); 
int8_t midiInstr=26;// electric guitar

//Basic pace settings
#define BPM 60 //Beats per minute, should be 120/60/40/30... etc
uint16_t minTimeInterval=7500/BPM; //The fastest note we can make is eighth note. In milisecond
uint16_t barInterval= 4*(2*minTimeInterval); //in a 4/4 beat, this is the time for a bar
unsigned long lastBarTime;
unsigned long recordBarTime;

unsigned long nextBarTime;

//MPR121 sensor settings
#define irqpin 3  //Pin Digital 3 is used for MPR121 interrupt
int8_t noteMapping[14]={60,62,64,65,67,69,71,72,74,76,77,79,81,83};
uint16_t touched;
int8_t noteActionStates[12];//tells what new action should be done, 0=not active, 1=note on, 2=active, 3=note off
//SD card
File myFile;

//////////////////////////////////////////////////////////////////////SETUP
void setup() {
	Serial.begin(9600);
	Serial.println("VS1053+MPR121 AirBeats v0.2");

  if (!SD.begin(CARDCS)) { //if there is no sd card
    Serial.println(("SD failed, or not present"));
    while (1);  // don't do anything more
  }else if (SD.exists("MIDI.txt")){
    SD.remove("MIDI.txt"); //delete the old file
  }
  myFile = SD.open("MIDI.txt", FILE_WRITE);
  myFile.close();

	pinMode(irqpin, INPUT);//enable Pin:3 for MPR121
	digitalWrite(irqpin, HIGH); //enable pullup resistor
  Wire.begin();
  mpr121_setup();//a function down below to set default values for MPR121
  
  
  VS1053_MIDI.begin(31250); // 3906 byte/second
  midiSetChannelBank(0, VS1053_BANK_MELODY);//0x B0 00 79, channel 0 is using melody bank
  midiSetInstrument(0, midiInstr);// 0x C0 01
  midiSetChannelVolume(0, 127);//0x B0 07 3F

  midiSetChannelBank(9, VS1053_BANK_DRUMS2);// channel 9 is using drum2 bank
  midiSetChannelVolume(9,127);
}
///////////////////////////////////////////////////////////////////////LOOP
void loop() { 
  //unsigned long currentTime = millis();
	readTouchInputs();
  fileIO();
  writeOutputs();
}


////////////////////////////////////////////////////////////////////FUNCTIONS
void readTouchInputs(){ // Read input, write action arry
  
  if(!checkInterrupt()){
    Wire.requestFrom(0x5A,2); //read the touch state from the MPR121
    
    byte LSB = Wire.read(); //0000 0000
    byte MSB = Wire.read(); //0000 0000
    
    touched = ((MSB << 8) | LSB); //16bits that make up the touch states 

  }

  for (int i=0; i < 12; i++){  // Check what electrodes were pressed

    if(touched & (1<<i)){
      if(noteActionStates[i] == 0){
        noteActionStates[i] = 1;
      }else if(noteActionStates[i] == 1){
        noteActionStates[i] = 2;
      }
    }
    else{
      if(noteActionStates[i] == 2){
        noteActionStates[i] = 3;
      }else if(noteActionStates[i] == 3){
        noteActionStates[i] = 0;
      }
    }
  }
}

void fileIO(){
  if(noteActionStates[7]==1){ //start recording
    unsigned long recordBarTime=lastBarTime;
    myFile = SD.open("MIDI.txt", FILE_WRITE);
  }
  else if(noteActionStates[7]==2){ //recoding

    for(int i=0; i<7; i++){
      if(noteActionStates[i] == 1){
        String newline=String(i)+"O"+String(millis()-recordBarTime);
        myFile.println(newline);
        myFile.flush();
      }else if (noteActionStates[i] == 3){
        String newline=String(i)+"F"+String(millis()-recordBarTime);
        myFile.println(newline);
        myFile.flush();
      }
    }

  }else if(noteActionStates[7]==3){ //finish recording
    myFile.close();
    nextBarTime=lastBarTime+barInterval;
    myFile= SD.open("MIDI.txt");
    Serial.println("file opened for reading");
  }else if(noteActionStates[7]==0){ //reading file
    if(myFile){ //if there is a file
      if(myFile.available()){  //if we havn't finished reading it
        char thisline[10];
        char thisbyte;
        int8_t i=0;
        do{
          thisbyte=myFile.read();
          thisline[i] = thisbyte;
          i++;
        }while('\r' != thisbyte);
        Serial.print(thisline);
        Serial.println("reading to "+myFile.position());
      }
      else if(){ //if we have finished reading it . NEEDS REWRITE
        noteActionStates[7]=3;
        Serial.println("reading to the end, reset to state 3");
      }
    }
    //if the file is not yet created, do nothing.
  }

}

void writeOutputs(){
  for(int i=0; i<7; i++){
    if(noteActionStates[i]==1){//notes,
      midiNoteOn(0, noteMapping[i], 127);  
    }else if(noteActionStates[i]==3){
      midiNoteOff(0, noteMapping[i], 127);
    }
  }

  if(millis() % barInterval == 0){ //beats, play the bass dumm 36, in a vel of 127
    lastBarTime=millis();
    midiNoteOn(9, 36, 127);
  }

}

void midiSetInstrument(uint8_t chan, uint8_t inst) {
  if (chan > 15) return;
  inst --; // page 32 has instruments starting with 1 not 0 :(
  if (inst > 127) return;
  
  VS1053_MIDI.write(MIDI_CHAN_PROGRAM | chan);  
  VS1053_MIDI.write(inst);
}


void midiSetChannelVolume(uint8_t chan, uint8_t vol) {
  if (chan > 15) return;
  if (vol > 127) return;
  
  VS1053_MIDI.write(MIDI_CHAN_MSG | chan);
  VS1053_MIDI.write(MIDI_CHAN_VOLUME);
  VS1053_MIDI.write(vol);
}


void midiSetChannelBank(uint8_t chan, uint8_t bank) {
  if (chan > 15) return;
  if (bank > 127) return;
  
  VS1053_MIDI.write(MIDI_CHAN_MSG | chan);
  VS1053_MIDI.write((uint8_t)MIDI_CHAN_BANK);
  VS1053_MIDI.write(bank);
}


void midiNoteOn(uint8_t chan, uint8_t n, uint8_t vel) {
  if (chan > 15) return;
  if (n > 127) return;
  if (vel > 127) return;
  
  VS1053_MIDI.write(MIDI_NOTE_ON | chan);
  VS1053_MIDI.write(n);
  VS1053_MIDI.write(vel);
}

void midiNoteOff(uint8_t chan, uint8_t n, uint8_t vel) {
  if (chan > 15) return;
  if (n > 127) return;
  if (vel > 127) return;
  
  VS1053_MIDI.write(MIDI_NOTE_OFF | chan);
  VS1053_MIDI.write(n);
  VS1053_MIDI.write(vel);
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
  set_register(0x5A, ELE_CFG, 0x0C);  // Enables all 12 Electrodes
  
  
  // Section F
  // Enable Auto Config and auto Reconfig
  set_register(0x5A, ATO_CFG0, 0x0B);
  set_register(0x5A, ATO_CFGU, 0xC9);  // USL = (Vdd-0.7)/vdd*256 = 0xC9 @3.3V   set_register(0x5A, ATO_CFGL, 0x82);  // LSL = 0.65*USL = 0x82 @3.3V
  set_register(0x5A, ATO_CFGT, 0xB5);  // Target = 0.9*USL = 0xB5 @3.3V
  
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
