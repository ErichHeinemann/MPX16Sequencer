// E.Heinemann
// e.heinemann@hman-project.de
// 30.10.2016 - E.Heinemann  Bonn Germany
// 2021-02-07 - E.Heinemann Updated and modified the MIDI-Note-Numbers and their names to map it to the MPX16-Defaults
// 2021-02-07 - E.Heinemann, new attempt to combine Menu-Button as a DN-Button with the Step-Buttons

// Helping sources:
// https://learn.sparkfun.com/tutorials/midi-shield-hookup-guide/example-1-clock-generator--receiver

#include <Wire.h>
#include <MIDI.h>
#include <LiquidCrystal_I2C.h>

// http://www.86duino.com/?p=8254
#include "TimerOne.h"

// http://playground.arduino.cc/Main/MsTimer2
// #in clude <MsTimer2.h>

// other interesting Project:
// http://skagmo.com
// https://www.youtube.com/watch?v=q9LyRmzGL5g
// https://github.com/Catmacey/DrumMachine


// Projects to build the Drum-Synths
// http://dmitry.gr/index.php?r=05.Projects&proj=02.%20Single-chip%20audio%20Player
// http://www.enide.net/webcms/index.php?page=pcm2pwm
// Coron DS7 http://m.bareille.free.fr/ds7clone/ds7.htm
// http://electro-music.com/forum/phpbb-files/dr_55_rimshot_clone_196.pdf
// Monotribe schematic
// Boss DR110 Scheamtics
// http://www.sdiy.org/richardc64/new_drums/dr110/dr110a1.html
// http://www.freeinfosociety.com/electronics/schemview.php?id=129
// http://www.sdiy.org/richardc64/new_drums/dr110/clap_etc.html
// Good example of needed sounds: http://delptronics.com/ldb2e.php
// http://pdp7.org/boss_dr_sync/bossdr110.html
// http://www.theninhotline.net/dr110/

// Pi Zero as SamplePlayer
// https://www.raspberrypi.org/forums/viewtopic.php?f=38&t=127585

// Mozzi-based Drum
// https://github.com/fakebitpolytechnic/cheapsynth/blob/master/Mozzi_drumsDG0_0_2BETA/Mozzi_drumsDG0_0_2BETA.ino

/*
 *  MPX16 receives on Channel 1 but sends on Channel 10 !
 *  Midi-Notes of the 16 Samples by default:
 *  40  41  42  43 | 48 49 50 51 
 *  36  37  38  39 | 44 45 46 47
 *  
 *  Simply Samples from Midi 36 to 51
 *  
 */ 


#include <EEPROM.h>



// Softserial is used to send MIDI via Pin TX 2, RX 3 
#include <SoftwareSerial.h>
SoftwareSerial softSerial( 2, 3 );
MIDI_CREATE_INSTANCE( SoftwareSerial, softSerial, midiA );


// #define NBR_INST          16
// #define NBR_PATTERN       16
// #define NBR_MIDI_CH       10

//Midi message define
// #define MIDI_START 0xfa
// #define MIDI_STOP  0xfc
// #define MIDI_CLOCK 0xf8

// Not implemented yet
uint16_t bpm = 125;          // Default BPM
uint16_t old_bpm = bpm;      // Default BPM

uint8_t midi_channel = 1;    // Default Midi Channel
uint8_t midi_channel_arp1 = 1; // Default-Channel for ARP1 fast 
// uint8_t midi_channel_arp2 = 2; // Default-Channel for ARP2 slow 
// uint8_t midi_channel_arp3 = 3; // Default-Channel for ARP3 bass


uint8_t midi_sync    = 0;     // 1 == Slave, 0 = Master - default

uint8_t LCD_Address = 0x27;   // LCD is integrated via PCF8574 on Port 0x27
LiquidCrystal_I2C lcd( LCD_Address, 16, 2 );  // simple LCD with 16x2

uint8_t address1 = 0x3C;   // Address of the PCF8574 for first 8 Buttons and LEDs
uint8_t address2 = 0x38;   // Address of the second PCF for LEDS & Buttons 9 - 16

