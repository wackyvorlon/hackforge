/*
  LiquidCrystal Library - Hello World
 
 Demonstrates the use a 16x2 LCD display.  The LiquidCrystal
 library works with all LCD displays that are compatible with the 
 Hitachi HD44780 driver. There are many of them out there, and you
 can usually tell them by the 16-pin interface.
 
 This sketch prints "Hello World!" to the LCD
 and shows the time.
 
  The circuit:
 * LCD RS pin to digital pin 12
 * LCD Enable pin to digital pin 11
 * LCD D4 pin to digital pin 5
 * LCD D5 pin to digital pin 4
 * LCD D6 pin to digital pin 3
 * LCD D7 pin to digital pin 2
 * LCD R/W pin to ground
 * 10K resistor:
 * ends to +5V and ground
 * wiper to LCD VO pin (pin 3)
 
 Library originally added 18 Apr 2008
 by David A. Mellis
 library modified 5 Jul 2009
 by Limor Fried (http://www.ladyada.net)
 example added 9 Jul 2009
 by Tom Igoe
 modified 22 Nov 2010
 by Tom Igoe
 
 This example code is in the public domain.

 http://www.arduino.cc/en/Tutorial/LiquidCrystal
 */

// include the library code:
#include <LiquidCrystal.h>
#include <EEPROM.h>

#include "PinChangeInt.h"

uint8_t latest_interrupted_pin;
int addr=1;
int endaddr=0;


void quicfunc() {
  latest_interrupted_pin=PCintPort::arduinoPin;
  if (latest_interrupted_pin == 6) {
    read1();
  }
  if (latest_interrupted_pin == 7) {
    read0();
  }
 /*if (latest_interrupted_pin == 9) {
    if (digitalRead(9)== LOW) {
      writecard();
    }
  }*/
}

// initialize the library with the numbers of the interface pins
LiquidCrystal lcd(12, 11, 5, 4, 3, 2);

void setup() {
  // set up the LCD's number of columns and rows: 
  lcd.begin(16, 2);
  // Print a message to the LCD.
  //lcd.print("hello, world!");
  PCintPort::attachInterrupt(6, &quicfunc, CHANGE);
  PCintPort::attachInterrupt(7, &quicfunc, CHANGE);
  //PCintPort::attachInterrupt(9, &quicfunc, CHANGE);
  Serial.begin(9600);
  Serial.print("\nSystem waiting for tag.\n");
  endaddr=EEPROM.read(0);
  Serial.print("Current EEPROM end address: ");
  Serial.println(endaddr);
  addr=endaddr;
}

volatile unsigned long r1=0;
volatile unsigned long r2=0;
unsigned long r3=0;
volatile int r1Count=0;

void loop() {
  // set the cursor to column 0, line 1
  // (note: line 1 is the second row, since counting begins with 0):
  lcd.setCursor(0, 0);
  lcd.print("Waiting for tag...");
  lcd.setCursor(0,1);
  if (r1Count >= 26) {
    checkcard();
    lcd.clear();
  }
  //delay(500);
  //lcd.print(r1);
  // print the number of seconds since reset:
  //lcd.print(millis()/1000);
  if (digitalRead(9) == LOW) {
    writecard();
  }
}

// Functions to read the bits

void unlock(void) {
  Serial.print("Unlocking\n");
  digitalWrite(10, HIGH);
  delay(1000);
  digitalWrite(10, LOW);
}

byte value; //Value written to eeprom
void writecard(void) {
  Serial.print("Writing card to EEPROM...\n");
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("Swipe card:");
  delay(3000);
  
  if (r1Count < 26) {
    lcd.setCursor(0,1);
    lcd.print("Card not read.");
    r1Count = 0;
    delay(2000);
    return;
  } else {
    lcd.setCursor(0,0);
    Serial.print("Read in: ");
    Serial.print(r1);
    Serial.print("\n");
    r3=r1; // Copy value into a spare variable.
    // Split value into four bytes, and write each into EEPROM
    // Neither of these are working right.
    // Starting to think I should just use the last byte.
    Serial.print("\nCurrent address in EEPROM: ");
    Serial.println(addr);
    Serial.print("Writing bytes: ");
    for (int i=0; i<4; i++) {
      value=r3&0xFF;
      r3>>=8;
      EEPROM.write(addr,value);
      Serial.print(value);
      Serial.print(" ");
      addr=addr+1;
      //lcd.print(value);
    }
    // Read from EEPROM and verify
    // The way the previous loop runs, it leaves us one address too far.
    addr=addr-1;
    Serial.print("\nReading bytes: ");
    
    lcd.setCursor(0,1);
    r2=0;
    
    // Move address back to beginning of region just written to.
    addr=addr-3;
    byte tmp[4];
    for (int i=3;i>=0;i--) {
      value=EEPROM.read(addr);
      //r2 |= value;
      Serial.print(value);
      Serial.print(" ");
      tmp[i]=value;
      //r2<<=8;
      addr=addr+1;
      //lcd.print(value);
    }
    Serial.print("\nCurrent address in EEPROM: ");
    Serial.println(addr);
    Serial.print("\nRebuilding number: ");
    //Put the number back together.
    for (int i=0; i<4;i++)
      {
        r2 |= tmp[i];
        Serial.print(tmp[i]);
        Serial.print(" ");
        if (i<3) r2<<=8;
        
      }
      
    if (r2 != r1) {
      lcd.setCursor(0,0);
      lcd.print("EEPROM Write Failed!");
      Serial.print("\nWrite failed, wrote: ");
      Serial.print(r1);
      Serial.print(" read: ");
      Serial.print(r2);
      Serial.print("\n");
      lcd.setCursor(0,1);
      //lcd.print(r2);
      //lcd.print(" ");
      //lcd.print(r1);
      delay(500);
      r1=0;
      return;
    } else {
      lcd.setCursor(0,1);
      lcd.print("Written!");
      EEPROM.write(0,addr);
      r1=0;
    }
  }
  r1Count=0;
  
}

