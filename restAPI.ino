/* 
***************************************************************************  
**  Program  : restAPI
**  Version  : v0.7.1
**
**  Copyright (c) 2021 Robert van den Breemen
**     based on Framework ESP8266 from Willem Aandewiel
**
**  TERMS OF USE: MIT License. See bottom of file.                                                            
***************************************************************************      
*/


//=======================================================================
void processAPI() 
{
  char fName[40] = "";
  char URI[50]   = "";
  String words[10];

  strlcpy( URI, httpServer.uri().c_str(), sizeof(URI) );

  if (httpServer.method() == HTTP_GET)
        DebugTf("from[%s] URI[%s] method[GET] \r\n"
                                  , httpServer.client().remoteIP().toString().c_str()
                                        , URI); 
  else  DebugTf("from[%s] URI[%s] method[PUT] \r\n" 
                                  , httpServer.client().remoteIP().toString().c_str()
                                        , URI); 

  if (ESP.getFreeHeap() < 8500) // to prevent firmware from crashing!
  {
    DebugTf("==> Bailout due to low heap (%d bytes))\r\n", ESP.getFreeHeap() );
    httpServer.send(500, "text/plain", "500: internal server error (low heap)\r\n"); 
    return;
  }

  int8_t wc = splitString(URI, '/', words, 10);
  
  if (Verbose) 
  {
    DebugT(">>");
    for (int w=0; w<wc; w++)
    {
      Debugf("word[%d] => [%s], ", w, words[w].c_str());
    }
    Debugln(" ");
  }

  if (words[1] == "api"){

    if (words[2] == "v1") 
    { //v1 API calls
      if (words[3] == "otgw"){
        if (words[4] == "otmonitor") {
          // GET /api/v1/otgw/otmonitor
          // Response: see json response
          sendOTmonitor();
        } else if (words[4] == "id"){
          //what the heck should I do?
          // /api/v1/otgw/id/{msgid}   msgid = OpenTherm Message Id (0-127)
          // Response: label, value, unit
          // {
          //   "label": "Tr",
          //   "value": "0.00",
          //   "unit": "°C"
          // }
          sendOTGWvalue(words[5].toInt());  
        } else if (words[4] == "label"){
          //what the heck should I do?
          // /api/v1/otgw/label/{msglabel} = OpenTherm Label (matching string)
          // Response: label, value, unit
          // {
          //   "label": "Tr",
          //   "value": "0.00",
          //   "unit": "°C"
          // }   
          sendOTGWlabel(CSTR(words[5]));
        } else if (words[4] == "command"){
          if (httpServer.method() == HTTP_PUT || httpServer.method() == HTTP_POST)
          {
            /* how to post a command to OTGW
            ** POST or PUT = /api/v1/otgw/command/{command} = Any command you want
            ** Response: 200 OK
            ** @@Todo: Check if command was executed correctly.
            */
            //Send a command to OTGW
            sendOTGW(CSTR(words[5]), words[5].length());
            httpServer.send(200, "text/plain", "OK");
          } else sendApiNotFound(URI);
        }
        else sendApiNotFound(URI);
      }
      else sendApiNotFound(URI);
    } 
    else if (words[2] == "v0")
    { //v0 API calls
      if (words[3] == "otgw"){
        //what the heck should I do?
        // /api/v0/otgw/{msgid}   msgid = OpenTherm Message Id
        // Response: label, value, unit
        // {
        //   "label": "Tr",
        //   "value": "0.00",
        //   "unit": "°C"
        // }
        sendOTGWvalue(words[4].toInt()); 
      } 
      else if (words[3] == "devinfo")
      {
        sendDeviceInfo();
      }
      else if (words[3] == "devtime")
      {
        sendDeviceTime();
      }
      else if (words[3] == "settings")
      {
        if (httpServer.method() == HTTP_PUT || httpServer.method() == HTTP_POST)
        {
          postSettings();
        }
        else
        {
          sendDeviceSettings();
        }
      } else sendApiNotFound(URI);
    } else sendApiNotFound(URI);
  } else sendApiNotFound(URI);
} // processAPI()


