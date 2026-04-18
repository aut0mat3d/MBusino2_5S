AsyncWebServer server(80);
AsyncWebSocket ws("/ws"); // access at ws://[esp ip]/ws
AsyncEventSource events("/events"); // event source (Server-Sent events)

// Global flags to track changes across multiple fields
bool settingsChanged = false;
bool wlanChanged = false;

// --- HELPER FUNCTIONS FOR DELTA-CHECKING ---
// These functions check if a received value differs from the RAM value.
// If it does, it saves the new value and flags 'settingsChanged = true'.

bool updateStringIfChanged(AsyncWebServerRequest *request, const char* paramName, char* targetVar, size_t maxLen) {
  if (request->hasParam(paramName, true)) { // 'true' means search in POST body
    String newVal = request->getParam(paramName, true)->value();
    
    // Ignore **** (dummy password fields)
    if (newVal == "****") {
      return false; 
    }

    // Custom logic to handle empty web passwords
    if (strcmp(paramName, "webPassword") == 0 && (newVal == "none" || newVal == "NONE" || newVal == "")) {
      newVal = "NONE";
    }
    
    if (newVal != String(targetVar)) {
      strlcpy(targetVar, newVal.c_str(), maxLen);
      settingsChanged = true;
      return true; // Indicates an actual change
    }
  }
  return false;
}

template <typename T>
void updateIntIfChanged(AsyncWebServerRequest *request, const char* paramName, T &targetVar) {
  if (request->hasParam(paramName, true)) {
    T newVal = (T)request->getParam(paramName, true)->value().toInt();
    if (newVal != targetVar) {
      targetVar = newVal;
      settingsChanged = true;
    }
  }
}

void updateFloatIfChanged(AsyncWebServerRequest *request, const char* paramName, float &targetVar) {
  if (request->hasParam(paramName, true)) {
    float newVal = request->getParam(paramName, true)->value().toFloat();
    if (newVal != targetVar) {
      targetVar = newVal;
      settingsChanged = true;
    }
  }
}

void updateBoolIfChanged(AsyncWebServerRequest *request, const char* paramName, bool &targetVar) {
  if (request->hasParam(paramName, true)) {
    bool newVal = (request->getParam(paramName, true)->value() == "1");
    if (newVal != targetVar) {
      targetVar = newVal;
      settingsChanged = true;
    }
  }
}

