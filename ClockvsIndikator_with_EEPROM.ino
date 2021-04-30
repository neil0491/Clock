#include <LedControl.h>
#include <EEPROM.h>
#include <FastLED.h>

#include <DS3232RTC.h>
#include <Time.h>
#include <Wire.h>

#include <OneWire.h>
#include <DallasTemperature.h>


#include "GyverButton.h"
#define BTN_PIN 2   // кнопка подключена сюда 
GButton touch(BTN_PIN);


#define ONE_WIRE_BUS 9// линия данных подключена к цифровому выводу датчика температуры DS18B20
#define NUM_LEDS 29 // Number of LED controles (remember I have 3 leds / controler

#define COLOR_ORDER BRG  // Define color order for your strip
#define DATA_PIN 6  // Data pin for led comunication


#define PERIOD 1
uint32_t timer = 0;

#define auto_bright 1         // автоматическая подстройка яркости от уровня внешнего освещения (1 - включить, 0 - выключить)
#define max_bright 250        // максимальная яркость (0 - 255)
#define min_bright 10         // минимальная яркость (0 - 255)
#define BRI_PIN A3            // фоторезистор
#define bright_constant 300  // константа усиления от внешнего света (0 - 1023), чем МЕНЬШЕ константа, тем "резче" будет прибавляться яркость
#define coef 0.85

#define INIT_ADDR 1023  // номер резервной ячейки
#define INIT_KEY 50     // ключ первого запуска. 0-254, на выбор


OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);

CRGB leds[NUM_LEDS]; // Define LEDs strip
CRGB led[NUM_LEDS]; // Define LED strip
CRGB ledColor;

LedControl lc = LedControl(11, 13, 10, 1);

static uint8_t thisstep = 1;
unsigned long bright_timer, off_timer;
int new_bright, new_bright_f;
bool TempShow = false;

bool single_touch = true;
bool double_touch = false;
bool threeple_touch = false;
// Change ColorPallete





byte digits[12][7] = {{0, 1, 1, 1, 1, 1, 1}, // Digit 0
  {0, 1, 0, 0, 0, 0, 1}, // Digit 1
  {1, 1, 1, 0, 1, 1, 0}, // Digit 2
  {1, 1, 1, 0, 0, 1, 1}, // Digit 3
  {1, 1, 0, 1, 0, 0, 1}, // Digit 4
  {1, 0, 1, 1, 0, 1, 1}, // Digit 5
  {1, 0, 1, 1, 1, 1, 1}, // Digit 6
  {0, 1, 1, 0, 0, 0, 1}, // Digit 7
  {1, 1, 1, 1, 1, 1, 1}, // Digit 8
  {1, 1, 1, 1, 0, 1, 1}, // Digit9
  {1, 0, 0, 0, 0, 0, 0}, // Symbol "-"
  {1, 1, 1, 1, 0, 0, 0} // Symbol "0"
};  // Digit 9 | 2D Array for numbers on 7 segment

const byte number[10] = {  // маска для 7 сигментного индикатора
  B11111100, // 0
  B01100000, // 1
  B11011010, // 2
  B11110010, // 3
  B01100110, // 4
  B10110110, // 5
  B10111110, // 6
  B11100000, // 7
  B11111110, // 8
  B11110110, // 9
};


long ColorTable[10] = {
  CRGB::Amethyst,
  CRGB::Green,
  CRGB::Blue,
  CRGB::BlueViolet,
  CRGB::DarkMagenta,
  CRGB::Yellow,
  CRGB::White,
  CRGB::Red,
  CRGB::Crimson,
  CRGB::Magenta
};

static uint8_t change_Hue = 0; // Change color Single click
bool Dot = true;  //Dot state
//bool DST = false; //DST state


void setup() {
  Serial.begin(9600);
  sensors.begin();
  Wire.begin();
  LEDS.addLeds<WS2811, DATA_PIN, COLOR_ORDER>(leds, NUM_LEDS); // Set LED strip type
  LEDS.addLeds<WS2811, 3, COLOR_ORDER>(led, NUM_LEDS); // Set 1 LED strip type
  LEDS.setBrightness(255); // Set initial brightness

  //pinMode(2, INPUT_PULLUP); // Define DST adjust button pin
  pinMode(4, INPUT_PULLUP); // Define Minutes adjust button pin
  pinMode(5, INPUT_PULLUP); // Define Hours adjust button pin
  TempShow = false;
  lc.shutdown(0, false);
  lc.setIntensity(0, 0);
  lc.clearDisplay(0);

  if (EEPROM.read(INIT_ADDR) != INIT_KEY) { // первый запуск
    EEPROM.write(INIT_ADDR, INIT_KEY);    // записали ключ

    EEPROM.write(0, single_touch);
    EEPROM.write(1, double_touch);
    EEPROM.write(2, threeple_touch);
    EEPROM.write(4, change_Hue);
  }

  EEPROM.get(0, single_touch);
  EEPROM.get(1, double_touch);
  EEPROM.get(2, threeple_touch);
  EEPROM.get(4, change_Hue);



}

