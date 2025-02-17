#ifndef DATASTRUCTS_WIFI_AP_CANDIDATES_H
#define DATASTRUCTS_WIFI_AP_CANDIDATES_H

#include "../../ESPEasy_common.h"

#include "../DataStructs/MAC_address.h"
struct WiFi_AP_Candidate {
  // Construct from stored credentials
  // @param index  The index of the stored credentials
  // @param ssid_c SSID of the credentials
  // @param pass   Password/key of the credentials
  WiFi_AP_Candidate(byte          index,
                    const String& ssid_c,
                    const String& pass);


  // Construct using index from WiFi scan result
  WiFi_AP_Candidate(uint8_t networkItem);
  #ifdef ESP8266
  WiFi_AP_Candidate(const bss_info& ap);
  #endif


  WiFi_AP_Candidate(const WiFi_AP_Candidate& other) = default;

  // Default constructor
  WiFi_AP_Candidate();

  // Return true when this one is preferred over 'other'.
  bool               operator<(const WiFi_AP_Candidate& other) const;

  bool               operator==(const WiFi_AP_Candidate& other) const;

  WiFi_AP_Candidate& operator=(const WiFi_AP_Candidate& other) = default;

  // Check if the candidate data can be used to actually connect to an AP.
  bool               usable() const;

  // Check if the candidate was recently seen
  bool               expired() const;

  // For quick connection the channel and BSSID are needed
  bool               allowQuickConnect() const;

  // Check to see if the BSSID is set
  bool               bssid_set() const;

  bool               bssid_match(const uint8_t bssid_c[6]) const;
  bool               bssid_match(const MAC_address& other) const;

  // Create a formatted string
  String             toString(const String& separator = " ") const;

  String             encryption_type() const;

  String  ssid;
  String  key;

  unsigned long last_seen = 0;
  int32_t rssi     = 0;
  int32_t channel  = 0;
  MAC_address bssid;
  byte    index    = 0;     // Index of the matching credentials
  byte    enc_type = 0;     // Encryption used (e.g. WPA2)
  bool    isHidden = false; // Hidden SSID
  bool    lowPriority = false; // Try as last attempt
  bool    isEmergencyFallback = false;
};

#endif // ifndef DATASTRUCTS_WIFI_AP_CANDIDATES_H
