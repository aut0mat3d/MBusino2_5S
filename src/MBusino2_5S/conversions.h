#ifndef CONVERSIONS_H
#define CONVERSIONS_H

#include <Arduino.h>

// Structure for normalized Value
struct NormalizedValue {
  double value;
  String unit;
};

// Auto-Normalize Function
NormalizedValue normalizeMBusValue(double rawValue, String rawUnit) {
  NormalizedValue result;
  result.value = rawValue;
  result.unit = rawUnit;
  
  rawUnit.trim(); // Remove Whitespaces

  // 1. ENERGY (Counter Values) to "kWh"
  if (rawUnit == "Wh") {
    result.value = rawValue / 1000.0;
    result.unit = "kWh";
  } 
  else if (rawUnit == "MWh") {
    result.value = rawValue * 1000.0;
    result.unit = "kWh";
  }
  else if (rawUnit == "J") {
    result.value = rawValue / 3600000.0;
    result.unit = "kWh";
  }
  else if (rawUnit == "kJ") {
    result.value = rawValue / 3600.0;
    result.unit = "kWh";
  }
  else if (rawUnit == "MJ") {
    result.value = rawValue / 3.6;
    result.unit = "kWh";
  }
  else if (rawUnit == "GJ") {
    result.value = rawValue * 277.777778;
    result.unit = "kWh";
  }

  // 2. POWER to "kW"
  else if (rawUnit == "W") {
    result.value = rawValue / 1000.0;
    result.unit = "kW";
  }
  else if (rawUnit == "MW") {
    result.value = rawValue * 1000.0;
    result.unit = "kW";
  }

  // 3. FLOW to "m³/h"
  else if (rawUnit == "l/h" || rawUnit == "L/h") {
    result.value = rawValue / 1000.0;
    result.unit = "m³/h";
  }
  else if (rawUnit == "l/min" || rawUnit == "L/min") {
    result.value = rawValue * 0.06;
    result.unit = "m³/h";
  }
  else if (rawUnit == "m^3/h") {
    result.unit = "m³/h"; 
  }

  // 4. VOLUME to "m³"
  else if (rawUnit == "l" || rawUnit == "L") {
    result.value = rawValue / 1000.0;
    result.unit = "m³";
  }
  else if (rawUnit == "m^3") {
    result.unit = "m³";
  }

  return result;
}

#endif