//====[ implementing REST API ]====
void sendOTGWvalue(int msgid){
  StaticJsonDocument<256> doc;
  JsonObject root  = doc.to<JsonObject>();
  if (OTmap[msgid].type==ot_undef) {  //message is undefined, return error
    root["error"] = "message undefined: reserved for future use";
  } else if (msgid>= 0 && msgid<= OT_MSGID_MAX) 
  { //message id's need to be between 0 and 127
    //Debug print the values first
    DebugTf("%s = %s %s\r\n", OTmap[msgid].label, getOTGWValue(msgid).c_str(), OTmap[msgid].unit);
    //build the json
    root["label"] = OTmap[msgid].label;
    if (OTmap[msgid].type == ot_f88) {
      root["value"] = getOTGWValue(msgid).toFloat(); 
    } else {// all other message types convert to integer
      root["value"] = getOTGWValue(msgid).toInt();
    }
    root["unit"] = OTmap[msgid].unit;    
  } else {
    root["error"] = "message id: reserved for future use";
  }
  String sBuff;
  serializeJsonPretty(root, sBuff);
  //DebugTf("Json = %s\r\n", sBuff.c_str());
  //reply with json
  httpServer.sendHeader("Access-Control-Allow-Origin", "*");
  httpServer.setContentLength(CONTENT_LENGTH_UNKNOWN);
  httpServer.send(200, "application/json", sBuff);
}

void sendOTGWlabel(const char *msglabel){
  StaticJsonDocument<256> doc;
  JsonObject root  = doc.to<JsonObject>();
  int msgid;
  for (msgid = 0; msgid<= OT_MSGID_MAX; msgid++){
    if (stricmp(OTmap[msgid].label, msglabel)==0) break;
  }
  if (msgid > OT_MSGID_MAX){
    root["error"] = "message id: reserved for future use";
  } else if (OTmap[msgid].type==ot_undef) {  //message is undefined, return error
    root["error"] = "message undefined: reserved for future use";
  } else 
  { //message id's need to be between 0 and OT_MSGID_MAX
    //Debug print the values first
    DebugTf("%s = %s %s\r\n", OTmap[msgid].label, getOTGWValue(msgid).c_str(), OTmap[msgid].unit);
    //build the json
    root["label"] = OTmap[msgid].label;
    if (OTmap[msgid].type == ot_f88) {
      root["value"] = getOTGWValue(msgid).toFloat(); 
    } else {// all other message types convert to integer
      root["value"] = getOTGWValue(msgid).toInt();
    }
    root["unit"] = OTmap[msgid].unit;    
  } 
  String sBuff;
  serializeJsonPretty(root, sBuff);
  //DebugTf("Json = %s\r\n", sBuff.c_str());
  //reply with json
  httpServer.sendHeader("Access-Control-Allow-Origin", "*");
  httpServer.setContentLength(CONTENT_LENGTH_UNKNOWN);
  httpServer.send(200, "application/json", sBuff);
}

//=======================================================================
void sendOTmonitor() 
{
  DebugTln("sending OT monitor values ...\r");

  sendStartJsonObj("otmonitor");
  
  sendJsonOTmonObj("flamestatus", CONOFF(isFlameStatus()),"");
  sendJsonOTmonObj("chmodus", CONOFF(isCentralHeatingActive()),"");
  sendJsonOTmonObj("chenable", CONOFF(isCentralHeatingEnabled()),"");
  sendJsonOTmonObj("dhwmode", CONOFF(isDomesticHotWaterActive()),"");
  sendJsonOTmonObj("dhwenable", CONOFF(isDomesticHotWaterEnabled()),"");
  sendJsonOTmonObj("diagnosticindicator", CONOFF(isDiagnosticIndicator()),"");
  sendJsonOTmonObj("faultindicator", CONOFF(isFaultIndicator()),"");

  sendJsonOTmonObj("outsidetemperature", OTdataObject.Toutside, "°C");
  sendJsonOTmonObj("roomtemperatature", OTdataObject.Tr, "°C");
  sendJsonOTmonObj("roomsetpoint", OTdataObject.TrSet, "°C");
  sendJsonOTmonObj("remoteroomsetpoint", OTdataObject.TrOverride, "°C");
  sendJsonOTmonObj("relmodlvl", OTdataObject.RelModLevel,"%");
  sendJsonOTmonObj("maxrelmodlvl", OTdataObject.MaxRelModLevelSetting, "%");
  sendJsonOTmonObj("chwaterpressure", OTdataObject.CHPressure, "bar");

  sendJsonOTmonObj("boilertemperature", OTdataObject.Tboiler, "°C");
  sendJsonOTmonObj("returnwatertemperature", OTdataObject.Tret,"°C");
  sendJsonOTmonObj("controlsetpoint", OTdataObject.Tset,"°C");
  sendJsonOTmonObj("maxchwatersetpoint", OTdataObject.MaxTSet,"°C");
  sendJsonOTmonObj("dhwtemperature", OTdataObject.Tdhw,"°C");
  sendJsonOTmonObj("dhwsetpoint", OTdataObject.TdhwSet,"°C");
  sendJsonOTmonObj("oemfaultcode", OTdataObject.OEMDiagnosticCode, "");

  
  //sendJsonSettingObj("string",   settingString,   "s", sizeof(settingString)-1);
  //sendJsonSettingObj("float",    settingFloat,    "f", 0, 10,  5);
  //sendJsonSettingObj("intager",  settingInteger , "i", 2, 60);

  sendEndJsonObj();

} // sendDeviceSettings()

