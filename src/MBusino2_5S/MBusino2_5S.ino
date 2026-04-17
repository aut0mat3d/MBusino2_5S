/*
# MBusino for multiple slaves
## ESP8266 D1 Mini or ESP32 S2 Mini
Original Repository (Base Code): https://github.com/Zeppelin500/MBusino/
Repository for this Code: https://github.com/aut0mat3d/MBusino2_5S/

# MBusino v2.0.0 Modifications:
****************************************************
- OS & File System: 
  Migrated from EEPROM to LittleFS. 
  Implemented a robust, dynamic JSON-based configuration file system.
  Safe Legacy-Bridge for seamless OTA updates from older EEPROM versions.
- User Interface (Web-UI):
  Complete frontend redesign with CSS Flexbox & Grid.
  Live-Data AJAX updates for M-Bus values, Sensor offsets, Uptime, and Free Heap.
  Profile Manager for uploading/deleting custom M-Bus JSON profiles.
  Smart Calibration tool for DS18B20 sensors (Average & Target calibration).
  Safe Reboot mechanism preventing "Race Conditions" on unsaved changes.
- M-Bus Engine:
  Advanced Status Byte parsing (Standard & Custom bits).
  Dynamic Profile-Engine via JSON templates instead of hardcoded structs.
  Auto-Assign feature for automatic profile matching via Manufacturer ID.
  Plausibility checks, Min/Max Limits, Deadband filtering, and Bouncer-Hold logic.
  Engelmann Bugfix (Dynamic Power calculation via Flow & DeltaT).
- Networking & Reliability:
  Smart WiFi Recovery & Island Mode: Stays alive in AP-Mode without reboot loops if router is missing.
  QoS Tracking (WiFi Reconnects, MQTT Downtimes).
  User-selectable AP Channel to prevent 2.4GHz interference.
- ESP-NOW Integration (Industry Gateway feature):
  Broadcasts structured M-Bus & Sensor data directly to other ESPs (e.g., Solar Controllers) 
  bypassing WiFi/Router completely for maximum reliability.
****************************************************

## Licence
****************************************************
This program is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

See the GNU General Public License for more details.
You should have received a copy of the GNU General Public License along with this program.
If not, see <http://www.gnu.org/licenses/>.
****************************************************
*/

#include <PubSubClient.h>
#include <OneWire.h>            // Library for OneWire Bus
#include <DallasTemperature.h>  // Library for DS18B20 Sensors
#include <Wire.h>
#include <ESPAsyncWebServer.h> 

#include <DNSServer.h>
#include <ArduinoOTA.h>

#include <MBusinoLib.h>  // Library for decode M-Bus
#include <MBusCom.h>     // Library M-Bus communication
#include <ArduinoJson.h>
#include <EEPROM.h>
#include <LittleFS.h>

#include <Adafruit_Sensor.h>
#include <Adafruit_BME280.h>

#if defined(ESP8266)
#include <ESP8266WiFi.h>
#include <ESP8266WiFiMulti.h>
#include <ESPAsyncTCP.h>
MBusCom mbus(&Serial);
ESP8266WiFiMulti wifiMulti;
#elif defined(ESP32)
#include <WiFi.h>
#include <WiFiMulti.h>
#include <AsyncTCP.h>
HardwareSerial MbusSerial(1);
MBusCom mbus(&MbusSerial,37,39);
WiFiMulti wifiMulti;
#endif

#if defined(ESP8266)
#include <espnow.h>
#elif defined(ESP32)
#include <esp_now.h>
#endif

#define MBUSINO_VERSION "2.0.0"

// --- Project Repository URL ---
// Change this to your own fork if you host custom firmware!
const char* projectURL = "https://github.com/aut0mat3d/MBusino2_5S/";

#if defined(ESP8266)
#define ONE_WIRE_BUS1 2   //D4
#define ONE_WIRE_BUS2 13  //D7
#define ONE_WIRE_BUS3 12  //D6
#define ONE_WIRE_BUS4 14  //D5
#define ONE_WIRE_BUS5 0   //D3
#define ONE_WIRE_BUS6 5   //D1
#define ONE_WIRE_BUS7 4   //D2
#elif defined(ESP32)
#define ONE_WIRE_BUS1 16  //2   //D4
#define ONE_WIRE_BUS2 11  //13  //D7
#define ONE_WIRE_BUS3 9   //12  //D6
#define ONE_WIRE_BUS4 7   //14  //D5
#define ONE_WIRE_BUS5 18  //0   //D3
#define ONE_WIRE_BUS6 35  //5   //D1
#define ONE_WIRE_BUS7 33  //4   //D2
#endif

#define SEALEVELPRESSURE_HPA (1013.25)
Adafruit_BME280 bme; // I2C
MBusinoLib payload(254);
WiFiClient espClient;
DNSServer dnsServer;

// Setup a oneWire instance to communicate with any OneWire devices (not just Maxim/Dallas temperature ICs)
OneWire oneWire1(ONE_WIRE_BUS1);
OneWire oneWire2(ONE_WIRE_BUS2);
OneWire oneWire3(ONE_WIRE_BUS3);
OneWire oneWire4(ONE_WIRE_BUS4);
OneWire oneWire5(ONE_WIRE_BUS5);

OneWire oneWire6(ONE_WIRE_BUS6);
OneWire oneWire7(ONE_WIRE_BUS7);

// Pass our oneWire reference to Dallas Temperature.
DallasTemperature sensor1(&oneWire1);
DallasTemperature sensor2(&oneWire2);
DallasTemperature sensor3(&oneWire3);
DallasTemperature sensor4(&oneWire4);
DallasTemperature sensor5(&oneWire5);

DallasTemperature sensor6(&oneWire6);
DallasTemperature sensor7(&oneWire7);

// --- Variables for Web-UI Unlock via 2x Reset ---
bool tempWebUnlock = false;
unsigned long unlockTimer = 0;

// --- Variables for WLAN-Failover & QoS ---
unsigned long mqttDowntimeStart = 0;
bool mqttWasConnected = false;
bool usingFallbackWlan = false; // Holds acutal active WLAN
uint16_t wifiDisconnectCounter = 0;  // QoS

// --- Backup original v501 Structure ---
struct settings_v501 {
  char ssid[65];
  char password[65];
  char mbusinoName[31];
  char broker[65];
  uint16_t mqttPort;
  uint16_t extension;
  char mqttUser[65];
  char mqttPswrd[65]; 
  uint32_t sensorInterval;
  uint32_t mbusInterval; 
  uint8_t mbusSlaves;
  uint8_t mbusAddress1;
  uint8_t mbusAddress2;
  uint8_t mbusAddress3;
  uint8_t mbusAddress4;
  uint8_t mbusAddress5;
  bool haAutodisc;
  bool telegramDebug;
} oldUserData_v501;

// --- The New v502 Structure ---
struct settings {
  char ssid[65];
  char password[65];
  char ssid2[65];           // Fallback WLAN
  char password2[65];     // Fallback WLAN Password
  uint8_t apChannel;
  char webPassword[65];   // Web-UI Password (empty = open Access)
  char otaPassword[65];   // OTA Password
  char mbusinoName[31];
  char broker[65];
  uint16_t mqttPort;
  uint8_t owSensors;
  uint8_t i2cMode;
  char mqttUser[65];
  char mqttPswrd[65]; 
  uint32_t sensorInterval;
  uint32_t mbusInterval; 
  uint8_t mbusSlaves;
  uint8_t mbusAddress1;
  uint8_t mbusAddress2;
  uint8_t mbusAddress3;
  uint8_t mbusAddress4;
  uint8_t mbusAddress5;
  char slaveName1[31];
  char slaveName2[31];
  char slaveName3[31];
  char slaveName4[31];
  char slaveName5[31];
  char sensorName1[31];
  char sensorName2[31];
  char sensorName3[31];
  char sensorName4[31];
  char sensorName5[31];
  char sensorName6[31];
  char sensorName7[31];
  char bmeName[31];
  bool haAutodisc;
  bool telegramDebug;
  float minFlow[5];
  float maxFlow[5];
  float maxPower[5];
  float sensorOffset[7];
  float deadbandFlow[5];
  char slaveProfile[5][32];
  bool espNowEnable;
  char espNowMac[20]; // Target MAC, e.g. "FF:FF:FF:FF:FF:FF" (Broadcast)
} userData = {
  "SSID", "Password", 
  "", "", 
  1,                                      // Default AP Channel: 1
  "", "mbusino",                          // Web Passwd, OTA Passwd
  "MBusino", "192.168.1.8", 1883, 
  5, 1, // <-- 5x DS18B20, I2C Mode 1 (BME)
  "mqttUser", "mqttPasword", 
  5000, 120000, 1, 0xFE, 0, 0, 0, 0, 
  "", "", "", "", "",       // M-Bus Names
  "", "", "", "", "", "", "", // Sensor Names
  "",                       // BME Name
  true, false, //haAutodisc Flag, telegramDebug Flag
  {0.0, 0.0, 0.0, 0.0, 0.0},             // Defaults: Min Flow
  {999.0, 999.0, 999.0, 999.0, 999.0},   // Defaults: Max Flow (Pass all)
  {999.0, 999.0, 999.0, 999.0, 999.0},   // Defaults: Max Power (Pass all)
  {0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0},   // Defaults: Sensor Offsets
  {0.0, 0.0, 0.0, 0.0, 0.0},             // Defaults: Flow Deadband (0.0 = Inactive)
  {"", "", "", "", ""},                  // M-Bus Slave Profiles, Defaults: Not set
  
  false,                                 // Default: ESP-NOW disabled
  "FF:FF:FF:FF:FF:FF"                    // Default: ESP-NOW Broadcast MAC
};

