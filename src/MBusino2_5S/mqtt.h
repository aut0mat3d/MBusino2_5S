PubSubClient client(espClient);

void reconnect() {
  // Loop until we're reconnected
  char lwBuffer[30] = {0};
  sprintf(lwBuffer, userData.mbusinoName, "/lastwill");
  if (client.connect(userData.mbusinoName,userData.mqttUser,userData.mqttPswrd,lwBuffer,0,false,"I am going offline")) {
    // Once connected, publish an announcement...
    #if defined(ESP32)
    Serial.println("MQTT connected to server");
    #endif    
    conCounter++;
    if(conCounter == 1){
      client.publish(String(String(userData.mbusinoName) + "/start").c_str(), "Bin hochgefahren, WLAN und MQTT steht");
    }
    else{
      client.publish(String(String(userData.mbusinoName) + "/reconnect").c_str(), "Online again!");
      adMbusMessageCounter = 2;
      adSensorMessageCounter = 2;
    }
    // ... and resubscribe
    client.subscribe(String(String(userData.mbusinoName) + "/calibrateAverage").c_str());
    client.subscribe(String(String(userData.mbusinoName) + "/calibrateSensor").c_str());
    client.subscribe(String(String(userData.mbusinoName) + "/calibrateValue").c_str());
    client.subscribe(String(String(userData.mbusinoName) + "/calibrateBME").c_str());
    client.subscribe(String(String(userData.mbusinoName) + "/calibrateSet0").c_str());
    client.subscribe(String(String(userData.mbusinoName) + "/mbusPolling").c_str());
    client.subscribe(String(String(userData.mbusinoName) + "/settings/cmd/#").c_str());
  }
  else{
    #if defined(ESP32)
    Serial.println("MQTT unable to connect server");
    #endif
    pulseInterval = 400;
  }
}

void callback(char* topic, byte* payload, unsigned int length) {
  if(userData.owSensors > 0 || userData.i2cMode > 0){
    if (strcmp(topic,String(String(userData.mbusinoName) + "/calibrateAverage").c_str())==0){  
      calibrationAverage();
    }
    if (strcmp(topic,String(String(userData.mbusinoName) + "/calibrateSensor").c_str())==0){  
      calibrationSensor(atoi((char*)payload)-1);
    }
    if (strcmp(topic,String(String(userData.mbusinoName) + "/calibrateValue").c_str())==0){  
      calibrationValue(atof((char*)payload));
    }  
    if(userData.i2cMode == 1){
      if (strcmp(topic,String(String(userData.mbusinoName) + "/calibrateBME").c_str())==0){  
        calibrationBME();
      }
    }
    if (strcmp(topic,String(String(userData.mbusinoName) + "/calibrateSet0").c_str())==0){  
      calibrationSet0();
    } 
  }
  if (strcmp(topic,String(String(userData.mbusinoName) + "/mbusPolling").c_str())==0){  
    pollingAddress = atoi((char*)payload);
    polling = true;
  }
  
  // --- REMOTE CONFIGURATION PARSER ---
  String topicStr = String(topic);
  String cmdPrefix = String(userData.mbusinoName) + "/settings/cmd/";
  
  if (topicStr.startsWith(cmdPrefix)) {
    // Extract the exact setting name (e.g., "minFlow1" or "deadFlow3")
    String setting = topicStr.substring(cmdPrefix.length());
    
    // Parse the payload into a string
    String payloadStr = "";
    for (int i = 0; i < length; i++) payloadStr += (char)payload[i];
    
    float valFloat = payloadStr.toFloat();
    bool valueChanged = false;

    // Check if the setting relates to a slave array variable (ends with 1-5)
    int slaveIndex = setting.substring(setting.length() - 1).toInt() - 1; 

    if (slaveIndex >= 0 && slaveIndex <= 4) {
      if (setting.startsWith("minFlow")) {
        userData.minFlow[slaveIndex] = valFloat; valueChanged = true;
      } 
      else if (setting.startsWith("maxFlow")) {
        userData.maxFlow[slaveIndex] = valFloat; valueChanged = true;
      } 
      else if (setting.startsWith("maxPower")) {
        userData.maxPower[slaveIndex] = valFloat; valueChanged = true;
      } 
      else if (setting.startsWith("deadFlow")) {
        userData.deadbandFlow[slaveIndex] = valFloat; valueChanged = true;
      }

      // Check for Sensor Offsets (ends with 1-7)
      int sensorIndex = setting.substring(setting.length() - 1).toInt() - 1; 
      if (sensorIndex >= 0 && sensorIndex <= 6) {
        if (setting.startsWith("offset")) {
          userData.sensorOffset[sensorIndex] = valFloat; valueChanged = true;
        }
      }
  
      // Check for System Intervals (convert seconds from HA back to milliseconds for ESP)
      if (setting == "mbusInterval") {
        userData.mbusInterval = (uint32_t)valFloat * 1000; valueChanged = true;
      }
      else if (setting == "sensorInterval") {
        userData.sensorInterval = (uint32_t)valFloat * 1000; valueChanged = true;
      }
    }

    // If a setting was matched and updated
    if (valueChanged) {
      // 1. Flag for EEPROM Save in main loop()
      credentialsReceived = true; 
      
      // 2. Acknowledge back to Home Assistant (State Topic)
      // This is crucial, otherwise the HA slider will jump back!
      String stateTopic = String(userData.mbusinoName) + "/settings/" + setting;
      client.publish(stateTopic.c_str(), payloadStr.c_str(), true); // Retained state
      
      #if defined(ESP32) || defined(DEBUG)
      Serial.println("MQTT Remote Config applied: " + setting + " = " + payloadStr);
      #endif
    }
  }
  // --------------------------------------------------
}   
