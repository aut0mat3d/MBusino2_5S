struct autodiscover {
  char bufferValue[800] = {0}; // Vergrößert für die neuen Namen
  char bufferTopic[150] = {0};
  char haName[30] = {0};
  char haUnits[30] = {0};
  char stateClass[30] = {0};
  char deviceClass[30] = {0};
  char deviceClassString[50] = {0};
} adVariables; 

const char bmeValue[4][12] = {"temperature","pressure","altitude","humidity"};
const char bmeDeviceClass[4][12] = {"temperature","pressure","distance","humidity"};
const char bmeUnits[4][5] = {"°C","mbar","m","%"};

const char adValueSys[] PROGMEM = R"rawliteral({"unique_id":"%s_sys_%s","state_topic":"%s/settings/%s","name":"%s","device":{"ids":["%s_sys"],"name":"%s System","manufacturer":"MBusino","mdl":"V%s"},"entity_category":"diagnostic"%s})rawliteral";
const char adTopicSys[] PROGMEM = R"rawliteral(homeassistant/sensor/%s/sys_%s/config)rawliteral";

// --- CONFIGURATION ENTITIES (NUMBER PLATFORM) ---
const char adValueNumber[] PROGMEM = R"rawliteral({"unique_id":"%s_cfg_%s","state_topic":"%s/settings/%s","command_topic":"%s/settings/cmd/%s","name":"%s","device":{"ids":["%s_%s"],"name":"%s","manufacturer":"MBusino","mdl":"V%s"},"entity_category":"config","min":%s,"max":%s,"step":%s,"unit_of_meas":"%s","mode":"box"})rawliteral";
const char adTopicNumber[] PROGMEM = R"rawliteral(homeassistant/number/%s/%s_cfg_%s/config)rawliteral";
// ---------------------------------------------------------------

const char adValueBinary[] PROGMEM = R"rawliteral({"unique_id":"%s_%s_%s","state_topic":"%s/%s/status/%s","name":"%s","device":{"ids":["%s_%s"],"name":"%s","manufacturer":"MBusino","mdl":"V%s"},"device_class":"%s","payload_on":"1","payload_off":"0","entity_category":"diagnostic"})rawliteral";
const char adTopicBinary[] PROGMEM = R"rawliteral(homeassistant/binary_sensor/%s/%s_%s/config)rawliteral";

const char adValueText[] PROGMEM = R"rawliteral({"unique_id":"%s_%s_%s","state_topic":"%s/%s/status/%s","name":"%s","device":{"ids":["%s_%s"],"name":"%s","manufacturer":"MBusino","mdl":"V%s"},"icon":"%s","entity_category":"diagnostic"})rawliteral";

const char adValueMbus[] PROGMEM = R"rawliteral({"unique_id":"%s_%s_%s","state_topic":"%s/%s/%s","name":"%s","value_template":"{{value_json if value_json is defined else 0}}","unit_of_meas":"%s","state_class":"%s","device":{"ids": ["%s_%s"],"name":"%s","manufacturer": "MBusino","mdl":"V%s"},%s"availability_mode":"all"})rawliteral";
const char adTopicMbus[] PROGMEM = R"rawliteral(homeassistant/sensor/%s/%s_%s/config)rawliteral";

const char adValueSensor[] PROGMEM = R"rawliteral({"unique_id":"%s_%s","state_topic":"%s/%s","name":"%s","value_template":"{{value_json if value_json is defined else 0}}","unit_of_meas":"°C","state_class":"measurement","device":{"ids": ["%s_%s"],"name":"%s","manufacturer": "MBusino","mdl":"V%s"},"device_class":"temperature","availability_mode":"all"})rawliteral";
const char adTopicSensor[] PROGMEM = R"rawliteral(homeassistant/sensor/%s/%s/config)rawliteral";

const char adValueBME[] PROGMEM = R"rawliteral({"unique_id":"%s_%s_%s","state_topic":"%s/%s/%s","name":"%s","value_template":"{{value_json if value_json is defined else 0}}","unit_of_meas":"%s","state_class":"measurement","device":{"ids": ["%s_%s"],"name":"%s","manufacturer": "MBusino","mdl":"V%s"},"device_class":"%s","availability_mode":"all"})rawliteral";
const char adTopicBME[] PROGMEM = R"rawliteral(homeassistant/sensor/%s/%s_%s/config)rawliteral";

// Hilfsfunktion: Wandelt Leerzeichen für die Unique_IDs in Unterstriche um
String makeSafeId(String input) {
  String safe = input;
  safe.replace(" ", "_");
  safe.replace("-", "_");
  return safe;
}

