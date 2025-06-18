// WhatsApp_test.ino  This code sends messages using the whatsapp on my mobile phone. The 
// plan is to send a general freezer temp at 7 oclock morning and night and an alarm if 
// the temp exceeds zero. Need to make the system able to restart without user intevention
// so temp sensors need to be disconnected or switched of at startup. 
// also sends an alarm if temp of any sensor exceeds zero or if the temp changes more than 
// 1.0 degrees in an hour 28 Dec 2024. 
// May be better to find out what the minimum temps are and give a couple of warnings when 
// various thresholds are reached. 29 Dec 2024. 
// Rejigged wifi to check connection before sending data. Added coolroom that works at 0 to 5C. 
// Appears to work OK now but haven't had coolroom operating yet. 8 Jan 2024.  

#include <LiquidCrystal_I2C.h>
// #include <ESP8266WiFi.h>                 // wifi
#include <ESP8266WiFiMulti.h>               // setup for both networks
#include <WiFiUdp.h>                        // ntp time uses this
#include <OneWire.h>                        // Temp Sensor 
#include <DallasTemperature.h>              // libraries
#include <Callmebot_ESP8266.h>              // messaging to Whats App

// initialize the library by associating any needed LCD interface pins with the arduino pin number it is connected to
LiquidCrystal_I2C lcd(0x27,16,2);  // set the LCD address to 0x27 for a 16 chars and 2 line display

ESP8266WiFiMulti wifiMulti;
const uint32_t connectTimeoutMs = 10000;      // WiFi connect timeout per AP. Increase when connecting takes longer.

// static const char _days_short[7][4] = {"Sun","Mon","Tue","Wed","Thu","Fri","Sat"};
// static const char _months_short[12][4] = {"Jan","Feb","Mar","Apr","May","Jun","Jul","Aug","Sep","Oct","Nov","Dec"};
// static const uint8_t _monthLength[2][12] = {
//     {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31},
//     {31, 29, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31}}; // Leap year

//*********************************Time server***********************************************
unsigned int localPort = 2390;        // local port to listen for UDP packets
IPAddress timeServerIP;               // time.nist.gov NTP server address. Don't hardwire the IP address or we won't get the benefits of the pool.
const char* ntpServerName = "time.nist.gov";
const int NTP_PACKET_SIZE = 48;       // NTP time stamp is in the first 48 bytes of the message
byte packetBuffer[NTP_PACKET_SIZE];   // buffer to hold incoming and outgoing packets
WiFiUDP udp;                          // A UDP instance to let us send and receive packets over UDP

//********************************Dallas OneWire*********************************************
#define ONE_WIRE_BUS 12               // Data wire is connected to GPIO12 on pin 6
OneWire oneWire(ONE_WIRE_BUS);        // Setup a oneWire instance to communicate with any OneWire device
DallasTemperature sensors(&oneWire);  // Pass oneWire reference to DallasTemperature library
DeviceAddress deviceAddress;          // arrays to hold device address

int numberOfSensors = 0;              // there will be two freezers and possibly a cool room for some events. 
const char *sensorNames[3] = {"Freezer1", "Freezer2", "Coolroom"};   // names to make string creation easier
const char *shortSensorNames[3] = {"Frzr1", "Frzr2", "Clrm"};   // names to make string creation shorter
char sensorTemps[3][6];               // and an array of strings for the converted temp values
float sensorMinimums[3];              // and minimum values so that we can set 

bool overTempAlarm = false;           // set this true if any of the sensors go overtemp
bool risingTempAlarm = false;         // set this true if the rate of rise is too high.
bool sensorFault = false;             // set true if sensor temp is -127.0, what you get if the sensor disconnected 
long alarmGap = 0;                    // how lomg we wait between alarms
char alarmTxt[20] = "";

//*************************************WhatsApp**********************************************
//@author Hafidh Hidayat (hafidhhidayat@hotmail.com)   https://github.com/hafidhh/Callmebot_ESP8266
// apiKey : Follow instruction on https://www.callmebot.com/blog/free-api-whatsapp-messages/
String phoneNumber = "+61437325596";
String apiKey = "1848095";

char cmbMessage[80] = "";
char _buffer[40];
char _buffer2[40];

unsigned long millisToSeven = 43200000UL;   // half a day  
unsigned long nextStatus = 0;
unsigned long previousMillis = 0UL;
// unsigned long delayInterval = 1000UL;



