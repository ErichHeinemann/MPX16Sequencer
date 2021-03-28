# MPX16Sequencer

The AKAI MPX16 is a more or less unasable Sampler for me.

I bought a Akai MPC2500 at the same time, which has a Sequencer and a Volca Beats which is very nice.
After some tests with the Arduino and some PCF8574 I2C - Multiplexers I found a solution to create a Step-SEquencer in the well known TR-Style.

When the Beat is ongoing the sound could be selected using a PAD of the MPX.
To activate the steps, You have to press on of the small 16 Buttons. Each Button has an assigned LEDs, which displays the current played step and the status of the step.

https://www.youtube.com/watch?v=Tr9Y971fDvM

The Menü is displayed in the display and the UI consist of 3 buttons for Start/Stop and Select.
The Menü is managed by a POT.

My Experience:
As this was only a test, how "low" or "cheap" I could go with additional UI.

In a final stage, these 3 Buttons are not enough to provide a acceptable UI and the POT has to be replaced by a Rotary Encoder.

I had developed this in around 2015 and did not use it till Corona in 2020. End of 2020 I grabbed that old unfinished project and wanted to add an internal Sound-Engine.
The Arduino UNO is not good enough for 44.1KHz-Samples and 16 Bit. So i decided to do a restart with the ESP32 or ESP8266.

Luckely, I found a new developer which does exaclty this:
Marcel Licence
https://www.youtube.com/watch?v=vvA7vfouk84

Marcel Licence has already developed some really cool sounding Sound-Engines which are using the ESP32.

The MPX16-Sequencer has some bad things:
- Currently I have some missing MIDI-Bits sometimes. - probably a GM-Doughterboard  which uses the same Port or it depends on Softserial which is less good as Hardware-Serial
- MIDI-Clock on another Port has to be implemented to enable the system to be synchronized to other stuff, like to an Akai MPC.
- With that code, the Arduino at it´s end, - RAM is used too much by the libraries. A solution could be to drop the MIDI-Lib and go back to native Serial.
- The Sequences are not stored yet. I had build a version with storage in Eeprom but I lost that code.

If You plan to build it, the sequences are simply stored as 2 Bytes for all 16 Steps.
If You think, this is useful for Your existing MPX16, this could be a funny project for a weekend. If You think about buying a MPX16 for that, - rethink it more than 3 times.


I made 2 other projects which use the same hardware and slightly modified code.
1. - a simpel Drum-Sequencer which uses other Menu and uses an internal Sound-Engine based on a simpel MIDI GM-Sound-Board with a SAM-Chip.
2. - a Arpeggio/Drum-Sequencer which uses the first 4 or 5 Instruments to play Arpeggios on MIDI Channel 1 and play 10 Drum-Instruments on Channel 10. The Arpeggio plays the notes which arrived by MIDI-IN.

The Arpeggio/Drum-Sequencer is the most promising project for me but it uses too much RAM.

For me, as I tested other multiplexers, I would say that the PCF8574-Solution works as the best one.

As soon I have ported the code to the ESP32, I will let You know.

Best Regards
Erich

Copyright: - All Open Source, take it and make something better with it.
