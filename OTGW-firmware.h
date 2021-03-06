/* 
***************************************************************************  
**  Program  : OTGW-firmware.h
**  Version  : v0.7.1
**
**  Copyright (c) 2021 Robert van den Breemen
**
**  TERMS OF USE: MIT License. See bottom of file.                                                            
***************************************************************************      
*/

#include <ezTime.h>             // https://github.com/ropg/ezTime
#include <TelnetStream.h>       // https://github.com/jandrassy/TelnetStream/commit/1294a9ee5cc9b1f7e51005091e351d60c8cddecf
#include <ArduinoJson.h>        // https://arduinojson.org/
#include "Wire.h"
#include "Debug.h"
#include "safeTimers.h"
#include "networkStuff.h"
#include "OTGW-Core.h"

//Defaults and macro definitions
#define _HOSTNAME   "OTGW"
#define SETTINGS_FILE   "/settings.ini"
#define CMSG_SIZE 512
#define JSON_BUFF_MAX   1024
#define CSTR(x) x.c_str()
#define CBOOLEAN(x) (x?"True":"False")
#define CONOFF(x) (x?"On":"Off")

//Global variables
WiFiClient  wifiClient;
bool        Verbose = false;
char        cMsg[CMSG_SIZE];
char        fChar[10];
String      lastReset   = "";
uint32_t    upTimeSeconds=0;
uint32_t    rebootCount=0;
Timezone    CET; 

const char *weekDayName[]  {  "Unknown", "Zondag", "Maandag", "Dinsdag", "Woensdag", "Donderdag", "Vrijdag", "Zaterdag", "Unknown" };
const char *flashMode[]    { "QIO", "QOUT", "DIO", "DOUT", "Unknown" };


//All things that are settings 
String    settingHostname = _HOSTNAME;
//MQTT settings
String    settingMQTTbroker= "192.168.88.254";
int16_t   settingMQTTbrokerPort = 1883; 
String    settingMQTTuser = "";
String    settingMQTTpasswd = "";
String    settingMQTTtopTopic = "OTGW";

// That's all folks...
