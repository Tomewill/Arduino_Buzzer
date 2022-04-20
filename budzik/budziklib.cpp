#include "budziklib.h"
#include "Arduino.h"
#include "HardwareSerial.h"
#include <OneWire.h>
#include <Wire.h>
#include <TM1637Display.h>

//defines to display letters than are not defined in TM1637Display.h
#define DISPLAY_LETTER_O_UPPER 0b01100011
#define DISPLAY_LETTER_C 0b00111001

void initRest(void) {
  unsigned long LED_BlinkTime=100ul;
  unsigned long BuzzkTime=100ul;
  //KLs
  pinMode(KL1, INPUT);
  pinMode(KL2, INPUT);
  pinMode(KL3, INPUT);
  //Buzz
  pinMode(BUZZ, OUTPUT);
  digitalWrite(BUZZ, HIGH); //enbuzz
  delay(BuzzkTime);
  digitalWrite(BUZZ, LOW);
  //LEDs
  pinMode(LED1, OUTPUT);
  digitalWrite(LED1, HIGH);
  delay(LED_BlinkTime);
  digitalWrite(LED1, LOW);
  delay(LED_BlinkTime);

  pinMode(LED2, OUTPUT);
  digitalWrite(LED2, HIGH);
  delay(LED_BlinkTime);
  digitalWrite(LED2, LOW);
  delay(LED_BlinkTime);

  pinMode(LED3, OUTPUT);
  digitalWrite(LED3, HIGH);
  delay(LED_BlinkTime);
  digitalWrite(LED3, LOW);
  delay(LED_BlinkTime+LED_BlinkTime);
}
//time ----------------------------------------------------------------------------------------
void displayTime(TM1637Display display, DateTime time) {
  //time LED signalization
  digitalWrite(LED1, HIGH);
  digitalWrite(LED2, LOW);
  digitalWrite(LED3, LOW);
  
  uint8_t hour, min;
  hour = time.hour();
  min = time.minute();

  display.showNumberDecEx(hour*100+min, 64, true);
  Serial.print("1 ");
  Serial.print(hour);
  Serial.print(":");
  Serial.println(min);
}

void displayChangingTime(TM1637Display display, uint8_t hour, uint8_t min) {
  display.showNumberDecEx(hour*100+min, 64, true);

  Serial.print("100 ");
  Serial.print(hour);
  Serial.print(":");
  Serial.println(min);
}

//date ------------------------------------------------------------------------------------------
void displayDate(TM1637Display display, DateTime time) {
  //date LED signalization
  digitalWrite(LED1, LOW);
  digitalWrite(LED2, HIGH);
  digitalWrite(LED3, LOW);

  uint8_t day, month;
  day = time.day();
  month = time.month();
  
  display.showNumberDecEx(day*100+month, 64, true);
  Serial.print("2: ");
  Serial.print(day);
  Serial.print(".");
  Serial.println(month);
}

void displayChangingDate(TM1637Display display, uint8_t day, uint8_t month) {
  display.showNumberDecEx(day*100+month, 64, true);
  Serial.print("200: ");
  Serial.print(day);
  Serial.print(".");
  Serial.println(month);
}
//temperature -------------------------------------------------------------------------------------
float rawVal2Celc(byte *data) {
  int16_t raw = (data[1]<<8) | data[0];

  //read configuration byte (R1 and R0)
  byte cfg = data[4] & 0x60;

  if (cfg ==0x0) {
    raw = raw & ~7; // resolution 9 bits
  } else if (cfg ==0x20) {
    raw = raw & ~3; // 10 bits
  } else if (cfg ==0x40) {
    raw = raw & ~1; // 11 bits
  }

  return ((double)raw / 16.0);
}

void displayTemp(byte address[], OneWire DS18B20, TM1637Display display) {
  //conversion state sequence
  if (DS18B20.reset()) {      //reset impulse response
    DS18B20.select(address);  
    DS18B20.write(0x44, 1);   //start writting
  }

  if (DS18B20.reset()) {
    DS18B20.select(address);
    DS18B20.write(0xBE, 1);   //read scratchpad memory
  }
  byte data[9];
  for (uint8_t i=0; i<9; i++) {
    data[i] = DS18B20.read(); //read 9 bytes
  }

  //temp calculation
  int8_t temperature = (int8_t)rawVal2Celc(data); //trunkate number
  
  //display 
  Serial.print("3. ");
  Serial.println(temperature);
  uint8_t dispData[4];
  //temperature LED signalization
  digitalWrite(LED1, LOW);
  digitalWrite(LED2, LOW);
  digitalWrite(LED3, HIGH);
  //temp
  dispData[1]=display.encodeDigit(temperature%10);
  temperature/=10;
  dispData[0]=display.encodeDigit(temperature%10);
  //grade sign
  dispData[2]=DISPLAY_LETTER_O_UPPER;
  dispData[3]=DISPLAY_LETTER_C;
  display.setSegments(dispData);
}


void changeDayToProper (uint8_t setMonth, uint8_t &setDay, uint16_t yearFromRTC) {
  switch(setMonth) {
    case 1: case 3: case 5: case 7: case 8: case 10: case 12:
      if (setDay>31) setDay=1;
      break;
    case 4: case 6: case 9: case 11:
      if (setDay>30) setDay=1;
      break;
    case 2:
      if(((yearFromRTC % 4 == 0) && (yearFromRTC % 100 != 0)) || (yearFromRTC % 400 == 0)) {
        if (setDay > 29) setDay=1;
      } else if (setDay > 28) setDay=1;
    break;
    default : setDay = 0; break;//at any case that month wil be out of range of (1,12)
  }
}
void fixDayAfterMonthChange(uint8_t setMonth, uint8_t &setDay, uint16_t yearFromRTC) {
  switch(setMonth) {
    case 4: case 6: case 9: case 11:
      if (setDay>30) setDay=30;
      break;
    case 2:
      if(((yearFromRTC % 4 == 0) && (yearFromRTC % 100 != 0)) || (yearFromRTC % 400 == 0)) {
        if (setDay > 29) setDay=29;
      } else if (setDay > 28) setDay=28;
    break;
  }
}
