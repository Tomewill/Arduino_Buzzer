#include <Wire.h>
#include <RTClib.h>
#include <OneWire.h>
#include <TM1637Display.h>

//LEDs
#define LED1 A3
#define LED2 A2
#define LED3 A1

//Buttons
#define KL1 2
#define KL2 3
#define KL3 4

//RTC
#define SDA A4
#define SCL A5

//Additional
#define BUZZ 10
#define TEMP_PIN 9

//Display
#define DCLK 6  // conection to 7
#define DDIO A0 // conection to 6


void initRest(void);
void displayTime(TM1637Display display, DateTime time);
void displayDate(TM1637Display display, DateTime time);
float rawVal2Celc(byte data[]);
void displayTemp(byte address[], OneWire DS18B20, TM1637Display display);
void displayChangingTime(TM1637Display display, uint8_t hour, uint8_t minute);
void displayChangingDate(TM1637Display display, uint8_t day, uint8_t month);
void changeDayToProper (uint8_t setMonth, uint8_t &setDay, uint16_t yearFromRTC);
void fixDayAfterMonthChange(uint8_t setMonth, uint8_t &setDay, uint16_t yearFromRTC);