// Button-Pins
const int buttonPinS = 4;   // Menu/FN/Select-Button ..near to the POT
const int buttonPinL = 5;   // Left- or Start-Button
const int buttonPinR = 6;   // Right- or Stop-Button

// Value of the POT
int     aPin3 = 3;
int     aVal3;
int     old_aVal3;
uint8_t newNote; // Variable for the MidiNote in the Menu

uint8_t count_step =  0;  // Step counter
uint8_t count_bars = 16;  // count steps per Pattern, .. with 2 PCF8574, I am able to define 16 Steps max.... virtually perhaps 32....
uint8_t count_ppqn =- 1;  // 24 MIDI-Clock-Pulse per quart note counter

// Menu
String   Modes[] = { "Instr", "Velo", "Speed", "Bars", "Note", "Scale", "Sync" };
uint8_t  ModesNum[] = { 0, 1, 2, 3 , 4, 5, 6 };

// Is the Sequencer running or not
boolean  playBeats = true;

// Current Menu Settings
uint8_t curModeNum = 0;
String  curMode = Modes[0]; // Sound or Play

// Instruments, Accent is not an Instrument but internally handled as an instrument .. therefore 17 Instruments from 0 to 16
const String shortSounds[] ={ "ACC"   , "PAD 1", "PAD 2" , "PAD 3" , "PAD 4", "PAD 5", "PAD 6", "PAD 7", "PAD 8", "PAD 9", "PAD 10", "PAD 11", "PAD 12","PAD 13","PAD 14","PAD 15","PAD 16" };
const String Sounds[]      ={ "Accent", "PAD 1", "PAD 2" , "PAD 3" , "PAD 4", "PAD 5", "PAD 6", "PAD 7", "PAD 8", "PAD 9", "PAD 10", "PAD 11", "PAD 12","PAD 13","PAD 14","PAD 15","PAD 16" };
uint8_t iSound[] ={ -1,  36, 37, 38, 39, 40, 41, 42, 43,  44, 45, 46, 47, 48, 49, 50, 51 }; // MIDI-Sound, edited via Menu 50=TOM, 44=closed HH, 
uint8_t iVelo[]  ={ 127, 90, 90, 90, 90, 90, 90, 90, 90,  90, 90, 90, 90, 90, 90, 90, 90 }; // Velocity, edited via Menu
uint8_t inotes1[]={ 255,255,255,255,255,255,255,255,255, 255,255,255,255,255,255,255, 255 };
uint8_t inotes2[]={ 255,255,255,255,255,255,255,255,255, 255,255,255,255,255,255,255, 255 };

uint8_t arp_in_notes_list[]  = { 0,0,0,0,0 ,0,0,0,0,0 }; // 10 Noten sollten als Speicher reichen
uint8_t arp_in_notes_count = 4; //nach vier Noten fällt die älteste wieder raus


// Current Instrument .. the first selected
int curIns = 1; // 1 = PAD  1, 0= Accent

uint16_t timer_time=5000; //Time in microsecond of the callback fuction
uint32_t tempo_delay;


uint8_t bar_value[]={ 1, 2, 3, 4, 6, 8, 12, 16};

// array of notes would be better, 2 bytes in notes for every instrument
uint8_t notes1; 
uint8_t notes2;

// Old MIDI-Tricks, HH-Sounds first, Cymbals first, Snare, BD at least,  ... to keep a tight beat
uint8_t oldStatus1=B00000000;
uint8_t oldStatus2=B00000000;
uint8_t bits1 = 0;
uint8_t bits2 = 0;

// Scale, Menu to change Scale is yet not implemented
uint8_t scale=1 ;//Default scale  1/16
uint8_t      scale_value[]  = {     3,      6,    12,     24,       8,       16 };
const String scale_string[] = { "1/32", "1/16", "1/8", "1/4",  "1/8T",   "1/4T" };
  

