/* 09:32 15/03/2023 - change triggering comment */
#include "just_do_coffee.h"
#include "../lcd/lcd.h"

extern unsigned long steamTime;
// inline static float TEMP_DELTA(float d) { return (d*DELTA_RANGE); }
inline static float TEMP_DELTA(float d, const SensorState &currentState) {
  return (
    d * (currentState.pumpFlow < 1.f
      ? currentState.pumpFlow / 7.f
      : currentState.pumpFlow / 5.f
    )
  );
}

void justDoCoffee(const eepromValues_t &runningCfg, const SensorState &currentState, const bool brewActive) {
  lcdTargetState((int)HEATING::MODE_brew); // setting the target mode to "brew temp"
  float brewTempSetPoint = ACTIVE_PROFILE(runningCfg).setpoint + runningCfg.offsetTemp;
  float sensorTemperature = currentState.temperature + runningCfg.offsetTemp;

  if (brewActive || !currentState.brewSwitchState) { // keep steam boiler supply valve open while steaming/descale only
    closeSteamValve();
  }

  if (brewActive) { //if brewState == true
    if(sensorTemperature <= brewTempSetPoint - 5.f) {
      setBrewBoilerOnSteamOff();
    } else {
      float deltaOffset = 0.f;
      if (runningCfg.brewDeltaState) {
        float tempDelta = TEMP_DELTA(brewTempSetPoint, currentState);
        float BREW_TEMP_DELTA = mapRange(sensorTemperature, brewTempSetPoint, brewTempSetPoint + tempDelta, tempDelta, 0, 0);
        deltaOffset = constrain(BREW_TEMP_DELTA, 0, tempDelta);
      }
      if (sensorTemperature <= brewTempSetPoint + deltaOffset) {
        pulseHeaters(runningCfg.hpwr, runningCfg.mainDivider, runningCfg.brewDivider, brewActive);
      } else {
        setBrewBoilerOffSteamOn();
      }
    }
  } else { //if brewState == false
    if (sensorTemperature <= ((float)brewTempSetPoint - 10.f)) {
      setBrewBoilerOnSteamOff();
    } else {
      int HPWR_LOW = runningCfg.hpwr / runningCfg.mainDivider;
      // Calculating the boiler heating power range based on the below input values
      int HPWR_OUT = mapRange(sensorTemperature, brewTempSetPoint - 10, brewTempSetPoint, runningCfg.hpwr, HPWR_LOW, 0);
      HPWR_OUT = constrain(HPWR_OUT, HPWR_LOW, runningCfg.hpwr);  // limits range of sensor values to HPWR_LOW and HPWR

      if (sensorTemperature <= ((float)brewTempSetPoint - 5.f)) {
        pulseHeaters(HPWR_OUT, 1, runningCfg.mainDivider, brewActive);
      } else if (sensorTemperature < ((float)brewTempSetPoint)) {
        pulseHeaters(HPWR_OUT,  runningCfg.brewDivider, runningCfg.brewDivider, brewActive);
      } else {
        setBrewBoilerOffSteamOn();
      }
    }
  }
}

void pulseHeaters(const uint32_t pulseLength, const int factor_1, const int factor_2, const bool brewActive) {
  static uint32_t heaterWave;
  static bool heaterState;
  if (!heaterState && ((millis() - heaterWave) > (pulseLength * factor_1))) {
    brewActive ? setBrewBoilerOff() : setBrewBoilerOnSteamOff();
    heaterState=!heaterState;
    heaterWave=millis();
  } else if (heaterState && ((millis() - heaterWave) > (pulseLength / factor_2))) {
    brewActive ? setBrewBoilerOnSteamOff() : setBrewBoilerOff();
    heaterState=!heaterState;
    heaterWave=millis();
  }
}

//#############################################################################################
//################################____STEAM_POWER_CONTROL____##################################
//#############################################################################################
void steamCtrl(const eepromValues_t &runningCfg, SensorState &currentState) {
  currentState.steamSwitchState ? lcdTargetState((int)HEATING::MODE_steam) : lcdTargetState((int)HEATING::MODE_brew); // setting the steam/hot water target temp

  if (currentState.smoothedPressure > steamThreshold_) {
    // Since the steam boiler has its own OPV, and thermostat, no need to turn off power
    // Just keep it hot while steaming but only add water based on pressure demands
    setSteamBoilerOff();
    closeSteamValve();
    setPumpOff();
  } else {
    // make sure the brew boiler is off, steam boiler is on
    // THIS IS SPECIFIC TO DUAL BOILER BUILDS USING PCBV2 DEFINE
    setBrewBoilerOffSteamOn();
    openSteamValve();
    #ifndef DREAM_STEAM_DISABLED // disabled for bigger boilers which have no  need of adding water during steaming
      if (currentState.smoothedPressure < activeSteamPressure_) {
        setPumpToRawValue(8);
      } else {
        setPumpOff();
      }
    #endif
  }

  /*In case steam is forgotten ON for more than 15 min*/
  if (currentState.smoothedPressure > passiveSteamPressure_) {
    currentState.isSteamForgottenON = millis() - steamTime >= STEAM_TIMEOUT;
  } else steamTime = millis();
}

/*Water mode and all that*/
void hotWaterMode(const SensorState &currentState) {
  closeBrewValve();
  openSteamValve();
  setPumpToRawValue(80);
  setBrewBoilerOffSteamOn();
}