void setup() {
  lcd.init();                     // initialize the lcd 
  lcd.backlight();
  lcd.begin(16, 2);               // set up the LCD's number of columns and rows:
  lcd.display();

  displayOnLCD("Freezer Monitor", "Ver 250105", 2000);

	Serial.begin(115200);           // serial on 115200 baud

  WiFi.mode(WIFI_STA);            // Set in station mode

  // Register multi WiFi networks
  wifiMulti.addAP("Telstra55B601", "6xcrs24vng");
  wifiMulti.addAP("Airport", "Hol_0406");
  wifiMulti.addAP("realme 5", "evaevaeva");

  while (wifiMulti.run(connectTimeoutMs) != WL_CONNECTED) {
		delay(500);
		lcd.print(".");
	}
  
  sprintf(_buffer, "%s", WiFi.SSID().c_str());
  displayOnLCD("Connected", _buffer, 2000);

  //************************************* Time ***************************************
  displayOnLCD("Starting UDP", "Getting NTP time", 1000);
  udp.begin(localPort);

  millisToSeven = getTime();
  sprintf(_buffer, "%ld ms", millisToSeven);
  displayOnLCD(_buffer, "to 7 oclock", 2000);

  nextStatus = millis() + millisToSeven;

  //************************************ Temp sensors ********************************
  sensors.begin();        // locate devices on the bus
  numberOfSensors = sensors.getDeviceCount();
  sensors.requestTemperatures();    // Request all on the bus to perform a temp conversion

  if (!sensors.getAddress(deviceAddress, 0)){
    displayOnLCD(0,"Cant find Sensors!", 2000);
  } else {
    sprintf(_buffer, "%s %d %s", "Found", numberOfSensors, "Sensors.");
    displayOnLCD(0, _buffer, 2000);

    for(int i=0;i<numberOfSensors; i++){          // Loop through each device, print out temps      
      if(sensors.getAddress(deviceAddress, i)) {  // Search the wire for address
        sprintf(_buffer, "found sensor %d", i);
        dtostrf(sensors.getTempC(deviceAddress), 3, 1, sensorTemps[i]); 
        sprintf(_buffer2, "%s: %sC", sensorNames[i], sensorTemps[i]);
        displayOnLCD(_buffer, _buffer2, 2000);
      } else {
        sprintf(_buffer, "ghost device at %d", i);
        displayOnLCD(_buffer, "couldn't get add", 2000);
      }
    }
  }

  sensors.setResolution(deviceAddress, 12);       // set the resolution to 12 bit
  sensors.requestTemperatures();                  // Request all on the bus to perform a temp conversion

  //********************************** start Call Me Bot service ***************************************
  displayOnLCD("Setting up", "WhatsApp service", 2000);

  Callmebot.begin();
  strcpy(cmbMessage, "Start.");
  for(int i=0;i<numberOfSensors; i++){          // Loop through each device, print out address      
    float thisTemp = sensors.getTempCByIndex(i);
    dtostrf(thisTemp, 3, 1, sensorTemps[i]);    // convert to a char array -> sensortemps  
    sprintf(_buffer, " %s: %sC", shortSensorNames[i], sensorTemps[i]); // create char array of names and temps
    strcat(cmbMessage, _buffer);                // append that to the cmb message
    sensorMinimums[i] = thisTemp;               // save the current temps as the minimum first time through
  }

  strcat(cmbMessage, ".");                      // add a full stop
 
  Callmebot.whatsappMessage(phoneNumber, apiKey, cmbMessage);	 // and send the message
	displayOnLCD(0, Callmebot.debug(), 2000);     // display the response
  
  displayOnLCD("Message Sent", "not Disconnecting", 1000);
  //WiFi.disconnect();
}