// --- ÜBERGABE AN HOME ASSISTANT ---
void haHandoverMbus(const char* slaveName, bool applyPowerBugfix){ 
  if(adVariables.deviceClass[0] != 0){
    strcpy(adVariables.deviceClassString,String("\"device_class\": \"" + String(adVariables.deviceClass) + "\",").c_str());
  }
  
  String safeSlave = makeSafeId(slaveName);
  String safeSensor = makeSafeId(adVariables.haName);

  sprintf(adVariables.bufferValue, adValueMbus, 
    userData.mbusinoName, safeSlave.c_str(), safeSensor.c_str(), // unique_id
    userData.mbusinoName, slaveName, adVariables.haName,         // state_topic
    adVariables.haName,                                          // Name (nur "energy", HA stellt den Zählernamen automatisch davor)
    adVariables.haUnits, adVariables.stateClass,                 // unit, state_class
    userData.mbusinoName, safeSlave.c_str(),                     // device ids
    slaveName,                                                   // device name
    MBUSINO_VERSION, adVariables.deviceClassString);             // version, device_class

  sprintf(adVariables.bufferTopic, adTopicMbus, userData.mbusinoName, safeSlave.c_str(), safeSensor.c_str());
  client.publish(adVariables.bufferTopic, adVariables.bufferValue, true); 

  if(applyPowerBugfix == true && String(adVariables.haName) == "volume_flow"){  // Sensostar Bugfix an Volume Flow koppeln   
    sprintf(adVariables.bufferValue, adValueMbus, 
      userData.mbusinoName, safeSlave.c_str(), "power_calc",
      userData.mbusinoName, slaveName, "power_calc",
      "power_calc", "W", "measurement",
      userData.mbusinoName, safeSlave.c_str(), slaveName,
      MBUSINO_VERSION, "\"device_class\": \"power\",");
    sprintf(adVariables.bufferTopic, adTopicMbus, userData.mbusinoName, safeSlave.c_str(), "power_calc");
    client.publish(adVariables.bufferTopic, adVariables.bufferValue, true);                   
  } 
  
  adVariables.bufferTopic[0] = 0; adVariables.bufferValue[0] = 0;
  adVariables.deviceClass[0] = 0; adVariables.deviceClassString[0] = 0;
  adVariables.stateClass[0] = 0;  adVariables.haUnits[0] = 0;
}

void haHandoverOw(const char* sensorName){
  String safeSensor = makeSafeId(sensorName);
  
  sprintf(adVariables.bufferValue, adValueSensor, 
    userData.mbusinoName, safeSensor.c_str(),   // unique id
    userData.mbusinoName, sensorName,           // state topic
    sensorName,                                 // name
    userData.mbusinoName, safeSensor.c_str(),   // device id
    sensorName, MBUSINO_VERSION                 // device name, version
  );
  sprintf(adVariables.bufferTopic, adTopicSensor, userData.mbusinoName, safeSensor.c_str());
  client.publish(adVariables.bufferTopic, adVariables.bufferValue, true); 

  // --- SENSOR OFFSET SLIDERS ---
  auto sendNumberCfg = [&](const char* id, const char* name, const char* minV, const char* maxV, const char* stepV, const char* unit) {
    sprintf(adVariables.bufferValue, adValueNumber, 
      userData.mbusinoName, id, userData.mbusinoName, id, userData.mbusinoName, id, name, 
      userData.mbusinoName, safeSensor.c_str(), sensorName, MBUSINO_VERSION, minV, maxV, stepV, unit);
    sprintf(adVariables.bufferTopic, adTopicNumber, userData.mbusinoName, safeSensor.c_str(), id);
    client.publish(adVariables.bufferTopic, adVariables.bufferValue, true);
  };

  int idx = -1;
  if (strcmp(sensorName, userData.sensorName1) == 0) idx = 1;
  else if (strcmp(sensorName, userData.sensorName2) == 0) idx = 2;
  else if (strcmp(sensorName, userData.sensorName3) == 0) idx = 3;
  else if (strcmp(sensorName, userData.sensorName4) == 0) idx = 4;
  else if (strcmp(sensorName, userData.sensorName5) == 0) idx = 5;
  else if (strcmp(sensorName, userData.sensorName6) == 0) idx = 6;
  else if (strcmp(sensorName, userData.sensorName7) == 0) idx = 7;

  if (idx != -1) {
    String idOff = "offset" + String(idx);
    sendNumberCfg(idOff.c_str(), "Temperature Offset", "-20.0", "20.0", "0.1", "°C");
  }
  
  adVariables.bufferTopic[0] = 0; adVariables.bufferValue[0] = 0;
}

