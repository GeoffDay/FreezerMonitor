//WhatsApp_test.ino This code sends messages using the whatsapp on my mobile phone. 
// the plan is to send a general freezer temp at 7 oclock morning and night and an 
// alarm if the temp exceeds zero. Currently this code connects to the internet, gets NTP time,
// finds the temp sensors and displays the temp on serial and displays time on the LCD.

#include <LiquidCrystal_I2C.h>
#include <ESP8266WiFi.h>                    // wifi 
#include <WiFiUdp.h>                        // ntp time uses this
#include <OneWire.h>                        // Temp Sensor 
#include <DallasTemperature.h>              // libraries
#include <Callmebot_ESP8266.h>              // messaging to Whats App

// initialize the library by associating any needed LCD interface pins with the arduino pin number it is connected to
LiquidCrystal_I2C lcd(0x27,16,2);  // set the LCD address to 0x27 for a 16 chars and 2 line display

//const int rs = 8, en = 7, d4 = 6, d5 = 5, d6 = 4, d7 = 3;
//LiquidCrystal lcd(rs, en, d4, d5, d6, d7);

const char* ssid = "Telstra55B601";
const char* password = "6xcrs24vng";

//*********************************Time server***********************************************
unsigned int localPort = 2390;  // local port to listen for UDP packets
IPAddress timeServerIP;         // time.nist.gov NTP server address. Don't hardwire the IP address or we won't get the benefits of the pool.
const char* ntpServerName = "time.nist.gov";
const int NTP_PACKET_SIZE = 48;       // NTP time stamp is in the first 48 bytes of the message
byte packetBuffer[NTP_PACKET_SIZE];   // buffer to hold incoming and outgoing packets
WiFiUDP udp;                          // A UDP instance to let us send and receive packets over UDP

//********************************Dallas OneWire*********************************************
#define ONE_WIRE_BUS 12               // Data wire is connected to GPIO12 on pin 6
OneWire oneWire(ONE_WIRE_BUS);        // Setup a oneWire instance to communicate with any OneWire device
DallasTemperature sensors(&oneWire);  // Pass oneWire reference to DallasTemperature library
DeviceAddress insideThermometer;      // arrays to hold device address


//*************************************WhatsApp*********************************************
//@author Hafidh Hidayat (hafidhhidayat@hotmail.com)   https://github.com/hafidhh/Callmebot_ESP8266
// apiKey : Follow instruction on https://www.callmebot.com/blog/free-api-whatsapp-messages/
String phoneNumber = "+61437325596";
String apiKey = "1848095";
String start_messsage = "Freezer Monitor has started";

int counter = 0;
char _buffer[15];
char _buffer2[10];
long timeToSeven = 86400;         // we dont know what the 

void setup() {
  lcd.init();                      // initialize the lcd 
  lcd.backlight();
  lcd.begin(16, 2);                 // set up the LCD's number of columns and rows:
  lcd.setCursor(0, 0);
  lcd.display();
	Serial.begin(115200);           // serial on 115200 baud

	WiFi.begin(ssid, password);
	Serial.println("Connecting");
  lcd.println("Connected          ");
	while(WiFi.status() != WL_CONNECTED) {
		delay(500);
		Serial.print(".");
	}

	Serial.println("");
	Serial.print("Connected to WiFi network with IP Address: ");
	Serial.println(WiFi.localIP());
  
  //************************************* Time ***************************************
  Serial.println("Starting UDP");
  udp.begin(localPort);
  Serial.print("Local port: ");
  Serial.println(udp.localPort());

  //************************************ Temp sensors ********************************
  // locate devices on the bus
  Serial.print("Locating devices...");
  sensors.begin();
  Serial.print("Found ");
  Serial.print(sensors.getDeviceCount(), DEC);
  Serial.println(" devices.");
 
  if (!sensors.getAddress(insideThermometer, 0)) Serial.println("Unable to find address for Device 0");
 
  // set the resolution to 12 bit (Each Dallas/Maxim device is capable of several different resolutions)
  sensors.setResolution(insideThermometer, 12);

  Serial.print("Device 0 Resolution: ");
  Serial.print(sensors.getResolution(insideThermometer), DEC);
  Serial.println();

  //********************************** start Call Me Bot service ***************************************
	Callmebot.begin();
	Callmebot.whatsappMessage(phoneNumber, apiKey, start_messsage);         // Whatsapp Message
	Serial.println(Callmebot.debug());

}

void loop() {
  getTime();

  sensors.requestTemperatures();    // Read and print temperatures in Celsius

  Serial.print("Temperature Sensor 1: ");
  Serial.print(sensors.getTempCByIndex(0));
  Serial.println(" °C");

  	delay(500);
		Serial.print(".");
	counter += 1;
  if (counter%100 == 0){
    Serial.print("fire");
    Serial.println(counter);
  }

if (counter%120 == 0){
  dtostrf(sensors.getTempCByIndex(0), 3, 1, _buffer2);
  sprintf(_buffer, "Temp is %s C", _buffer2);
  Callmebot.whatsappMessage(phoneNumber, apiKey, _buffer);
    Serial.print("call");
    Serial.println(counter);
  }

  
}

void getTime() {
  WiFi.hostByName(ntpServerName, timeServerIP);   // get a random server from the pool
  sendNTPpacket(timeServerIP);                    // send an NTP packet to a time server
  delay(1000);                                    // wait to see if a reply is available

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
    Serial.print("Timezone is +9.5 hours");
    epoch += 9.5 * 3600;
    
  lcd.setCursor(0, 0);
    // print the hour, minute and second:
    Serial.print("The UTC time is ");       // UTC is the time at Greenwich Meridian (GMT)
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
    lcd.println(epoch % 60);             // print the second
  }

  delay(10000);                             // wait ten seconds before asking for the time again
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
