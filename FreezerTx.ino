/*  Freezer Monitor Transmitter
 * LCD connections - get a couple of i2C displays if possible
 * LCD RS pin to digital pin 8
 * LCD Enable pin to digital pin 7
 * LCD D4 pin to digital pin 6
 * LCD D5 pin to digital pin 5
 * LCD D6 pin to digital pin 4
 * LCD D7 pin to digital pin 3
 * LCD R/W pin to ground
 * 10K resistor:
 * ends to +5V and ground
 * wiper to LCD VO pin (pin 3)

 Three temp sensors - Two freezers and a cool room.
 If the coolroom is unplugged a pulldown resistor will give a 
 zero value and no alarm will be raised. We need a way to show that the Coolroom is engaged.
 Perhaps we use a cannon plug with a pin pulled high to indicate coolroom in use.
 
 if a value over 0 degrees is flagged then an alarm condition will be raised.
 Temps should be averaged over a 10 minute interval and a status byte will be logged
 and transmitted to indicate, well, status. 10Bits on the temp sensor is more than we need so 
 we will remap to -15 to +36.2C range 9 bits. 

 A pushbutton will allow a temporary halt on the alarms so that we can open the freezers for 
 getting and inserting food.  
*/

// include the library code:
#include <LiquidCrystal.h>
#include <SPI.h>
#include <LoRa.h>

// initialize the library by associating any needed LCD interface pinc with the arduino pin number it is connected to
const int rs = 8, en = 7, d4 = 6, d5 = 5, d6 = 4, d7 = 3;
LiquidCrystal lcd(rs, en, d4, d5, d6, d7);

int ledPin = 13;

float scale = 1100.0/1024.0;
float temp1 = 0;
float temp2 = 0;
float temp3 = 0;
float ave1 = 0;
float ave2 = 0;
float ave3 = 0;
float prev1 = 0;
float prev2 = 0;
float prev3 = 0;

int count = 0;              // keep count of the samples
unsigned long nextTx = 0;   // were cou
int pause = 0;
char prefix = 'n';          // & is the normal prefix, ~ is the paused prefix
bool OT = false;            // not over zero
bool trending = true;       // cooling
long tmp = 0;
char msg[10];

void setup() {
  lcd.begin(16, 2);          // set up the LCD's number of columns and rows:
  lcd.display();             // idealy an i2c display would save a few pins.

  analogReference(INTERNAL); // set AD converter to 1.1V Full scale
  pinMode(ledPin, OUTPUT);   // the LED pin. Also clock for the LoRa module

  pinMode(A3, INPUT_PULLUP); // use A3 analog pin as a digital input to read the pause switch

  Serial.begin(9600);        // setup ending to the serial monitor
  
  while (!Serial);
  Serial.println("LoRa Sender"); // ID
 
  if (!LoRa.begin(433E6)) {   // setting up the LoRa module
    Serial.println("Starting LoRa failed!");
    while (1);
  }
}

void loop() {
  // read the values from the sensors:
  temp1 = scaleValue(analogRead(A0), 500, scale);
  temp2 = scaleValue(analogRead(A1), 500, scale);
  temp3 = scaleValue(analogRead(A2), 500, scale);

  // display on LCD
  lcd.setCursor(0,0);
  lcd.print(temp1, 1);
  lcd.print(" ");
  lcd.print(temp2, 1);
  lcd.print(" ");
  lcd.print(temp3, 1);  

  lcd.setCursor(0,1);   // next line

  // pause button - open freezer without setting off alarm - pause alarms for 30 mins
  if (digitalRead(A3) == LOW) pause = 1800; // 1800 seconds - 30 mins

  if (pause > 0) {              // if pause is positive then decrement
    pause -= 1;                 // this loop goes every second 
    prefix = '~';               // and change the prefix so that the remote station knows too
    lcd.print("Al. Paused ");   // print a message
    lcd.print(pause/60);
    lcd.println("min");
  } else {
    prefix = '|';               // put it back to normal
    if ((temp1 <-20.0) || (temp2 <-20.0) || (temp3 <-20.0)) lcd.println("Sensor Fault");
    if ((temp1 > 0.0) || (temp2 > 0.0) || (temp3 > 0.0)) {
      lcd.print("Over Temp");
      OT = true;            // over zero!!!
    }
  }
    
  count = count + 1;        // how many samples
  ave1 = ave1 + temp1;      // sum many values
  ave2 = ave2 + temp2;
  ave3 = ave3 + temp3;

  delay(1000);              // wait a second

  if (millis() > nextTx){
    nextTx = millis() + 20000;   // current time + 10 mins in milliseconds 
    
    ave1 = ave1 / count;         // lets calc the ave temp over last 10 mins
    ave2 = ave2 / count;         // and stuff it into ave now.
    ave3 = ave3 / count;
    count = 0;                    // zero the count
 
    prev1 = ave1;                // make a copy of the previous temps
    prev2 = ave2;
    prev3 = ave3;

    // create a 30 bit long version of combined temps and trends 
    tmp = (constructReading(ave1, prev1) << 20) + (constructReading(ave2, prev2) << 10) + constructReading(ave3, prev3);
    ltoa(tmp, msg, 36);   // create a string version encoded to base 36

    // send packet
    LoRa.beginPacket();   // setup LoRa
    LoRa.print(prefix);   // this is the first char. n for normal, p for paused
    LoRa.print(msg);      // this is the 3 samples encoded
    LoRa.print('.');
    LoRa.endPacket();     // end the packet. May need to add a checksum for stability

    Serial.print("long:");
    Serial.println(tmp);
    Serial.print("msg:"); 
    Serial.println(msg);
    Serial.print("Temp:");
    Serial.print(ave1, 1);
    Serial.print("C ");
    Serial.print(ave2, 1);
    Serial.print("C ");
    Serial.print(ave3, 1);
    Serial.println("C ");
    ave1 = 0;
    ave2 = 0; 
    ave3 = 0;
  }
}

float scaleValue(int value, int offset, float gain){
  return ((value - offset) * gain) / 10.0;
}

// remap a 9 bit range and work out the trend of temp - important for the pause
long constructReading(float temp, float prev){
  long tVal = short(temp * 10);
  
  tVal = map(tVal, -150, 361, 0, 511);

  if (temp <= prev) bitSet(tVal, 9);   // Trend - if temp less than or equal to previous then its OK
  return tVal;
}

