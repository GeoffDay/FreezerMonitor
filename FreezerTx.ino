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
short hex1 = 0;
short hex2 = 0;
short hex3 = 0;
int count = 0;              // keep count of the samples
unsigned long nextTx = 0;
int pause = 0;
char prefix = 'n';          // & is the normal prefix, ~ is the paused prefix
bool OT = false;            // not over zero
bool trending = true;       // cooling

void setup() {
  lcd.begin(16, 2);          // set up the LCD's number of columns and rows:
  lcd.display();             // idealy an i2c display would save a few pins.

  analogReference(INTERNAL); // set AD converter to 1.1V Full scale
  pinMode(ledPin, OUTPUT);   // the LED pin. Also clock for the LoRa module

  pinMode(A3, INPUT_PULLUP); // use A3 analog pin as a digital input to read the pause switch


  Serial.begin(9600);        // sending to the serial monitor
  
  while (!Serial);
  Serial.println("LoRa Sender");
 
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

  lcd.setCursor(0,0);
  lcd.print(temp1, 1);
  lcd.print(" ");
  lcd.print(temp2, 1);
  lcd.print(" ");
  lcd.print(temp3, 1);  

  lcd.setCursor(0,1);

  if (digitalRead(A3) == LOW) { // have we paused the alarms while the freezer is opened?           
    pause = 1800;               // 1800 seconds - 30 mins
    lcd.print("Al. Paused ");   // print a message
    lcd.print(1800/60);
    lcd.print("min");
  } else {                      // and don't do any local alarming
    if ((temp1 <-20.0) || (temp2 <-20.0) || (temp3 <-20.0)) lcd.print("Sensor Fault");
    if ((temp1 > 0.0) || (temp2 > 0.0) || (temp3 > 0.0)) {
      lcd.print("Over Temp");
      OT = true;            // over zero!!!
    }
  }

  if (pause > 0) {              // if pause is positive then decrement
    pause -= 1;                 // this loop goes every second 
    prefix = 'p';               // and change the prefix so that the remote station knows too
  }  else {
    prefix = 'n';               // put it back to normal
  }

  count = count + 1;
  ave1 = ave1 + temp1; 
  ave2 = ave2 + temp2;
  ave3 = ave3 + temp3;
  
  delay(1000);

  if (millis() > nextTx){
    nextTx = millis() + 60000;   // current time + 10 mins in milliseconds 
    
    ave1 = ave1 / count;         // lets calc the ave temp over last 10 mins
    ave2 = ave2 / count;         // and stuff it into temp now.
    ave3 = ave3 / count;
    count = 0;                    // zero the count
  
    prev1 = ave1;                // make a copy of the previous temps
    prev2 = ave2;
    prev3 = ave3;
   
  Serial.println(constructReading(ave1, prev1), HEX);
    // send packet
    LoRa.beginPacket();
    LoRa.print(prefix);
    LoRa.print(constructReading(ave1, prev1), HEX);
    LoRa.print(constructReading(ave2, prev2), HEX);
    LoRa.print(constructReading(ave3, prev3), HEX);
    LoRa.endPacket();

    Serial.print(" Temp:");
    Serial.print(ave1);
    Serial.print("C ");
    Serial.print(ave2);
    Serial.print("C ");
    Serial.print(ave3);
    Serial.print("C ");
  }
}

float scaleValue(int value, int offset, float gain){
  return ((value - offset) * gain) / 10.0;
}

int constructReading(float temp, float prev){
  short tVal = short(temp * 10);
  
  tVal = map(tVal, -150, 361, 0, 511);
  if (temp <-20.0) bitSet(tVal, 9);     // Fault - if temp below -20C then it's most likely a faulty sensor
  if (temp > 0.0) bitSet(tVal, 10);     // Over temp - if its above zero we have a problem so alarm again
  if (temp <= prev) bitSet(tVal, 11);   // Trend - if temp less than or equal to previous then its OK
  return tVal;
}

