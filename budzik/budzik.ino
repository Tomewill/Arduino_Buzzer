#include <Wire.h>
#include <RTClib.h>
#include <OneWire.h>
#include <TM1637Display.h>
#include "budziklib.h"

bool noErrors = true;

//RTC
RTC_DS3231 rtc;
DateTime now;

//Term
OneWire DS18B20(TEMP_PIN);
byte dsAddr[8];

//Display
TM1637Display display(DCLK, DDIO);
byte dispData[] = { 0xff, 0xff, 0xff, 0xff }; 



void setup() {
  
  //RTC------------------------------------------
  Serial.begin(9600);
  if (rtc.begin()) Serial.println("RTC found");
  else {
    Serial.println("RTC not found");
    noErrors = false;
  }
  //Term-----------------------------------------
  DS18B20.reset_search();
  if (DS18B20.search(dsAddr)) {
    Serial.println("1-wire found");
    if (dsAddr[0] == 0x28) Serial.println("DS18B20 found");
    else {
      Serial.println("1-wire not found");
      noErrors = false;
    }
  } else {
    Serial.println("DS18B20 not found");
    noErrors = false;
  }
  while(!noErrors);
  //Display------------------------------------
  display.setBrightness(0x01);
  //others--------------------------------------
  initRest();
  if (digitalRead(KL3)==HIGH && digitalRead(KL1)) rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
  while(digitalRead(KL3)==HIGH || digitalRead(KL1)); //waiting for releasing buttons
  now=rtc.now();
  displayTime(display, now);
}
unsigned long getMillis;
unsigned long statePreviuosMillis=0ul; //flag for changing states
unsigned long stateDeltaMillis=5000ul; //how long actual state (temp, date, time) will be diplaying

uint8_t setWakingHour=now.hour();
uint8_t setWakingMinute=now.minute();
bool isSetWakingUp=false;

uint8_t state=1;//0-time 1-date 2 temp