// BPM to MS Conversion
// http://www.sengpielaudio.com/Rechner-bpmtempotime.htm
  
// myRefresh is only a counter .. the higher the lower the BPM! To display a good BPM this value has to be translated
int myRefresh = 500;
int myStep  =  0;

int veloAccent  = 100;
int velocity    = 100;

int step_position = 0;
int b = 10;

uint8_t count_instr = 00;

boolean oldStateS=1;
boolean buttonStateS=1;
boolean oldStateL=1;
boolean buttonStateL=1;
boolean oldStateR=1;
boolean buttonStateR=1;

String curPattern1="xxxxxxxxx";
String curPattern2="xxxxxxxxx";

// ### Base-Function for PCF8574 and WIRE ### 
// PCF8574 Explosion Demo (using same pin for Input AND Output)
// Hari Wiguna, 2016
void WriteIo( uint8_t bits,uint8_t thisAddress ){
  Wire.beginTransmission(thisAddress);
  Wire.write(bits);
  Wire.endTransmission();
}

//  ### Base-Function for PCF8574 and WIRE ### 
// PCF8574 Explosion Demo (using same pin for Input AND Output)
// Hari Wiguna, 2016
uint8_t ReadIo( uint8_t address ){  
  WriteIo (B11111111, address );        // PCF8574 require us to set all outputs to 1 before doing a read.
  Wire.beginTransmission( address );
  // Wire.write(B11111111 );
  Wire.requestFrom( (int) address, 1 );  // Ask for 1 byte from slave
  uint8_t bits = Wire.read();         // read that one byte
  Wire.requestFrom( (int) address1, 1 ); // Ask for 1 byte from slave
  Wire.endTransmission(); 
  return bits;
}


// Plug MIDI-Out of MPX16 into MIDI-In of the arduino 
// Press the PAD to select the PAD
void handleNoteOn( byte inChannel, byte inNote, byte inVelocity ){

  // has the user pressed a PAD on the MPX16??
  if( inChannel == 10 && inNote >=36 && inNote <= 51){
    Select_Instr( inNote - 35 );  
   }else{
    if( inVelocity > 0 && inNote >=12 ){
       for( int i=arp_in_notes_count; i>=0; i-- ){
         byte notex =  arp_in_notes_list[i];
         arp_in_notes_list[ i+1 ] = notex;
       }
       arp_in_notes_list[ 0 ] = inNote;

       // lcd.setCursor(0,1);
       // lcd.print( arp_in_notes_list[ 0 ] );
       // lcd.setCursor(5,1);
       // lcd.print( arp_in_notes_list[ 1 ] );  
       // lcd.setCursor(10,1);
       // lcd.print( arp_in_notes_list[ 2 ] );

       
     /*
         Serial.print( "Midi IN: " );
        Serial.print( inChannel );
        Serial.print( "  " );
        Serial.print( inNote );
        Serial.print( "  " );
        Serial.println( inVelocity );  
      */
    }
    
   }

   
    // const bool firstNote = midiNotes.empty();
    // midiNotes.add(MidiNote(inNote, inVelocity));
    // handleNotesChanged(firstNote);

  // Fill the Array with the notes
  /*
  if( inChannel <9  && inNote >=10 && inNote <= 80 & inVelocity > 0 ){
    // Shift the Notes
    for( int i=arp_in_notes_count; i>0; i-- ){
      arp_in_notes_list[ i+1  ] = arp_in_notes_list[i];
    }
    // add the newest Note as the first
    arp_in_notes_list[ 0 ] = inNote;
    
    Serial.print( "Midi IN: " );
    Serial.print( inChannel );
    Serial.print( "  " );
    Serial.print( inNote );
    Serial.print( "  " );
    Serial.println( inVelocity );  
  }
  */
    
}