//=======================================================================
void sendDeviceInfo() 
{
  sendStartJsonObj("devinfo");

  sendNestedJsonObj("author", "Robert van den Breemen");
  sendNestedJsonObj("fwversion", _FW_VERSION);

  snprintf(cMsg, sizeof(cMsg), "%s %s", __DATE__, __TIME__);
  sendNestedJsonObj("compiled", cMsg);
  sendNestedJsonObj("hostname", CSTR(settingHostname));
  sendNestedJsonObj("ipaddress", WiFi.localIP().toString().c_str());
  sendNestedJsonObj("macaddress", WiFi.macAddress().c_str());
  sendNestedJsonObj("freeheap", ESP.getFreeHeap());
  sendNestedJsonObj("maxfreeblock", ESP.getMaxFreeBlockSize());
  sendNestedJsonObj("chipid", String( ESP.getChipId(), HEX ).c_str());
  sendNestedJsonObj("coreversion", String( ESP.getCoreVersion() ).c_str() );
  sendNestedJsonObj("sdkversion", String( ESP.getSdkVersion() ).c_str());
  sendNestedJsonObj("cpufreq", ESP.getCpuFreqMHz());
  sendNestedJsonObj("sketchsize", formatFloat( (ESP.getSketchSize() / 1024.0), 3));
  sendNestedJsonObj("freesketchspace", formatFloat( (ESP.getFreeSketchSpace() / 1024.0), 3));

  snprintf(cMsg, sizeof(cMsg), "%08X", ESP.getFlashChipId());
  sendNestedJsonObj("flashchipid", cMsg);  // flashChipId
  sendNestedJsonObj("flashchipsize", formatFloat((ESP.getFlashChipSize() / 1024.0 / 1024.0), 3));
  sendNestedJsonObj("flashchiprealsize", formatFloat((ESP.getFlashChipRealSize() / 1024.0 / 1024.0), 3));

  SPIFFS.info(SPIFFSinfo);
  sendNestedJsonObj("spiffssize", formatFloat( (SPIFFSinfo.totalBytes / (1024.0 * 1024.0)), 0));

  sendNestedJsonObj("flashchipspeed", formatFloat((ESP.getFlashChipSpeed() / 1000.0 / 1000.0), 0));

  FlashMode_t ideMode = ESP.getFlashChipMode();
  sendNestedJsonObj("flashchipmode", flashMode[ideMode]);
  sendNestedJsonObj("boardtype",
#ifdef ARDUINO_ESP8266_NODEMCU
     "ESP8266_NODEMCU"
#endif
#ifdef ARDUINO_ESP8266_GENERIC
     "ESP8266_GENERIC"
#endif
#ifdef ESP8266_ESP01
     "ESP8266_ESP01"
#endif
#ifdef ESP8266_ESP12
     "ESP8266_ESP12"
#endif
  );
  sendNestedJsonObj("ssid", WiFi.SSID().c_str());
  sendNestedJsonObj("wifirssi", WiFi.RSSI());
  // sendNestedJosnObj("mqttconnected", CBOOLEAN(getMQTTconnectstatus()));
//sendNestedJsonObj("uptime", upTime());

  sendNestedJsonObj("lastreset", lastReset);

  httpServer.sendContent("\r\n]}\r\n");

} // sendDeviceInfo()


//=======================================================================
void sendDeviceTime() 
{
  char actTime[50];
  
  sendStartJsonObj("devtime");
  snprintf(actTime, 49, "%04d-%02d-%02d %02d:%02d:%02d", year(), month(), day()
                                                       , hour(), minute(), second());
  sendNestedJsonObj("dateTime", actTime); 
  sendNestedJsonObj("epoch", (int)now());

  sendEndJsonObj();

} // sendDeviceTime()