void showOneLed(CRGB ledColor) {
  for (int i = 0; i < NUM_LEDS; i++) {
    led[i] = ledColor;
  }
  LEDS.show();
}
// 7 сигментный индикатор
void showDisplay() {
  tmElements_t Now;
  RTC.read(Now);
  //time_t Now = RTC.Now();// Getting the current Time and storing it into a DateTime object
  int hour = Now.Hour;
  int minutes = Now.Minute;
  int second = Now.Second;
  int GetTime =  hour * 100 + minutes;
  int last_min = minutes % 10;
  int first_min = minutes / 10;
  int last_hour = hour % 10;
  int first_hour = hour / 10;
  int arr_time[4] = {last_min, first_min, last_hour, first_hour};

  lc.setColumn(0, 1, number[first_hour]);
  lc.setColumn(0, 2, number[last_hour]);
  if(second%2){
    lc.setColumn(0, 2, number[last_hour] + B00000001);
  }
  
  lc.setColumn(0, 3, number[first_min]);
  lc.setColumn(0, 4, number[last_min]);

}

// Get time in a single number, if hours will be a single digit then time will be displayed 155 instead of 0155
int GetTime() {
  tmElements_t Now;
  RTC.read(Now);
  //time_t Now = RTC.Now();// Getting the current Time and storing it into a DateTime object
  int hour = Now.Hour;
  int minutes = Now.Minute;
  int second = Now.Second;
  if (second % 2 == 0) {
    Dot = false;
  }
  else {
    Dot = true;
  };
  return (hour * 100 + minutes);
};

// Check Light sensor and set brightness accordingly
//void BrightnessCheck() {
//  const byte sensorPin = 3; // light sensor pin
//  const byte brightnessLow = 50; // Low brightness value
//  const byte brightnessHigh = 254; // High brightness value
//  int sensorValue = digitalRead(sensorPin); // Read sensor
//  if (sensorValue == 0) {
//    LEDS.setBrightness(brightnessHigh);
//  }
//  else {
//    LEDS.setBrightness(brightnessLow);
//  }
//};

void BrightnessCheck() {
  if (auto_bright) {
    if (millis() - bright_timer > 500) {     // каждые 100 мс
      bright_timer = millis();
      int bright = analogRead(BRI_PIN);
      //Serial.print("Bright - "); Serial.print(bright); Serial.println();
      int bright_r;
      bright_r = bright % 10 > 5 ? ((bright / 10) * 10) + 10 : (bright / 10) * 10;
      new_bright = constrain(new_bright, min_bright, max_bright);
      new_bright = map(bright_r, 0, bright_constant, min_bright, max_bright);   // считать показания с фоторезистора, перевести диапазон

      new_bright_f = new_bright_f * coef + new_bright * (1 - coef);
      //Serial.print(new_bright_f );
      new_bright_f += 1;
      LEDS.setBrightness(new_bright_f);      // установить новую яркость
    }
  }
};

// Convert time to array needet for display
void TimeToArray() {

  int Now = GetTime();  // Get time
  int cursor = 29;

  //Serial.print("Value ihue - ");Serial.print(ihue); Serial.println();

  //  Serial.print("Time is: ");Serial.println(Now);
  //  if (DST) {  // if DST is true then add one hour
  //    Now += 100;
  //    //   Serial.print("DST is ON, time set to : ");Serial.println(Now);
  //  };
  if (Dot) {
    leds[14] = ledColor;
  }
  else {
    leds[14] = 0x000000;
  };
  for (int i = 1; i <= 4; i++) {
    int digit = Now % 10; // get last digit in time
    //Serial.print("what is NOW");Serial.print(Now);Serial.print(" ");

    if (i == 1) {

      // Serial.print("Digit 4 is : ");Serial.print(digit);Serial.print(" ");
      cursor = 22;
      for (int k = 0; k <= 6; k++) {
        //        Serial.print(digits[digit][k]);
        if (digits[digit][k] == 1) {
          leds[cursor] = ledColor;
        }
        else if (digits[digit][k] == 0) {
          leds[cursor] = 0x000000;
        };
        cursor ++;
        LEDS.show();

      };
      //Serial.println();


    }// fin if
    else if (i == 2) {
      //      Serial.print("Digit 3 is : ");Serial.print(digit);Serial.print(" ");
      cursor = 15;
      for (int k = 0; k <= 6; k++) {
        //        Serial.print(digits[digit][k]);
        if (digits[digit][k] == 1) {
          leds[cursor] = ledColor;
        }
        else if (digits[digit][k] == 0) {
          leds[cursor] = 0x000000;
        };
        cursor ++;
        LEDS.show();

      };
      //      Serial.println();

    }

    else if (i == 3) {
      //      Serial.print("Digit 2 is : ");Serial.print(digit);Serial.print(" ");
      cursor = 7;
      for (int k = 0; k <= 6; k++) {
        //        Serial.print(digits[digit][k]);
        if (digits[digit][k] == 1) {
          leds[cursor] = ledColor;
        }
        else if (digits[digit][k] == 0) {
          leds[cursor] = 0x000000;
        };
        cursor ++;
        LEDS.show();

      };
    }
    else if (i == 4) {
      //      Serial.print("Digit1 is : ");Serial.print(digit);Serial.print(" ");
      cursor = 0;
      for (int k = 0; k <= 6; k++) {
        //        Serial.print(digits[digit][k]);
        if (digits[digit][k] == 1) {
          leds[cursor] = ledColor;
        }
        else if (digits[digit][k] == 0) {
          leds[cursor] = 0x000000;
        };
        cursor ++;
        LEDS.show();

      };

    }
    Now /= 10;
  };
};