struct oldSettings {
  char ssid[30];
  char password[30];
  char mbusinoName[11];
  char broker[20];
  uint16_t mqttPort;
  uint16_t extension;
  char mqttUser[30];
  char mqttPswrd[30];
  uint32_t sensorInterval;
  uint32_t mbusInterval; 
  uint8_t mbusSlaves;
  uint8_t mbusAddress1;
  uint8_t mbusAddress2;
  uint8_t mbusAddress3;
  uint8_t mbusAddress4;
  uint8_t mbusAddress5;
  bool haAutodisc;
  bool telegramDebug;
} oldUserData = {"SSID","Password","MBusino","192.168.1.8",1883,5,"mqttUser","mqttPasword",5000,120000,1,0xFE,0,0,0,0,true,false};

struct CustomStateDef {
  char haName[21];
  uint8_t bitMask;
  uint8_t bitShift;
  char states[8][16];
};

struct ActiveProfile {
  bool isValid;                  // Profile loaded and valid?
  char profileName[25];          
  char manufacturerID[4];        // e.g. "LUG"
  bool applyEngelmannBugfix;
  uint8_t ignoreRecords[35];
  uint8_t ignoreCount;
  CustomStateDef customStates[3];
  uint8_t customStateCount;
};
ActiveProfile slaveProfiles[5]; // The active rules for our 5 slaves

// ==============================================================================
// --- ESP-NOW PAYLOAD STRUCTURE (175 Bytes) ---
// This is the exact binary layout broadcasted to the Solar-Controller.
// The __attribute__((packed)) prevents the compiler from adding hidden padding bytes.
// ==============================================================================
typedef struct __attribute__((packed)) {
  char magic[5];            // Magic Header: 'M', 'B', 'I', 'S', 'P' (Filters out foreign packets)
  uint8_t version;          // Protocol Version (currently 1)
  uint32_t system_id;       // Unique ID (FNV-1a Hash of the MBusino Network Name)

  struct {
    uint32_t fab_number;    // Fabrication Number (e.g., 72532544). 0 = unused/offline
    uint8_t status;         // Standardized Status Byte (0-7):
                            // 0: OK
                            // 1: Deadband active (Flow/Power forced to 0.0)
                            // 2: Min Flow Limit warning
                            // 3: Max Flow Limit warning
                            // 4: Max Power Limit warning
                            // 5: Bouncer Hold (Values frozen due to implausibility)
                            // 6: Bouncer Timeout (Values forced to 0.0)
                            // 7: Offline / Fatal Error (Values are NaN)
    float energy;           // Current energy reading (usually kWh or MWh)
    float volume_flow;      // Current flow rate in m³/h
    float power;            // Current power in kW
    float flow_temp;        // Forward temperature in °C
    float return_temp;      // Return temperature in °C
  } meters[5];              // Data for up to 5 M-Bus Slaves (5 * 25 = 125 Bytes)

  float ds18b20[7];         // Data for 7 OneWire Sensors in °C (-127.0 = invalid/unplugged)
  
  struct {
    float temperature;      // °C (-127.0 = invalid)
    float pressure;         // hPa (0.0 = invalid)
    float humidity;         // % (-1.0 = invalid)
  } bme;                    // BME280 Environment Sensor (12 Bytes)
} ESPNowPayload;

ESPNowPayload espNowData;   // The global payload object
// ==============================================================================

uint8_t mbusAddress[5] = {0};
uint16_t currentManID = 0;  // Holds the Manufacturer ID for the current decoding loop

bool mqttcon = false;
bool apMode = false;
bool credentialsReceived = false;
uint16_t conCounter = 0;

uint8_t newAddress = 0;
bool newAddressReceived = false;
bool waitToSetAddress = false;
uint8_t currentAddress = 0;
uint8_t addressCounter = 0;
uint8_t currentAddressCounter = 0;
uint8_t pollingAddress = 0;

int Startadd = 0x13; // Start address for decoding

float OW[7] = {-127.0, -127.0, -127.0, -127.0, -127.0, -127.0, -127.0}; // variables for DS18B20 Onewire sensors 
uint8_t sensorStatus = 0;
float temperatur = 0; // Variables for the BME280 on I2C
float druck = 0;
float hoehe = 0;
float feuchte = 0;
bool bmeStatus;

// --- LIVE DATA CACHE (For Web-UI) ---
uint16_t liveManID[5] = {0};                // Manufacturer ID
String liveFab[5] = {"", "", "", "", ""};   // Fabrication number
String liveError[5] = {"", "", "", "", ""}; // 0: all good
bool liveConnected[5] = {false, false, false, false, false};
String liveTelegram[5] = {"", "", "", "", ""};
uint8_t liveStatusByte[5] = {0};            // Caches the hardware status byte 17

struct FilterState {
  uint32_t lastValidTime = 0;
  double lastValidFlow = 0.0;
  double lastValidPower = 0.0;
  bool isError = false;
};
FilterState filterStates[5]; // Tracks bouncer timeouts per slave
// ---------------------------------------

uint8_t mbusLoopStatus = 0;
bool shc = true; // Switch Has Changed
uint8_t fields = 0;
bool fcb[5] = {0}; // M-Bus Frame Count Bit
bool initializeSlave[5] = {true}; // m-bus normalizing is needed
bool firstrun = true;
uint8_t recordCounter[5] = {0}; // count the received records for multible telegrams
char jsonstring[4092] = { 0 };
uint8_t address = 0; 
bool waitForRestart = false;
bool polling = false;
bool mtPolling = false;
bool wifiReconnect = false;
bool ledStatus = false;

unsigned long timerMQTT = 15000;
unsigned long timerSensorRefresh1 = 0;
unsigned long timerSensorRefresh2 = 0;
unsigned long timerMbus = 0;
unsigned long timerInitialize = 0;
unsigned long timerSerialAvailable = 0;
unsigned long timerMbusDecoded = 0;
unsigned long timerMbusReq = 0;
unsigned long timerDebug = 0;
unsigned long timerReconnect = 0;
unsigned long timerWifiReconnect = 0;
unsigned long timerReboot = 0;
unsigned long timerSetAddress = 0;
unsigned long timerPulse = 0;
uint32_t lastUptimeMillis = 0;
uint32_t overflowCount = 0;

void mbus_request_data(byte address);
void mbus_short_frame(byte address, byte C_field);
bool mbus_get_response(byte *pdata, unsigned char len_pdata);
void mbus_normalize(byte address);
void mbus_clearRXbuffer();
void calibrationAverage();
void calibrationSensor(uint8_t sensor);
void calibrationValue(float value);
void calibrationSet0();
void calibrationBME();
void setupServer();

uint8_t eeAddrCalibrated = 0;
uint8_t eeAddrCredentialsSaved = 32;
uint16_t calibrated = 123;       // shows if EEPROM was used before for offsets
uint16_t credentialsSaved = 123; // shows if EEPROM was used before for credentials
float OWwO[7] = {-127.0, -127.0, -127.0, -127.0, -127.0, -127.0, -127.0}; // Variables for DS18B20 Onewire Sensors with Offset (One Wire1 with Offset)
bool OWnotconnected[7] = {false};
uint8_t sensorToCalibrate = 0;

uint8_t adMbusMessageCounter = 0;   // Counter for autodiscover mbus message.
uint8_t adSensorMessageCounter = 0; // Counter for autodiscover mbus message.
uint32_t minFreeHeap = 0;
uint16_t pulseInterval = 1000;

// --- HELPER: Calculate a unique 32-bit Hash from a string (FNV-1a) ---
uint32_t calculateSystemId(const char* name) {
  uint32_t hash = 2166136261u;
  while (*name) {
    hash ^= (uint32_t)*name++;
    hash *= 16777619u;
  }
  return hash;
}

// --- HELPER: Parse MAC Address String ---
void parseMacAddress(const char* macStr, uint8_t* macArr) {
  if (strcmp(macStr, "FF:FF:FF:FF:FF:FF") == 0 || strlen(macStr) < 17) {
    memset(macArr, 0xFF, 6); // Default: Broadcast to all
    return;
  }
  sscanf(macStr, "%hhx:%hhx:%hhx:%hhx:%hhx:%hhx", 
         &macArr[0], &macArr[1], &macArr[2], &macArr[3], &macArr[4], &macArr[5]);
}

// --- HELPER: Get Formatted Uptime String ---
String getUptimeString() {
  uint32_t currentMillis = millis();
  if (currentMillis < lastUptimeMillis) overflowCount++; // Catch the rollover
  lastUptimeMillis = currentMillis;
  uint64_t totalMillis = ((uint64_t)overflowCount << 32) + currentMillis;
  uint32_t totalSecs = totalMillis / 1000;

  uint32_t d = totalSecs / 86400;
  uint8_t h = (totalSecs % 86400) / 3600;
  uint8_t m = (totalSecs % 3600) / 60;
  uint8_t s = totalSecs % 60;

  char uptimeStr[25];
  sprintf(uptimeStr, "%ud %02x:%02x:%02x", d, h, m, s); // English format: days hh:mm:ss
  return String(uptimeStr);
}

//outsourced program parts
#include "jsonHandling.h"
#include "profileTemplates.h"
#include "html.h"
#include "guiServer.h"
#include "mqtt.h"
#include "calibration.h"
#include "conversions.h"
#include "sensorRefresh.h"
#include "autodiscover.h"
#if defined(ESP32)
#include "networkEvents.h"
#endif

//#define DEBUG
#ifdef DEBUG
  #define DEBUG_PRINT(x) Serial.println(x)
#else
  #define DEBUG_PRINT(x)
#endif

