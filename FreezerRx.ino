#include <LiquidCrystal.h>
#include <SPI.h>
#include <LoRa.h>
 
// initialize the library by associating any needed LCD interface pinc with the arduino pin number it is connected to
const int rs = 8, en = 7, d4 = 6, d5 = 5, d6 = 4, d7 = 3;
LiquidCrystal lcd(rs, en, d4, d5, d6, d7);

int count = 0;
char msg[10];
long tmp = 0;
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
int pause = 0;

void setup() 
{
  lcd.begin(16, 2);                 // set up the LCD's number of columns and rows:
  lcd.display();
  lcd.print("Reciever");       // Print a message to the LCD.
  
  Serial.begin(9600);
  while (!Serial);
  Serial.println("LoRa Receiver");

  if (!LoRa.begin(433E6)) {
    Serial.println("Starting LoRa failed!");
    while (1);
  }

  str = (char*)msg;
  endptr = &str;
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
    Serial.println(packetSize);
    // lcd.begin(16,2);
    lcd.setCursor(0, 1);
    strcpy(msg,"   ");
    str = (char*)msg;
    endptr = &str;
    count = 0;

    // read packet
    while (LoRa.available()) {
      char incoming = (char)LoRa.read();    
      
      if (incoming == '|') pause = 0;     // n is normal mode - alarms on
      if (incoming == '~') pause = 1800;  // p is paused mode - alarms off for 30 mins. freezer opened

      if ((incoming == '|') || (incoming == '~')){
       count = 0; 
      } else {
        msg[count] = incoming;
        count += 1;
      }
      
      if (incoming == '.') {
        // tmp = strtoull(str, endptr, 36);
        Serial.println(str);
        tmp = strtol(str, 0, 36);
        t1 = (tmp & 0x3ff00000) >> 20;
        t2 = (tmp & 0x000FFC00) >> 10;
        t3 = tmp & 0x000003ff;
        Serial.println(tmp);
        Serial.println(count);

        if (t1 > 512) {
          t1 -= 512;
          trend1 = true;
        } else {
          trend1 = false;
        }

        temp1 = map(t1, 0, 511, -150, 361)/10.0;

        if (t2 > 512) {
          t2 -= 512;
          trend2 = true;
        } else {
          trend2 = false;
        }
        
        temp2 = map(t2, 0, 511, -150, 361)/10.0;

        if (t3 > 512) {
          t3 -= 512;
          trend3 = true;
        } else {
          trend3 = false;
        }

        temp3 = map(t3, 0, 511, -150, 361)/10.0;
      }

      lcd.print(incoming);

      // Serial.println(msg);


      lcd.setCursor(0,0);
      lcd.clear();
      lcd.print(temp1, 1);
      lcd.print(" ");
      lcd.print(temp2, 1);
      lcd.print(" ");
      lcd.print(temp3, 1); 
      lcd.println("  ");  
      
      lcd.setCursor(0,1);

      if (pause == 0) {
        if ((temp1 <-20.0) || (temp2 <-20.0) || (temp3 <-20.0)) lcd.println("Sens. Fault ");
        if ((temp1 > 0.0) || (temp2 > 0.0) || (temp3 > 0.0)){
          lcd.print("Over Temp  ");
        } else {
          lcd.print("below zero ");
        }

        lcd.println(LoRa.rssi());
      } else {
        pause -= 600;                 // this loop goes every 10mins or 600 seconds
        lcd.print("Al. Paused ");
        lcd.print(pause/60);
        lcd.println(" min");
      }
    


      Serial.println(msg);
      Serial.println(temp1, 1);
      Serial.println(temp2, 1);
      Serial.println(temp3, 1);

      Serial.println(LoRa.rssi());


    }
  }
}