void loop() {
  sensors.requestTemperatures();              // Request all on the bus to perform a temp conversion

  overTempAlarm = false;                      // reset each time through the loop 
  risingTempAlarm = false;
  sensorFault = false;
  strcpy(alarmTxt, "");                       // set this to null too.

  for(int i=0;i<numberOfSensors; i++) {       // Loop through each temp sensor 
    float thisTemp = sensors.getTempCByIndex(i);// get the flaot value
    
    dtostrf(thisTemp, 3, 1, sensorTemps[i]);  // convert float to a string -> sensorTemps 
    sprintf(_buffer, "%s: %sC", sensorNames[i], sensorTemps[i]);  // create the string for the whatsapp message

    if (i == 2) {                             // cool room only
      if (thisTemp > 6.0) {                   // assume 5C is the operating temp
        overTempAlarm = true;                 // set the over temp flag
        strcpy(alarmTxt, "OverTemp! ");       // we're too Hot!!!!
      } 
    } else {
      if (thisTemp > (sensorMinimums[i] / 4)) { // Freezers are below zero so /4 or /2 temps are warmer. 
        overTempAlarm = true;                   // set the over temp flag
        strcpy(alarmTxt, "Over Temp! ");         // we're Melting Now!!!!
      } else if (thisTemp > (sensorMinimums[i] / 2)) {
        risingTempAlarm = true;                 // or the rising temp flag
        strcpy(alarmTxt, "Rising Temp! ");      // alarm message
      }
    }
    
    if (thisTemp == -127.0) { 
      sensorFault = true;
      strcpy(alarmTxt, "Sensor Fault! ");       // We've been disconnected
    } else {
      if (thisTemp < sensorMinimums[i]) sensorMinimums[i] = thisTemp; // record the minimums in sensor is OK  
    }	    
    displayOnLCD(_buffer, alarmTxt, 1000);    // display on LCD
  }

  if ((overTempAlarm) || (risingTempAlarm) || (sensorFault)) { // we have an alarm so create a message
    strcpy(cmbMessage, alarmTxt);             // stick the alarm message into the CallMeBot message

    if (alarmGap % 2 == 0) lcd.backlight();   // backlight on.
    if (alarmGap % 2 == 1) lcd.noBacklight(); // backlight off.

    if (alarmGap-- < 1) {
      for(int i=0;i<numberOfSensors; i++) {     // Loop through each temp sensor 
        sprintf(_buffer, "%s: %sC ", shortSensorNames[i], sensorTemps[i]);  // create the string for the whatsapp message
        strcat(cmbMessage, _buffer);            // and append it to the CallMeBot message
      }

      displayOnLCD(0, "Connecting", 2000);

	    if (wifiMulti.run(connectTimeoutMs) == WL_CONNECTED) {
        sprintf(_buffer, "%s", WiFi.SSID().c_str());
        displayOnLCD("Connected", _buffer, 1000);
        
        Callmebot.whatsappMessage(phoneNumber, apiKey, cmbMessage); // send a whatsapp message
        alarmGap = 600 / numberOfSensors;       // with the 1 second delay this gives 10 mins between alarm messages
        displayOnLCD("Message Sent", "not Disconnecting", 1000);
        //WiFi.disconnect();
      }
    }
  } else {
      alarmGap = 0;                           // if no alarms set gap to zero
      lcd.backlight();                        // make sure you finish with the backlight on.
  }
     
   
  if (millis() > nextStatus){ 
    strcpy(cmbMessage, "Current: ");

    for(int i=0;i<numberOfSensors; i++){      // Loop through each device     
      dtostrf(sensors.getTempCByIndex(i), 3, 1, sensorTemps[i]);  // convert to char array 
      sprintf(_buffer, " %s: %sC", shortSensorNames[i], sensorTemps[i]);  // combine with names
      strcat(cmbMessage, _buffer);  // append
    }

    strcat(cmbMessage, "Minimums: ");
    
    for(int i=0;i<numberOfSensors; i++){      // Loop through each device     
      dtostrf(sensorMinimums[i], 3, 1, sensorTemps[i]); 
      sprintf(_buffer, " %s: %sC", shortSensorNames[i], sensorTemps[i]);
      strcat(cmbMessage, _buffer);
    }

    displayOnLCD(0, "Connecting", 2000);

    if (wifiMulti.run(connectTimeoutMs) == WL_CONNECTED) {
      sprintf(_buffer, "%s", WiFi.SSID().c_str());
      displayOnLCD("Connected", _buffer, 1000);
      
      Callmebot.whatsappMessage(phoneNumber, apiKey, cmbMessage); // send a whatsapp message
      millisToSeven = getTime();

      displayOnLCD("Message Sent", "not Disconnecting", 1000);
      //WiFi.disconnect();
    }

    Serial.print(millisToSeven);                      
    Serial.println(" milliseconds to 7 oclock.");
    nextStatus = millis() + millisToSeven;
  }
}



