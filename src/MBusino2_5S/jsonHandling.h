#ifndef JSON_HANDLING_H
#define JSON_HANDLING_H

#include <ArduinoJson.h>
#include <LittleFS.h>

// --- HELPER: Decode 16-bit M-Bus Manufacturer ID to 3-letter ASCII ---
String decodeManId(uint16_t manId) {
  if (manId == 0) return "GEN";
  char str[4];
  str[0] = ((manId >> 10) & 0x001F) + 64;
  str[1] = ((manId >> 5)  & 0x001F) + 64;
  str[2] = (manId & 0x001F) + 64;
  str[3] = 0; // Null-Terminator
  return String(str);
}

// --- JSON CONFIG HANDLER (STAGING) ---
bool saveConfig() {
  JsonDocument doc;
  // --- 0. META HEADER ---
  JsonObject meta = doc["_meta"].to<JsonObject>();
  meta["config_version"] = 200; // V2.0.0
  meta["firmware"] = MBUSINO_VERSION;

  // --- 1. SYSTEM SETTINGS ---
  JsonObject sys = doc["system"].to<JsonObject>();
  sys["deviceName"] = userData.mbusinoName;
  sys["webPassword"] = userData.webPassword;
  sys["otaPassword"] = userData.otaPassword;
  sys["haAutodisc"] = userData.haAutodisc;
  sys["telegramDebug"] = userData.telegramDebug;

  // --- 2. NETWORK SETTINGS ---
  JsonObject net = doc["network"].to<JsonObject>();
  JsonObject wifi = net["wifi"].to<JsonObject>();
  wifi["ssid1"] = userData.ssid;
  wifi["pwd1"] = userData.password;
  wifi["ssid2"] = userData.ssid2;
  wifi["pwd2"] = userData.password2;
  wifi["apChannel"] = userData.apChannel;
  
  JsonObject mqtt = net["mqtt"].to<JsonObject>();
  mqtt["broker"] = userData.broker;
  mqtt["port"] = userData.mqttPort;
  mqtt["user"] = userData.mqttUser;
  mqtt["pwd"] = userData.mqttPswrd;

  // --- ESP-NOW SETTINGS ---
  JsonObject espnow = net["espnow"].to<JsonObject>();
  espnow["enable"] = userData.espNowEnable;
  espnow["mac"] = userData.espNowMac;

  // --- 3. M-BUS CONFIGURATION ---
  JsonObject mbus = doc["mbus"].to<JsonObject>();
  mbus["activeSlaves"] = userData.mbusSlaves;
  mbus["pollInterval_ms"] = userData.mbusInterval;

  JsonArray slaves = mbus["slaves"].to<JsonArray>();
  for(int i = 0; i < 5; i++) {
    JsonObject s = slaves.add<JsonObject>();
    s["address"] = (i==0) ? userData.mbusAddress1 : (i==1) ? userData.mbusAddress2 : (i==2) ? userData.mbusAddress3 : (i==3) ? userData.mbusAddress4 : userData.mbusAddress5;
    s["name"] = (i==0) ? userData.slaveName1 : (i==1) ? userData.slaveName2 : (i==2) ? userData.slaveName3 : (i==3) ? userData.slaveName4 : userData.slaveName5;
    s["minFlow"] = userData.minFlow[i];
    s["maxFlow"] = userData.maxFlow[i];
    s["maxPower"] = userData.maxPower[i];
    s["deadband"] = userData.deadbandFlow[i];
    s["profile"] = userData.slaveProfile[i];
  }

  // --- 4. SENSOR CONFIGURATION ---
  JsonObject sens = doc["sensors"].to<JsonObject>();
  sens["activeOnewire"] = userData.owSensors;
  sens["pollInterval_ms"] = userData.sensorInterval;
  sens["i2cMode"] = userData.i2cMode;
  sens["bmeName"] = userData.bmeName;
  
  JsonArray ow = sens["onewire"].to<JsonArray>();
  for(int i = 0; i < 7; i++) {
    JsonObject o = ow.add<JsonObject>();
    o["name"] = (i==0) ? userData.sensorName1 : (i==1) ? userData.sensorName2 : (i==2) ? userData.sensorName3 : (i==3) ? userData.sensorName4 : (i==4) ? userData.sensorName5 : (i==5) ? userData.sensorName6 : userData.sensorName7;
    o["offset"] = userData.sensorOffset[i];
  }

  // --- WRITE TO FILE ---
  File configFile = LittleFS.open("/config.json", "w");
  if (!configFile) {
    #ifdef DEBUG
    DEBUG_PRINT("Failed to open config file for writing");
    #endif
    return false;
  }
  
  serializeJsonPretty(doc, configFile); 
  configFile.close();
  return true;
}

