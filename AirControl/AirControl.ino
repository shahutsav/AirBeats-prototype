/*
AirBeats Project Code for Arduino Uno

V 0.4.6

Last edit: Yebai Zhao
--------------------
Pin usage list. 
Even you don't see a wire coming out of a pin doesn't mean it is not used.
[#] [USEAGE]
D2  Serial to 1053, THIS HAS TO BE CONNECTED TO RX Pin
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
uint16_t minTimeInterval=7500/BPM; //The fastest note we can make is eighth note. In millisecond
uint16_t barInterval= 4*(2*minTimeInterval); //in a 4/4 beat, this is the time for a bar
unsigned long lastBarTime;
unsigned long recordBarTime;
unsigned long nextBarTime;

//General IO
int8_t noteMapping[16]={60,62,64,65, 67,69,71,72, 74,76,77,79, 40,35,48,56};
int8_t noteActionStates[16];//tells what new action should be done, 0=not active, 1=note on, 2=active, 3=note off

//MPR121 sensor settings
#define irqpin 3  //Pin Digital 3 is used for MPR121 interrupt
boolean muteChannels=false;
uint16_t touched;// The state of all input in this cycle 0000 0000 0000 0000

//SD card
File myFile;

//Glove
uint8_t glove_Analog[4]; //All four analog resistors readings

//////////////////////////////////////////////////////////////////////SETUP
void setup() {
	Serial.begin(9600);
	Serial.println("VS1053+MPR121 AirBeats v0.4");

  if (!SD.begin(CARDCS)) { //if there is no SD card
    Serial.println(("SD failed, or not present"));
    while (1);  // don't do anything more
  }else if (SD.exists("MIDI.txt")){
    SD.remove("MIDI.txt"); //delete the old file
  }
  myFile = SD.open("MIDI.txt", FILE_WRITE);
  myFile.close();

	pinMode(irqpin, INPUT);//enable Pin:3 for MPR121
	digitalWrite(irqpin, HIGH); //enable pull-up resistor
  Wire.begin();
  mpr121_setup();//a function down below to set default values for MPR121
  
  ////////////////////Glove input pin setting 
  pinMode(A0, INPUT);
  pinMode(A1, INPUT);
  pinMode(A2, INPUT);
  pinMode(A3, INPUT);

  ///////////////////

  VS1053_MIDI.begin(31250); // 3906 byte/second
  midiSetChannelBank(0, VS1053_BANK_MELODY);//0x B0 00 79, channel 0 is using melody bank
  midiSetInstrument(0, midiInstr);// 0x C0 01
  midiSetChannelVolume(0, 127);//0x B0 07 3F
  midiSetChannelBank(9, VS1053_BANK_DRUMS2);// channel 9 is using drum2 bank
  midiSetChannelVolume(9,127);
}
///////////////////////////////////////////////////////////////////////LOOP
void loop() { 
  unsigned long currentTime = millis();
	readTouchInputs();
  //fileIO();
  writeOutputs(); 
}


////////////////////////////////////////////////////////////////////FUNCTIONS
void readTouchInputs(){ // Read input, write action array
  
  if(!checkInterrupt()){
    Wire.requestFrom(0x5A,2); //read the touch state from the MPR121
    
    byte LSB = Wire.read(); //0000 0000
    byte MSB = Wire.read(); //0000 0000
    touched = ((MSB << 8) | LSB); //16 bits that make up the touch states 
  }else{
    touched&=0xfff;
  }

    byte glove =(readAnalog(3)<<3)|(readAnalog(2)<<2)|(readAnalog(1)<<1)|(readAnalog(0));
    touched=( (glove<<12) |touched);

  for (int i=0; i < 16; i++){  // Check what electrodes were pressed

    if(touched & (1<<i)){ //if that bit is 1
      if(noteActionStates[i] == 0){// if that bit was 0
        noteActionStates[i] = 1;//make it 1
      }else if(noteActionStates[i] == 1){//if that bit was 1 too
        noteActionStates[i] = 2;//make it 2
      }
    }
    else{//if that bit is 0
      if(noteActionStates[i] == 2){//if that bit was 2
        noteActionStates[i] = 3;//make it 3
      }else if(noteActionStates[i] == 3){
        noteActionStates[i] = 0;
      }else if(noteActionStates[i] == 1){
        noteActionStates[i] = 0;//////////////////////////
      }
    }
  }
}

// void fileIO(){
//   if(noteActionStates[7]==1){ //start recording
//     unsigned long recordBarTime=lastBarTime;
//     myFile = SD.open("MIDI.txt", FILE_WRITE);
//   }
//   else if(noteActionStates[7]==2){ //recoding

//     for(int i=0; i<7; i++){
//       if(noteActionStates[i] == 1){
//         String newline=String(i)+"O"+String(millis()-recordBarTime);
//         myFile.println(newline);
//         myFile.flush();
//       }else if (noteActionStates[i] == 3){
//         String newline=String(i)+"F"+String(millis()-recordBarTime);
//         myFile.println(newline);
//         myFile.flush();
//       }
//     }

//   if(noteActionStates[6]==1){ //finish recording
//     myFile.close();
//     nextBarTime=lastBarTime+barInterval;
//     myFile= SD.open("MIDI.txt");
//     Serial.println("file opened for reading");
//   if(noteActionStates[6]==0){ //reading file
//     if(myFile){ //if there is a file
//       if(myFile.available()){  //if we haven't finished reading it
//         char thisline[10];
//         char thisbyte;
//         int8_t i=0;
//         do{
//           thisbyte=myFile.read();
//           thisline[i] = thisbyte;
//           i++;
//         }while('\r' != thisbyte);
//         Serial.print(thisline);
//         Serial.println("reading to "+myFile.position());
//       }
//       else if(){ //if we have finished reading it . NEEDS REWRITE
//         noteActionStates[7]=3;
//         Serial.println("reading to the end, reset to state 3");
//       }
//     }
//     //if the file is not yet created, do nothing.
//   }
// }

void writeOutputs(){
  if(noteActionStates[0]==2){ //if you press the pad 1

    for(int i=2; i<6; i++){
      if(noteActionStates[i]==1){
        if(noteMapping[i+10]!=87){
        noteMapping[i+10]=noteMapping[i+10]+1; 
        midiNoteOn(9, noteMapping[i+10],127);
        }else {
          noteMapping[i+10]=27;
          midiNoteOn(9, noteMapping[i+10],127);
        }
      }
    }
  }
  if(noteActionStates[1]==2){ //if you press the pad 1

    for(int i=2; i<6; i++){
      if(noteActionStates[i]==1){
        if(noteMapping[i+10]!=27){
        noteMapping[i+10]=noteMapping[i+10]-1; 
        midiNoteOn(9, noteMapping[i+10],127);
        }else {
          noteMapping[i+10]=87;
          midiNoteOn(9, noteMapping[i+10],127);
        }
      }
    }
  }


  if(noteActionStates[6]==1){ //press capacity pad 7 to mute all
    if(muteChannels==false){
      midiSetChannelVolume(0,0);
      midiSetChannelVolume(9,0);
      muteChannels=true;
    }else if(muteChannels==true){
      midiSetChannelVolume(0,127);
      midiSetChannelVolume(9,127);
      muteChannels=false;
    }

  }

  for(int i=12; i<16; i++){//glove
    if(noteActionStates[i]==1){
      Serial.print(noteActionStates[12]);
      Serial.print(noteActionStates[13]);
      Serial.print(noteActionStates[14]);
      Serial.print(noteActionStates[15]);
      Serial.println();
      midiNoteOn(9, noteMapping[i],127);
    }

    // if( noteActionStates[i]==1){ //testing code for pressure volume changing. NOTE: add "uint8_t pressDelayCounter" as universal variable
    //   pressDelayCounter=0;
    // }else if(noteActionStates[i]==2 && pressDelayCounter==100){
    //   midiNoteOn(9, noteMapping[i], glove_Analog[i-12]);
    //   Serial.println(glove_Analog[i-12]);
    // }
   else if(noteActionStates[i]==3){
     midiNoteOff(9, noteMapping[i], 0);
   }
  }

if(millis() % barInterval == 0){ //beats, play the bass dumm 36, in a vel of 127
   lastBarTime=millis();
   midiNoteOn(9, 46, 127); 
 }
}

boolean readAnalog(int pinN){ 
  uint8_t analogReading= analogRead(pinN);
  if(analogReading<200){
    return false;
  }else if (analogReading<950){
    glove_Analog[pinN]=(analogReading-100)/14+64;
    return true;
  }else{
    glove_Analog[pinN]=127;
    return true;
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