unsigned long getTime() {
  WiFi.hostByName(ntpServerName, timeServerIP);   // get a random server from the pool
  sendNTPpacket(timeServerIP);                    // send an NTP packet to a time server
  delay(1000);                                    // wait to see if a reply is available
  
  unsigned long secondsToSeven = 0;
  int cb = udp.parsePacket();

  if (!cb) {
    displayOnLCD(0, "no packet yet", 1000);
  } else {
    displayOnLCD(0, "packet received", 1000);
    Serial.println(cb);
  
    udp.read(packetBuffer, NTP_PACKET_SIZE);                            // We've received a packet, read the data into the buffer

    unsigned long highWord = word(packetBuffer[40], packetBuffer[41]);  // the timestamp starts at byte 40 of the received packet and is four bytes,
    unsigned long lowWord = word(packetBuffer[42], packetBuffer[43]);   //  or two words, long. First, esxtract the two words:

    // combine the four bytes (two words) into a long integer this is NTP time (seconds since Jan 1 1900):
    unsigned long secsSince1900 = highWord << 16 | lowWord;
    Serial.print("Seconds since Jan 1 1900 = ");
    Serial.println(secsSince1900);

    // now convert NTP time into everyday time:
    const unsigned long seventyYears = 2208988800UL;                    // Unix time starts on Jan 1 1970. In seconds, that's 2208988800:
    unsigned long epoch = secsSince1900 - seventyYears;                 // subtract seventy years:
        
    sprintf(_buffer, "Unix: %ld", epoch);       // print Unix time:
    sprintf(_buffer2, "Timezone +9.5 hrs.");    // show timezone
    displayOnLCD(_buffer, _buffer2, 2000);
    epoch += 9.5 * 3600;                        // adjust for timezone

    sprintf(_buffer, "Day of week %ld.", ((epoch / 86400) + 4) % 7);
    displayOnLCD(0, _buffer, 2000);

    unsigned long dailySeconds = epoch % 86400;
 
    if (dailySeconds < 25200) {
      secondsToSeven = 25200 - dailySeconds;
    } else if (dailySeconds < 68400){
      secondsToSeven = 68400 - dailySeconds; 
    } else {
      secondsToSeven = 86400 - dailySeconds + 25200;
    }
   
    // print the hour, minute and second:
    sprintf(_buffer, "%ld:", (epoch % 86400L) / 3600);    // print the hour (86400 equals secs per day)

    if (((epoch % 3600) / 60) < 10) strcat(_buffer, "0"); // In the first 10 minutes of each hour, we'll want a leading '0'
    
    sprintf(_buffer2, "%ld:", (epoch % 3600) / 60);       // print the minute (3600 equals secs per minute)
    strcat(_buffer, _buffer2);                            // join the strings into _buffer

    if ((epoch % 60) < 10) strcat(_buffer, "0");          // In the first 10 seconds of each minute, we'll want a leading '0'
 
    sprintf(_buffer2, "%ld",epoch % 60);                  // print the second
    strcat(_buffer, _buffer2);                            // join the strings into _buffer

    displayOnLCD(0, _buffer, 2000);
  }
  return secondsToSeven * 1000;
}




void displayOnLCD(int row, String charArray, int dDelay)
{
  lcd.clear();
  lcd.setCursor(0, row); 
  lcd.print(charArray);
  delay(dDelay);
}


void displayOnLCD(String charArray, String char_Array, int dDelay)
{
  lcd.clear();
  lcd.setCursor(0, 0); 
  lcd.print(charArray);
  lcd.setCursor(0, 1); 
  lcd.print(char_Array);
  delay(dDelay);
}


// send an NTP request to the time server at the given address
void sendNTPpacket(IPAddress& address) {
  Serial.println("sending NTP packet...");
  memset(packetBuffer, 0, NTP_PACKET_SIZE);   // set all bytes in the buffer to 0
  
  // Initialize values needed to form NTP request (see URL above for details on the packets)
  packetBuffer[0] = 0b11100011;  // LI, Version, Mode
  packetBuffer[1] = 0;           // Stratum, or type of clock
  packetBuffer[2] = 6;           // Polling Interval
  packetBuffer[3] = 0xEC;        // Peer Clock Precision
  // 8 bytes of zero for Root Delay & Root Dispersion
  packetBuffer[12] = 49;
  packetBuffer[13] = 0x4E;
  packetBuffer[14] = 49;
  packetBuffer[15] = 52;

  // all NTP fields have been given values, now you can send a packet requesting a timestamp:
  udp.beginPacket(address, 123);  // NTP requests are to port 123
  udp.write(packetBuffer, NTP_PACKET_SIZE);
  udp.endPacket();
}