void haHandoverBME(const char* bName){
  String safeSensor = makeSafeId(bName);
  for(uint8_t i=0; i<4;i++){  
    sprintf(adVariables.bufferValue, adValueBME, 
      userData.mbusinoName, safeSensor.c_str(), bmeValue[i], // unique id
      userData.mbusinoName, bName, bmeValue[i],              // state topic
      bmeValue[i], bmeUnits[i],                              // name, unit
      userData.mbusinoName, safeSensor.c_str(),              // device id
      bName, MBUSINO_VERSION, bmeDeviceClass[i]              // device name, version, class
    );
    sprintf(adVariables.bufferTopic, adTopicBME, userData.mbusinoName, safeSensor.c_str(), bmeValue[i]);
    client.publish(adVariables.bufferTopic, adVariables.bufferValue, true); 
    
    adVariables.bufferTopic[0] = 0; adVariables.bufferValue[0] = 0;
  }
}

void haHandoverSystem() {
  // Construct a generic diagnostic Sensor
  auto sendSysSensor = [](const char* id, const char* name, const char* extra) {
    sprintf(adVariables.bufferValue, adValueSys, 
      userData.mbusinoName, id,                  // unique_id
      userData.mbusinoName, id,                  // state_topic (e.g. .../settings/RSSI)
      name,                                      // name
      userData.mbusinoName, userData.mbusinoName,// device ids, device name
      MBUSINO_VERSION, extra                     // version, extra config
    );
    sprintf(adVariables.bufferTopic, adTopicSys, userData.mbusinoName, id);
    client.publish(adVariables.bufferTopic, adVariables.bufferValue, true);
  };

  // 1. RSSI (Signal Strength)
  sendSysSensor("RSSI", "WiFi Signal", ",\"unit_of_meas\":\"dBm\",\"device_class\":\"signal_strength\"");
  // 2. Active SSID
  sendSysSensor("activeSSID", "Active WLAN", ",\"icon\":\"mdi:wifi\"");
  // 3. Reconnects
  sendSysSensor("WiFiReconnections", "WiFi Reconnects", ",\"icon\":\"mdi:wifi-sync\"");
  // 4. Free Heap (RAM)
  sendSysSensor("freeHeap", "Free RAM", ",\"unit_of_meas\":\"B\",\"icon\":\"mdi:memory\"");
  // 5. Uptime
  sendSysSensor("uptime", "System Uptime", ",\"icon\":\"mdi:clock-outline\"");

  // --- SYSTEM CONFIG SLIDERS ---
  auto sendNumberSysCfg = [&](const char* id, const char* name, const char* minV, const char* maxV, const char* stepV, const char* unit) {
    sprintf(adVariables.bufferValue, adValueNumber, 
      userData.mbusinoName, id,                  // unique_id
      userData.mbusinoName, id,                  // state_topic
      userData.mbusinoName, id,                  // command_topic
      name,                                      // name
      userData.mbusinoName, "sys",               // device ids (matches MBusino01_sys)
      "MBusino System", MBUSINO_VERSION,         // device name, version
      minV, maxV, stepV, unit                    // min, max, step, unit
    );
    sprintf(adVariables.bufferTopic, adTopicNumber, userData.mbusinoName, "sys", id);
    client.publish(adVariables.bufferTopic, adVariables.bufferValue, true);
  };

  sendNumberSysCfg("mbusInterval", "Poll Interval M-Bus", "10", "3600", "1", "s");
  sendNumberSysCfg("sensorInterval", "Poll Interval Sensors", "5", "3600", "1", "s");
}