// Callback from Timer1
void callback(){ 
  if( old_bpm != bpm ){
    Timer1.initialize(tempo_delay); old_bpm = bpm;
  }
  count_ppqn++;    
  if( count_ppqn >= scale_value[scale] ){  
    step_position++;
    if( step_position >= count_bars ){
      step_position=0;
    }
    Update_Midi();
    count_ppqn =0;
    digitalWrite(13, digitalRead(13) ^ 1);
  }     

  // lcd.setCursor(0,1);
  // lcd.print ("Step:" + String(step_position) + " " + scale_value[scale] );
        
}

// ########################### Show Notes and current Step via LEDs #########################
void showStep ( int mystep, uint8_t address1, uint8_t address2, uint8_t notes1, uint8_t notes2 ){
  uint8_t bitdp1 = notes1;
  uint8_t bitdp2 = notes2;

  // Current Step would be only shown if played
  if( playBeats==true ){ 
   if( mystep <  8 ){ bitClear( bitdp1, step_position ); }
   if( mystep >= 8 ){ bitClear( bitdp2, ( step_position-8 ) ); }
  }
   
  WriteIo( bitdp1, address1 );
  WriteIo( bitdp2, address2 );
}


// ############################ Play the Midi-Notes ##################################### 
void Update_Midi() {
  // first 8 beats
  if( playBeats==true ){
    
    veloAccent = 100; // Normal Velocity by default
    // first Byte or first 8 Hits
    // "song_position" is the current step 
    if( step_position<8 ){ // play notes1
       // Accent set?
       if( bitRead( inotes1[0],step_position ) == 0 ){
         // Accent is set
         veloAccent = iVelo[0];
       } 

       /*
       // loop through all instruments .. But ignore Accent with 0       
       for( int i = 1; i < count_instr; i++ ){
         if( bitRead( inotes1[i],step_position ) == 0 ){
           velocity  = round( iVelo[i] * veloAccent / 100);
           if( velocity > 127 ) {
             velocity = 127; 
           }
           midiA.sendNoteOff( iSound[i],         0, midi_channel );       
           midiA.sendNoteOn(  iSound[i], velocity ,midi_channel );
         }  
       }
       */

       
       // ARP
       for( int i = 1; i < 4; i++ ){
         if( bitRead( inotes1[i], step_position ) == 0 ){
           velocity  = round( iVelo[i] * veloAccent / 100);
           if( velocity > 127 ){
             velocity = 127; 
           }

           if( arp_in_notes_list[i] > 32 ){
             midiA.sendNoteOff( arp_in_notes_list[i],         0, midi_channel_arp1 );     
             midiA.sendNoteOn(  arp_in_notes_list[i],  velocity, midi_channel_arp1 );
           }          
           // midiA.sendNoteOff( iSound[i],         0, midi_channel );       
           // midiA.sendNoteOn(  iSound[i], velocity ,midi_channel );
           
         }  
       }
       
       
    }
    
    // second Byte or second 8 Steps
    // bitClear(bitdp2, (a-8));
    if( step_position >= 8 ){ 
       // play Notes2
      if (bitRead( inotes2[0], step_position-8 ) == 0){ // Accent is set
        veloAccent = iVelo[0];
      } 

      /*
      // loop through all instruments .. but ignore the Accent with 0
      for( int i = 1; i < count_instr; i++ ){
        if( bitRead( inotes2[i], ( step_position-8 ) ) == 0 ){
          velocity  = round(iVelo[i] * veloAccent / 100);
          if( velocity > 127 ){
            velocity = 127; 
          }
          midiA.sendNoteOff( iSound[i],         0, midi_channel );       
          midiA.sendNoteOn(  iSound[i], velocity , midi_channel );
        }  
      }   
      */


      // ARP-Instruments
       for( int i = 1; i <= 4; i++ ){
         if( bitRead( inotes2[i], step_position-8 ) == 0 ){
           velocity  = round( iVelo[i] * veloAccent / 100);
           if( velocity > 127 ){
             velocity = 127; 
           }

           if( arp_in_notes_list[i] > 32 ){
             midiA.sendNoteOff( arp_in_notes_list[i],         0, midi_channel_arp1 );     
             midiA.sendNoteOn(  arp_in_notes_list[i],  velocity, midi_channel_arp1 );
           }          
           // midiA.sendNoteOff( iSound[i],         0, midi_channel );       
           // midiA.sendNoteOn(  iSound[i], velocity ,midi_channel );
           
         }  
       }
       
    }
  }
}