//Code to verify whether access should be granted.
void checkcard( void ) 
{
  int ckaddr = 1;
  byte tmpr=0;
  byte tmpe=0;
  boolean deny=false;
  endaddr=EEPROM.read(0);
  
  //Copy value into a temp buffer.
  r3=r1;
  tmpr=r3&0xFF;
  r3>>=8;
  Serial.print("\nRead number: ");
  Serial.println(r1);
  Serial.print("\nByte to search for: ");
  Serial.println(tmpr);
  for (int i=1;i<endaddr;i=i+4)
    {
      tmpe=EEPROM.read(i);
      if (tmpe==tmpr) {
        Serial.print("\nFirst byte found at address: ");
        Serial.println(i);
        // Check entire number.
        for (int j=1; j<4;j++)
        {
          tmpr=r3&0xFF;
          tmpe=EEPROM.read(i+j);
          Serial.print("\nChecking byte ");
          Serial.println(j);
          if (tmpr!=tmpe) {
            //lcd.print("Denied.");
            Serial.println("Nope, not here!");
            //return;
            deny=true;
            //break;
          }
          r3>>=8;
        }
        
        //There may or may not be a bug here.
        //If first byte shared by two cards, with differing subsequent bytes,
        //may inappropriately deny access.
        if (deny && i>=endaddr-3) {
          Serial.print("\nCurrent address: ");
          Serial.println(i);
          Serial.println("Access denied.");
          return;
        } 
      printtag();
      }
    }
    
  //lcd.print("Denied.");
  //Serial.println("Access denied.");
  //printtag();
  r1=0;
  r1Count=0;
  
}

void delcard( void ) {
  byte tmpr,tmpe;
  lcd.clear();
  lcd.print("Swipe to delete.");
  while (r1Count < 26) {
    //Wait for tag..
  }
  
  r3=r1;
  tmpr=r3&0xFF;
  r3>>=8;
  Serial.print("\nRead number: ");
  Serial.println(r1);
  Serial.print("\nByte to search for: ");
  Serial.println(tmpr);
  for (int i=1;i<endaddr;i=i+4)
    {
      tmpe=EEPROM.read(i);
      if (tmpe==tmpr) {
        Serial.print("\nFirst byte found at address: ");
        Serial.println(i);
        // Check entire number.
        for (int j=1; j<4;j++)
        {
          tmpr=r3&0xFF;
          tmpe=EEPROM.read(i+j);
          Serial.print("\nChecking byte ");
          Serial.println(j);
          if (tmpr!=tmpe) {
            //lcd.print("Denied.");
            Serial.println("Nope, not here!");
            //return;
            deny=true;
            //break;
          }
          r3>>=8;
        }
        
        //There may or may not be a bug here.
        //If first byte shared by two cards, with differing subsequent bytes,
        //may inappropriately deny access.
        if (deny && i>=endaddr-3) {
          Serial.print("\nCurrent address: ");
          Serial.println(i);
          Serial.println("Access denied.");
          return;
        } 
      printtag();
      }
    }
    
  //lcd.print("Denied.");
  //Serial.println("Access denied.");
  //printtag();
  r1=0;
  r1Count=0;
  
}
  
  

void printtag(void) {
  lcd.setCursor(0,1);
  lcd.print(r1);
  r2=r1;
  unlock();
  r1=0;
  r1Count=0;
}

// These two functions read the bits in from the Wiegand format
// connection.

void read1(void) {
  if (digitalRead(6)==LOW) {
    r1Count++;
    r1 = r1 << 1;
    r1 |= 1;
  }
}

void read0(void) {
  if (digitalRead(7)==LOW) {
    r1Count++;
    r1=r1 << 1;
  }
}