void haHandoverStatus(const char* slaveName, ActiveProfile* profile) {
// We pass the active profile so we can read the custom state mapping
  String safeSlave = makeSafeId(slaveName);
  
  // Helper for binary sensors (Battery, Hardware errors)
  auto sendBinary = [&](const char* id, const char* name, const char* devClass) {
    sprintf(adVariables.bufferValue, adValueBinary, 
      userData.mbusinoName, safeSlave.c_str(), id, userData.mbusinoName, slaveName, id, 
      name, userData.mbusinoName, safeSlave.c_str(), slaveName, MBUSINO_VERSION, devClass);
    sprintf(adVariables.bufferTopic, adTopicBinary, userData.mbusinoName, safeSlave.c_str(), id);
    client.publish(adVariables.bufferTopic, adVariables.bufferValue, true);
  };

  // Helper for text sensors (Data Quality, App Status, and Custom Bits)
  auto sendText = [&](const char* id, const char* name, const char* icon) {
    sprintf(adVariables.bufferValue, adValueText, 
      userData.mbusinoName, safeSlave.c_str(), id, userData.mbusinoName, slaveName, id, 
      name, userData.mbusinoName, safeSlave.c_str(), slaveName, MBUSINO_VERSION, icon);
    sprintf(adVariables.bufferTopic, adTopicMbus, userData.mbusinoName, safeSlave.c_str(), id);
    client.publish(adVariables.bufferTopic, adVariables.bufferValue, true);
  };

  if(strlen(slaveName) > 0) {
    // 1. Standard EN 13757-3 Sensors
    /*
     * HOME ASSISTANT STATUS SENSOR MAPPING:
     * * 1. Binary Sensors (Payload "0" / "1"):
     * - device_class "battery": "0" is shown as "Normal", "1" is shown as "Low"
     * - device_class "problem": "0" is shown as "OK", "1" is shown as "Problem"
     * * 2. Text Sensors:
     * - data_quality: 
     * "real"  = Value is plausible, passing through.
     * "held"  = Value out of limits (Bouncer active). Last known good value is frozen.
     * "error" = Bouncer timeout reached. Hard fail, value forced to 0.0.
     * - application: Translates M-Bus Bit 0 & 1 into "OK", "Busy", "Error", "Abnormal".
     */
    sendBinary("battery_low", "Battery Low", "battery");
    sendBinary("permanent_err", "Permanent HW Error", "problem");
    sendBinary("temporary_err", "Temporary Error", "problem");
    sendText("application", "Application Status", "mdi:information-outline");
    sendText("data_quality", "Data Quality (Bouncer)", "mdi:shield-check");

    // 2. Custom Manufacturer Sensors (Bits 5-7) aus dem RAM-Struct laden!
    for (uint8_t k = 0; k < profile->customStateCount; k++) {
      if (strlen(profile->customStates[k].haName) > 0) {
        String safeId = makeSafeId(profile->customStates[k].haName);
        safeId.toLowerCase(); // Make it URL/MQTT safe
        sendText(safeId.c_str(), profile->customStates[k].haName, "mdi:state-machine");
      }
    }
  } // End of if(strlen(slaveName) > 0)

  // Helper to generate a number input entity linked to this specific meter
  auto sendNumberCfg = [&](const char* id, const char* name, const char* minV, const char* maxV, const char* stepV, const char* unit) {
    sprintf(adVariables.bufferValue, adValueNumber, 
      userData.mbusinoName, id,                  // unique_id
      userData.mbusinoName, id,                  // state_topic
      userData.mbusinoName, id,                  // command_topic
      name,                                      // name
      userData.mbusinoName, safeSlave.c_str(),   // device ids
      slaveName, MBUSINO_VERSION,                // device name, version
      minV, maxV, stepV, unit                    // min, max, step, unit
    );
    sprintf(adVariables.bufferTopic, adTopicNumber, userData.mbusinoName, safeSlave.c_str(), id);
    client.publish(adVariables.bufferTopic, adVariables.bufferValue, true);
  };

  // Determine the array index (1 to 5) based on the slaveName to map the correct variable
  int idx = -1;
  if (strcmp(slaveName, userData.slaveName1) == 0) idx = 1;
  else if (strcmp(slaveName, userData.slaveName2) == 0) idx = 2;
  else if (strcmp(slaveName, userData.slaveName3) == 0) idx = 3;
  else if (strcmp(slaveName, userData.slaveName4) == 0) idx = 4;
  else if (strcmp(slaveName, userData.slaveName5) == 0) idx = 5;

  if (idx != -1) {
    String idMinF = "minFlow" + String(idx);
    String idMaxF = "maxFlow" + String(idx);
    String idMaxP = "maxPower" + String(idx);
    String idDead = "deadFlow" + String(idx);

    sendNumberCfg(idMinF.c_str(), "Min Flow Limit", "-999.0", "999.0", "0.01", "m³/h");
    sendNumberCfg(idMaxF.c_str(), "Max Flow Limit", "-999.0", "999.0", "0.01", "m³/h");
    sendNumberCfg(idMaxP.c_str(), "Max Power Limit", "-999.0", "999.0", "0.01", "kW");
    sendNumberCfg(idDead.c_str(), "Deadband", "0.0", "999.0", "0.001", "m³/h");
  }
}