// ############################## Select another Instrument and update all variables and LCD ###########
void Select_Instr( int newIns ){
 //  if ( newIns != curIns ){
  curIns = newIns;
  if( newIns >= count_instr ){
    curIns = count_instr-1; 
  }   
  lcd.setCursor(0,0);
  lcd.print( curMode  + " " + Sounds[ curIns ]  + " " + iVelo[ curIns ] +"    ");
  
  curPattern1="";
  curPattern2="";
  
  notes1 = inotes1[curIns];
  notes2 = inotes2[curIns]; 
  // Show new Pattern on the Display
  for( int i = 1; i < 8; i++){
    if( bitRead( notes1, i ) == 0 ){ 
      curPattern1 = curPattern1 + "X";
    }else{ 
      curPattern1 = curPattern1 + "-";
    } 
    if( bitRead( notes2, i ) == 0 ){ 
      curPattern2 = curPattern2 + "X";
    }else{ 
      curPattern2 = curPattern2 + "-";
    } 
  }
  lcd.setCursor(0,1);
  lcd.print( curPattern1 + curPattern2 );
}



// ################################ Check the Potentiometer ##############################
void Check_POT() {
      // Check the POT -- Menu-Functions
  aVal3 = round( analogRead( aPin3 ) /8 );
  if ( aVal3 != old_aVal3  ){ 
    // Correction
    myStep = myStep + 120; 

      // Select Instrument Menu
      if( curModeNum==0 ){
        Select_Instr( round(aVal3/8) );      
      }
      
      // Velocity
      if( curModeNum==1 ){
          if ( aVal3 > 127 ) { aVal3=127;}
          lcd.setCursor(0,0);
          lcd.print( curMode + " " + String(aVal3) + " " + Sounds[ curIns ] + "     ");
          iVelo[ curIns ] = aVal3;  
      }

      // Speed-Menu
      if( curModeNum==2 ){
          old_bpm = bpm;
          bpm = round(analogRead(aPin3)/4 + 40);
          lcd.setCursor(0,0);
          lcd.print( curMode + " " + String(bpm) +" bpm   ");
          tempo_delay = 60000000 / bpm / 24;
      }
      
      // Bars
      if( curModeNum==3 ){
          if ( aVal3 > 127 ) { aVal3=127;}
          count_bars = round(aVal3 / 8)+1;
          if ( count_bars>16 ) 
            { count_bars=16; } // max of this setup
             lcd.setCursor(0,0);
          lcd.print( curMode + "  " + String( count_bars ) +"      ");

      }

      // Midi-Notes, perhaps we find a good Midi-Reference to replace Note and Name
      if( curModeNum==4  && curIns>0 ){   // 30 to 70 make sense
          newNote = round(aVal3/3) + 25;
          if ( newNote > 70 ) { newNote =70;}
          if ( newNote < 30 ) { newNote =30;}
          lcd.setCursor(0,0);
          lcd.print( curMode + " " + String(newNote) + " " + Sounds[ curIns ] + "     ");
          iSound[ curIns ] = newNote; 
      }

      // Scale
      if( curModeNum==5 ){
          if( aVal3 > 127 ) { aVal3=127; }
          scale = round(aVal3 / 16 );
          if( scale >= sizeof( scale_string ) ){ scale = sizeof( scale_string )-1 ; }
          lcd.setCursor(0,0);
          lcd.print( curMode + "  " + scale_string[ scale ] + "      " );
      }
      
      old_aVal3 = aVal3;    
    }
}

