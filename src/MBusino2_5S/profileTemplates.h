#ifndef PROFILE_TEMPLATES_H
#define PROFILE_TEMPLATES_H

// --- 1. GENERIC PROFILE (The Ultimate Manual & Blank Canvas) ---
const char profile_generic_json[] PROGMEM = R"rawliteral({
  "_meta": {
    "schema_version": 100,
    "profile_type": "system",
    "profile_version": 1,
    "_help_filename": "Naming convention: Use only A-Z, a-z, 0-9, underscores (_), and hyphens (-). NO spaces or special characters! Multiple profiles per manufacturerID are explicitly allowed (e.g., LUG_T230_mp.json, LUG_T350_mp.json).",
    "_help_info": "Generic profile. Use this deeply documented file as a master template for creating custom profiles."
  },
  "manufacturerID": "GEN",
  "profileName": "Generic Standard",
  "_help_bugfix": "Set to true if the meter (e.g., Engelmann) does not deliver power values. The MBusino will then calculate power manually via Flow * DeltaT * 1163.",
  "applyEngelmannBugfix": false,
  "_help_ignore": "Blacklist for garbage records (1-based index). E.g., [2, 5]. Max 35 entries allowed. Set to [] to process all records.",
  "ignoreRecords": [],
  "_help_customStates_intro": "HOW TO USE THE CUSTOM STATUS MAPPING: The M-Bus Status-Byte is 8 Bits. Bits 0-4 are standard. Bits 5, 6, and 7 are manufacturer specific!",
  "_help_customStates_masks": "BITMASKS & SHIFTS: Bit 5 only: Mask 32 (0x20) | Shift 5. Bit 6 only: Mask 64 (0x40) | Shift 6. Bit 7 only: Mask 128 (0x80) | Shift 7.",
  "_help_customStates_combos": "COMBINED BITS: Bits 5+6: Mask 96 (0x60) | Shift 5. Bits 6+7: Mask 192 (0xC0) | Shift 6. Bits 5+6+7: Mask 224 (0xE0) | Shift 5.",
  "_help_customStates_states": "STATES ARRAY: Up to 8 mapped string values depending on the bitmask. If the array is shorter than the possible states, undefined states default to 'Unknown'.",
  "customStates": [
    {
      "haName": "example_single_flag",
      "bitMask": 32,
      "bitShift": 5,
      "states": ["Normal", "Flag Set"]
    },
    {
      "haName": "example_combined_flag",
      "bitMask": 192,
      "bitShift": 6,
      "states": ["State 0", "State 1", "State 2", "State 3"]
    }
  ]
})rawliteral";


// --- 2. LANDIS+GYR PROFILE (T230 / Ultraheat 50) ---
const char profile_lug_json[] PROGMEM = R"rawliteral({
  "_meta": {
    "schema_version": 100,
    "profile_type": "system",
    "profile_version": 1,
    "_help_info": "Standard profile for Landis+Gyr meters (T230, T350, etc.)."
  },
  "manufacturerID": "LUG", 
  "profileName": "Landis+Gyr Standard",
  "applyEngelmannBugfix": false,
  "_help_ignore": "Ignores timers (1, 2, 11) and archive/key date data (15-34) with broken timestamps.",
  "ignoreRecords": [1, 2, 11, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33, 34],
  "customStates": [
    {
      "haName": "negative_power",
      "bitMask": 32,
      "bitShift": 5,
      "states": ["Normal", "Negative Power"]
    },
    {
      "haName": "negative_flow",
      "bitMask": 64,
      "bitShift": 6,
      "states": ["Normal", "Negative Flow"]
    },
    {
      "haName": "negative_deltat",
      "bitMask": 128,
      "bitShift": 7,
      "states": ["Normal", "Negative Delta T"]
    }
  ]
})rawliteral";


// --- 3. ENGELMANN PROFILE (Sensostar) ---
const char profile_engelmann_json[] PROGMEM = R"rawliteral({
  "_meta": {
    "schema_version": 100,
    "profile_type": "system",
    "profile_version": 1,
    "_help_info": "Profile for Engelmann Sensostar meters (includes power bugfix)."
  },
  "manufacturerID": "EFE",
  "profileName": "Engelmann Sensostar",
  "applyEngelmannBugfix": true,
  "ignoreRecords": [],
  "customStates": []
})rawliteral";

#endif