// --- TEMPLATE PROCESSOR ---
// Fills the HTML placeholders with the actual variables
String processor(const String& var) {
  if(var == "MBUSINO_NAME") return String(userData.mbusinoName);
  if(var == "MBUSINO_VERSION") return String(MBUSINO_VERSION);
 
  if(var == "PWD_TYPE") return "password";
  if(var == "WEB_PWD") return (strcmp(userData.webPassword, "NONE") == 0 || strlen(userData.webPassword) == 0) ? "NONE" : "****";
  if(var == "OTA_PWD") return strlen(userData.otaPassword) > 0 ? "****" : "";
  
  if(var == "HA_SEL_0") return userData.haAutodisc == 0 ? "selected" : "";
  if(var == "HA_SEL_1") return userData.haAutodisc == 1 ? "selected" : "";
  if(var == "DBG_SEL_0") return userData.telegramDebug == 0 ? "selected" : "";
  if(var == "DBG_SEL_1") return userData.telegramDebug == 1 ? "selected" : "";
  
  if(var == "SSID1") return String(userData.ssid);
  if(var == "PWD1") return strlen(userData.password) > 0 ? "****" : "";
  if(var == "SSID2") return String(userData.ssid2);
  if(var == "PWD2") return strlen(userData.password2) > 0 ? "****" : "";
  if(var == "AP_CHAN") return String(userData.apChannel);
  
  if(var == "BROKER") return String(userData.broker);
  if(var == "PORT") return String(userData.mqttPort);
  if(var == "MQTT_USER") return String(userData.mqttUser);
  if(var == "MQTT_PWD") return strlen(userData.mqttPswrd) > 0 ? "****" : "";

  if(var == "MBUS_SLAVES") return String(userData.mbusSlaves);
  if(var == "MBUS_INT") return String(userData.mbusInterval / 1000);
  
  if(var == "ADDR1") return String(userData.mbusAddress1); if(var == "MNAME1") return String(userData.slaveName1); if(var == "PROF1") return String(userData.slaveProfile[0]);
  if(var == "MINF1") return String(userData.minFlow[0]); if(var == "MAXF1") return String(userData.maxFlow[0]); if(var == "MAXP1") return String(userData.maxPower[0]); 
  if(var == "DEAD1") return isnan(userData.deadbandFlow[0]) ? "0.000" : String(userData.deadbandFlow[0], 3);
  
  if(var == "ADDR2") return String(userData.mbusAddress2); if(var == "MNAME2") return String(userData.slaveName2); if(var == "PROF2") return String(userData.slaveProfile[1]);
  if(var == "MINF2") return String(userData.minFlow[1]); if(var == "MAXF2") return String(userData.maxFlow[1]); if(var == "MAXP2") return String(userData.maxPower[1]); 
  if(var == "DEAD2") return isnan(userData.deadbandFlow[1]) ? "0.000" : String(userData.deadbandFlow[1], 3);

  if(var == "ADDR3") return String(userData.mbusAddress3); if(var == "MNAME3") return String(userData.slaveName3); if(var == "PROF3") return String(userData.slaveProfile[2]);
  if(var == "MINF3") return String(userData.minFlow[2]); if(var == "MAXF3") return String(userData.maxFlow[2]); if(var == "MAXP3") return String(userData.maxPower[2]); 
  if(var == "DEAD3") return isnan(userData.deadbandFlow[2]) ? "0.000" : String(userData.deadbandFlow[2], 3);

  if(var == "ADDR4") return String(userData.mbusAddress4); if(var == "MNAME4") return String(userData.slaveName4); if(var == "PROF4") return String(userData.slaveProfile[3]);
  if(var == "MINF4") return String(userData.minFlow[3]); if(var == "MAXF4") return String(userData.maxFlow[3]); if(var == "MAXP4") return String(userData.maxPower[3]); 
  if(var == "DEAD4") return isnan(userData.deadbandFlow[3]) ? "0.000" : String(userData.deadbandFlow[3], 3);

  if(var == "ADDR5") return String(userData.mbusAddress5); if(var == "MNAME5") return String(userData.slaveName5); if(var == "PROF5") return String(userData.slaveProfile[4]);
  if(var == "MINF5") return String(userData.minFlow[4]); if(var == "MAXF5") return String(userData.maxFlow[4]); if(var == "MAXP5") return String(userData.maxPower[4]); 
  if(var == "DEAD5") return isnan(userData.deadbandFlow[4]) ? "0.000" : String(userData.deadbandFlow[4], 3);

  if(var == "SENS_NUM") return String(userData.owSensors);
  if(var == "SENS_INT") return String(userData.sensorInterval / 1000);

  if(var == "SNAME1") return String(userData.sensorName1); if(var == "SOFF1") return String(userData.sensorOffset[0]);
  if(var == "SNAME2") return String(userData.sensorName2); if(var == "SOFF2") return String(userData.sensorOffset[1]);
  if(var == "SNAME3") return String(userData.sensorName3); if(var == "SOFF3") return String(userData.sensorOffset[2]);
  if(var == "SNAME4") return String(userData.sensorName4); if(var == "SOFF4") return String(userData.sensorOffset[3]);
  if(var == "SNAME5") return String(userData.sensorName5); if(var == "SOFF5") return String(userData.sensorOffset[4]);
  if(var == "SNAME6") return String(userData.sensorName6); if(var == "SOFF6") return String(userData.sensorOffset[5]);
  if(var == "SNAME7") return String(userData.sensorName7); if(var == "SOFF7") return String(userData.sensorOffset[6]);

  if(var == "I2C_SEL_0") return userData.i2cMode == 0 ? "selected" : "";
  if(var == "I2C_SEL_1") return userData.i2cMode == 1 ? "selected" : "";
  if(var == "I2C_SEL_2") return userData.i2cMode == 2 ? "selected" : "";
  if(var == "BME_NAME") return String(userData.bmeName);

  if(var == "ESPNOW_EN_0") return userData.espNowEnable == 0 ? "selected" : "";
  if(var == "ESPNOW_EN_1") return userData.espNowEnable == 1 ? "selected" : "";
  if(var == "ESPNOW_MAC") return String(userData.espNowMac);
  
  // --- ESP-NOW IDENTIFIER ---
  if(var == "ESPNOW_SYSID") {
    uint32_t sysId = calculateSystemId(userData.mbusinoName);
    char buf[12];
    sprintf(buf, "0x%08X", sysId); // Format as Hex: 0x8A4F1B3C
    return String(buf);
  }

  if(var == "REPO_URL") return String(projectURL);

  return String();
}

