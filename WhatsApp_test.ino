// WhatsApp_test.ino  This code sends messages using the whatsapp on my mobile phone. The 
// plan is to send a general freezer temp at 7 oclock morning and night and an alarm if 
// the temp exceeds zero. Need to make the system able to restart without user intevention
// so temp sensors need to be disconnected or switched of at startup. 
#include <LiquidCrystal_I2C.h>
#include <ESP8266WiFi.h>                    // wifi 
#include <WiFiUdp.h>                        // ntp time uses this
#include <OneWire.h>                        // Temp Sensor 
#include <DallasTemperature.h>              // libraries
#include <Callmebot_ESP8266.h>              // messaging to Whats App

// initialize the library by associating any needed LCD interface pins with the arduino pin number it is connected to
LiquidCrystal_I2C lcd(0x27,16,2);  // set the LCD address to 0x27 for a 16 chars and 2 line display

const char* ssid = "Airport";       //"Telstra55B601";
const char* password = "Hol_0406";  //"6xcrs24vng";


//*********************************Time server***********************************************
unsigned int localPort = 2390;        // local port to listen for UDP packets
IPAddress timeServerIP;               // time.nist.gov NTP server address. Don't hardwire the IP address or we won't get the benefits of the pool.
const char* ntpServerName = "time.nist.gov";
const int NTP_PACKET_SIZE = 48;       // NTP time stamp is in the first 48 bytes of the message
byte packetBuffer[NTP_PACKET_SIZE];   // buffer to hold incoming and outgoing packets
WiFiUDP udp;                          // A UDP instance to let us send and receive packets over UDP
long alarmGap = 0;                    // how lomg we wait between alarms

//********************************Dallas OneWire*********************************************
#define ONE_WIRE_BUS 12               // Data wire is connected to GPIO12 on pin 6
OneWire oneWire(ONE_WIRE_BUS);        // Setup a oneWire instance to communicate with any OneWire device
DallasTemperature sensors(&oneWire);  // Pass oneWire reference to DallasTemperature library
DeviceAddress deviceAddress;          // arrays to hold device address
const char *sensorNames[3] = {"Freezer1", "Freezer2", "Coolroom"};   // names to make string creation easier
const char *shortSensorNames[3] = {"Frzr1", "Frzr2", "Clrm"};   // names to make string creation easier
char sensorTemps[3][6];               // and an array of strings for the converted temp values

//*************************************WhatsApp**********************************************
//@author Hafidh Hidayat (hafidhhidayat@hotmail.com)   https://github.com/hafidhh/Callmebot_ESP8266
// apiKey : Follow instruction on https://www.callmebot.com/blog/free-api-whatsapp-messages/
String phoneNumber = "+61437325596";
String apiKey = "1848095";
char cmbMessage[60] = "";

int counter = 0;
char _buffer[45];
char _buffer2[10];
unsigned long millisToSeven = 43200000UL;   // half a day  
unsigned long nextStatus = 0;
int numberOfSensors = 0;


void setup() {
  lcd.init();                     // initialize the lcd 
  lcd.backlight();
  lcd.begin(16, 2);               // set up the LCD's number of columns and rows:
  lcd.display();

	Serial.begin(115200);           // serial on 115200 baud

	WiFi.begin(ssid, password);
	Serial.println("Connecting");
	while(WiFi.status() != WL_CONNECTED) {
		delay(500);
		Serial.print(".");
	}

	Serial.println("");
	Serial.print("Connected to WiFi network with IP Address: ");
	Serial.println(WiFi.localIP());
  sprintf(_buffer, "%s", ssid);
  displayOnLCD("Connected", _buffer, 2000);

  //************************************* Time ***************************************
  Serial.println("Starting UDP");
  displayOnLCD(0,"Getting NTP time", 1000);
  udp.begin(localPort);
  Serial.print("Local port: ");
  Serial.println(udp.localPort());

  millisToSeven = getTime();
  Serial.print(millisToSeven);
  Serial.println("milliseconds to 7 oclock");
  nextStatus = millis() + millisToSeven;

  //************************************ Temp sensors ********************************
  // locate devices on the bus
  Serial.println("Locating devices...");
  sensors.begin();
  numberOfSensors = sensors.getDeviceCount();

  lcd.setCursor(0,0);
  if (!sensors.getAddress(deviceAddress, 0)){
    displayOnLCD(0,"Cant find Sensors!", 2000);
  } else {
    sprintf(_buffer, "%s %d %s", "Found", numberOfSensors, "Sensors.");
    displayOnLCD(0, _buffer, 2000);


    for(int i=0;i<numberOfSensors; i++){          // Loop through each device, print out address      
      if(sensors.getAddress(deviceAddress, i)){// Search the wire for address
        Serial.print("Found device ");
        Serial.print(i, DEC);
         
        Serial.println();
        Serial.print(sensorNames[i]);
        Serial.print(" ");
        Serial.print(sensors.getTempC(deviceAddress));
        Serial.println("C.");
      } else {
        Serial.print("Found ghost device at ");
        Serial.print(i, DEC);
        Serial.print("but could not detect address. Check power and cabling");
      }


      delay(1000);
    }
  }

  sensors.setResolution(deviceAddress, 12);// set the resolution to 12 bit
  delay(2000);
    
  lcd.setCursor(0,0);   

  Serial.print("Device 0 Resolution: ");
  Serial.print(sensors.getResolution(deviceAddress), DEC);
  Serial.println();
  sensors.requestTemperatures();    // Request all on the bus to perform a temp conversion


  //********************************** start Call Me Bot service ***************************************
  displayOnLCD("Setting up", "WhatsApp service", 2000);
  lcd.clear();

  Callmebot.begin();
  strcpy(cmbMessage, "Startup.");
  for(int i=0;i<numberOfSensors; i++){          // Loop through each device, print out address      
    dtostrf(sensors.getTempCByIndex(i), 3, 1, sensorTemps[i]); 
    sprintf(_buffer, " %s: %sC", shortSensorNames[i], sensorTemps[i]);
    strcat(cmbMessage, _buffer);
  }

  strcat(cmbMessage, ".");
  Serial.println(cmbMessage);

  Callmebot.whatsappMessage(phoneNumber, apiKey, cmbMessage);	
	displayOnLCD(0, Callmebot.debug(), 2000);
}