// --- JSON CONFIG LOADER ---
bool loadConfig() {
  File configFile = LittleFS.open("/config.json", "r");
  if (!configFile) return false; 

  JsonDocument doc;
  DeserializationError error = deserializeJson(doc, configFile);
  configFile.close();

  if (error) {
    #ifdef DEBUG
    DEBUG_PRINT("JSON Parse Error. File Corrupted.");
    #endif
    return false; 
  }

  // --- 1. SYSTEM SETTINGS ---
  JsonObject sys = doc["system"];
  strlcpy(userData.mbusinoName, sys["deviceName"] | "MBusino", sizeof(userData.mbusinoName));
  strlcpy(userData.webPassword, sys["webPassword"] | "", sizeof(userData.webPassword));
  strlcpy(userData.otaPassword, sys["otaPassword"] | "mbusino", sizeof(userData.otaPassword));
  userData.haAutodisc = sys["haAutodisc"] | true;
  userData.telegramDebug = sys["telegramDebug"] | false;

  // --- 2. NETWORK SETTINGS ---
  JsonObject net = doc["network"];
  strlcpy(userData.ssid, net["wifi"]["ssid1"] | "SSID", sizeof(userData.ssid));
  strlcpy(userData.password, net["wifi"]["pwd1"] | "Password", sizeof(userData.password));
  strlcpy(userData.ssid2, net["wifi"]["ssid2"] | "", sizeof(userData.ssid2));
  strlcpy(userData.password2, net["wifi"]["pwd2"] | "", sizeof(userData.password2));
  userData.apChannel = net["wifi"]["apChannel"] | 1;
  strlcpy(userData.broker, net["mqtt"]["broker"] | "192.168.1.8", sizeof(userData.broker));
  userData.mqttPort = net["mqtt"]["port"] | 1883;
  strlcpy(userData.mqttUser, net["mqtt"]["user"] | "mqttUser", sizeof(userData.mqttUser));
  strlcpy(userData.mqttPswrd, net["mqtt"]["pwd"] | "mqttPasword", sizeof(userData.mqttPswrd));

  // --- 2.1 ESP-NOW SETTINGS ---
  userData.espNowEnable = net["espnow"]["enable"] | false;
  strlcpy(userData.espNowMac, net["espnow"]["mac"] | "FF:FF:FF:FF:FF:FF", sizeof(userData.espNowMac));

  // --- 3. M-BUS CONFIGURATION ---
  JsonObject mbus = doc["mbus"];
  userData.mbusSlaves = mbus["activeSlaves"] | 1;
  userData.mbusInterval = mbus["pollInterval_ms"] | 120000;
  
  JsonArray slaves = mbus["slaves"];
  for (int i = 0; i < 5 && i < slaves.size(); i++) {
    JsonObject s = slaves[i];
    uint8_t addr = s["address"] | 0;
    const char* sName = s["name"] | "";
    
    if(i==0) { userData.mbusAddress1 = addr; strlcpy(userData.slaveName1, sName, 31); }
    else if(i==1) { userData.mbusAddress2 = addr; strlcpy(userData.slaveName2, sName, 31); }
    else if(i==2) { userData.mbusAddress3 = addr; strlcpy(userData.slaveName3, sName, 31); }
    else if(i==3) { userData.mbusAddress4 = addr; strlcpy(userData.slaveName4, sName, 31); }
    else { userData.mbusAddress5 = addr; strlcpy(userData.slaveName5, sName, 31); }

    userData.minFlow[i] = s["minFlow"] | 0.0;
    userData.maxFlow[i] = s["maxFlow"] | 999.0;
    userData.maxPower[i] = s["maxPower"] | 999.0;
    userData.deadbandFlow[i] = s["deadband"] | 0.0;
    strlcpy(userData.slaveProfile[i], s["profile"] | "", 32);
  }

  // --- 4. SENSOR CONFIGURATION ---
  JsonObject sens = doc["sensors"];
  userData.owSensors = sens["activeOnewire"] | 0;
  userData.sensorInterval = sens["pollInterval_ms"] | 5000;
  userData.i2cMode = sens["i2cMode"] | 0;
  strlcpy(userData.bmeName, sens["bmeName"] | "", sizeof(userData.bmeName));

  JsonArray ow = sens["onewire"];
  for (int i = 0; i < 7 && i < ow.size(); i++) {
    JsonObject o = ow[i];
    const char* oName = o["name"] | "";
    
    if(i==0) strlcpy(userData.sensorName1, oName, 31);
    else if(i==1) strlcpy(userData.sensorName2, oName, 31);
    else if(i==2) strlcpy(userData.sensorName3, oName, 31);
    else if(i==3) strlcpy(userData.sensorName4, oName, 31);
    else if(i==4) strlcpy(userData.sensorName5, oName, 31);
    else if(i==5) strlcpy(userData.sensorName6, oName, 31);
    else strlcpy(userData.sensorName7, oName, 31);

    userData.sensorOffset[i] = o["offset"] | 0.0;
  }
  return true;
}

