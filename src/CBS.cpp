#include "CBS.h"
#include "EEPROM.h"

CBS::CBS() {

}

void CBS::clearEEPROM(){
  EEPROM.begin(512);
  for (int i = 0; i < 512; i++) { EEPROM.write(i, 0); }
  EEPROM.end();
}

void CBS::ledBlink(int ledPin){
  digitalWrite(ledPin, HIGH);
  delay(500);
  digitalWrite(ledPin, LOW);
}