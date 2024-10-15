#include <LiquidCrystal.h>
#include <SPI.h>
#include <LoRa.h>
 
// initialize the library by associating any needed LCD interface pinc with the arduino pin number it is connected to
const int rs = 8, en = 7, d4 = 6, d5 = 5, d6 = 4, d7 = 3;
LiquidCrystal lcd(rs, en, d4, d5, d6, d7);
int count = 0;
char msg[10];
char *str;
char **endptr;
int t1 = 0;
int t2 = 0;
int t3 = 0;
float temp1 = 0;
float temp2 = 0;
float temp3 = 0;
bool trend1 = false;
bool trend2 = false;
bool trend3 = false;
bool ot1 = false;
bool ot2 = false;
bool ot3 = false;
bool fault1 = false;
bool fault2 = false;
bool fault3 = false;

void setup() 
{
  lcd.begin(16, 2);                 // set up the LCD's number of columns and rows:
  lcd.print("Reciever");       // Print a message to the LCD.
  
  Serial.begin(9600);
  while (!Serial);
  Serial.println("LoRa Receiver");
  if (!LoRa.begin(433E6)) {
    Serial.println("Starting LoRa failed!");
    while (1);
  }
}
 
void loop() {
  // try to parse packet
  int packetSize = LoRa.parsePacket();
  if (packetSize) 
  {
    // received a paket
    Serial.println("");
    Serial.println("...................................");
    Serial.println("Received packet: ");
    lcd.setCursor(0, 1);
    // read packet
    while (LoRa.available()) {
      char incoming = (char)LoRa.read();
       
      lcd.print(incoming);
      if ((incoming == 'n') || (incoming == 'p')) {
        count = 0;
      } else {
        count += 1;
      }

      switch (count){
        case 0: break;

        case 1:
        case 2:
        case 3: msg[count - 1] = incoming;
                t1 = strtoul(msg, 3, 16);

                if (t1 > 2048) {
                  t1 -= 2048;
                  trend1 = true;
                } else {
                  trend1 = false;
                }

                if (t1 > 1024) {
                  t1 -= 1024;
                  ot1 = true;
                } else {
                  ot1 = false;
                }

                if (t1 > 512) {
                  t1 -= 512;
                  fault1 = true;
                } else {
                  fault1 = false;
                }
                break;

        case 4:
        case 5:
        case 6: msg[count - 4] = incoming;
                t2 = strtoul(msg, 3, 16);

                if (t2 > 2048) {
                  t2 -= 2048;
                  trend2 = true;
                } else {
                  trend2 = false;
                }

                if (t2 > 1024) {
                  t2 -= 1024;
                  ot2 = true;
                } else {
                  ot2 = false;
                }

                if (t2 > 512) {
                  t2 -= 512;
                  fault2 = true;
                } else {
                  fault2 = false;
                }
                break;

        case 7:
        case 8:
        case 9: msg[count - 7] = incoming;
                t3 = strtoul(msg, 3, 16);

                if (t3 > 2048) {
                  t3 -= 2048;
                  trend3 = true;
                } else {
                  trend3 = false;
                }

                if (t3 > 1024) {
                  t3 -= 1024;
                  ot3 = true;
                } else {
                  ot3 = false;
                }

                if (t3 > 512) {
                  t3 -= 512;
                  fault3 = true;
                } else {
                  fault3 = false;
                }                
                break;

        default: break;
      }

                  Serial.println(msg);
                  Serial.println(t1);
                  Serial.println(trend1);
                  Serial.println(ot1);
                  Serial.println(fault1);
                  Serial.println(t2);
                  Serial.println(trend2);
                  Serial.println(ot2);
                  Serial.println(fault2);
                  Serial.println(t3);
                  Serial.println(trend3);
                  Serial.println(ot3);
                  Serial.println(fault3);
    }
  }


}
// int reconstructReading(float temp, float prev){
//   short tVal = short(temp * 10);
  
//   tVal = map(tVal, -150, 361, 0, 511);
//   if (temp <-20.0) bitSet(tVal, 9);     // Fault - if temp below -20C then it's most likely a faulty sensor
//   if (temp > 0.0) bitSet(tVal, 10);     // Over temp - if its above zero we have a problem so alarm again
//   if (temp <= prev) bitSet(tVal, 11);   // Trend - if temp less than or equal to previous then its OK
//   return tVal;
// }