// ################################ Check Menu - Buttons --#######################
void Check_MENU() {
    // Check if Menu-Button or Start or Stop was pressed
    buttonStateS = digitalRead( buttonPinS ); // OLD: Menu changes on buttonpress .. not on release. NEW: It has to change the Menu on "Release" of the button
                                              // Acting by release offers to do something while the button is pressed. ... faster Change 
    buttonStateL = digitalRead( buttonPinL );
    buttonStateR = digitalRead( buttonPinR );
    
    if( oldStateS==1 && buttonStateS == 0 ){
      curModeNum = curModeNum +1;
      if( curModeNum >(5-playBeats) ){ curModeNum = 0 ;} 
      curMode= Modes[curModeNum];
      lcd.setCursor(0,0);
      if( curModeNum != 3 && curModeNum != 2 && curModeNum != 5 ){
        lcd.print( curMode  + " " + Sounds[ curIns ] +" " + buttonStateS  + "      ");
      }else{
        if( curModeNum == 3 ){
          lcd.print( curMode + "  " + String( count_bars ) +"      " );
        } 
        if( curModeNum == 2 ){ // Speed
          lcd.print(  curMode + " " + String(bpm) +" bpm    "   );
        }
        if( curModeNum == 5 ){
          lcd.setCursor(0,0);
          lcd.print( curMode + "  " + scale_string[ scale ] +"      "  );
        }
      } 
    }
    
    if( playBeats == false && buttonStateL != oldStateL && buttonStateL == LOW ){
      // Start-Button
      step_position=0;
      playBeats = true;
      lcd.setCursor(0,1);
      lcd.print( "Started Beating " );
      // MIDI-Clock
      midiA.sendRealTime( MIDI_NAMESPACE::Start );
    }
    if( playBeats == true && buttonStateR != oldStateR && buttonStateR == LOW ){
      // Stop-Button
      step_position=0;
      playBeats = false;
      lcd.setCursor(0,1);
      lcd.print( "Stopped Beating ");
      midiA.sendRealTime(MIDI_NAMESPACE::Stop);
    }
    oldStateS = buttonStateS;
    oldStateL = buttonStateL;
    oldStateR = buttonStateR;
}

// ############################## Check the Buttons for Notes #################################
void Check_DrumButtons() {
  
  //-- Don't do anything unless they press a switch --
  
   // Unless they're all high...  
  if ( bits1 != B11111111 && oldStatus1 != bits1 ){  
    //-- Find lowest pressed switch --
    for( byte bitIndex = 0; bitIndex < 8; bitIndex++ ){
      if( bitRead(bits1, bitIndex) == 0) {
        if( oldStateL == HIGH ) {
         if( bitRead(notes1, bitIndex) == 1) {
           bitClear( notes1, bitIndex ); 
          }else{
            bitSet( notes1, bitIndex );
          } 
          inotes1[curIns] = notes1;
        }else{
          // Instrument_Select
          Select_Instr( bitIndex );
        }
        exit;
      }    
    }  
  }

  // Unless they're all high...
  if ( bits2 != B11111111  && oldStatus2 != bits2 ){
    //-- Find lowest pressed switch --
    for( byte bitIndex = 0; bitIndex < 8; bitIndex++){
      if( bitRead(bits2, bitIndex) == 0 ){
        //  Both Buttons areL and S are not pressed!
        
        if( oldStateL == HIGH ){ 
          // && oldStateS == HIGH )
          if( bitRead(notes2, bitIndex ) == 1){
             bitClear(notes2, bitIndex ); 
           }else{
             bitSet( notes2, bitIndex ); 
           }  
           inotes2[curIns] = notes2;
           // Serial.println ("Bit Cleared");
           // } 
           // ExplosionAnimation(bitIndex, address2);
          
        }else{
          // left 2 Bits to change the Scale
          if( bitIndex == 0 ){ 
            if( scale > 0 ){ scale = scale-1; }
            lcd.setCursor(0,0);
            lcd.print( Modes[5] + "  " + scale_string[ scale ] +"      "  );
          }
          // left 2 Bits to change the Scale
          if( bitIndex == 1 ){ 
            scale = scale+1;
            if ( scale >=4){ scale=0; }// only llop through straight scales 
            lcd.setCursor(0,0);
            lcd.print( Modes[5] + "  " + scale_string[ scale ] +"      "  );
          }

          // left 4 Bits to change the Scale
          if( bitIndex==6 ){
             old_bpm = bpm;
             bpm = bpm - 2;
             if( bpm < 40 ){ bpm=40; }
             lcd.setCursor(0,0);
             lcd.print( Modes[2] + " " + String(bpm) +" bpm   "  );
             tempo_delay = 60000000 / bpm / 24;
          }
          
          // add some speed to BPM
          if( bitIndex==7 ){
            old_bpm = bpm;
            bpm = bpm + 2;
            if (bpm > 300){ bpm=300; }
            lcd.setCursor(0,0);
            lcd.print( Modes[2] + " " + String(bpm) +" bpm   "  );
            tempo_delay = 60000000 / bpm / 24;
          }
        } 
        exit;
      }    
    }
  }
  oldStatus1 = bits1;
  oldStatus2 = bits2; 
}