void TimeToArrayRainbow() {
  int Now = GetTime();  // Get time
  int cursor = 29;
  static uint8_t ihue = 1;
  static uint8_t thissat = 255;


  ihue = ihue + thisstep;
  //Serial.print("Value ihue - ");Serial.print(ihue); Serial.println();

  //  Serial.print("Time is: ");Serial.println(Now);

  if (Dot) {
    leds[14] = CHSV(ihue + 96, thissat, 255);
  }
  else {
    leds[14] = 0x000000;
  };
  for (int i = 1; i <= 4; i++) {
    int digit = Now % 10; // get last digit in time
    //Serial.print("what is NOW");Serial.print(Now);Serial.print(" ");

    if (i == 1) {
      // Serial.print("Digit 4 is : ");Serial.print(digit);Serial.print(" ");
      cursor = 22;
      for (int k = 0; k <= 6; k++) {
        //        Serial.print(digits[digit][k]);
        if (digits[digit][k] == 1) {
          leds[cursor] = CHSV(ihue + 32, thissat, 255);
        }
        else if (digits[digit][k] == 0) {
          leds[cursor] = 0x000000;
        };
        cursor ++;
        LEDS.show();

      };
      //Serial.println();


    }// fin if
    else if (i == 2) {
      //      Serial.print("Digit 3 is : ");Serial.print(digit);Serial.print(" ");
      cursor = 15;
      for (int k = 0; k <= 6; k++) {
        //        Serial.print(digits[digit][k]);
        if (digits[digit][k] == 1) {
          leds[cursor] = CHSV(ihue + 64, thissat, 255);
        }
        else if (digits[digit][k] == 0) {
          leds[cursor] = 0x000000;
        };
        cursor ++;
        LEDS.show();

      };
      //      Serial.println();

    }

    else if (i == 3) {
      //      Serial.print("Digit 2 is : ");Serial.print(digit);Serial.print(" ");
      cursor = 7;
      for (int k = 0; k <= 6; k++) {
        //        Serial.print(digits[digit][k]);
        if (digits[digit][k] == 1) {
          leds[cursor] = CHSV(ihue + 112, thissat, 255);
        }
        else if (digits[digit][k] == 0) {
          leds[cursor] = 0x000000;
        };
        cursor ++;
        LEDS.show();

      };
    }
    else if (i == 4) {
      //      Serial.print("Digit1 is : ");Serial.print(digit);Serial.print(" ");
      cursor = 0;
      for (int k = 0; k <= 6; k++) {
        //        Serial.print(digits[digit][k]);
        if (digits[digit][k] == 1) {
          leds[cursor] = CHSV(ihue + 128, thissat, 255);
        }
        else if (digits[digit][k] == 0) {
          leds[cursor] = 0x000000;
        };
        cursor ++;
        LEDS.show();

      };

    }
    Now /= 10;
  };
};