void loop() 
{
  sensors.requestTemperatures();    // Request all on the bus to perform a temp conversion

  for(int i=0;i<numberOfSensors; i++){          // Loop through each device, print out temps 
    float thisTemp = sensors.getTempCByIndex(i);
    char *alarmTxt;
    
    dtostrf(thisTemp, 3, 1, sensorTemps[i]);    // convert float to a string 
    sprintf(_buffer, "%s: %sC", sensorNames[i], sensorTemps[i]);  // create the string
    
    if (thisTemp > 0.0) {
      alarmTxt = "OverTemp"; 
      strcpy(cmbMessage, "OverTemp!!! "); // alarm message

      if (alarmGap-- == 0) {
        strcat(cmbMessage, _buffer);
        Callmebot.whatsappMessage(phoneNumber, apiKey, cmbMessage);
        alarmGap = 300;           // with the 2 second delay this gives 10 mins between alarm messages
      }

    } else {
      alarmTxt = " ";             // no alarm
      alarmGap = 0;
    }

    displayOnLCD(_buffer, alarmTxt, 2000);      // display on LCD
   }
   
  if (millis() > nextStatus){ 
    strcpy(cmbMessage, "");

    for(int i=0;i<numberOfSensors; i++){          // Loop through each device, print out address      
      dtostrf(sensors.getTempCByIndex(i), 3, 1, sensorTemps[i]); 
      sprintf(_buffer, " %s: %sC", shortSensorNames[i], sensorTemps[i]);
      strcat(cmbMessage, _buffer);
    }

  Callmebot.whatsappMessage(phoneNumber, apiKey, cmbMessage);
  millisToSeven = getTime();
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
    Serial.println("no packet yet");
  } else {
    Serial.print("packet received, length=");
    Serial.println(cb);
  
    udp.read(packetBuffer, NTP_PACKET_SIZE);                            // We've received a packet, read the data into the buffer

    unsigned long highWord = word(packetBuffer[40], packetBuffer[41]);  // the timestamp starts at byte 40 of the received packet and is four bytes,
    unsigned long lowWord = word(packetBuffer[42], packetBuffer[43]);   //  or two words, long. First, esxtract the two words:

    // combine the four bytes (two words) into a long integer this is NTP time (seconds since Jan 1 1900):
    unsigned long secsSince1900 = highWord << 16 | lowWord;
    Serial.print("Seconds since Jan 1 1900 = ");
    Serial.println(secsSince1900);

    
    Serial.print("Unix time = ");           	                          // now convert NTP time into everyday time:
    const unsigned long seventyYears = 2208988800UL;                    // Unix time starts on Jan 1 1970. In seconds, that's 2208988800:
    unsigned long epoch = secsSince1900 - seventyYears;                 // subtract seventy years:
    Serial.println(epoch);                                              // print Unix time:
    Serial.print("Timezone is +9.5 hours.");
    epoch += 9.5 * 3600;

lcd.setCursor(0, 0);
lcd.clear();
    // sprintf(_buffer, "")
    Serial.print(". Day of week ");
    Serial.print(((epoch/86400) + 4)%7);
    Serial.print(". ");
    unsigned long dailySeconds = epoch % 86400;
 

    if (dailySeconds < 25200) {
      secondsToSeven = 25200 - dailySeconds;
    } else if (dailySeconds < 68400){
      secondsToSeven = 68400 - dailySeconds; 
    } else {
      secondsToSeven = 86400 - dailySeconds + 25200;
    }
    Serial.print("Seconds to Seven = ");
    Serial.println(secondsToSeven);
    
    // print the hour, minute and second:
    Serial.print(". The UTC time is ");       // UTC is the time at Greenwich Meridian (GMT)
    Serial.print((epoch % 86400L) / 3600);  // print the hour (86400 equals secs per day)
    lcd.print((epoch % 86400L) / 3600);  // print the hour (86400 equals secs per day)
    Serial.print(':');
    lcd.print(':');
    if (((epoch % 3600) / 60) < 10) {
      Serial.print('0');                    // In the first 10 minutes of each hour, we'll want a leading '0'
      lcd.print('0');                    // In the first 10 minutes of each hour, we'll want a leading '0'
    }
    Serial.print((epoch % 3600) / 60);      // print the minute (3600 equals secs per minute)
    lcd.print((epoch % 3600) / 60);      // print the minute (3600 equals secs per minute)
    Serial.print(':');
    lcd.print(':');
    if ((epoch % 60) < 10) {
      Serial.print('0');                    // In the first 10 seconds of each minute, we'll want a leading '0'
      lcd.print('0');                    // In the first 10 seconds of each minute, we'll want a leading '0'
    }
    Serial.println(epoch % 60);             // print the second
    lcd.print(epoch % 60);             // print the second
  }

  delay(10000);                             // wait ten seconds before asking for the time again
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
