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
int remap = 0;              // convert the temp values and remap to save bits
short status = 0;
int count = 0;              // keep count of the samples
unsigned long nextTx = 0;

void setup() {
  lcd.begin(16, 2);          // set up the LCD's number of columns and rows:
  lcd.display();             // idealy an i2c display would save a few pins.
  lcd.print("hello ");       // Print a message to the LCD.
  
  analogReference(INTERNAL); // set AD converter to 1.1V Full scale
  pinMode(ledPin, OUTPUT);   // the LED pin. Also clock for the LoRa module
 
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
  lcd.print("C ");
  lcd.print(temp2, 1);
  lcd.print("C ");

  lcd.setCursor(0,1);
  lcd.print(temp3, 1);  
  lcd.print("C ");

  lcd.print(status);
  // lcd.print((int)(temp3 * 10));
  // lcd.print("int");

  count = count + 1;
  ave1 = ave1 + temp1; 
  ave2 = ave2 + temp2;
  ave3 = ave3 + temp3;
  
  delay(500);


  if (millis() > nextTx){
    nextTx = millis() + 600000;   // current time + 10 mins in milliseconds 
    
    temp1 = ave1 / count;         // lets calc the ave temp over last 10 mins
    temp2 = ave2 / count;         // and stuff it into temp now.
    temp3 = ave3 / count;
    count = 0;                    // zero the count

    status = 0;
    if (temp1 <-20.0) bitSet(status, 0);    // Fault - if temp below -20C then it's most likely a faulty sensor
    if (temp2 <-20.0) bitSet(status, 1);    // this should give a red alert on the first two freezers
    if (temp3 <-20.0) bitSet(status, 2);    // and on this one if its connected.
    if (temp1 > 0.0) bitSet(status, 3);     // Over temp - if its above zero we have a problem so alarm again
    if (temp2 > 0.0) bitSet(status, 4); 
    if (temp3 > 0.0) bitSet(status, 5);
    if (temp1 <= prev1) bitSet(status, 6);  // Trend - if temp less than or equal to previous then its OK
    if (temp2 <= prev2) bitSet(status, 7);  // read this in conjunction with Over temp. OK for temp to rise a 
    if (temp3 <= prev3) bitSet(status, 8);  // bit if we are still sub zero. 
  
    prev1 = temp1;                // make a copy of the previous temps
    prev2 = temp2;
    prev3 = temp3;


    // send packet
    LoRa.beginPacket();
    LoRa.print("Temp: ");
    LoRa.print(temp1);
    LoRa.print("C");
    

    Serial.print(" Temperature:");
    Serial.print(temp1);
    Serial.print("C ");
    Serial.print(temp2);
    Serial.print("C ");
    Serial.print(temp3);
    Serial.print("C ");
    Serial.println(""); 
    Serial.println(status);
    LoRa.endPacket();
  }
}

float scaleValue(int value, int offset, float gain){
  return ((value - offset) * gain) / 10.0;
}


