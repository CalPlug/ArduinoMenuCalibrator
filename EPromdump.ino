/*
 * EEPROM Read
 *
 * Reads the value of each byte of the EEPROM and prints it
 * to the computer.
 * This example code is in the public domain.
 */
#include <Arduino.h>
#include <EEPROM.h>

// start reading from the first byte (address 0) of the EEPROM
int address = 0; //start at position specified through end of EEPROM as specified by EEPROM.length()
byte value;

void setup() {
  // initialize serial and wait for port to open:
  Serial.begin(115200);
  while (!Serial) {
    ; // wait for serial port to connect. Needed for native USB port only
  }
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
    address = 0;
  }

  /***
    As the EEPROM sizes are powers of two, wrapping (preventing overflow) of an
    EEPROM address is also doable by a bitwise and of the length - 1.

    ++address &= EEPROM.length() - 1;
  ***/

  delay(100);
}
