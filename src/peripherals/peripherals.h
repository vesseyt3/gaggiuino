/* 09:32 15/03/2023 - change triggering comment */
#ifndef PERIPHERALS_H
#define PERIPHERALS_H

#include "pindef.h"
#include "peripherals.h"
#include <Arduino.h>

static inline void pinInit(void) {
  #if defined(LEGO_VALVE_RELAY)
    pinMode(valvePin, OUTPUT_OPEN_DRAIN);
  #else
    pinMode(valvePin, OUTPUT);
  #endif
  pinMode(relayPin, OUTPUT);
  #ifdef steamValveRelayPin
  pinMode(steamValveRelayPin, OUTPUT);
  #endif
  #ifdef steamBoilerRelayPin
  pinMode(steamBoilerRelayPin, OUTPUT);
  #endif
  pinMode(brewPin,  INPUT_PULLUP);
  pinMode(steamPin, INPUT_PULLUP);
  #ifdef waterPin
  pinMode(waterPin, INPUT_PULLUP);
  #endif
}

static inline void openSteamValve(void) {
  #ifdef steamValveRelayPin
  digitalWrite(steamValveRelayPin, HIGH);  // steamValveRelayPin -> HIGH
  #endif
}

static inline void closeSteamValve(void) {
  #ifdef steamValveRelayPin
  digitalWrite(steamValveRelayPin, LOW);  // steamValveRelayPin -> LOW
  #endif
}

static int brewBoilerTimeTurnedOff = 0;

static inline void setSteamBoilerOn(void) {
  #ifdef steamBoilerRelayPin
  digitalWrite(steamBoilerRelayPin, HIGH);  // steamBoilerRelayPin -> HIGH
  #endif
}

static inline void setSteamBoilerOff(void) {
  #ifdef steamBoilerRelayPin
  digitalWrite(steamBoilerRelayPin, LOW);  // steamBoilerRelayPin -> LOW
  #endif
}

static inline void setBrewBoilerOff(void) {
  digitalWrite(relayPin, LOW);  // boilerPin -> LOW

  if (brewBoilerTimeTurnedOff == 0) {
    brewBoilerTimeTurnedOff = millis();
  }
}

// Actuating the heater element for brew 
static inline void setBrewBoilerOnSteamOff(void) {
  setSteamBoilerOff();
  digitalWrite(relayPin, HIGH);  // boilerPin -> HIGH
  brewBoilerTimeTurnedOff = 0;
}

static inline void setBrewBoilerOffSteamOn(void) {
  digitalWrite(relayPin, LOW);  // boilerPin -> LOW
  
  // This is just used to de-bounce the calls to this function  when the temp is 
  // hovering around the set point -- only kick this on if 
  // excessive toggling of the steam relay    
  if (brewBoilerTimeTurnedOff == 0) {
    brewBoilerTimeTurnedOff = millis();
  }

  if (millis() - brewBoilerTimeTurnedOff > 1000) {
    setSteamBoilerOn();
  }
}

//Function to get the state of the brew switch button
//returns true or false based on the read P(power) value
static inline bool brewState(void) {
  return digitalRead(brewPin) == LOW; // pin will be low when switch is ON.
}

// Returns HIGH when switch is OFF and LOW when ON
// pin will be high when switch is ON.
static inline bool steamState(void) {
  return digitalRead(steamPin) == LOW; // pin will be low when switch is ON.
}

static inline bool waterPinState(void) {
  #ifdef waterPin
  return digitalRead(waterPin) == LOW; // pin will be low when switch is ON.
  #else
  return false;
  #endif
}

static inline void openBrewValve(void) {
  #if defined LEGO_VALVE_RELAY
    digitalWrite(valvePin, LOW);
  #else
    digitalWrite(valvePin, HIGH);
  #endif
}

static inline void closeBrewValve(void) {
  #if defined LEGO_VALVE_RELAY
    digitalWrite(valvePin, HIGH);
  #else
    digitalWrite(valvePin, LOW);
  #endif
}

#endif
