#include <LiquidCrystal.h>
#include <SPI.h>
#include <LoRa.h>
 
// initialize the library by associating any needed LCD interface pinc with the arduino pin number it is connected to
const int rs = 8, en = 7, d4 = 6, d5 = 5, d6 = 4, d7 = 3;
LiquidCrystal lcd(rs, en, d4, d5, d6, d7);
int count = 0;
char msg[4];
int t1 = 0;
int t2 = 0;
int t3 = 0;


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
      
      // Serial.print(incoming);
      
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
                break;

        case 4:
        case 5:
        case 6: msg[count - 4] = incoming;
                t2 = strtoul(msg, 3, 16);
                break;

        case 7:
        case 8:
        case 9: msg[count - 7] = incoming;
                t3 = strtoul(msg, 3, 16);
                break;

        default: break;
      }

                  Serial.println(msg);
                  Serial.println(t1);
                  Serial.println(t2);
                  Serial.println(t3);        
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