void TempToArray() {
  tmElements_t tm;
  RTC.read(tm);
  if (tm.Second != 20) {
    TempShow = false;
    //Serial.print("Second ");Serial.println(tm.Second);
    return;
  }
  static uint8_t ihue = 1;
  static uint8_t thissat = 255;
  ihue = ihue + thisstep;

  TempShow = true;
  sensors.requestTemperatures();
  int t = sensors.getTempCByIndex(0);
  int celsius = t * 10;
  if (celsius < 0) {
    celsius = celsius * (-1);
  }
  //  Serial.print("Temp is: ");Serial.println(celsius);
  //  Serial.print(sensors.getTempCByIndex(0));Serial.println();
  int cursor = 29; // last led number

  leds[14] = 0x000000;

  for (int i = 1; i <= 4; i++) {
    int digit = celsius % 10; // get last digit in time

    if (i == 1) {
      //Serial.print("Digit 4 is : "); Serial.print(digit); Serial.print(" ");
      cursor = 22;

      for (int k = 0; k <= 6; k++) {
        //  Serial.print(digits[11][k]);
        if (digits[11][k] == 1) {
          leds[cursor] = 0x0000FF;
        }
        else if (digits[11][k] == 0) {
          leds[cursor] = 0x000000;
        };
        cursor ++;
        LEDS.show();


      };
      //  Serial.println();
    }
    else if (i == 2) {
      //Serial.print("Digit 3 is : "); Serial.print(digit); Serial.print(" ");
      cursor = 15;

      for (int k = 0; k <= 6; k++) {
        //   Serial.print(digits[10][k]);
        if (digits[digit][k] == 1) {
          leds[cursor] = 0x0000FF;
        }
        else if (digits[digit][k] == 0) {
          leds[cursor] = 0x000000;
        };
        cursor ++;
        LEDS.show();

      };
      //  Serial.println();
    }
    else if (i == 3) {
      //Serial.print("Digit 2 is : "); Serial.print(digit); Serial.print(" ");
      cursor = 7;
      for (int k = 0; k <= 6; k++) {
        //  Serial.print(digits[digit][k]);
        if (digits[digit][k] == 1) {
          leds[cursor] = 0x0000FF;
        }
        else if (digits[digit][k] == 0) {
          leds[cursor] = 0x000000;
        };
        cursor ++;
        LEDS.show();

      };
      // Serial.println();
    }
    else if (i == 4 ) {
      //Serial.print("Digit 1 is : "); Serial.print(digit); Serial.print(" ");
      cursor = 0;
      if (t < 0) {
        for (int k = 0; k <= 6; k++) {
          //   Serial.print(digits[digit][k]);
          if (digits[10][k] == 1) {
            leds[cursor] = 0x0000FF;
          }
          else if (digits[10][k] == 0) {
            leds[cursor] = 0x000000;
          };
          cursor ++;
          LEDS.show();
        };
      }
      else {
        for (int k = 0; k <= 6; k++) {
          leds[cursor] = 0x000000;
          cursor ++;
          LEDS.show();

        }
      }
      //  Serial.println();
    }
    celsius /= 10;
    //Serial.print("Temp is /10: ");Serial.println(celsius);
  };
};


void TimeAdjust() {
  int buttonH = digitalRead(5);
  int buttonM = digitalRead(4);
  if (buttonH == LOW || buttonM == LOW) {
    delay(500);
    tmElements_t Now;
    RTC.read(Now);
    int hour = Now.Hour;
    int minutes = Now.Minute;
    int second = Now.Second;
    if (buttonH == LOW) {
      if (Now.Hour == 23) {
        Now.Hour = 0;
      }
      else {
        Now.Hour += 1;
      };
    } else {
      if (Now.Minute == 59) {
        Now.Minute = 0;
      }
      else {
        Now.Minute += 1;
      };
    };
    RTC.write(Now);
  }
}


void loop()  // Main loop
{
  showDisplay(); //Show 7 segment display
  BrightnessCheck(); // Check brightness


  touch.tick();
  if (millis() - timer >= PERIOD) {
    if (touch.isSingle()) {
      single_touch = true;
      double_touch = false;
      threeple_touch = false;
      change_Hue += 32;
      if (change_Hue > 256) change_Hue = 0;
      // Serial.print(change_Hue); Serial.println();

    }

    if (single_touch) {
      ledColor = CHSV(change_Hue, 255, 255);
      EEPROM.update(4, change_Hue);
      //Serial.print("One click = "); Serial.print(change_Hue); Serial.println();
      TimeToArray();
    }

    if (touch.isDouble() || double_touch) {
      single_touch = false;
      double_touch = true;
      threeple_touch = false;
      if (millis() - timer >= 1000) {
        static uint8_t ihue = 1;
        static uint8_t thissat = 255;
        int thisstep = 1 ;
        ihue = ihue + thisstep;
        ledColor = CHSV(ihue, thissat, 255); //Serial.print("Double click" );
        timer += 1000;
      }
      TimeToArray();
    }
    if (touch.isTriple() || threeple_touch) {

      single_touch = false;
      double_touch = false;
      threeple_touch = true;
      //Serial.print("Threeple click"); Serial.println();
      TimeToArrayRainbow();
    }
    //showDisplay();
    //      DSTcheck(); // Check DST
    TimeAdjust(); // Check to se if time is geting modified
    // Get leds array with required configuration
    TempToArray();
    if (TempShow == true) delay (8000);

    // Записали в память
    EEPROM.update(0, single_touch);
    EEPROM.update(1, double_touch);
    EEPROM.update(2, threeple_touch);

    timer += PERIOD;
  }
  showOneLed(ledColor);


}