void setupServer(){
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    if (strlen(userData.webPassword) > 0 && String(userData.webPassword) != "NONE") {
      if (!request->authenticate("admin", userData.webPassword)) {
        return request->requestAuthentication();
      }
    }
    request->send(200, "text/html", index_html, processor); 
  });
    
  // --- UNIVERSAL SAVE ROUTINE (POST) ---
  server.on("/saveAll", HTTP_POST, [](AsyncWebServerRequest *request){
    // 1. Security Check
    if (strlen(userData.webPassword) > 0 && String(userData.webPassword) != "NONE") {
      if (!request->authenticate("admin", userData.webPassword)) {
        return request->requestAuthentication();
      }
    }

    settingsChanged = false;
    wlanChanged = false;

    // --- TAB: GENERAL ---
    updateStringIfChanged(request, "name", userData.mbusinoName, sizeof(userData.mbusinoName));
    updateStringIfChanged(request, "webPassword", userData.webPassword, sizeof(userData.webPassword));
    updateStringIfChanged(request, "otaPassword", userData.otaPassword, sizeof(userData.otaPassword));
    updateBoolIfChanged(request, "haAd", userData.haAutodisc);
    updateBoolIfChanged(request, "telegramDebug", userData.telegramDebug);

    // --- TAB: NETWORK ---
    wlanChanged |= updateStringIfChanged(request, "ssid", userData.ssid, sizeof(userData.ssid));
    wlanChanged |= updateStringIfChanged(request, "password", userData.password, sizeof(userData.password));
    wlanChanged |= updateStringIfChanged(request, "ssid2", userData.ssid2, sizeof(userData.ssid2));
    wlanChanged |= updateStringIfChanged(request, "password2", userData.password2, sizeof(userData.password2));
    
    uint8_t oldChan = userData.apChannel;
    updateIntIfChanged(request, "apChannel", userData.apChannel);
    if(oldChan != userData.apChannel) wlanChanged = true;

    updateStringIfChanged(request, "broker", userData.broker, sizeof(userData.broker));
    updateIntIfChanged(request, "mqttPort", userData.mqttPort);
    updateStringIfChanged(request, "mqttUser", userData.mqttUser, sizeof(userData.mqttUser));
    updateStringIfChanged(request, "mqttPswrd", userData.mqttPswrd, sizeof(userData.mqttPswrd));

    updateBoolIfChanged(request, "espNowEnable", userData.espNowEnable);
    updateStringIfChanged(request, "espNowMac", userData.espNowMac, sizeof(userData.espNowMac));

    // --- TAB: M-BUS ---
    updateIntIfChanged(request, "mbusSlaves", userData.mbusSlaves);
    
    // Convert seconds from UI back to ms for ESP
    if(request->hasParam("mbusInterval", true)) {
      uint32_t newVal = request->getParam("mbusInterval", true)->value().toInt() * 1000;
      if(newVal != userData.mbusInterval) { userData.mbusInterval = newVal; settingsChanged = true; }
    }

    // Slave 1
    updateIntIfChanged(request, "mbusAddress1", userData.mbusAddress1);
    updateStringIfChanged(request, "slaveName1", userData.slaveName1, sizeof(userData.slaveName1));
    updateStringIfChanged(request, "profile1", userData.slaveProfile[0], sizeof(userData.slaveProfile[0]));
    updateFloatIfChanged(request, "minFlow1", userData.minFlow[0]);
    updateFloatIfChanged(request, "maxFlow1", userData.maxFlow[0]);
    updateFloatIfChanged(request, "maxPower1", userData.maxPower[0]);
    updateFloatIfChanged(request, "deadFlow1", userData.deadbandFlow[0]);

    // Slave 2
    updateIntIfChanged(request, "mbusAddress2", userData.mbusAddress2);
    updateStringIfChanged(request, "slaveName2", userData.slaveName2, sizeof(userData.slaveName2));
    updateStringIfChanged(request, "profile2", userData.slaveProfile[1], sizeof(userData.slaveProfile[1]));
    updateFloatIfChanged(request, "minFlow2", userData.minFlow[1]);
    updateFloatIfChanged(request, "maxFlow2", userData.maxFlow[1]);
    updateFloatIfChanged(request, "maxPower2", userData.maxPower[1]);
    updateFloatIfChanged(request, "deadFlow2", userData.deadbandFlow[1]);

    // Slave 3
    updateIntIfChanged(request, "mbusAddress3", userData.mbusAddress3);
    updateStringIfChanged(request, "slaveName3", userData.slaveName3, sizeof(userData.slaveName3));
    updateStringIfChanged(request, "profile3", userData.slaveProfile[2], sizeof(userData.slaveProfile[2]));
    updateFloatIfChanged(request, "minFlow3", userData.minFlow[2]);
    updateFloatIfChanged(request, "maxFlow3", userData.maxFlow[2]);
    updateFloatIfChanged(request, "maxPower3", userData.maxPower[2]);
    updateFloatIfChanged(request, "deadFlow3", userData.deadbandFlow[2]);

    // Slave 4
    updateIntIfChanged(request, "mbusAddress4", userData.mbusAddress4);
    updateStringIfChanged(request, "slaveName4", userData.slaveName4, sizeof(userData.slaveName4));
    updateStringIfChanged(request, "profile4", userData.slaveProfile[3], sizeof(userData.slaveProfile[3]));
    updateFloatIfChanged(request, "minFlow4", userData.minFlow[3]);
    updateFloatIfChanged(request, "maxFlow4", userData.maxFlow[3]);
    updateFloatIfChanged(request, "maxPower4", userData.maxPower[3]);
    updateFloatIfChanged(request, "deadFlow4", userData.deadbandFlow[3]);

    // Slave 5
    updateIntIfChanged(request, "mbusAddress5", userData.mbusAddress5);
    updateStringIfChanged(request, "slaveName5", userData.slaveName5, sizeof(userData.slaveName5));
    updateStringIfChanged(request, "profile5", userData.slaveProfile[4], sizeof(userData.slaveProfile[4]));
    updateFloatIfChanged(request, "minFlow5", userData.minFlow[4]);
    updateFloatIfChanged(request, "maxFlow5", userData.maxFlow[4]);
    updateFloatIfChanged(request, "maxPower5", userData.maxPower[4]);
    updateFloatIfChanged(request, "deadFlow5", userData.deadbandFlow[4]);

    // --- TAB: SENSORS ---
    updateIntIfChanged(request, "owSensors", userData.owSensors);
    
    // Convert seconds from UI back to ms for ESP
    if(request->hasParam("sensorInterval", true)) {
      uint32_t newVal = request->getParam("sensorInterval", true)->value().toInt() * 1000;
      if(newVal != userData.sensorInterval) { userData.sensorInterval = newVal; settingsChanged = true; }
    }
    
    updateIntIfChanged(request, "i2cMode", userData.i2cMode);
    updateStringIfChanged(request, "bmeName", userData.bmeName, sizeof(userData.bmeName));

    // Sanity Check: If > 5 Sensors, force disable I2C
    if(userData.owSensors > 5 && userData.i2cMode != 0) {
      userData.i2cMode = 0;
      settingsChanged = true;
    }

    updateStringIfChanged(request, "sensorName1", userData.sensorName1, sizeof(userData.sensorName1));
    updateFloatIfChanged(request, "offset1", userData.sensorOffset[0]);
    updateStringIfChanged(request, "sensorName2", userData.sensorName2, sizeof(userData.sensorName2));
    updateFloatIfChanged(request, "offset2", userData.sensorOffset[1]);
    updateStringIfChanged(request, "sensorName3", userData.sensorName3, sizeof(userData.sensorName3));
    updateFloatIfChanged(request, "offset3", userData.sensorOffset[2]);
    updateStringIfChanged(request, "sensorName4", userData.sensorName4, sizeof(userData.sensorName4));
    updateFloatIfChanged(request, "offset4", userData.sensorOffset[3]);
    updateStringIfChanged(request, "sensorName5", userData.sensorName5, sizeof(userData.sensorName5));
    updateFloatIfChanged(request, "offset5", userData.sensorOffset[4]);
    updateStringIfChanged(request, "sensorName6", userData.sensorName6, sizeof(userData.sensorName6));
    updateFloatIfChanged(request, "offset6", userData.sensorOffset[5]);
    updateStringIfChanged(request, "sensorName7", userData.sensorName7, sizeof(userData.sensorName7));
    updateFloatIfChanged(request, "offset7", userData.sensorOffset[6]);


    // --- EVALUATION & RESPONSE ---
    if (settingsChanged) {
      saveConfig();
      credentialsReceived = true; // Block UI auto-reboot overrides
    }

    if (apMode == true || wlanChanged == true) {
      request->send(200, "text/plain", "WLAN_CHANGED");
    } else if (settingsChanged) {
      request->send(200, "text/plain", "SAVED_AND_CHANGED");
    } else {
      request->send(200, "text/plain", "UNCHANGED");
    }
  });

  // --- AJAX ENDPOINT: SET ADDRESS (For initial setup of M-Bus Slaves) ---
  server.on("/setaddressApi", HTTP_GET, [](AsyncWebServerRequest *request){
    if (strlen(userData.webPassword) > 0 && String(userData.webPassword) != "NONE") {
      if (!request->authenticate("admin", userData.webPassword)) {
        return request->requestAuthentication();
      }
    }
    
    if (request->hasParam("newAddress")) {
      newAddress = request->getParam("newAddress")->value().toInt();
      newAddressReceived = true;
      request->send(200, "text/plain", "Address command sent");
    } else {
      request->send(400, "text/plain", "Bad Request");
    }
  });

  // --- AJAX ENDPOINT: GET PASSWORDS (OBFUSCATED) ---
  server.on("/getPasswords", HTTP_GET, [](AsyncWebServerRequest *request){
    if (strlen(userData.webPassword) > 0 && String(userData.webPassword) != "NONE") {
      if (!request->authenticate("admin", userData.webPassword)) {
        return request->requestAuthentication();
      }
    }
    
    String nonce = "0";
    if (request->hasParam("nonce")) {
      nonce = request->getParam("nonce")->value();
    }
    
    String salt = "MBusinoSecretSalt";
    String key = nonce + String(userData.mbusinoName) + salt;
    
    auto obfuscate = [](String text, String k) -> String {
      if(text.length() == 0) return "";
      String hexOut = "";
      for(unsigned int i = 0; i < text.length(); i++) {
        char x = text[i] ^ k[i % k.length()];
        char buf[3];
        sprintf(buf, "%02X", (unsigned char)x);
        hexOut += buf;
      }
      return hexOut;
    };

    String json = "{";
    json += "\"p1\":\"" + obfuscate(String(userData.password), key) + "\",";
    json += "\"p2\":\"" + obfuscate(String(userData.password2), key) + "\",";
    json += "\"wp\":\"" + obfuscate(String(userData.webPassword), key) + "\",";
    json += "\"op\":\"" + obfuscate(String(userData.otaPassword), key) + "\",";
    json += "\"mp\":\"" + obfuscate(String(userData.mqttPswrd), key) + "\"";
    json += "}";

    request->send(200, "application/json", json);
  });

  // --- AJAX ENDPOINT: LIVE DATA JSON ---
  server.on("/liveData", HTTP_GET, [](AsyncWebServerRequest *request){
    String json = "{";
    
    json += "\"sys\":{";
    json += "\"mInt\":" + String(userData.mbusInterval / 1000) + ",";
    json += "\"sInt\":" + String(userData.sensorInterval / 1000) + ",";
    json += "\"uptime\":\"" + getUptimeString() + "\",";
    json += "\"heap\":" + String(ESP.getFreeHeap() / 1024); // in KB
    json += "},";

    json += "\"offsets\":[";
    for(int i=0; i<7; i++) {
      json += String(userData.sensorOffset[i], 1);
      if(i < 6) json += ",";
    }
    json += "],";
    
    // 1. Onewire Sensors (We use OWwO which already includes the user's offset)
    json += "\"sensors\":[";
    for(int i=0; i<7; i++) {
      json += String(OWwO[i], 2);
      if(i < 6) json += ",";
    }
    json += "],";
    
    // 2. M-Bus Slaves
    json += "\"mbus\":[";
    for(int i=0; i<5; i++) {
      json += "{";
      json += "\"conn\":" + String(liveConnected[i] ? "true" : "false") + ",";
      
      // M-Bus Manufacturer ID Conversion (Hex & ASCII)
      String asciiMan = "GEN";
      char hexMan[7] = "0x0000";
      if (liveManID[i] > 0) {
        asciiMan = decodeManId(liveManID[i]);
        sprintf(hexMan, "0x%04X", liveManID[i]);
      }
      
      json += "\"manAscii\":\"" + asciiMan + "\",";
      json += "\"manHex\":\"" + String(hexMan) + "\",";
      json += "\"fab\":\"" + liveFab[i] + "\",";
      json += "\"err\":\"" + liveError[i] + "\",";
      
      // Live-Sync Limits
      json += "\"minF\":" + String(userData.minFlow[i], 2) + ",";
      json += "\"maxF\":" + String(userData.maxFlow[i], 2) + ",";
      json += "\"maxP\":" + String(userData.maxPower[i], 2) + ",";
      json += "\"dead\":" + String(userData.deadbandFlow[i], 3);
      
      json += "}";
      if(i < 4) json += ",";
    }
    json += "]";
    json += "}";
    
    request->send(200, "application/json", json);
  });

  // --- AJAX ENDPOINT: GET RAW TELEGRAM ---
  server.on("/raw", HTTP_GET, [](AsyncWebServerRequest *request){
    if (request->hasParam("id")) {
      int id = request->getParam("id")->value().toInt();
      if(id >= 0 && id < 5) {
        if(liveTelegram[id].length() > 0) {
          request->send(200, "text/plain", liveTelegram[id]);
        } else {
          request->send(200, "text/plain", "NO_DATA_YET");
        }
        return;
      }
    }
    request->send(400, "text/plain", "Bad Request");
  });

  // --- HELPER: CHECK FILE CREDENTIALS (LINUX STYLE) ---
  auto isSystemFile = [](String fname) -> bool {
    File mf = LittleFS.open("/profiles/.sysfiles", "r");
    if (!mf) return false; 
    
    bool isProtected = false;
    while (mf.available()) {
      String line = mf.readStringUntil('\n');
      line.trim(); 
      if (line == fname) {
        isProtected = true;
        break;
      }
    }
    mf.close();
    return isProtected;
  };

  // --- AJAX ENDPOINT: LIST ALL PROFILES ---
  server.on("/api/profiles", HTTP_GET, [isSystemFile](AsyncWebServerRequest *request){
    // Security: Only logged in users
    if (strlen(userData.webPassword) > 0 && String(userData.webPassword) != "NONE") {
      if (!request->authenticate("admin", userData.webPassword)) {
        return request->requestAuthentication();
      }
    }

    String json = "[";
    bool first = true;

    #if defined(ESP8266)
      Dir dir = LittleFS.openDir("/profiles");
      while (dir.next()) {
        String fname = dir.fileName();
        if (fname.startsWith(".") || dir.isDirectory()) continue; 
        
        File f = LittleFS.open("/profiles/" + fname, "r");
        String pName = "Unknown"; String pMan = "GEN"; 
        
        String pType = isSystemFile(fname) ? "system" : "custom";

        if (f) {
          JsonDocument doc;
          if (!deserializeJson(doc, f)) {
            pName = doc["profileName"] | "Unknown";
            pMan = doc["manufacturerID"] | "GEN";
          }
          f.close();
        }
        
        if (!first) json += ",";
        json += "{\"file\":\"" + fname + "\",\"name\":\"" + pName + "\",\"man\":\"" + pMan + "\",\"type\":\"" + pType + "\"}";
        first = false;
      }
    #elif defined(ESP32)
      File root = LittleFS.open("/profiles");
      if (root && root.isDirectory()) {
        File file = root.openNextFile();
        while (file) {
          String fname = String(file.name());
          if (fname.startsWith("/profiles/")) fname = fname.substring(10); 
          
          if (fname.startsWith(".") || file.isDirectory()) { file = root.openNextFile(); continue; } 
          
          File f = LittleFS.open("/profiles/" + fname, "r");
          String pName = "Unknown"; String pMan = "GEN"; 
          
          String pType = isSystemFile(fname) ? "system" : "custom";

          if (f) {
            JsonDocument doc;
            if (!deserializeJson(doc, f)) {
              pName = doc["profileName"] | "Unknown";
              pMan = doc["manufacturerID"] | "GEN";
            }
            f.close();
          }
          
          if (!first) json += ",";
          json += "{\"file\":\"" + fname + "\",\"name\":\"" + pName + "\",\"man\":\"" + pMan + "\",\"type\":\"" + pType + "\"}";
          first = false;
          file = root.openNextFile();
        }
      }
    #endif

    json += "]";
    request->send(200, "application/json", json);
  });

  // Allow direct Download of Profiles via Web-UI
  server.serveStatic("/profiles", LittleFS, "/profiles");

  // --- AJAX ENDPOINT: UPLOAD CUSTOM PROFILE ---
  server.on("/api/uploadProfile", HTTP_POST, [](AsyncWebServerRequest *request) {
    request->send(200);
  }, [isSystemFile](AsyncWebServerRequest *request, String filename, size_t index, uint8_t *data, size_t len, bool final) {
    if (!index) {
      // Safety-Checks for the first Data Packet
      bool allowed = true;
      if (!filename.endsWith(".json")) allowed = false;
      if (filename.length() > 31) allowed = false; 
      if (isSystemFile(filename)) allowed = false; 
      
      if (allowed) {
        request->_tempFile = LittleFS.open("/profiles/" + filename, "w");
      }
    }
    
    // Write Data
    if (request->_tempFile) {
      if (request->_tempFile.write(data, len) != len) {
        // Error writing
      }
    }

    // Done
    if (final && request->_tempFile) {
      request->_tempFile.close();
    }
  });

  // --- AJAX ENDPOINT: DELETE PROFILE ---
  server.on("/api/deleteProfile", HTTP_GET, [isSystemFile](AsyncWebServerRequest *request){
    if (strlen(userData.webPassword) > 0 && String(userData.webPassword) != "NONE") {
      if (!request->authenticate("admin", userData.webPassword)) return request->requestAuthentication();
    }
    
    if (request->hasParam("file")) {
      String fname = request->getParam("file")->value();
      
      // SECURITY Check against Manifest
      if (isSystemFile(fname)) {
        request->send(403, "text/plain", "System profiles are protected by manifest and cannot be deleted!");
      } else {
        if (LittleFS.remove("/profiles/" + fname)) {
          request->send(200, "text/plain", "Deleted");
        } else {
          request->send(500, "text/plain", "File not found or IO error");
        }
      }
    } else {
      request->send(400, "text/plain", "Bad Request");
    }
  });

  // --- ENDPOINT: DOWNLOAD CONFIG JSON ---
  server.on("/backup.json", HTTP_GET, [](AsyncWebServerRequest *request){
    if (strlen(userData.webPassword) > 0 && String(userData.webPassword) != "NONE") {
      if (!request->authenticate("admin", userData.webPassword)) {
        return request->requestAuthentication();
      }
    }
    if (LittleFS.exists("/config.json")) {
      request->send(LittleFS, "/config.json", "application/json", true); // 'true' forces download
    } else {
      request->send(404, "text/plain", "Config not found on LittleFS.");
    }
  });

  // HTML Pages
  server.on("/setaddress", HTTP_GET, [](AsyncWebServerRequest *request){
    if (strlen(userData.webPassword) > 0 && String(userData.webPassword) != "NONE") {
        if (!request->authenticate("admin", userData.webPassword)) {
          return request->requestAuthentication();
        }
      }  
    request->send(200, "text/html", setAddress_html);
  });

  server.on("/reboot", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(200, "text/html", "<!doctype html><html lang='en'><body style='background-color:#438287; color:#fff; text-align:center; padding-top:50px; font-family:sans-serif;'><h1>MBusino reboots...</h1><p>Please wait 10 seconds.</p><script>setTimeout(function(){ window.location.href = '/'; }, 10000);</script></body></html>");
    timerReboot = millis() - 500; 
    waitForRestart = true;
  });

}

void onRequest(AsyncWebServerRequest *request){
  request->send(404);
}

class CaptiveRequestHandler : public AsyncWebHandler {
public:
  CaptiveRequestHandler() {}
  virtual ~CaptiveRequestHandler() {}

  bool canHandle(AsyncWebServerRequest *request) const override {
    return true;
  }

  void handleRequest(AsyncWebServerRequest *request) {
    request->send(200, "text/html", index_html, processor); 
  }
};
