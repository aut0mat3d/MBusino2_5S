// --- DS18B20 85°C Glitch Filter ---
float filterDS18B20(float newTemp, float oldTemp) {
  // 85.0°C is the hardware Power-On-Reset value.
  if (newTemp == 85.0f) {
    // If we have no valid previous reading (-127), or the previous 
    // reading is far away from 85°C, we reject the glitch!
    // Note: If the Sensor restarts, the verry first reading is always 85°C (Design flaw)
    //       - we want to avoid false 85°C Readings so we discard 85°C Values if they are
    //         (statistically) impossible - this is not 100% Save, but....
    if (oldTemp == -127.0f || oldTemp < 80.0f || oldTemp > 90.0f) {
      return oldTemp; 
    }
  }
  return newTemp;
}
// ---------------------------------------

void sensorRefresh1() {  // Triggers the temperature conversion
  if(userData.owSensors > 0) sensor1.requestTemperatures();
  if(userData.owSensors > 1) sensor2.requestTemperatures();
  if(userData.owSensors > 2) sensor3.requestTemperatures();
  if(userData.owSensors > 3) sensor4.requestTemperatures();
  if(userData.owSensors > 4) sensor5.requestTemperatures();
  if(userData.owSensors > 5) sensor6.requestTemperatures();
  if(userData.owSensors > 6) sensor7.requestTemperatures();
}

void sensorRefresh2() {  // Fetches the updated values and applies the glitch filter
  if(userData.owSensors > 0) { OW[0] = filterDS18B20(sensor1.getTempCByIndex(0), OW[0]); OWwO[0] = OW[0] + userData.sensorOffset[0]; }
  if(userData.owSensors > 1) { OW[1] = filterDS18B20(sensor2.getTempCByIndex(0), OW[1]); OWwO[1] = OW[1] + userData.sensorOffset[1]; }
  if(userData.owSensors > 2) { OW[2] = filterDS18B20(sensor3.getTempCByIndex(0), OW[2]); OWwO[2] = OW[2] + userData.sensorOffset[2]; }
  if(userData.owSensors > 3) { OW[3] = filterDS18B20(sensor4.getTempCByIndex(0), OW[3]); OWwO[3] = OW[3] + userData.sensorOffset[3]; }
  if(userData.owSensors > 4) { OW[4] = filterDS18B20(sensor5.getTempCByIndex(0), OW[4]); OWwO[4] = OW[4] + userData.sensorOffset[4]; }
  if(userData.owSensors > 5) { OW[5] = filterDS18B20(sensor6.getTempCByIndex(0), OW[5]); OWwO[5] = OW[5] + userData.sensorOffset[5]; }
  if(userData.owSensors > 6) { OW[6] = filterDS18B20(sensor7.getTempCByIndex(0), OW[6]); OWwO[6] = OW[6] + userData.sensorOffset[6]; }

  // BME readout
  if(userData.i2cMode > 0){
    temperatur = bme.readTemperature();
    druck = bme.readPressure() / 100.0F;
    hoehe = bme.readAltitude(SEALEVELPRESSURE_HPA);
    feuchte = bme.readHumidity();
  }
}