// #### Setup Setup Setup Setup Setup Setup Setup Setup Setup Setup Setup Setup Setup Setup Setup Setup Setup Setup Setup #####

void setup(){

  // for Debugging
  // Serial.begin( 115200 );

  pinMode(13, OUTPUT);
  
  lcd.init(); // initialize the lcd 
  // Print a message to the LCD
  lcd.backlight(); // I have not enabled that pin... at my box, the backlight is always on, via hardwire.
  lcd.print( "hman-Projects.de" ); // << put in Your prefered Name for this Sequencer into this variable:)

  count_instr = sizeof(iSound);

  Wire.begin(); // Arduino UNO uses (SDA=A4,SCL=A5)
  // Wire.begin(0,2); // ESP8266 needs (SDA=GPIO0,SCL=GPIO2)

  pinMode(buttonPinS, INPUT); // Selecct Button
  pinMode(buttonPinL, INPUT); // Left- or Start-Button
  pinMode(buttonPinR, INPUT); // Right- or Stop-Button
  
  // virtual Pull-Up-Resitor activated, Pins are by default "HIGH"
  digitalWrite( buttonPinS, HIGH); // Selecct Button
  digitalWrite( buttonPinL, HIGH); // Left- or Start-Button
  digitalWrite( buttonPinR, HIGH); // Right- or Stop-Button

  notes1 = B11111111; // Bitwise per Instrument, No Note equals "1", note to play equals "0" !
  notes2 = B11111111;
  midiA.setHandleNoteOn( handleNoteOn );
  midiA.begin( MIDI_CHANNEL_OMNI );   // Midi-Input for Sync
  midiA.turnThruOff(); // this prevents the MPX16 to collaps
  
  tempo_delay = 60000000/bpm/24;      // delay in Microseconds ....
  Timer1.initialize( tempo_delay );   // initialize timer1, and set a 1/2 second period
  Timer1.attachInterrupt( callback );  
}


void Read_Switches(){
  bits1 = ReadIo( address1 ); // Read all switches
  bits2 = ReadIo( address2 ); // Read all switches
}



// ############################################ void loop #################################################

void loop() {
  // b is a simple Counter to do nothing  
  b ++;
  // read the IO-Pins every 5 cycles
  if( b > 4 ){ 
    // Update the LEDs
    showStep( step_position, address1, address2, notes1, notes2 );
    Check_MENU();
    b = 0;
  }

  myStep ++; // This is second counter to find the right time to get the new values from the BUTTONS and the POT
  
  if( myStep >= myRefresh ){
    myStep = 0;
    Read_Switches();
    Check_POT();
    Check_MENU();
    Check_DrumButtons(); 
    // lcd.setCursor(0,1);
    // lcd.print ("Step:" + String(step_position) + " " + scale_value[scale] + " " + String(scale) + "   " );
  } 


  // Check MIDI IN
  // Is there a MIDI message incoming ?
  midiA.read();
  /*
    switch( midiA.getType()){      // Get the type of the message we caught
      // case midi::ProgramChange:       // If it is a Program Change,
      
        // BlinkLed(MIDI.getData1());  // blink the LED a number of times
                                            // correponding to the program number
                                            // (0 to 127, it can last a while..)
      // case midi::                                            
      break;
        // See the online reference for other message types
        default:
        break;
     }
  */
}

   