void loop() {
  if (digitalRead(KL1)==HIGH) {//setting actual time
    digitalWrite(LED1, HIGH);
    digitalWrite(LED2, HIGH);
    digitalWrite(LED3, LOW);
    while(digitalRead(KL1)==HIGH);
    bool breakLoop=false;
    now=rtc.now();
    uint8_t setHour=now.hour();
    uint8_t setMinute=now.minute();
    uint8_t setDay=now.day();
    uint8_t setMonth=now.month();
    uint16_t yearFromRTC=now.year();
    uint16_t blinkTime=500;
    unsigned long lastBlinkTime=millis();

    bool buttonStateKL1, lastButtonStateKL1=LOW, readButtonKL1;
    unsigned long lastKL1Time=millis();

    bool buttonStateKL2, lastButtonStateKL2=LOW, readButtonKL2;
    unsigned long lastKL2Time=millis();

    bool ledState=LOW;
    while (!breakLoop) { //adjust time
      //display time on display xd
      displayChangingTime(display, setHour, setMinute);
      //blink
      if (millis() - lastBlinkTime >= blinkTime) {
        lastBlinkTime=millis();
        digitalWrite(LED1, ledState);
        display.setBrightness(ledState);
        ledState=!ledState;
      }
      //changing time to display
      readButtonKL1 = digitalRead(KL2);
      readButtonKL2 = digitalRead(KL3);
      
      if (readButtonKL1 != lastButtonStateKL1) {
        lastKL1Time = millis();
      }
      if (readButtonKL2 != lastButtonStateKL2) {
        lastKL2Time = millis();
      }

      //set hour using KL2 without debounce
      if (millis()-lastKL1Time>=10) {
        if (readButtonKL1 != buttonStateKL1) {
          buttonStateKL1 = readButtonKL1;
          if (buttonStateKL1==HIGH) {
            setHour++;
            if (setHour>23) setHour=0;
          }
        }
      }
      lastButtonStateKL1 = readButtonKL1;

      //set minute using KL3 without debounce
      if (millis()-lastKL2Time>=10) {
        if (readButtonKL2 != buttonStateKL2) {
          buttonStateKL2 = readButtonKL2;
          if (buttonStateKL2==HIGH) {
            setMinute++;
            if (setMinute>59) setMinute=0;
          }
        }
      }
      lastButtonStateKL2 = readButtonKL2;

      //we have actual time
      if (digitalRead(KL1)==HIGH) breakLoop=true;;
    }
    while(digitalRead(KL1)==HIGH); //wait to release button 1 to prevent from setting date without changing it
    breakLoop=false;
    digitalWrite(LED1,HIGH);
    lastKL1Time=millis();
    lastKL2Time=millis();
    lastBlinkTime=millis();
    ledState=LOW;

    while (!breakLoop) { //adjust date
      //display date on display xd
      displayChangingDate(display, setDay, setMonth);

      //blink
      if (millis() - lastBlinkTime >= blinkTime) {
        lastBlinkTime=millis();
        digitalWrite(LED2, ledState);
        display.setBrightness(ledState);
        ledState=!ledState;
      }

      //changing date to adjust
      readButtonKL1 = digitalRead(KL2);
      readButtonKL2 = digitalRead(KL3);
      
      if (readButtonKL1 != lastButtonStateKL1) {
        lastKL1Time = millis();
      }
      if (readButtonKL2 != lastButtonStateKL2) {
        lastKL2Time = millis();
      }

      //set day using KL2 without debounce
      if (millis()-lastKL1Time>=10) {
        if (readButtonKL1 != buttonStateKL1) {
          buttonStateKL1 = readButtonKL1;
          if (buttonStateKL1==HIGH) {
            setDay++;
            changeDayToProper(setMonth, setDay, yearFromRTC);
          }
        }
      }
      lastButtonStateKL1 = readButtonKL1;

      //set month using KL3 without debounce
      if (millis()-lastKL2Time>=10) {
        if (readButtonKL2 != buttonStateKL2) {
          buttonStateKL2 = readButtonKL2;
          if (buttonStateKL2==HIGH) {
            setMonth++;
            if (setMonth>12) setMonth=1;
            fixDayAfterMonthChange(setMonth, setDay, yearFromRTC);
          }
        }
      }
      lastButtonStateKL2 = readButtonKL2;

      //we have actual date, set it on
      if (digitalRead(KL1)==HIGH) {
        breakLoop=true;
        rtc.adjust(DateTime(now.year(), setMonth, setDay, setHour, setMinute, 0));
        while(digitalRead(KL1)==HIGH); //wait to release button
        state=0;
      }
    }
  } else if (digitalRead(KL2)==HIGH) {//setting wakeup time
    bool ledState=HIGH;
    digitalWrite(LED1, HIGH);
    digitalWrite(LED2, LOW);
    digitalWrite(LED3, HIGH); 
    digitalWrite(BUZZ, HIGH); //enbuzz
    while(digitalRead(KL2)==HIGH);
    digitalWrite(BUZZ, LOW);
    bool breakLoop=false;

    uint16_t blinkTime=500;
    unsigned long lastBlinkTime=millis();

    bool buttonStateKL1, lastButtonStateKL1=LOW, readButtonKL1;
    unsigned long lastKL1Time=millis();

    bool buttonStateKL2, lastButtonStateKL2=LOW, readButtonKL2;
    unsigned long lastKL2Time=millis();

    while (!breakLoop) { //adjust waking time
      //display time on display xd
      displayChangingTime(display, setWakingHour, setWakingMinute);
      //blink
      if (millis() - lastBlinkTime >= blinkTime) {
        lastBlinkTime=millis();
        digitalWrite(LED1, ledState);
        display.setBrightness(ledState);
        ledState=!ledState;
      }
      //changing waking time to display
      readButtonKL1 = digitalRead(KL1);
      readButtonKL2 = digitalRead(KL3);
      
      if (readButtonKL1 != lastButtonStateKL1) {
        lastKL1Time = millis();
      }
      if (readButtonKL2 != lastButtonStateKL2) {
        lastKL2Time = millis();
      }

      //set waking hour using KL1 without debounce
      if (millis()-lastKL1Time>=10) {
        if (readButtonKL1 != buttonStateKL1) {
          buttonStateKL1 = readButtonKL1;
          if (buttonStateKL1==HIGH) {
            setWakingHour++;
            if (setWakingHour>23) setWakingHour=0;
          }
        }
      }
      lastButtonStateKL1 = readButtonKL1;

      //set waking minute using KL3 without debounce
      if (millis()-lastKL2Time>=10) {
        if (readButtonKL2 != buttonStateKL2) {
          buttonStateKL2 = readButtonKL2;
          if (buttonStateKL2==HIGH) {
            setWakingMinute++;
            if (setWakingMinute>59) setWakingMinute=0;
          }
        }
      }
      lastButtonStateKL2 = readButtonKL2;

      //we have actual waking time
      if (digitalRead(KL2)==HIGH) {
        breakLoop=true;
        isSetWakingUp=true;
        digitalWrite(BUZZ, HIGH); //enbuzz
      }
    }
    while(digitalRead(KL2)==HIGH); //wait for release button 2 to prevent from setting waking time without changing it
    digitalWrite(BUZZ, LOW);
  }
  // KNOCK KNOCK!!!
  if (isSetWakingUp) {
    now=rtc.now();
    if ((now.hour() == setWakingHour) && (now.minute() == setWakingMinute)) {
      const uint16_t buzzerDelta=500;
      unsigned long lastBuzzTime=millis();
      bool buzzState=LOW;
      digitalWrite(BUZZ, HIGH);

      while(digitalRead(KL3)==LOW) {//wait to click time
        if (millis()-lastBuzzTime>=buzzerDelta) {
          lastBuzzTime=millis();

          digitalWrite(BUZZ, buzzState);
          digitalWrite(LED1, buzzState);
          digitalWrite(LED2, buzzState);
          digitalWrite(LED3, buzzState);

          buzzState=!buzzState;
        } 
      }

      digitalWrite(BUZZ, LOW);
      isSetWakingUp=false;

      while(digitalRead(KL3)==HIGH);
      state=0;//display hour
    }
  } 
  //display state(time, date, temperature)
  if ((millis()-statePreviuosMillis>=stateDeltaMillis) || (digitalRead(KL3)==HIGH)) {
    while(digitalRead(KL3)==HIGH);
    statePreviuosMillis=millis();

    switch(state)
    {
      case 0: displayTime(display, rtc.now()); break;
      case 1: displayDate(display, rtc.now()); break;
      case 2: displayTemp(dsAddr, DS18B20, display); break;
    }
    state++;
    if (state>=3) state=0;
  }
}

/** addnotation
  codes used in this project
  
  *Instrukcja do zajęć zdalnego laboratorium - spotkanie 3R Interfejsy komunikacyjne
    Authors:dr inż. Paweł Dąbal,drinż. Krzysztof Sieczkowski
      used for code to use buttons without debounce 
      (variables changed to english for better understading code)
  
  Instrukcja laboratoryjna nr 3 Projektowanie systemów cyfrowych
  Temat: Programowanie i testowanie komponentów systemu cyfrowego
    Author: dr inż. Krzysztof Sieczkowski

*/


/*TODO:
dodać nastawianie godziny do budzenia (metoda Copiego-Paste'a) edit dodać miganie i bajery
    
                                      naprawa błędu ustawienia dany np 30 luty - tym razem dla zmiany miesiąca to zmienia dzień odpowiednio

                                      zmiana state przyciskiem KL3
                                      upewnić się że wszyskie kl3 są sprawdzane że są w stanie niskim

wyłączanie alarmu po długim naciśnięciu KL3
*/