void setup() {
  #if defined(DEBUG)
    Serial.begin(115200);
    delay(2000);
    Serial.println("\n\n=== MBUSINO BOOT SEQUENCE START ===");
  #else
    #if defined(ESP32)
    pinMode(LED_BUILTIN, OUTPUT);
    Serial.begin(115200);
    delay(1500);
    #endif
    mbus.begin();
  #endif
  
  minFreeHeap = ESP.getFreeHeap();

  // --- Double Reset Detection ---
  // This is used to enable the WebIF by pressing reset twice in 2 seconds
  // When a Double Reset is detected you can access the WebIF without a Password
  uint32_t rtcMagic = 0;
  ESP.rtcUserMemoryRead(0, &rtcMagic, sizeof(rtcMagic)); // Read RTC Mem
  
  if (rtcMagic == 1337) {
    // 2x Reset pressed!
    tempWebUnlock = true;
    unlockTimer = millis();
    rtcMagic = 0; // Clear Marker
    ESP.rtcUserMemoryWrite(0, &rtcMagic, sizeof(rtcMagic));
    
    // Visual Feedback: 2x fast blink
    pinMode(LED_BUILTIN, OUTPUT);
    for(int i=0; i<2; i++){
      digitalWrite(LED_BUILTIN, LOW); 
      delay(250);
      digitalWrite(LED_BUILTIN, HIGH); 
      delay(250);
    }
  } else {
    // First Start, set our Marker
    rtcMagic = 1337;
    ESP.rtcUserMemoryWrite(0, &rtcMagic, sizeof(rtcMagic));
    // Wait 1.5s for a second Click
    delay(1500);
    
    // Delay passed by - so, no second Click
    rtcMagic = 0;
    ESP.rtcUserMemoryWrite(0, &rtcMagic, sizeof(rtcMagic));
  }
  // ------------------------------

  // --- Mount File System ---
  if (!LittleFS.begin()) {
    #ifdef DEBUG
    DEBUG_PRINT("LittleFS Mount Failed. Formatting...");
    #endif
    LittleFS.format();
    LittleFS.begin();
  }

  //#define REMOVEPROFILES
  #ifdef REMOVEPROFILES
  // --- TEMPORARY WIPE FOR Debugging ---
  LittleFS.remove("/profiles/.init_done");
  #endif

  // --- Auto-Generate System Profiles ---
  if (!LittleFS.exists("/profiles/.init_done")) {
    #ifdef DEBUG
    DEBUG_PRINT("Initializing system profiles for the first time...");
    #endif
    LittleFS.mkdir("/profiles");

    // --- WIPE EVERYTHING IN /profiles/ ---
    #if defined(ESP8266)
      Dir dir = LittleFS.openDir("/profiles");
      while (dir.next()) {
        String fullPath = "/profiles/" + dir.fileName(); 
        LittleFS.remove(fullPath);
      }
    #elif defined(ESP32)
      File root = LittleFS.open("/profiles");
      if (root) {
        File file = root.openNextFile();
        while (file) {
          String p = String(file.name());
          if (!p.startsWith("/profiles/")) {
             p = "/profiles/" + p;
          }
          LittleFS.remove(p);
          file = root.openNextFile();
        }
      }
    #endif
    // Delete the old Manifest
    LittleFS.remove("/profiles/.sysfiles");

    // Lambda for JSON-Merge 
    auto writeMergedProfile = [](const char* path, const __FlashStringHelper* overrideJson) {
      JsonDocument baseDoc;
      // ALWAYS load the fully documented master template (Generic) first
      deserializeJson(baseDoc, FPSTR(profile_generic_json));
      
      if (overrideJson != nullptr) {
        JsonDocument overDoc;
        deserializeJson(overDoc, overrideJson);
        
        // Overwrite only the keys that are different in the override template
        for (JsonPair kv : overDoc.as<JsonObject>()) {
          if (strcmp(kv.key().c_str(), "_meta") == 0) {
            for (JsonPair metaKv : overDoc["_meta"].as<JsonObject>()) {
              baseDoc["_meta"][metaKv.key()] = metaKv.value();
            }
          } else {
            baseDoc[kv.key()] = kv.value();
          }
        }
      }

      // Save the final merged, well-formatted JSON to flash
      File f = LittleFS.open(path, "w");
      if (f) {
        serializeJsonPretty(baseDoc, f);
        f.close();
        
        // Extract File Name (Example: "/profiles/LUG.json" -> "LUG.json")
        String pathStr = String(path);
        String fname = pathStr.substring(pathStr.lastIndexOf('/') + 1);

        // append names into manifest
        File mf = LittleFS.open("/profiles/.sysfiles", "a");
        if (mf) {
          mf.println(fname);
          mf.close();
        }
        // ------------------------------------
      }
    };

    // 1. Write the generic profile
    writeMergedProfile("/profiles/GEN_Generic_mp.json", nullptr);
    // 2. Write the Landis+Gyr Profile
    writeMergedProfile("/profiles/LUG_Landis_Gyr_std_mp.json", FPSTR(profile_lug_json));
    // 3. Write the Engelmann Profile
    writeMergedProfile("/profiles/EFE_Sensostar_std_mp.json", FPSTR(profile_engelmann_json));
    
    // Set the invisible Flag .init_done
    File f = LittleFS.open("/profiles/.init_done", "w");
    if (f) { f.print("1"); f.close(); }
  }
  // --------------------------------------------------------

  // --- OS 2.0 BOOT SEQUENCE ---
  bool configLoaded = loadConfig();
  
  if (!configLoaded) {
    #ifdef DEBUG
    DEBUG_PRINT("No valid config.json found! Checking EEPROM for legacy data...");
    #endif
    
    // START LEGACY EEPROM BRIDGE
    EEPROM.begin(2048);
    EEPROM.get(eeAddrCredentialsSaved, credentialsSaved);
    
    if(credentialsSaved == 500){  // 500 is the code that credentials are saved in an old version before 1.0 (size of some variables has changed)
    EEPROM.get(100, oldUserData );
    
    strcpy(userData.ssid,oldUserData.ssid);
    strcpy(userData.password,oldUserData.password);
    strcpy(userData.mbusinoName,oldUserData.mbusinoName);
    strcpy(userData.broker,oldUserData.broker);
    userData.mqttPort = oldUserData.mqttPort;
    if(oldUserData.extension == 5) {
      userData.owSensors = 5;
      userData.i2cMode = 1;
    } else if (oldUserData.extension == 4) {
      userData.owSensors = 4;
      userData.i2cMode = 0;
    } else if (oldUserData.extension == 7) {
      userData.owSensors = 7;
      userData.i2cMode = 0;
    } else {
      userData.owSensors = 0; userData.i2cMode = 0;
    }
    strcpy(userData.mqttUser,oldUserData.mqttUser);
    strcpy(userData.mqttPswrd,oldUserData.mqttPswrd); 
    userData.sensorInterval= oldUserData.sensorInterval;
    userData.mbusInterval = oldUserData.mbusInterval; 
    userData.mbusSlaves = oldUserData.mbusSlaves;
    userData.mbusAddress1 = oldUserData.mbusAddress1;
    userData.mbusAddress2 = oldUserData.mbusAddress2;
    userData.mbusAddress3 = oldUserData.mbusAddress3;
    userData.mbusAddress4 = oldUserData.mbusAddress4;
    userData.mbusAddress5 = oldUserData.mbusAddress5;
    userData.haAutodisc = oldUserData.haAutodisc;
    userData.telegramDebug = oldUserData.telegramDebug;

    EEPROM.put(100, userData);
    credentialsSaved = 501;
    EEPROM.put(eeAddrCredentialsSaved, credentialsSaved);
  }
  else if(credentialsSaved == 501){  // 501 is the last supported revision where Configs were stored in EEPROM 
    // 1. Load the old Block into the v501 Structure
    EEPROM.get(100, oldUserData_v501);
    
    // 2. Copy over the old Values into our new Struct
    strcpy(userData.ssid, oldUserData_v501.ssid);
    strcpy(userData.password, oldUserData_v501.password);
    
    // Set our new Fields as empty (they are not existant atm)
    strcpy(userData.ssid2, "");       
    strcpy(userData.password2, "");
    strcpy(userData.webPassword, "");
    strcpy(userData.otaPassword, "mbusino");

    strcpy(userData.mbusinoName, oldUserData_v501.mbusinoName);
    strcpy(userData.broker, oldUserData_v501.broker);
    userData.mqttPort = oldUserData_v501.mqttPort;
    
    if(oldUserData_v501.extension == 5) {
      userData.owSensors = 5; userData.i2cMode = 1; // 5x DS18B20 + BME
    } else if (oldUserData_v501.extension == 4) {
      userData.owSensors = 4; userData.i2cMode = 0; // 4x DS18B20
    } else if (oldUserData_v501.extension == 7) {
      userData.owSensors = 7; userData.i2cMode = 0; // 7x DS18B20
    } else {
      userData.owSensors = 0; userData.i2cMode = 0; // Only M-Bus
    }

    for(int i=0; i<5; i++) {
      userData.deadbandFlow[i] = 0.0;
    }
    
    strcpy(userData.mqttUser, oldUserData_v501.mqttUser);
    strcpy(userData.mqttPswrd, oldUserData_v501.mqttPswrd); 
    userData.sensorInterval = oldUserData_v501.sensorInterval;
    userData.mbusInterval = oldUserData_v501.mbusInterval;
    userData.mbusSlaves = oldUserData_v501.mbusSlaves;
    userData.mbusAddress1 = oldUserData_v501.mbusAddress1;
    userData.mbusAddress2 = oldUserData_v501.mbusAddress2;
    userData.mbusAddress3 = oldUserData_v501.mbusAddress3;
    userData.mbusAddress4 = oldUserData_v501.mbusAddress4;
    userData.mbusAddress5 = oldUserData_v501.mbusAddress5;
    userData.haAutodisc = oldUserData_v501.haAutodisc;
    userData.telegramDebug = oldUserData_v501.telegramDebug;

    // 3. Store the new Struct into EEPROM
    EEPROM.put(100, userData);
    
    // 4. Set new Version Number
    credentialsSaved = 502;
    EEPROM.put(eeAddrCredentialsSaved, credentialsSaved);
  }
    
    EEPROM.end();
    // END LEGACY EEPROM BRIDGE

    #ifdef DEBUG
    DEBUG_PRINT("Generating initial config.json...");
    #endif
    saveConfig();
  }
  // ----------------------------------------------

  #ifdef DEBUG
  DEBUG_PRINT("\n=== GELADENE CREDENTIALS ===");
  DEBUG_PRINT(String("SSID 1: '") + userData.ssid + "'");
  DEBUG_PRINT(String("PASS 1: '") + userData.password + "'");
  DEBUG_PRINT(String("SSID 2: '") + userData.ssid2 + "'");
  DEBUG_PRINT(String("PASS 2: '") + userData.password2 + "'");
  DEBUG_PRINT(String("WebUI Pwd: '") + userData.webPassword + "'");
  DEBUG_PRINT(String("OTA pwd: '") + userData.otaPassword + "'");
  DEBUG_PRINT("============================\n");
  #endif
    
  if(userData.telegramDebug > 1){
    userData.telegramDebug = 0;
  }
  //Manual enable of Debug Telegrams
  //userData.telegramDebug = true;
  
  #if defined(ESP32)
  WiFi.onEvent(WiFiEvent);
  #endif
  WiFi.hostname(userData.mbusinoName);
  client.setServer(userData.broker, userData.mqttPort);
  client.setCallback(callback);

  #ifdef DEBUG
    DEBUG_PRINT("DEBUG Check 3: Sprintf done. Starting WiFi search...");
  #endif
  
  WiFi.mode(WIFI_STA);
  
  bool ssid1set = (strcmp(userData.ssid, "SSID") != 0 && strlen(userData.ssid) > 0);
  bool ssid2set = (strlen(userData.ssid2) > 0);
  
  // --- 1. Try to connect fast ---
  if (ssid1set) {
    #ifdef DEBUG
      DEBUG_PRINT("Fast Connect: Trying Primary (SSID1)...");
    #endif
    WiFi.begin(userData.ssid, userData.password);
    uint8_t fastConnectTries = 0;
    
    while (WiFi.status() != WL_CONNECTED && fastConnectTries < 40) { 
      delay(100);
      #ifdef DEBUG
        Serial.print("*");
      #endif
      fastConnectTries++;
    }
  }

  if (WiFi.status() != WL_CONNECTED && ssid2set) {
    #ifdef DEBUG
      DEBUG_PRINT("\nFast Connect: SSID1 failed. Trying Fallback (SSID2)...");
    #endif
    WiFi.begin(userData.ssid2, userData.password2);
    uint8_t fastConnectTries = 0;
    
    while (WiFi.status() != WL_CONNECTED && fastConnectTries < 40) { 
      delay(100);
      #ifdef DEBUG
        Serial.print("*");
      #endif
      fastConnectTries++;
    }
  }

  // --- 2. FALLBACK TO WIFIMULTI & AP-MODE ---
  if(WiFi.status() == WL_CONNECTED) {
    #ifdef DEBUG
      DEBUG_PRINT("\nFast Connect successful! Connected to: " + WiFi.SSID());
    #endif
  } else {
    #ifdef DEBUG
      DEBUG_PRINT("\nFast Connect failed completely. Starting WiFiMulti scan...");
    #endif
    
    wifiMulti.addAP(userData.ssid, userData.password);
    if (ssid2set) {
      wifiMulti.addAP(userData.ssid2, userData.password2);
    }

    byte tries = 0;
    if (!ssid1set && !ssid2set) {
      tries = 99; // Forces immediate fallback to AP mode
    }
    
    while (wifiMulti.run() != WL_CONNECTED) {
      #ifdef DEBUG
        Serial.print(".");
      #endif
      delay(1000);
      
      if (tries++ > 5) { 
        #ifdef DEBUG
          DEBUG_PRINT("\nDEBUG Check 4: WLAN not found. Starting AP Mode!");
        #endif
        WiFi.mode(WIFI_AP);
        
        String apName = String("MBusino_Setup_") + String(userData.mbusinoName);
        WiFi.softAP(apName.c_str(), NULL, userData.apChannel, 0);
        
        apMode = true;
        break;
      }
    }
  }
  
  #ifdef DEBUG
    DEBUG_PRINT("\nDEBUG Check 5: WiFi loop finished. Starting Webserver...");
  #endif
  
  setupServer();
  if(apMode==true){
    dnsServer.start(53, "*", WiFi.softAPIP());
  }
  server.addHandler(new CaptiveRequestHandler()).setFilter(ON_AP_FILTER);//only when requested from AP

  //attach AsyncWebSocket
 // ws.onEvent(onEvent);
  server.addHandler(&ws);

  // attach AsyncEventSource
  server.addHandler(&events);
  
  // Simple Firmware Update Form
  server.on("/update", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(200, "text/html",  (uint8_t *)update_html, update_htmlLength);   
  });
  
  server.on("/update", HTTP_POST, [](AsyncWebServerRequest *request){
    waitForRestart = !Update.hasError();
    if(Update.hasError()==true){
      timerReboot = millis();
    }
    AsyncWebServerResponse *response = request->beginResponse(200, "text/plain", waitForRestart?"success, restart now":"FAIL");
    response->addHeader("Connection", "close");
    request->send(response);
  },[](AsyncWebServerRequest *request, String filename, size_t index, uint8_t *data, size_t len, bool final){
    if(!index){
      #if defined(ESP8266)
      Update.runAsync(true);
      #endif
      if(!Update.begin((ESP.getFreeSketchSpace() - 0x1000) & 0xFFFFF000)){
        Update.printError(Serial);
      }
    }
    if(!Update.hasError()){
      if(Update.write(data, len) != len){
        Update.printError(Serial);
      }
    }
    if(final){
      if(Update.end(true)){
        //Serial.printf("Update Success: %uB\n", index+len);
      } else {
        Update.printError(Serial);
      }
    }
  });
  
  ArduinoOTA.setPassword((const char *)userData.otaPassword);
  server.onNotFound(onRequest);
  #if defined(ESP8266)
  ArduinoOTA.begin(&server);
  #elif defined(ESP32)
  ArduinoOTA.begin();
  #endif
  server.begin();

  client.setBufferSize(6000);
  
  // Start up the library
  if(userData.owSensors > 0){
    sensor1.begin(); sensor1.setWaitForConversion(false);
    if(userData.owSensors > 1) { sensor2.begin(); sensor2.setWaitForConversion(false); }
    if(userData.owSensors > 2) { sensor3.begin(); sensor3.setWaitForConversion(false); }
    if(userData.owSensors > 3) { sensor4.begin(); sensor4.setWaitForConversion(false); }
    if(userData.owSensors > 4) { sensor5.begin(); sensor5.setWaitForConversion(false); }
  }
  
  if(userData.owSensors > 5){
    sensor6.begin(); sensor6.setWaitForConversion(false);
    if(userData.owSensors > 6) { sensor7.begin(); sensor7.setWaitForConversion(false); }
  }

  if(userData.i2cMode == 1){
    bmeStatus = bme.begin(0x76); // Original Sensors
  } else if (userData.i2cMode == 2){
    bmeStatus = bme.begin(0x77); // Diverse Clones
  }
  
  mbusAddress[0] = userData.mbusAddress1;
  mbusAddress[1] = userData.mbusAddress2;
  mbusAddress[2] = userData.mbusAddress3;
  mbusAddress[3] = userData.mbusAddress4;
  mbusAddress[4] = userData.mbusAddress5;
}


