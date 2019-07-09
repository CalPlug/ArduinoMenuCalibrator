/*
 * EEPROM Read
 *
 * Reads the value of each byte of the EEPROM and prints it
 * to the computer.
 * This example code is in the public domain.
 * //compiled from examples by Michael Klopfer, Ph.D. Univ. Of California, Irvine
 */
 
#include <Arduino.h>
#include <EEPROM.h>

// start reading from the first byte (address 0) of the EEPROM
int address = 0; //start at position specified through end of EEPROM as specified by EEPROM.length()
byte value;

//#define FLUSHEEPROM //Turn this on and the EEPROM will be wiped and this program's printout will verify this, NOTE: Besides the ovbious data loss issue, EEPROM has limited write cycles so avoid doing this too much

void setup() {

  // initialize serial and wait for port to open:
  Serial.begin(115200);
  while (!Serial) {
    ; // wait for serial port to connect. Needed for native USB port only
  }
  EEPROM.begin(); //this takes 0 arguments for AVR, but a value of 512 for ESP32
  delay(20);
  Serial.print("Current EEPROM Memory Total Size (in bytes): ");
  Serial.println(EEPROM.length(), DEC);
  Serial.println("");
  #ifdef FLUSHEEPROM
  eepromreset(0, EEPROM.length()); //clear between denoted addresses
  #endif
  Serial.println("");
  Serial.println("Current EEPROM Values:");
  Serial.println("EEPROM_Address(byte)   Interger_Value   Character_Value");
  Serial.println("_________________________________________________________");


  
}

void loop() {
  // read a byte from the current address of the EEPROM
  value = EEPROM.read(address);
  char chrvalue = (char)EEPROM.read(address); //read in interpretinbg this value as a character
  Serial.print(address);
  Serial.print("                              ");
  Serial.print(value, DEC);
  Serial.print("                        ");
  Serial.print(chrvalue);
  Serial.println();

  /***
    Advance to the next address, when at the end restart at the beginning.

    Larger AVR processors have larger EEPROM sizes, E.g:
    - Arduno Duemilanove: 512b EEPROM storage.
    - Arduino Uno:        1kb EEPROM storage.
    - Arduino Mega:       4kb EEPROM storage.

    Rather than hard-coding the length, you should use the pre-provided length function.
    This will make your code portable to all AVR processors.
  ***/
  address = address + 1;
  if (address == EEPROM.length()) {
    //address = 0; //use to repeat printout in a cont. loop
    while(1)
      {
        //stop and hold here after readout
      }
    }
  

  /***
    As the EEPROM sizes are powers of two, wrapping (preventing overflow) of an
    EEPROM address is also doable by a bitwise and of the length - 1.

    ++address &= EEPROM.length() - 1;
  ***/

  delay(50); //changes the speed of the printout process
}

void eepromreset(int lower, int upper)
{
int overwritecharacter = 255; //overwrite with default value of 255 (0's also can be used if preferred)
  //Mechanism called by another function to write pre-packaged data to the EEPROM
  Serial.print(F("Call to CLEAR EEPROM between the following addresses: "));
  Serial.print(lower);
  Serial.print(F(" and "));
  Serial.println(upper);
  delay (20);  //allow serial 
  for (int i = lower; (i <  upper); ++i){
    EEPROM.write(i, overwritecharacter); //write all the overwrite character to the specified address range
    delay(1);
  }
  //EEPROM.commit(); //Not required for AVR, only ESP32
  Serial.println(F("EEPROM Overwrite Complete"));
} //end of eepromreset function define