//////////////////////////////////////////////////////////////////
//This function is call by the timer depending Sync mode and BPM//
//////////////////////////////////////////////////////////////////
void Count_PPQN(){
  
//-----------------Sync SLAVE-------------------//  
/*  if(midi_sync){
    timer_time=5000;
    if (midiA.read())                // Is there a MIDI message incoming ?
     {
      byte data = midiA.getType();
      if(data == midi::Start ){
        if(playBeats==true) //mode==MODE_PATTERN_PLAY || mode==MODE_PATTERN_WRITE || mode==MODE_INST_SELECT)
        {
          playBeats=true;
          // play_pattern = 1;
          count_ppqn=-1;
        }
        
        //if(mode==MODE_SONG_PLAY || mode==MODE_SONG_WRITE){
        //  play_song = 1;
        //  count_ppqn=-1;
        //  song_position=0;
        // }
      }
      else if(data == midi::Stop ) {
        playBeats=false;
        // play_pattern = 0;
        // play_song = 0;
        // count_step=0;
        step_position=0;
        count_ppqn=-1;
        // song_position=0;
      }
      else if(data == midi::Clock && playBeats==true) //(play_pattern == 1 || play_song == 1))    case midi::Clock
      {
        count_ppqn++;
        count_step=count_ppqn/scale_value[scale];
        if(count_ppqn>=(count_bars * scale_value[scale])-1) {
          count_ppqn=-1; 
          step_position++;     
          // song_position++;
          // if (song_position==16) song_position=0;
          if ( step_position >= count_bars ) { step_position=0; }
          // Play Notes!!
          Update_Midi();
          // Request news from Drum-Buttons
          Check_DrumButtons(); 
          // Update the LEDs 
          showStep (step_position, address1, address2, notes1, notes2 );  
        }
        // if (count_ppqn>1) led_flag=0;//Led clignote reste ON 1 count sur 6
        // if (count_ppqn<=1) led_flag=1; 
        // led_flag=!led_flag;  
      }
      // if (data==MIDI_CLOCK && (play_pattern == 0 || play_song==0)){
      //  count_led++;
      //  if(count_led==12){
      //    count_led=0;
      //    led_flag=!led_flag;
      //  }
      //}
      
    }
  }
  //-----------------Sync MASTER-------------------//
  if(!midi_sync){
  */
    // timer_time=2500000/bpm;
    // midiA(MIDI_CLOCK);
     digitalWrite(13, digitalRead(13) ^ 1);
    /* 
    lcd.setCursor(0,1);
    lcd.print( " Timer" + String( count_ppqn ) ); 

    
    // midiA.sendRealTime(MIDI_NAMESPACE::Clock);
    if( playBeats==true ) //play_pattern||play_song)
    {   
      count_ppqn++;    
      count_step=count_ppqn/scale_value[scale];   
      if(count_ppqn>=(count_bars*scale_value[scale])-1){
        count_ppqn=-1;
        step_position++;
        // if (song_position==16) song_position=0;
        if ( step_position>= count_bars ) { step_position=0; }
        // Play Notes
        Update_Midi();
        Check_DrumButtons(); 
        // Update the LEDs 
        showStep (step_position, address1, address2, notes1, notes2 );
      }
      // if (count_ppqn>1) led_flag=0;//Led blink 1 count on 6
      // if (count_ppqn<=1) led_flag=1; 
      // led_flag=!led_flag;
    }
    else if(playBeats==false) // !play_pattern &&!play_song)
    {
      count_ppqn=-1;
      step_position=0;
      // count_led++;
      // song_position=0;
      // if(count_led==12){
      //  count_led=0;
      //  led_flag=!led_flag;
      // }
      
    }
//   }
*/
}