void loop() {
  heapCalc();

  // Reset temporary Web-Unlock after 15 Minutes Runtime (900.000 ms)
  if(tempWebUnlock && (millis() - unlockTimer > 900000)) {
    tempWebUnlock = false;
  }
  
  ArduinoOTA.handle();
  if(apMode == true){
    dnsServer.processNextRequest();
  }

  #if defined(ESP32)
  if(millis() - timerPulse >= pulseInterval){ // Blink of the internal LED to see MBusino is still alive and the network status
    timerPulse = millis();
    //Serial.println("pulse"); 
    if(ledStatus == false){
      ledStatus = true;
      digitalWrite(LED_BUILTIN, HIGH);
    }else{
      ledStatus = false;
      digitalWrite(LED_BUILTIN, LOW);
    }
  }

  if(wifiReconnect == true && (millis() - timerWifiReconnect > 2000)){
    Serial.println("try to reconnect");
    wifiReconnect = false;
    WiFi.reconnect();
  }
  #endif

  // ==============================================================================
  // --- SMART WIFI RECOVERY & ISLAND MODE LOGIC ---
  // ==============================================================================
  // Check if user actively requested Island Mode (No valid SSID provided)
  bool isApOnly = (strlen(userData.ssid) == 0 || String(userData.ssid) == "NONE" || String(userData.ssid) == "SSID");
  
  if (apMode == true) {
    if (!isApOnly) {
      // -----------------------------------------------------------
      // SMART RECOVERY: Router is lost, but we have credentials.
      // We only try to reconnect if NO user is surfing the Web-UI!
      // -----------------------------------------------------------
      if (WiFi.softAPgetStationNum() == 0) {
        static unsigned long lastReconnectAttempt = 0;
        
        // Try every 3 minutes (180000 ms)
        if (millis() - lastReconnectAttempt > 180000) { 
          lastReconnectAttempt = millis();
          
          #ifdef DEBUG
          DEBUG_PRINT("Smart Recovery: Nobody connected to Web-UI. Trying to find Router...");
          #endif
          
          // Switch to mixed mode to allow background scanning
          WiFi.mode(WIFI_AP_STA);
          WiFi.begin(userData.ssid, userData.password);
        }
        
        // If the background process successfully connected:
        if (WiFi.status() == WL_CONNECTED) {
          #ifdef DEBUG
          DEBUG_PRINT("Smart Recovery: Router found! Disabling AP.");
          #endif
          WiFi.mode(WIFI_STA);
          // Shut down AP to save power and clear the airwaves
          apMode = false;
        }
      } else {
        // A user is connected to the Web-UI. Hold still! Do not scan, do not switch modes!
        // We reset the timer so it doesn't immediately reconnect when the user leaves.
        static unsigned long lastReconnectAttempt = millis(); 
      }
    }
    // If isApOnly == true: Do absolutely nothing. Just survive and broadcast.
  }
  // ==============================================================================

  if(credentialsReceived == true && waitForRestart == false){
    if (saveConfig()) {
      #ifdef DEBUG
      DEBUG_PRINT("Config successfully saved to LittleFS!");
      #endif
    } else {
      #ifdef DEBUG
      DEBUG_PRINT("ERROR: Failed to save Config to LittleFS!");
      #endif
    }
    
    // No automatic Reboot - we do have Tabs,...
    credentialsReceived = false;
  }

  if(waitForRestart==true && (millis() - timerReboot) > 1500){
    ESP.restart();
  }

  // --- WLAN Auto-Reconnect if Router reboots ---
  if (WiFi.status() != WL_CONNECTED && apMode == false) {
    if ((millis() - timerWifiReconnect) > 5000) { // Try all 5 Seconds
      wifiDisconnectCounter++;
      
      if (usingFallbackWlan && strlen(userData.ssid2) > 0) {
        WiFi.begin(userData.ssid2, userData.password2);
      } else {
        WiFi.begin(userData.ssid, userData.password);
      }
      timerWifiReconnect = millis();
    }
  }

  // ==============================================================================
  // --- ESP-NOW BROADCASTER (Every 5 Seconds) ---
  // ==============================================================================
  static unsigned long lastEspNowSend = 0;
  
  if (userData.espNowEnable && (millis() - lastEspNowSend > 5000)) {
    lastEspNowSend = millis();
    
    // 1. Update Header (Just to be sure)
    memcpy(espNowData.magic, "MBISP", 5);
    espNowData.version = 1;
    espNowData.system_id = calculateSystemId(userData.mbusinoName);
    
    // 2. Determine correct channel
    // If connected to a Router, we MUST use the router's channel. If AP-Only, use our AP channel.
    uint8_t currentChannel = (WiFi.status() == WL_CONNECTED) ? WiFi.channel() : userData.apChannel;
    
    // 3. Prepare Target MAC
    uint8_t targetMac[6];
    parseMacAddress(userData.espNowMac, targetMac);
    
    // 4. Init & Send (Dynamic, non-blocking)
    #if defined(ESP8266)
      if (esp_now_init() == 0) {
        esp_now_set_self_role(ESP_NOW_ROLE_CONTROLLER);
        esp_now_add_peer(targetMac, ESP_NOW_ROLE_SLAVE, currentChannel, NULL, 0);
        esp_now_send(targetMac, (uint8_t *) &espNowData, sizeof(ESPNowPayload));
        esp_now_deinit(); // Free resources immediately after sending
      }
    #elif defined(ESP32)
      if (esp_now_init() == ESP_OK) {
        esp_now_peer_info_t peerInfo = {};
        memcpy(peerInfo.peer_addr, targetMac, 6);
        peerInfo.channel = currentChannel;
        peerInfo.encrypt = false;
        if (esp_now_add_peer(&peerInfo) == ESP_OK) {
          esp_now_send(targetMac, (uint8_t *) &espNowData, sizeof(ESPNowPayload));
        }
        esp_now_deinit();
      }
    #endif
  }
  // ==============================================================================

  // --- In Runtime Layer-3 / MQTT Check ---
  if(WiFi.status() == WL_CONNECTED && apMode == false){
    if (!client.connected()) { 
      // MQTT is dead...
      if (mqttWasConnected) {
        mqttDowntimeStart = millis();
        mqttWasConnected = false;
      }

      // 1. Try to reconnect every 5 Seconds
      if((millis() - timerReconnect) > 5000){
        reconnect();
        timerReconnect = millis();
      }

      // 2. FAILOVER ESCALATION: After 3 Minutes (180.000 ms) without MQTT -> Ping-Pong!
      if (millis() - mqttDowntimeStart > 180000) {
        WiFi.disconnect();
        
        // Switch Marker (Ping-Pong)
        if (!usingFallbackWlan && strlen(userData.ssid2) > 0) {
          usingFallbackWlan = true;
          WiFi.begin(userData.ssid2, userData.password2);
        } else {
          usingFallbackWlan = false;
          WiFi.begin(userData.ssid, userData.password);
        }
        
        mqttDowntimeStart = millis(); // Timer reset to give the new connected Wlan 3 Minutes to settle
      }
    }
    else{ 
      // MQTT lives...
      if (!mqttWasConnected) {
        mqttWasConnected = true; // Stop the Counter on reconnect
      }
    }
      client.loop(); //MQTT Function

      if(newAddressReceived == true){
        newAddressReceived = false;
        waitToSetAddress = true;
        timerSetAddress = millis();
        mbus.normalize(254);
        client.publish(String(String(userData.mbusinoName) + "/setAddress/1").c_str(), "done");
      }

      if(waitToSetAddress == true && (millis() - 500) > timerSetAddress){
        waitToSetAddress = false;
        mbus.set_address(254,newAddress);
        client.publish(String(String(userData.mbusinoName) + "/setAddress/2").c_str(), String(newAddress).c_str());
      }

      ///////////////// publish settings ///////////////////////////////////
      if((millis()-timerDebug) >10000){
        timerDebug = millis();
        
        // --- UPTIME CALCULATION (49.7 Days Overflow Safe) ---
        uint32_t currentMillis = millis();
        if (currentMillis < lastUptimeMillis) overflowCount++; // Catch the rollover
        lastUptimeMillis = currentMillis;
        // -------------------------------------------------------
        
        client.publish(String(String(userData.mbusinoName) + "/settings/uptime").c_str(), getUptimeString().c_str());
        client.publish(String(String(userData.mbusinoName) + "/settings/ssid").c_str(), userData.ssid);
        client.publish(String(String(userData.mbusinoName) + "/settings/broker").c_str(), userData.broker); 
        client.publish(String(String(userData.mbusinoName) + "/settings/port").c_str(), String(userData.mqttPort).c_str()); 
        client.publish(String(String(userData.mbusinoName) + "/settings/user").c_str(), userData.mqttUser);
        client.publish(String(String(userData.mbusinoName) + "/settings/name").c_str(), userData.mbusinoName); 
        client.publish(String(String(userData.mbusinoName) + "/settings/owSensors").c_str(), String(userData.owSensors).c_str());
        client.publish(String(String(userData.mbusinoName) + "/settings/i2cMode").c_str(), String(userData.i2cMode).c_str());
        client.publish(String(String(userData.mbusinoName) + "/settings/mbusInterval").c_str(), String(userData.mbusInterval).c_str());
        client.publish(String(String(userData.mbusinoName) + "/settings/sensorInterval").c_str(), String(userData.sensorInterval).c_str()); 
        client.publish(String(String(userData.mbusinoName) + "/settings/slaves").c_str(), String(userData.mbusSlaves).c_str());
        client.publish(String(String(userData.mbusinoName) + "/settings/address1").c_str(), String(userData.mbusAddress1).c_str());
        client.publish(String(String(userData.mbusinoName) + "/settings/address2").c_str(), String(userData.mbusAddress2).c_str());
        client.publish(String(String(userData.mbusinoName) + "/settings/address3").c_str(), String(userData.mbusAddress3).c_str());
        client.publish(String(String(userData.mbusinoName) + "/settings/address4").c_str(), String(userData.mbusAddress4).c_str());
        client.publish(String(String(userData.mbusinoName) + "/settings/address5").c_str(), String(userData.mbusAddress5).c_str());  
        client.publish(String(String(userData.mbusinoName) + "/settings/IP").c_str(), String(WiFi.localIP().toString()).c_str());
        client.publish(String(String(userData.mbusinoName) + "/settings/MQTTreconnections").c_str(), String(conCounter-1).c_str());
        long rssi = WiFi.RSSI();
        client.publish(String(String(userData.mbusinoName) + "/settings/RSSI").c_str(), String(rssi).c_str());
        
        // --- QoS & Live Status Info ---
        client.publish(String(String(userData.mbusinoName) + "/settings/WiFiReconnections").c_str(), String(wifiDisconnectCounter).c_str());
        client.publish(String(String(userData.mbusinoName) + "/settings/activeSSID").c_str(), WiFi.SSID().c_str());
        
        // Tells HA an if we are on our main- or fallback Network
        if(usingFallbackWlan) {
          client.publish(String(String(userData.mbusinoName) + "/settings/currentNetwork").c_str(), "Fallback (SSID2)");
        } 
        else {
          client.publish(String(String(userData.mbusinoName) + "/settings/currentNetwork").c_str(), "Primary (SSID1)");
        }
        
        client.publish(String(String(userData.mbusinoName) + "/settings/version").c_str(), MBUSINO_VERSION);
        client.publish(String(String(userData.mbusinoName) + "/settings/adcounter").c_str(), String(adMbusMessageCounter).c_str());     
        client.publish(String(String(userData.mbusinoName) + "/settings/freeHeap").c_str(), String(ESP.getFreeHeap()).c_str()); 
        client.publish(String(String(userData.mbusinoName) + "/settings/minFreeHeap").c_str(), String(minFreeHeap).c_str());
        
        // --- PUBLISH INITIAL SLIDER STATES ---
        // Push the actual values to the state_topics so HA displays the correct slider positions
        for(int i = 0; i < 5; i++) {
          if (strlen(userData.slaveName1) > 0 || i > 0) { // Slight optimization
            client.publish(String(String(userData.mbusinoName) + "/settings/minFlow" + String(i+1)).c_str(), String(userData.minFlow[i], 3).c_str(), true);
            client.publish(String(String(userData.mbusinoName) + "/settings/maxFlow" + String(i+1)).c_str(), String(userData.maxFlow[i], 3).c_str(), true);
            client.publish(String(String(userData.mbusinoName) + "/settings/maxPower" + String(i+1)).c_str(), String(userData.maxPower[i], 3).c_str(), true);
            client.publish(String(String(userData.mbusinoName) + "/settings/deadFlow" + String(i+1)).c_str(), String(userData.deadbandFlow[i], 3).c_str(), true);
          }
        }
        // Publish System Intervals (convert ms to seconds for HA)
        client.publish(String(String(userData.mbusinoName) + "/settings/mbusInterval").c_str(), String(userData.mbusInterval / 1000).c_str(), true);
        client.publish(String(String(userData.mbusinoName) + "/settings/sensorInterval").c_str(), String(userData.sensorInterval / 1000).c_str(), true);

        // Publish Sensor Offsets
        for(int i = 0; i < 7; i++) {
          client.publish(String(String(userData.mbusinoName) + "/settings/offset" + String(i+1)).c_str(), String(userData.sensorOffset[i], 1).c_str(), true);
        }
        // ----------------------------------------------------
      }
      ///////////////////////////////////////////////////////////

      if(userData.owSensors > 0 || userData.i2cMode > 0){      
        switch(sensorStatus){
          case 0:
            if((millis() - timerMQTT) > userData.sensorInterval) { 
              sensorRefresh1();
              timerMQTT = millis();
              sensorStatus = 1;
              timerSensorRefresh1 = millis();
            }
            break;
            
          case 1:
            if ((millis() - timerSensorRefresh1) > 800) {  
              sensorRefresh2();
              timerSensorRefresh1 = millis();
              sensorStatus = 2;
            }
            break;
            
          case 2:
            if((millis() - timerSensorRefresh1) > 200){
              adSensorMessageCounter++;
              for(uint8_t i = 0; i < userData.owSensors; i++){
                
                // --- Get Friendly Name for the Sensor ---
                const char* currentSensorName = "";
                if (i == 0) currentSensorName = userData.sensorName1;
                else if (i == 1) currentSensorName = userData.sensorName2;
                else if (i == 2) currentSensorName = userData.sensorName3;
                else if (i == 3) currentSensorName = userData.sensorName4;
                else if (i == 4) currentSensorName = userData.sensorName5;
                else if (i == 5) currentSensorName = userData.sensorName6;
                else if (i == 6) currentSensorName = userData.sensorName7;

                // --- Opt-In Logic: Only populate if a Name was given ---
                if (strlen(currentSensorName) > 0) {
                  if(userData.haAutodisc == true && adSensorMessageCounter == 3){
                    haHandoverOw(currentSensorName);
                  } 
                  if(OW[i] != -127){        
                    // Topic now uses the Sensor name instead of "S1"
                    client.publish(String(String(userData.mbusinoName) + "/" + String(currentSensorName)).c_str(), String(OWwO[i]).c_str());
                    client.publish(String(String(userData.mbusinoName) + "/" + String(currentSensorName) + "_offset").c_str(), String(userData.sensorOffset[i]).c_str());         
                  }
                }
                // --- ESP-NOW: Update OneWire Data ---
                // OWwO contains the value + offset. If error, OWwO is usually -127.0.
                espNowData.ds18b20[i] = OWwO[i];      
              }
            
              if(userData.i2cMode > 0){
                if (strlen(userData.bmeName) > 0) {
                  client.publish(String(String(userData.mbusinoName) + "/" + String(userData.bmeName) + "/temperature").c_str(), String(temperatur).c_str());
                  client.publish(String(String(userData.mbusinoName) + "/" + String(userData.bmeName) + "/pressure").c_str(), String(druck).c_str());
                  client.publish(String(String(userData.mbusinoName) + "/" + String(userData.bmeName) + "/altitude").c_str(), String(hoehe).c_str());
                  client.publish(String(String(userData.mbusinoName) + "/" + String(userData.bmeName) + "/humidity").c_str(), String(feuchte).c_str());
                  
                  // --- ESP-NOW: Update BME280 Data ---
                  espNowData.bme.temperature = temperatur;
                  espNowData.bme.pressure = druck;
                  espNowData.bme.humidity = feuchte;
                  if(userData.haAutodisc == true && adSensorMessageCounter == 3){
                    haHandoverBME(userData.bmeName);
                  }
                }
              }
              sensorStatus = 0;
            }
            break;
        }
      }
      
      ////////// M- Bus ###############################################
      if(firstrun == true){
        mbus.clearRXbuffer();
        for(uint8_t i = 0; i < userData.mbusSlaves; i++){
          initializeSlave[i] = false;
          mbus.normalize(mbusAddress[i]);
          client.publish(String(String(userData.mbusinoName) + "/MBus/debug/nomalizeAtStart/address").c_str(), String(mbusAddress[i]).c_str()); 
          delay(2000);
          byte rxbuffer[255] = {0};
          uint8_t rxbuffercontent = 0;
          rxbuffercontent = mbus.read_rxbuffer(rxbuffer, sizeof(rxbuffer));
          if(rxbuffercontent > 0 && rxbuffer[0] == 0xE5) {
            client.publish(String(String(userData.mbusinoName) + "/MBus/debug/nomalizeAtStart/ack").c_str(), String(rxbuffer[0]).c_str());
          }else{
            client.publish(String(String(userData.mbusinoName) + "/MBus/debug/nomalizeAtStart/no_ack").c_str(), String(mbusAddress[i]).c_str());
            for(uint8_t i=0; i<=rxbuffercontent; i++){
              client.publish(String(String(userData.mbusinoName) + "/MBus/debug/nomalizeAtStart/byte" + String(i)).c_str(), String(rxbuffer[i]).c_str());
            }
          }
        }
        firstrun = false;
      }

      switch(mbusLoopStatus){
        case 0:
          if(shc){ 
            client.publish(String(String(userData.mbusinoName) + "/MBus/debug/mbus_Loop").c_str(), String(mbusLoopStatus).c_str());
            shc = false;
          }
          if((millis() - timerMbus) > userData.mbusInterval || polling == true || mtPolling == true ){ 
            if(polling == false && mtPolling == false){ timerMbus = millis(); }
            if(polling == true){ 
              currentAddress = pollingAddress;
              polling = false;
            }else if(mtPolling == true){ 
              mtPolling = false;
            }else{ 
              if(addressCounter >= userData.mbusSlaves){
                addressCounter = 0;
              }
              currentAddress = mbusAddress[addressCounter];
              currentAddressCounter = addressCounter;
              addressCounter++;
              recordCounter[currentAddressCounter] = 0;
            }
            mbusLoopStatus = 1;
            shc = true;
          }
          break;
          
        case 1:
          if(shc){ 
            client.publish(String(String(userData.mbusinoName) + "/MBus/debug/mbus_Loop").c_str(), String(mbusLoopStatus).c_str());
            shc = false;
          }
          if(initializeSlave[currentAddressCounter] == true){
            initializeSlave[currentAddressCounter] = false;
            recordCounter[currentAddressCounter] = 0;
            mbus.clearRXbuffer();
            mbus.normalize(mbusAddress[currentAddressCounter]);    
            fcb[currentAddressCounter] = false;
            timerInitialize = millis();
            mbusLoopStatus = 2;
            shc = true;
            client.publish(String(String(userData.mbusinoName) + "/MBus/debug/nomalize/address").c_str(), String(mbusAddress[currentAddressCounter]).c_str()); 
          }else{
            mbusLoopStatus = 3;
            shc = true;
          }
          break;
          
        case 2: 
          if(shc){ 
            client.publish(String(String(userData.mbusinoName) + "/MBus/debug/mbus_Loop").c_str(), String(mbusLoopStatus).c_str());
            shc = false;
          }      
          if((millis() - timerInitialize) > 2000){
            if(mbus.available()){
              byte rxbuffer[255] = {0};
              uint8_t rxbuffercontent = 0;
              rxbuffercontent = mbus.read_rxbuffer(rxbuffer, sizeof(rxbuffer));
              if(rxbuffercontent == 1 && rxbuffer[0] == 0xE5) {
                client.publish(String(String(userData.mbusinoName) + "/MBus/debug/nomalize/ack").c_str(), String(rxbuffer[0]).c_str());
              }else{
                client.publish(String(String(userData.mbusinoName) + "/MBus/debug/nomalize/no_ack").c_str(), String(mbusAddress[currentAddressCounter]).c_str());
                for(uint8_t i=0; i<=rxbuffercontent; i++){
                  client.publish(String(String(userData.mbusinoName) + "/MBus/debug/nomalize/byte" + String(i)).c_str(), String(rxbuffer[i]).c_str());
                }
              }
            }else{
              client.publish(String(String(userData.mbusinoName) + "/MBus/debug/nomalize/no_answer").c_str(), String(mbusAddress[currentAddressCounter]).c_str());
            }
            mbusLoopStatus = 3;
            shc = true;
          }
          break;
          
        case 3: 
          if(shc){ 
            client.publish(String(String(userData.mbusinoName) + "/MBus/debug/mbus_Loop").c_str(), String(mbusLoopStatus).c_str());
            shc = false;
          }      
          mbus.clearRXbuffer();
          mbus.request_data(currentAddress,fcb[currentAddressCounter]);
          client.publish(String(String(userData.mbusinoName) + "/MBus/debug/request/currentAddress").c_str(), String(currentAddress).c_str()); 
          client.publish(String(String(userData.mbusinoName) + "/MBus/debug/request/fcb").c_str(),String(fcb[currentAddressCounter]).c_str()); 
          client.publish(String(String(userData.mbusinoName) + "/MBus/debug/request/cac").c_str(),String(currentAddressCounter).c_str()); 
          client.publish(String(String(userData.mbusinoName) + "/MBus/debug/request/ac").c_str(),String(addressCounter).c_str());
          mbusLoopStatus = 4;
          shc = true;
          timerMbusReq = millis();
          break;

        case 4: 
          if(shc){ 
            client.publish(String(String(userData.mbusinoName) + "/MBus/debug/mbus_Loop").c_str(), String(mbusLoopStatus).c_str());
            shc = false;
          }      
          if(mbus.available()){
            mbusLoopStatus = 5;
            shc = true;
            timerSerialAvailable = millis();
          }
          if(millis() - timerMbusReq > 2000){ 
            initializeSlave[currentAddressCounter] = true;
            recordCounter[currentAddressCounter] = 0;
            fcb[currentAddressCounter] = false;              
            client.publish(String(String(userData.mbusinoName)  +"/MBus/SlaveAddress"+String(currentAddress)+ "/MBUSerror").c_str(), "no_Data_received");
            mbusLoopStatus = 0;
            shc = true;
            mtPolling = false;
          }
          break;
          
        case 5: 
          if(shc){ 
            client.publish(String(String(userData.mbusinoName) + "/MBus/debug/mbus_Loop").c_str(), String(mbusLoopStatus).c_str());
            shc = false;
          }      
          if((millis() - timerSerialAvailable) > 1500){ 
            mbusLoopStatus = 6;
            shc = true;
            bool mbus_good_frame = false;
            byte mbus_data[MBUS_DATA_SIZE] = { 0 };
            mbus_good_frame = mbus.get_response(mbus_data, sizeof(mbus_data));
            
            if(userData.telegramDebug == true){
              char telegram[520] = {0};
              for(uint8_t i = 0; i <= mbus_data[1]+6; i++){                                                             
                char buffer[3];
                sprintf(buffer,"%02X",mbus_data[i]);                                                                    
                telegram[i*2] = buffer[0];
                telegram[(i*2)+1] = buffer[1];       
              }  
              client.publish(String(String(userData.mbusinoName) +"/MBus/debug/telegram"+ String(mbusAddress[currentAddressCounter])).c_str(), telegram);
            }
            if (mbus_good_frame) {
              if(fcb[currentAddressCounter] == true){ fcb[currentAddressCounter] = false; }
              else{ fcb[currentAddressCounter] = true; }

              int packet_size = mbus_data[1] + 6;
              JsonDocument jsonBuffer;
              JsonArray root = jsonBuffer.add<JsonArray>();  
              fields = payload.decode(&mbus_data[Startadd], packet_size - Startadd - 2, root); 
              address = mbus_data[5];
              uint8_t status_byte = mbus_data[16];
              liveStatusByte[currentAddressCounter] = status_byte;
              
              currentManID = (mbus_data[12] << 8) | mbus_data[11];
              
              // If no profile is assigned yet, try to find one automatically
              if (strlen(userData.slaveProfile[currentAddressCounter]) == 0) {
                autoAssignProfile(currentAddressCounter, currentManID);
              }
              
              // --- UPDATE LIVE CACHE (SUCCESS) ---
              liveConnected[currentAddressCounter] = true;
              liveManID[currentAddressCounter] = currentManID;
              liveError[currentAddressCounter] = payload.getError();

              // Get Fabrication Number from Bytes 7 to 10 (Little Endian / BCD)
              char fabBuf[9];
              sprintf(fabBuf, "%02X%02X%02X%02X", mbus_data[10], mbus_data[9], mbus_data[8], mbus_data[7]);
              liveFab[currentAddressCounter] = String(fabBuf);
              
              // Store raw telegram as Hex-String
              liveTelegram[currentAddressCounter] = "";
              for(uint8_t i = 0; i < packet_size; i++) {
                char buffer[3];
                sprintf(buffer,"%02X", mbus_data[i]);
                liveTelegram[currentAddressCounter] += String(buffer);
              }
              // -----------------------------------

              client.publish(String(String(userData.mbusinoName) + "/MBus/SlaveAddress"+String(address)+ "/status_byte").c_str(), String(status_byte).c_str());
              serializeJson(root, jsonstring);
              client.publish(String(String(userData.mbusinoName) + "/MBus/SlaveAddress"+String(address)+ "/error").c_str(), String(payload.getError()).c_str());  
              client.publish(String(String(userData.mbusinoName) + "/MBus/SlaveAddress"+String(address)+ "/jsonstring").c_str(), jsonstring);  
              client.publish(String(String(userData.mbusinoName) + "/MBus/SlaveAddress"+String(address)+ "/fcb").c_str(), String(fcb[currentAddressCounter]).c_str());
              heapCalc();
            } else {            
                initializeSlave[currentAddressCounter] = true;
                recordCounter[currentAddressCounter] = 0;
                fcb[currentAddressCounter] = false;
                jsonstring[0] = 0;
                
                // --- UPDATE LIVE CACHE (ERROR) ---
                liveConnected[currentAddressCounter] = false;
                liveError[currentAddressCounter] = "No good telegram";
                liveManID[currentAddressCounter] = 0;
                liveFab[currentAddressCounter] = "";
                // ---------------------------------
                
                client.publish(String(String(userData.mbusinoName)  +"/MBus/SlaveAddress"+String(currentAddress)+ "/MBUSerror").c_str(), "no_good_telegram");
                mbusLoopStatus = 0;
                shc = true;
                mtPolling = false;
            }
          } 
          break;
          
        case 6: // Send decoded M-Bus records via MQTT
          if(shc){ 
            client.publish(String(String(userData.mbusinoName) + "/MBus/debug/mbus_Loop").c_str(), String(mbusLoopStatus).c_str());
            shc = false;
          }
          
          if(millis() - timerMbusDecoded > 100){  
            mbusLoopStatus = 0;
            shc = true;

            // --- Load Profile if not already valid ---
            if (!slaveProfiles[currentAddressCounter].isValid) {
               loadProfileToRAM(currentAddressCounter, userData.slaveProfile[currentAddressCounter]);
            }

            // --- SILENT MODE CHECK ---
            if (!slaveProfiles[currentAddressCounter].isValid) {
               #ifdef DEBUG
               DEBUG_PRINT("SILENT MODE: No valid profile for Slave " + String(currentAddressCounter + 1));
               #endif
               // Optional: Publish a warning once to MQTT that a profile is missing
               return; // HARD EXIT: Do not decode or publish anything
            }

            // Use the RAM profile instead of the old getMeterProfile
            ActiveProfile* profile = &slaveProfiles[currentAddressCounter];
            JsonDocument root;
            deserializeJson(root, jsonstring); 
            jsonstring[0] = 0; 
            
            // --- 1. Determine friendly slave name globally for this run
            const char* currentSlaveName = "";
            if (currentAddressCounter == 0) currentSlaveName = userData.slaveName1;
            else if (currentAddressCounter == 1) currentSlaveName = userData.slaveName2;
            else if (currentAddressCounter == 2) currentSlaveName = userData.slaveName3;
            else if (currentAddressCounter == 3) currentSlaveName = userData.slaveName4;
            else if (currentAddressCounter == 4) currentSlaveName = userData.slaveName5;
            
            String baseTopic = String(userData.mbusinoName) + "/" + String(currentSlaveName) + "/";
            
            if (userData.haAutodisc == true && adMbusMessageCounter == 3 && currentAddressCounter == 0) {
              haHandoverSystem();
            }

            // --- 2. DEADBAND PRE-SCAN ---
            bool applyDeadband = false;
            bool limitError = false;
            double currentFlow = 0.0;
            double currentPower = 0.0;
            double currentDeltaT = 0.0;
            
            bool hasFlow = false;
            bool hasPower = false;
            bool hasDeltaT = false;

            // Fast forward scan to find Flow, Power, and DeltaT
            for (uint8_t j=0; j<fields; j++) {
              const char* n = root[j]["name"];
              const char* u = root[j]["units"];
              String uStr = (u != NULL) ? String(u) : "";
              
              if (strcmp(n, "volume_flow") == 0) { 
                NormalizedValue normF = normalizeMBusValue(root[j]["value_scaled"].as<double>(), uStr);
                currentFlow = normF.value; 
                hasFlow = true; 
              }
              if (strcmp(n, "power") == 0) { 
                NormalizedValue normP = normalizeMBusValue(root[j]["value_scaled"].as<double>(), uStr);
                currentPower = normP.value; 
                hasPower = true; 
              }
              if (strcmp(n, "temperature_diff") == 0) {
                currentDeltaT = root[j]["value_scaled"].as<double>();
                hasDeltaT = true;
              }
            }

            // Deadband Check
            if (hasFlow && userData.deadbandFlow[currentAddressCounter] > 0.0) {
              if (abs(currentFlow) <= userData.deadbandFlow[currentAddressCounter]) {
                // Around minimal Flow the Sensor cant deliver valid Measurements.
                // Say: 2500l/min max, a Gain of 1:100 gives us a valid minimum Flow of 25l/min - all below is not valid
                currentFlow = 0.0;
                currentPower = 0.0; // Physics: No flow = no power
              }
            }

            // Plausibility Limits Check
            String dataQuality = "real";
            
            if (hasFlow && (currentFlow < userData.minFlow[currentAddressCounter] || currentFlow > userData.maxFlow[currentAddressCounter])) {
              limitError = true;
            }
            if (hasPower && abs(currentPower) > userData.maxPower[currentAddressCounter]) {
              limitError = true;
            }

            // Dynamic Timeout Calculation (Poll interval dependent)
            uint32_t timeoutMillis = 1800000; // default 30 mins
            
            if (userData.mbusInterval > 361000) timeoutMillis = userData.mbusInterval * 2;
            else if (userData.mbusInterval > 181000) timeoutMillis = 3600000;

            if (limitError) {
              if (!filterStates[currentAddressCounter].isError) filterStates[currentAddressCounter].isError = true;
              
              if ((millis() - filterStates[currentAddressCounter].lastValidTime) > timeoutMillis) {
                dataQuality = "error";
                currentFlow = 0.0; // Hard fail state
                currentPower = 0.0;
              } else {
                dataQuality = "held";
                currentFlow = filterStates[currentAddressCounter].lastValidFlow; // Freeze values
                currentPower = filterStates[currentAddressCounter].lastValidPower;
              }
            } else {
              filterStates[currentAddressCounter].isError = false;
              filterStates[currentAddressCounter].lastValidTime = millis();
              filterStates[currentAddressCounter].lastValidFlow = currentFlow;
              filterStates[currentAddressCounter].lastValidPower = currentPower;
            }

            // --- 3. PUBLISH STATUS BITS ---
            if (strlen(currentSlaveName) > 0) {
              uint8_t sByte = liveStatusByte[currentAddressCounter];
              
              // 1. Standard Bits (0-4)
              uint8_t appStatus = sByte & 0x03;
              String appStr = (appStatus == 1) ? "Busy" : (appStatus == 2) ? "Error" : (appStatus == 3) ? "Abnormal" : "OK";
              
              client.publish(String(baseTopic + "status/application").c_str(), appStr.c_str());
              client.publish(String(baseTopic + "status/battery_low").c_str(), (sByte & 0x04) ? "1" : "0");
              client.publish(String(baseTopic + "status/permanent_err").c_str(), (sByte & 0x08) ? "1" : "0");
              client.publish(String(baseTopic + "status/temporary_err").c_str(), (sByte & 0x10) ? "1" : "0");
              client.publish(String(baseTopic + "status/data_quality").c_str(), dataQuality.c_str());

              // 2. Custom Bits (5-7) Parser Engine
              for (uint8_t k = 0; k < profile->customStateCount; k++) {
                // Check if a name is assigned (instead of != NULL)
                if (strlen(profile->customStates[k].haName) > 0) {
                  // Apply Mask and Shift
                  uint8_t val = (sByte & profile->customStates[k].bitMask) >> profile->customStates[k].bitShift;
                  
                  // Since we use a char array, we access it directly
                  const char* stateStr = profile->customStates[k].states[val];
                  if (strlen(stateStr) == 0) stateStr = "Unknown";
                  
                  // Build MQTT safe topic from name
                  String safeTopic = profile->customStates[k].haName;
                  safeTopic.replace(" ", "_");
                  safeTopic.replace("-", "_");
                  safeTopic.toLowerCase();
                  
                  client.publish(String(baseTopic + "status/" + safeTopic).c_str(), stateStr);
                }
              }
              
              if(userData.haAutodisc == true && adMbusMessageCounter == 3) {
                haHandoverStatus(currentSlaveName, profile);
              }
            }

            // --- 4. MAIN PUBLISH LOOP ---
            for (uint8_t i=0; i<fields; i++) {
              uint8_t code = root[i]["code"].as<int>();
              const char* name = root[i]["name"];
              const char* units = root[i]["units"];           
              double value = root[i]["value_scaled"].as<double>();
              const char* valueString = root[i]["value_string"];
              
              // Normalize
              String currentUnit = (units != NULL) ? String(units) : "";
              NormalizedValue norm = normalizeMBusValue(value, currentUnit);
              double finalValue = norm.value;
              String finalUnit = norm.unit;
              
              // APPLY PRE-SCAN OVERRIDES
              if (strcmp(name, "volume_flow") == 0) finalValue = currentFlow;
              if (strcmp(name, "power") == 0) finalValue = currentPower;

              bool telegramFollow = root[i]["telegramFollow"].as<int>();
              
              // Blacklist Filter
              if (!isRecordIgnored(profile, i + 1)) {
                if(userData.haAutodisc == true && adMbusMessageCounter == 3) {  
                  if(strlen(currentSlaveName) > 0) {
                    strcpy(adVariables.haName, name);
                    strcpy(adVariables.haUnits, finalUnit.c_str());
                    strcpy(adVariables.stateClass, payload.getStateClass(code));
                    strcpy(adVariables.deviceClass, payload.getDeviceClass(code));
                    haHandoverMbus(currentSlaveName, profile->applyEngelmannBugfix);
                  }
                } else {               
                  if(strlen(currentSlaveName) > 0) {
                    client.publish(String(baseTopic + String(name) + "_vs").c_str(), valueString);
                    client.publish(String(baseTopic + String(name)).c_str(), String(finalValue, 3).c_str());
                    client.publish(String(baseTopic + String(name) + "_unit").c_str(), finalUnit.c_str());
                  }
                }
              }

              // M-Bus Pagination Check
              if(fields == i+1) { 
                if(telegramFollow == 1) { 
                   client.publish(String(String(userData.mbusinoName) + "/MBus/SlaveAddress"+String(address)+ "/"+String(recordCounter[currentAddressCounter]+i+1)).c_str(),"--> More records follow in next telegram");
                  recordCounter[currentAddressCounter] = recordCounter[currentAddressCounter] + fields;
                  mtPolling = true;
                } else {
                  recordCounter[currentAddressCounter] = 0;
                  mtPolling = false; 
                  if(addressCounter == 1) adMbusMessageCounter++;                
                }
              }        
            } // End of Main Publish Loop

            // --- 5. ENGELMANN BUGFIX (DYNAMIC CALCULATION) ---
            if (profile->applyEngelmannBugfix && hasFlow && hasDeltaT && strlen(currentSlaveName) > 0) {
              // We only publish values during normal runs (not during Auto-Discovery)
              if (userData.haAutodisc == false || adMbusMessageCounter != 3) {
                // Formula: P (W) = Flow (m³/h) * DeltaT (K) * 1163 
                // Note: currentFlow is guaranteed to be normalized to m³/h by the Pre-Scan!
                float calc_power = currentDeltaT * currentFlow * 1163.0;          
                client.publish(String(baseTopic + "power_calc").c_str(), String(calc_power, 0).c_str());
              }
            }

            // ==============================================================================
            // --- ESP-NOW: UPDATE M-BUS METER DATA ---
            // ==============================================================================
            if (userData.espNowEnable) {
              uint8_t i = currentAddressCounter;
              
              // 1. Fabrication Number (Serial)
              espNowData.meters[i].fab_number = liveFab[i].toInt();
              
              // 2. Set Status Byte (0-7 Priority System)
              if (!liveConnected[i]) {
                espNowData.meters[i].status = 7; // Offline / Error
              } else if (dataQuality == "error") {
                espNowData.meters[i].status = 6; // Bouncer Timeout (Hard Null)
              } else if (dataQuality == "held") {
                espNowData.meters[i].status = 5; // Bouncer Hold
              } else if (limitError && hasPower && abs(currentPower) > userData.maxPower[i]) {
                espNowData.meters[i].status = 4; // Max Power Limit
              } else if (limitError && hasFlow && currentFlow > userData.maxFlow[i]) {
                espNowData.meters[i].status = 3; // Max Flow Limit
              } else if (limitError && hasFlow && currentFlow < userData.minFlow[i]) {
                espNowData.meters[i].status = 2; // Min Flow Limit
              } else if (applyDeadband) {
                espNowData.meters[i].status = 1; // Deadband Active
              } else {
                espNowData.meters[i].status = 0; // OK
              }

              // 3. Populate Values (Extraction from JSON)
              espNowData.meters[i].volume_flow = currentFlow;
              espNowData.meters[i].power = (profile->applyEngelmannBugfix && hasFlow && hasDeltaT) ? (currentDeltaT * currentFlow * 1163.0) : currentPower;
              
              // We need to loop through the parsed JSON once more to find Energy and Temps,
              // because they weren't cached in the Pre-Scan
              espNowData.meters[i].energy = 0.0;
              espNowData.meters[i].flow_temp = 0.0;
              espNowData.meters[i].return_temp = 0.0;
              
              for (uint8_t k = 0; k < fields; k++) {
                const char* n = root[k]["name"];
                if (strcmp(n, "energy") == 0) {
                  espNowData.meters[i].energy = normalizeMBusValue(root[k]["value_scaled"].as<double>(), root[k]["units"]).value;
                }
                if (strcmp(n, "flow_temperature") == 0) {
                  espNowData.meters[i].flow_temp = root[k]["value_scaled"].as<double>();
                }
                if (strcmp(n, "return_temperature") == 0) {
                  espNowData.meters[i].return_temp = root[k]["value_scaled"].as<double>();
                }
              }
              
              // If Offline/Error, wipe data to NaN to prevent false readings on Solar-Controller
              if (espNowData.meters[i].status >= 6) {
                espNowData.meters[i].energy = NAN;
                espNowData.meters[i].volume_flow = NAN;
                espNowData.meters[i].power = NAN;
                espNowData.meters[i].flow_temp = NAN;
                espNowData.meters[i].return_temp = NAN;
              }
            }
            // ==============================================================================

            address = 0;
          } // End of: if(millis() - timerMbusDecoded > 100)
          break;
      } 
  } 
} 

void heapCalc(){
  if(minFreeHeap > ESP.getFreeHeap()){
    minFreeHeap = ESP.getFreeHeap();
  }
}