// --- PROFILE TO RAM LOADER ---
bool loadProfileToRAM(uint8_t idx, const char* filename) {
  if (idx >= 5) return false;
  slaveProfiles[idx].isValid = false; 

  if (strlen(filename) == 0) return false;

  String path = "/profiles/" + String(filename);
  File f = LittleFS.open(path, "r");
  if (!f) return false;

  JsonDocument doc;
  DeserializationError error = deserializeJson(doc, f);
  f.close();

  if (error) return false;

  // --- Start parsing into RAM ---
  slaveProfiles[idx].isValid = true;
  strlcpy(slaveProfiles[idx].profileName, doc["profileName"] | "Unknown", 25);
  strlcpy(slaveProfiles[idx].manufacturerID, doc["manufacturerID"] | "GEN", 4);
  slaveProfiles[idx].applyEngelmannBugfix = doc["applyEngelmannBugfix"] | false;

  // Ignore Records
  JsonArray ignoreArr = doc["ignoreRecords"];
  slaveProfiles[idx].ignoreCount = 0;
  for (uint8_t i = 0; i < ignoreArr.size() && i < 35; i++) {
    slaveProfiles[idx].ignoreRecords[i] = ignoreArr[i];
    slaveProfiles[idx].ignoreCount++;
  }
  if (slaveProfiles[idx].ignoreCount < 35) {
    slaveProfiles[idx].ignoreRecords[slaveProfiles[idx].ignoreCount] = 0;
  }

  // Custom States
  JsonArray stateArr = doc["customStates"];
  slaveProfiles[idx].customStateCount = 0;
  for (uint8_t i = 0; i < stateArr.size() && i < 3; i++) {
    strlcpy(slaveProfiles[idx].customStates[i].haName, stateArr[i]["haName"] | "", 21);
    slaveProfiles[idx].customStates[i].bitMask = stateArr[i]["bitMask"] | 0;
    slaveProfiles[idx].customStates[i].bitShift = stateArr[i]["bitShift"] | 0;
    
    JsonArray sNames = stateArr[i]["states"];
    for (uint8_t j = 0; j < 8; j++) {
      if (j < sNames.size()) {
        strlcpy(slaveProfiles[idx].customStates[i].states[j], sNames[j], 16);
      } else {
        strlcpy(slaveProfiles[idx].customStates[i].states[j], "Unknown", 16);
      }
    }
    slaveProfiles[idx].customStateCount++;
  }

  return true;
}

void autoAssignProfile(uint8_t idx, uint16_t manId) {
  if (idx >= 5 || manId == 0) return;
  
  String targetMan = decodeManId(manId); // z.B. "LUG"
  String foundFile = "";
  uint8_t matchCount = 0;

  #if defined(ESP8266)
    Dir dir = LittleFS.openDir("/profiles");
    while (dir.next()) {
      String fname = dir.fileName();
      if (fname == ".init_done" || dir.isDirectory()) continue;
  #elif defined(ESP32)
    File root = LittleFS.open("/profiles");
    File file = root.openNextFile();
    while (file) {
      String fname = String(file.name());
      if (fname.startsWith("/profiles/")) fname = fname.substring(10);
      if (fname == ".init_done" || file.isDirectory()) { file = root.openNextFile(); continue; }
  #endif

      // Check this file's manufacturerID
      File f = LittleFS.open("/profiles/" + fname, "r");
      if (f) {
        JsonDocument doc;
        if (!deserializeJson(doc, f)) {
          String pMan = doc["manufacturerID"] | "GEN";
          if (pMan == targetMan) {
            foundFile = fname;
            matchCount++;
          }
        }
        f.close();
      }
  #if defined(ESP32)
      file = root.openNextFile();
  #endif
    }

  // Decision Logic
  if (matchCount == 1) {
    #ifdef DEBUG
    DEBUG_PRINT("Auto-Assign: Found unique match for " + targetMan + " -> " + foundFile);
    #endif
    strlcpy(userData.slaveProfile[idx], foundFile.c_str(), 32);
    saveConfig();             // Save the assignment to config.json
    loadProfileToRAM(idx, userData.slaveProfile[idx]); // Load it immediately
  } else {
    #ifdef DEBUG
    if (matchCount == 0) DEBUG_PRINT("Auto-Assign: No profile found for " + targetMan);
    else DEBUG_PRINT("Auto-Assign: Multiple profiles found for " + targetMan + ". Manual selection required.");
    #endif
  }
}

// --- HELPER: Is Record Ignored? ---
// (Adaptiert für das neue ActiveProfile Struct)
bool isRecordIgnored(ActiveProfile* profile, uint8_t recordIndex) {
  for(uint8_t i = 0; i < profile->ignoreCount; i++) {
    if(profile->ignoreRecords[i] == recordIndex) return true;
  }
  return false;
}

#endif
