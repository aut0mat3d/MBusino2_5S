void calibrationAverage() {
  client.publish(String(String(userData.mbusinoName) + "/cal/started").c_str(), "building average");

  float sum = 0;
  uint8_t connected = 0;
  for(uint8_t i = 0; i < userData.owSensors; i++){
    if(OW[i] == -127){ 
      OWnotconnected[i] = true;       
    } else {  
      sum += OW[i];
      connected++;
    }
  }
  
  if (connected == 0) return; // Prevent division by zero!
  
  float average = sum / connected;
  client.publish(String(String(userData.mbusinoName) + "/cal/connected").c_str(), String(connected).c_str());
  client.publish(String(String(userData.mbusinoName) + "/cal/sum").c_str(), String(sum).c_str());
  client.publish(String(String(userData.mbusinoName) + "/cal/average").c_str(), String(average).c_str()); 

  for(uint8_t i = 0; i < userData.owSensors; i++){
    if(!OWnotconnected[i]){
      userData.sensorOffset[i] = average - OW[i];
      client.publish(String(String(userData.mbusinoName) + "/cal/offsetS" + String(i+1)).c_str(), String(userData.sensorOffset[i]).c_str());
    }
  }
  
  // Trigger modern LittleFS save in main loop!
  credentialsReceived = true; 
}

void calibrationSensor(uint8_t sensor){
  if((sensor<userData.owSensors) && (OW[sensor]!= -127)){
    sensorToCalibrate = sensor;
  } else {
    client.publish(String(String(userData.mbusinoName) + "/cal/offsetS" + String(sensor+1)).c_str(), "No valid sensor");
  }
}

void calibrationValue(float value){
  client.publish(String(String(userData.mbusinoName) + "/cal/started").c_str(), "set new offset");
   
  if(OW[sensorToCalibrate] != -127){
    userData.sensorOffset[sensorToCalibrate] += value;
    client.publish(String(String(userData.mbusinoName) + "/cal/offsetS" + String(sensorToCalibrate+1)).c_str(), String(userData.sensorOffset[sensorToCalibrate]).c_str());
    
    // Trigger modern LittleFS save in main loop!
    credentialsReceived = true; 
  } else {
    client.publish(String(String(userData.mbusinoName) + "/cal/offsetS" + String(sensorToCalibrate)).c_str(), "No Sensor");
  }
}

void calibrationBME(){
  client.publish(String(String(userData.mbusinoName) + "/cal/started").c_str(), "set BME280 based offset");
  client.publish(String(String(userData.mbusinoName) + "/cal/BME").c_str(), String(temperatur).c_str());

  for(uint8_t i = 0; i < 5; i++){
    if(OW[i] == -127) OWnotconnected[i] = true;       
  }

  for(uint8_t i = 0; i < 5; i++){
    if(!OWnotconnected[i]){
      userData.sensorOffset[i] = temperatur - OW[i];
      client.publish(String(String(userData.mbusinoName) + "/cal/offsetS" + String(i+1)).c_str(), String(userData.sensorOffset[i]).c_str());
    }
  }
  
  // Trigger modern LittleFS save in main loop!
  credentialsReceived = true; 
}

void calibrationSet0(){
  client.publish(String(String(userData.mbusinoName) + "/cal/started").c_str(), "set all offsets 0");

  for(uint8_t i = 0; i < userData.owSensors; i++){
    if(OW[i] == -127) OWnotconnected[i] = true;       
  }
  
  for(uint8_t i = 0; i < userData.owSensors; i++){
    userData.sensorOffset[i] = 0;
    if(!OWnotconnected[i]){
      client.publish(String(String(userData.mbusinoName) + "/cal/offsetS" + String(i+1)).c_str(), String(userData.sensorOffset[i]).c_str());
    }
  }
  
  // Trigger modern LittleFS save in main loop!
  credentialsReceived = true; 
}