//=======================================================================
void sendDeviceSettings() 
{
  DebugTln("sending device settings ...\r");

  sendStartJsonObj("settings");
  
  //sendJsonSettingObj("string",   settingString,   "s", sizeof(settingString)-1);
  //sendJsonSettingObj("float",    settingFloat,    "f", 0, 10,  5);
  //sendJsonSettingObj("intager",  settingInteger , "i", 2, 60);

  sendJsonSettingObj("hostname", CSTR(settingHostname), "s", 32);
  sendJsonSettingObj("mqttbroker", CSTR(settingMQTTbroker), "s", 32);
  sendJsonSettingObj("mqttbrokerport", settingMQTTbrokerPort, "i", 0, 65535);
  sendJsonSettingObj("mqttuser", CSTR(settingMQTTuser), "s", 32);
  sendJsonSettingObj("mqttpasswd", CSTR(settingMQTTpasswd), "s", 32);
  sendJsonSettingObj("mqtttoptopic", CSTR(settingMQTTtopTopic), "s", 15);

  sendEndJsonObj();

} // sendDeviceSettings()


//=======================================================================
void postSettings()
{
  //------------------------------------------------------------ 
  // json string: {"name":"settingInterval","value":9}  
  // json string: {"name":"settingHostname","value":"abc"}  
  //------------------------------------------------------------ 
  // so, why not use ArduinoJSON library?
  // I say: try it yourself ;-) It won't be easy
      String wOut[5];
      String wPair[5];
      String jsonIn  = CSTR(httpServer.arg(0));
      char field[25] = "";
      char newValue[101]="";
      jsonIn.replace("{", "");
      jsonIn.replace("}", "");
      jsonIn.replace("\"", "");
      int8_t wp = splitString(jsonIn.c_str(), ',',  wPair, 5) ;
      for (int i=0; i<wp; i++)
      {
        //DebugTf("[%d] -> pair[%s]\r\n", i, wPair[i].c_str());
        int8_t wc = splitString(wPair[i].c_str(), ':',  wOut, 5) ;
        //DebugTf("==> [%s] -> field[%s]->val[%s]\r\n", wPair[i].c_str(), wOut[0].c_str(), wOut[1].c_str());
        if (wOut[0].equalsIgnoreCase("name"))  strCopy(field, sizeof(field), wOut[1].c_str());
        if (wOut[0].equalsIgnoreCase("value")) strCopy(newValue, sizeof(newValue), wOut[1].c_str());
      }
      DebugTf("--> field[%s] => newValue[%s]\r\n", field, newValue);
      updateSetting(field, newValue);
      httpServer.send(200, "application/json", httpServer.arg(0));

} // postSettings()


//====================================================
void sendApiNotFound(const char *URI)
{
  httpServer.sendHeader("Access-Control-Allow-Origin", "*");
  httpServer.setContentLength(CONTENT_LENGTH_UNKNOWN);
  httpServer.send ( 404, "text/html", "<!DOCTYPE HTML><html><head>");

  strlcpy(cMsg, "<style>body { background-color: lightgray; font-size: 15pt;}", sizeof(cMsg));
  strlcat(cMsg,  "</style></head><body>", sizeof(cMsg));
  httpServer.sendContent(cMsg);

  strlcpy(cMsg, "<h1>OTGW firmware</h1><b1>", sizeof(cMsg));
  httpServer.sendContent(cMsg);

  strlcpy(cMsg, "<br>[<b>", sizeof(cMsg));
  strlcat(cMsg, URI, sizeof(cMsg));
  strlcat(cMsg, "</b>] is not a valid ", sizeof(cMsg));
  httpServer.sendContent(cMsg);
  
  strlcpy(cMsg, "</body></html>\r\n", sizeof(cMsg));
  httpServer.sendContent(cMsg);

} // sendApiNotFound()


/***************************************************************************
*
* Permission is hereby granted, free of charge, to any person obtaining a
* copy of this software and associated documentation files (the
* "Software"), to deal in the Software without restriction, including
* without limitation the rights to use, copy, modify, merge, publish,
* distribute, sublicense, and/or sell copies of the Software, and to permit
* persons to whom the Software is furnished to do so, subject to the
* following conditions:
*
* The above copyright notice and this permission notice shall be included
* in all copies or substantial portions of the Software.
*
* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
* OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
* MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
* IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
* CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT
* OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR
* THE USE OR OTHER DEALINGS IN THE SOFTWARE.
* 
****************************************************************************
*/
