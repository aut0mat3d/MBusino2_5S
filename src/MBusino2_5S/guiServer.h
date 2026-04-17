AsyncWebServer server(80);
AsyncWebSocket ws("/ws"); // access at ws://[esp ip]/ws
AsyncEventSource events("/events"); // event source (Server-Sent events)

// --- TEMPLATE PROCESSOR ---
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
    request->send_P(200, "text/html", index_html, processor); 
  });
    
  server.on("/get", HTTP_GET, [] (AsyncWebServerRequest *request) {
      struct settings oldData = userData; // 1. Stora actual Data
      credentialsReceived = false;        // 2. Block automatic Save

      String inputMessage;
      String inputParam;
      bool wlanChanged = false;

      if (request->hasParam("ssid")) {
        inputMessage = request->getParam("ssid")->value();
        inputParam = "ssid";
        if(inputMessage != NULL){
          if(inputMessage != String(userData.ssid)) wlanChanged = true;
          inputMessage.toCharArray(userData.ssid, sizeof(userData.ssid));
          credentialsReceived = true;
        }
      }

      if (request->hasParam("password")) {
        inputMessage = request->getParam("password")->value();
        if(inputMessage != NULL && inputMessage != "****"){
          wlanChanged = true;
          inputMessage.toCharArray(userData.password, sizeof(userData.password));
          credentialsReceived = true;
        }
      }

      if (request->hasParam("ssid2")) {
        inputMessage = request->getParam("ssid2")->value();
        if(inputMessage != NULL){
          if(inputMessage != String(userData.ssid2)) wlanChanged = true;
          inputMessage.toCharArray(userData.ssid2, sizeof(userData.ssid2));
          credentialsReceived = true;
        }
      }

      if (request->hasParam("password2")) {
        inputMessage = request->getParam("password2")->value();
        if(inputMessage != NULL && inputMessage != "****"){
          wlanChanged = true;
          inputMessage.toCharArray(userData.password2, sizeof(userData.password2));
          credentialsReceived = true;
        }
      }

      if (request->hasParam("apChannel")) {
        inputMessage = request->getParam("apChannel")->value();
        if(inputMessage != NULL){
          uint8_t newChan = inputMessage.toInt();
          if(newChan >= 1 && newChan <= 13) {
            if(userData.apChannel != newChan) wlanChanged = true;
            userData.apChannel = newChan;
            credentialsReceived = true;
          }
        }
      }

      if (request->hasParam("webPassword")) {
        inputMessage = request->getParam("webPassword")->value();
        if(inputMessage.length() > 0 && inputMessage != "****"){
          if(inputMessage == "none" || inputMessage == "NONE"){
            strcpy(userData.webPassword, "NONE");
          } else {
            inputMessage.toCharArray(userData.webPassword, sizeof(userData.webPassword));
          }
          credentialsReceived = true;
        }
      }

      if (request->hasParam("otaPassword")) {
        inputMessage = request->getParam("otaPassword")->value();
        if(inputMessage != NULL && inputMessage != "****"){
          inputMessage.toCharArray(userData.otaPassword, sizeof(userData.otaPassword));
          credentialsReceived = true;
        }
      }

      if (request->hasParam("name")) {
        inputMessage = request->getParam("name")->value();
        inputParam = "name";
        if(inputMessage != NULL){
          inputMessage.toCharArray(userData.mbusinoName, sizeof(userData.mbusinoName));
          credentialsReceived = true;
        }
      }

      if (request->hasParam("broker")) {
        inputMessage = request->getParam("broker")->value();
        inputParam = "broker";
        if(inputMessage != NULL){
          inputMessage.toCharArray(userData.broker, sizeof(userData.broker));
          credentialsReceived = true;
        }
      }

      if (request->hasParam("mqttPort")) {
        inputMessage = request->getParam("mqttPort")->value();
        inputParam = "mqttPort";
        if(inputMessage != NULL){
          userData.mqttPort = inputMessage.toInt();
          credentialsReceived = true;
        }
      }

      if (request->hasParam("owSensors")) {
        inputMessage = request->getParam("owSensors")->value();
        if(inputMessage != NULL){
          userData.owSensors = inputMessage.toInt();
          credentialsReceived = true;
        }
      }

      if (request->hasParam("i2cMode")) {
        String inputMessage = request->getParam("i2cMode")->value();
        userData.i2cMode = inputMessage.toInt();
        credentialsReceived = true;
      }

      // Sanity Check: If the User choosed >5 OneWire Sensors, we must set userData.i2cMode to 0!
      if(userData.owSensors > 5) {
        userData.i2cMode = 0; // Zwangsabschaltung des I2C!
      }
      
      if (request->hasParam("haAd")) {
        userData.haAutodisc = (request->getParam("haAd")->value() == "1");
        credentialsReceived = true;
      }
      
      if (request->hasParam("telegramDebug")) {
        userData.telegramDebug = (request->getParam("telegramDebug")->value() == "1");
        credentialsReceived = true;
      }

      if (request->hasParam("sensorInterval")) {
        inputMessage = request->getParam("sensorInterval")->value();
        inputParam = "sensorInterval";
        if(inputMessage != NULL){
          userData.sensorInterval = inputMessage.toInt()  * 1000;
          credentialsReceived = true;
          }
      }

      if (request->hasParam("mbusInterval")) {
        inputMessage = request->getParam("mbusInterval")->value();
        inputParam = "mbusInterval";
          if(inputMessage != NULL){
            userData.mbusInterval = inputMessage.toInt() * 1000;
            credentialsReceived = true;
          }
      }

      if (request->hasParam("mqttUser")) {
        inputMessage = request->getParam("mqttUser")->value();
        inputParam = "mqttUser";
        if(inputMessage != NULL){
          inputMessage.toCharArray(userData.mqttUser, sizeof(userData.mqttUser));
          credentialsReceived = true;
        }
      }

      if (request->hasParam("mqttPswrd")) {
        inputMessage = request->getParam("mqttPswrd")->value();
        inputParam = "mqttPswrd";
        if(inputMessage != NULL){
          inputMessage.toCharArray(userData.mqttPswrd, sizeof(userData.mqttPswrd));
          credentialsReceived = true;
        }
      }

      if (request->hasParam("espNowEnable")) {
        bool newVal = (request->getParam("espNowEnable")->value() == "1");
        if (userData.espNowEnable != newVal) {
          userData.espNowEnable = newVal;
          credentialsReceived = true;
        }
      }

      if (request->hasParam("espNowMac")) {
        inputMessage = request->getParam("espNowMac")->value();
        if (inputMessage != NULL) {
          inputMessage.toCharArray(userData.espNowMac, sizeof(userData.espNowMac));
          credentialsReceived = true;
        }
      }

      if (request->hasParam("mbusSlaves")) {
        inputMessage = request->getParam("mbusSlaves")->value();
        inputParam = "mbusSlaves";
        if(inputMessage != NULL){
          userData.mbusSlaves = inputMessage.toInt();
          credentialsReceived = true;
        }
      }   

      if (request->hasParam("mbusAddress1")) {
        inputMessage = request->getParam("mbusAddress1")->value();
        inputParam = "mbusAddress1";
        if(inputMessage != NULL){
          userData.mbusAddress1 = inputMessage.toInt();
          credentialsReceived = true;
        }
      }  

      if (request->hasParam("mbusAddress2")) {
        inputMessage = request->getParam("mbusAddress2")->value();
        inputParam = "mbusAddress2";
        if(inputMessage != NULL){
          userData.mbusAddress2 = inputMessage.toInt();
          credentialsReceived = true;
        }
      } 

      if (request->hasParam("mbusAddress3")) {
        inputMessage = request->getParam("mbusAddress3")->value();
        inputParam = "mbusAddress3";
        if(inputMessage != NULL){
          userData.mbusAddress3 = inputMessage.toInt();
          credentialsReceived = true;
        }
      }

      if (request->hasParam("mbusAddress4")) {
        inputMessage = request->getParam("mbusAddress4")->value();
        inputParam = "mbusAddress4";
        if(inputMessage != NULL){
          userData.mbusAddress4 = inputMessage.toInt();
          credentialsReceived = true;
        }
      } 

      if (request->hasParam("mbusAddress5")) {
        inputMessage = request->getParam("mbusAddress5")->value();
        inputParam = "mbusAddress5";
        if(inputMessage != NULL){
          userData.mbusAddress5 = inputMessage.toInt();
          credentialsReceived = true;
        }
      }

      // --- SLAVE NAMES ---
      if (request->hasParam("slaveName1")) {
        request->getParam("slaveName1")->value().toCharArray(userData.slaveName1, sizeof(userData.slaveName1)); credentialsReceived = true;
      }
      if (request->hasParam("slaveName2")) {
        request->getParam("slaveName2")->value().toCharArray(userData.slaveName2, sizeof(userData.slaveName2)); credentialsReceived = true;
      }
      if (request->hasParam("slaveName3")) {
        request->getParam("slaveName3")->value().toCharArray(userData.slaveName3, sizeof(userData.slaveName3)); credentialsReceived = true;
      }
      if (request->hasParam("slaveName4")) {
        request->getParam("slaveName4")->value().toCharArray(userData.slaveName4, sizeof(userData.slaveName4)); credentialsReceived = true;
      }
      if (request->hasParam("slaveName5")) {
        request->getParam("slaveName5")->value().toCharArray(userData.slaveName5, sizeof(userData.slaveName5)); credentialsReceived = true;
      }

      // --- SLAVE PROFILES ---
      if (request->hasParam("profile1")) { 
        request->getParam("profile1")->value().toCharArray(userData.slaveProfile[0], 32); credentialsReceived = true; 
        }
      if (request->hasParam("profile2")) { 
        request->getParam("profile2")->value().toCharArray(userData.slaveProfile[1], 32); credentialsReceived = true; 
        }
      if (request->hasParam("profile3")) { 
        request->getParam("profile3")->value().toCharArray(userData.slaveProfile[2], 32); credentialsReceived = true; 
        }
      if (request->hasParam("profile4")) { 
        request->getParam("profile4")->value().toCharArray(userData.slaveProfile[3], 32); credentialsReceived = true; 
        }
      if (request->hasParam("profile5")) { 
        request->getParam("profile5")->value().toCharArray(userData.slaveProfile[4], 32); credentialsReceived = true; 
        }

      // --- SENSOR NAMES ---
      if (request->hasParam("sensorName1")) {
        request->getParam("sensorName1")->value().toCharArray(userData.sensorName1, sizeof(userData.sensorName1)); credentialsReceived = true;
      }
      if (request->hasParam("sensorName2")) {
        request->getParam("sensorName2")->value().toCharArray(userData.sensorName2, sizeof(userData.sensorName2)); credentialsReceived = true;
      }
      if (request->hasParam("sensorName3")) {
        request->getParam("sensorName3")->value().toCharArray(userData.sensorName3, sizeof(userData.sensorName3)); credentialsReceived = true;
      }
      if (request->hasParam("sensorName4")) {
        request->getParam("sensorName4")->value().toCharArray(userData.sensorName4, sizeof(userData.sensorName4)); credentialsReceived = true;
      }
      if (request->hasParam("sensorName5")) {
        request->getParam("sensorName5")->value().toCharArray(userData.sensorName5, sizeof(userData.sensorName5)); credentialsReceived = true;
      }
      if (request->hasParam("sensorName6")) {
        request->getParam("sensorName6")->value().toCharArray(userData.sensorName6, sizeof(userData.sensorName6)); credentialsReceived = true;
      }
      if (request->hasParam("sensorName7")) {
        request->getParam("sensorName7")->value().toCharArray(userData.sensorName7, sizeof(userData.sensorName7)); credentialsReceived = true;
      }
      
      // --- BME NAME ---
      if (request->hasParam("bmeName")) {
        request->getParam("bmeName")->value().toCharArray(userData.bmeName, sizeof(userData.bmeName)); credentialsReceived = true;
      }

      // --- LIMITS ---
      for(int i=0; i<5; i++) {
        String pMin = "minFlow" + String(i+1);
        if (request->hasParam(pMin)) { userData.minFlow[i] = request->getParam(pMin)->value().toFloat(); credentialsReceived = true; }
        
        String pMax = "maxFlow" + String(i+1);
        if (request->hasParam(pMax)) { userData.maxFlow[i] = request->getParam(pMax)->value().toFloat(); credentialsReceived = true; }
        
        String pPow = "maxPower" + String(i+1);
        if (request->hasParam(pPow)) { userData.maxPower[i] = request->getParam(pPow)->value().toFloat(); credentialsReceived = true; }
        
        // --- DEADBAND SAVING ---
        String pDead = "deadFlow" + String(i+1);
        if (request->hasParam(pDead)) { userData.deadbandFlow[i] = request->getParam(pDead)->value().toFloat(); credentialsReceived = true; }
      }
      
      // --- SENSOR OFFSETS ---
      for(int i=0; i<7; i++) {
        String pOff = "offset" + String(i+1);
        if (request->hasParam(pOff)) { userData.sensorOffset[i] = request->getParam(pOff)->value().toFloat(); credentialsReceived = true; }
      }

      if (request->hasParam("newAddress")) {
        inputMessage = request->getParam("newAddress")->value();       
        inputParam = "newAddress";
        if(inputMessage != NULL){
          newAddress = inputMessage.toInt();
          newAddressReceived = true;
        }
      }     

      if (strlen(userData.webPassword) > 0 && String(userData.webPassword) != "NONE") {
        if (!request->authenticate("admin", userData.webPassword)) {
          return request->requestAuthentication();
        }
      }
      // --- SMART SAVE-PAGE LOGIC (AJAX) ---
      // 3. Compare stored Data with actual Data bytewisse
      bool settingsChanged = (memcmp(&oldData, &userData, sizeof(settings)) != 0);
      
      if (settingsChanged) {
        credentialsReceived = true; // 4. Changes detected
      }

      if (apMode == true || wlanChanged == true) {
        request->send(200, "text/plain", "AP");
      } else if (settingsChanged) {
        request->send(200, "text/plain", "CHANGED");
      } else {
        request->send(200, "text/plain", "UNCHANGED");
      }
  });

  // --- AJAX ENDPOINT: GET PASSWORDS (OBFUSCATED) ---
  server.on("/getPasswords", HTTP_GET, [](AsyncWebServerRequest *request){
    // Security Check: Only logged in users can request this!
    if (strlen(userData.webPassword) > 0 && String(userData.webPassword) != "NONE") {
      if (!request->authenticate("admin", userData.webPassword)) {
        return request->requestAuthentication();
      }
    }
    
    String nonce = "0";
    if (request->hasParam("nonce")) {
      nonce = request->getParam("nonce")->value();
    }
    
    // Build the XOR Key
    String salt = "MBusinoSecretSalt";
    String key = nonce + String(userData.mbusinoName) + salt;
    
    // Lambda Function for XOR and HEX encoding
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

    // Pack everything into a JSON
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
    
    // --- NEW: System Intervals (Convert ms back to seconds for UI) ---
    json += "\"sys\":{";
    json += "\"mInt\":" + String(userData.mbusInterval / 1000) + ",";
    json += "\"sInt\":" + String(userData.sensorInterval / 1000) + ",";
    json += "\"uptime\":\"" + getUptimeString() + "\",";
    json += "\"heap\":" + String(ESP.getFreeHeap() / 1024); // in KB
    json += "},";

    // --- NEW: Sensor Offsets ---
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
    if (!mf) return false; // Wenn das Manifest fehlt, ist kein Profil geschützt
    
    bool isProtected = false;
    while (mf.available()) {
      String line = mf.readStringUntil('\n');
      line.trim(); // Bereinigt eventuelle \r oder Leerzeichen
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
        if (fname.startsWith(".") || dir.isDirectory()) continue; // Ignoriert .init_done UND .sysfiles
        
        // Open JSON briefly to get Name and ID
        File f = LittleFS.open("/profiles/" + fname, "r");
        String pName = "Unknown"; String pMan = "GEN"; 
        
        // --- Die absolute Wahrheit kommt jetzt aus dem Manifest, NICHT aus dem JSON! ---
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
          if (fname.startsWith("/profiles/")) fname = fname.substring(10); // Remove path if present
          
          if (fname.startsWith(".") || file.isDirectory()) { file = root.openNextFile(); continue; } // Ignoriert .init_done UND .sysfiles
          
          File f = LittleFS.open("/profiles/" + fname, "r");
          String pName = "Unknown"; String pMan = "GEN"; 
          
          // --- Die absolute Wahrheit kommt jetzt aus dem Manifest, NICHT aus dem JSON! ---
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
      // 1. Sicherheits-Checks beim ersten Datenpaket
      bool allowed = true;
      if (!filename.endsWith(".json")) allowed = false;
      if (filename.length() > 31) allowed = false; // Muss in char slaveProfile[32] passen
      if (isSystemFile(filename)) allowed = false; // System-Profile dürfen nicht überschrieben werden
      
      if (allowed) {
        request->_tempFile = LittleFS.open("/profiles/" + filename, "w");
        #ifdef DEBUG
        DEBUG_PRINT("Upload started: " + filename);
        #endif
      } else {
        #ifdef DEBUG
        DEBUG_PRINT("Upload REJECTED: " + filename);
        #endif
      }
    }
    
    // 2. Daten schreiben
    if (request->_tempFile) {
      if (request->_tempFile.write(data, len) != len) {
        // Fehler beim Schreiben
      }
    }

    // 3. Abschluss
    if (final && request->_tempFile) {
      request->_tempFile.close();
      #ifdef DEBUG
      DEBUG_PRINT("Upload complete.");
      #endif
    }
  });

  // --- AJAX ENDPOINT: DELETE PROFILE ---
  server.on("/api/deleteProfile", HTTP_GET, [isSystemFile](AsyncWebServerRequest *request){
    if (strlen(userData.webPassword) > 0 && String(userData.webPassword) != "NONE") {
      if (!request->authenticate("admin", userData.webPassword)) return request->requestAuthentication();
    }
    
    if (request->hasParam("file")) {
      String fname = request->getParam("file")->value();
      
      // SECURITY: Wir fragen das Manifest, ob die Datei Lösch-Rechte hat!
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
  server.on("/setaddress", HTTP_GET, [](AsyncWebServerRequest *request){
    if (strlen(userData.webPassword) > 0 && String(userData.webPassword) != "NONE") {
        if (!request->authenticate("admin", userData.webPassword)) {
          return request->requestAuthentication();
        }
      }  
    request->send_P(200, "text/html", setAddress_html);
  });

  server.on("/reboot", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(200, "text/html", "<!doctype html><html lang='en'><body style='background-color:#438287; color:#fff; text-align:center; padding-top:50px; font-family:sans-serif;'><h1>MBusino reboots...</h1><p>Please wait 10 seconds.</p><script>setTimeout(function(){ window.location.href = '/'; }, 10000);</script></body></html>");
    // Wir setzen den Reboot-Timer, damit der ESP noch Zeit hat, die Webseite an den Browser zu schicken!
    timerReboot = millis() - 500; 
    waitForRestart = true;
  });

}

void onRequest(AsyncWebServerRequest *request){
  //Handle Unknown Request
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
    request->send_P(200, "text/html", index_html, processor); 
  }
};
