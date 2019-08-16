#include <EEPROM.h>
#include <HX711.h>
#include <LedControl.h>
#include <DallasTemperature.h>
#include <OneWire.h>

const int LOADCELL_DOUT_PIN = 2;
const int LOADCELL_SCK_PIN = 3;
const int LED_DATA = 9;
const int LED_CLK = 8;
const int LED_CS = 10;
const int ONE_WIRE_BUS = 12;
const byte TEMP_RESOLUTION = 11;
const int TEMP_DELAY = 750 / (1 << (12 - TEMP_RESOLUTION));
const int BTN_DELAY = 500;
const int C_PLUS_PIN = 7;
const int C_MINUS_PIN = 6;
const int PLUS_PIN = 5;
const int MINUS_PIN = 4;
const float BEER_W = 8;


int offset;
int count = 0;
int lastCount = 0;
float temperature = -1;
int lastTempRequest;
float lastTemp = -1;
int lastBtnRead;
int plus;
int minus;
int milis;

LedControl lc = LedControl(LED_DATA, LED_CLK, LED_CS, 1);
HX711 scale;
OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);
DeviceAddress tempDeviceAddress;

void setupScale() {
  scale.begin(LOADCELL_DOUT_PIN, LOADCELL_SCK_PIN);
  scale.set_scale(2280.f);
  scale.tare();
}

void setupLCD() {
  lc.shutdown(0, false);
  lc.setIntensity(0, 8);
  lc.clearDisplay(0);
}

void setupTemp() {
  sensors.getAddress(tempDeviceAddress, 0);
  sensors.setResolution(tempDeviceAddress, TEMP_RESOLUTION);
  sensors.setWaitForConversion(false);
  sensors.begin();
  sensors.requestTemperatures();
  lastTempRequest = millis();
}

void printNumber(int v, int pos, int dot) {
  int ones;
  int tens;
  int hundreds;

  boolean negative = false;

  if (v < -999 || v > 999)
    return;
  if (v < 0) {
    negative = true;
    v = v * -1;
  }
  ones = v % 10;
  v = v / 10;
  tens = v % 10;
  v = v / 10; hundreds = v;
  if (negative) {
    //print character '-' in the leftmost column
    lc.setChar(0, 3, '-', false);
  }
  else {
    //print a blank in the sign column
    lc.setChar(0, pos + 3, ' ', false);
  }
  //Now print the number digit by digit
  if (hundreds != 0) {
    lc.setDigit(0, pos + 2, (byte)hundreds, (dot == 2));
  } else {
    lc.setChar(0, pos + 2, ' ', (dot == 2));
  }
  if (hundreds == 0 && tens == 0) {
    lc.setChar(0, pos + 1, ' ', (dot == 1));
  } else {
    lc.setDigit(0, pos + 1, (byte)tens, (dot == 1));
  }
  lc.setDigit(0, pos, (byte)ones, (dot == 0));
}

void printFragaria() {
  lc.setChar(0, 0, 'a', false);
  lc.setChar(0, 1, '1', false);
  lc.setRow(0, 2, 0x05);
  lc.setChar(0, 3, 'a', false);
  lc.setChar(0, 4, '9', false);
  lc.setChar(0, 5, 'a', false);
  lc.setRow(0, 6, 0x05);
  lc.setChar(0, 7, 'f', false);
}

void printErr(byte disp) {
  for (byte i; i < 4; i++) {
    lc.setChar(0, i + 4 * disp, '-', false);
  }
}

void setupButtons() {
  pinMode(PLUS_PIN, INPUT_PULLUP); // Define the arcade switch NANO pin as an Input using Internal Pullups
  digitalWrite(PLUS_PIN, HIGH);
  pinMode(MINUS_PIN, INPUT_PULLUP); // Define the arcade switch NANO pin as an Input using Internal Pullups
  digitalWrite(MINUS_PIN, HIGH);
  pinMode(C_PLUS_PIN, INPUT_PULLUP); // Define the arcade switch NANO pin as an Input using Internal Pullups
  digitalWrite(C_PLUS_PIN, HIGH);
  pinMode(C_MINUS_PIN, INPUT_PULLUP); // Define the arcade switch NANO pin as an Input using Internal Pullups
  digitalWrite(C_MINUS_PIN, HIGH);
  lastBtnRead = millis();
}

void saveOffset(int c) {
  EEPROM.put(0, c);
}

int loadOffset() { 
  int c;
  EEPROM.get(0, c);
  return c;
}

void sendData(float temp, int count, int timestamp) {
  static char outstr[4];
  dtostrf(temp, 4, 1, outstr);
  Serial.println("t:" + String(timestamp, DEC) + " count:" + String(count) + " temp:" + outstr);
}

void setup() {
  Serial.begin(38400);
  Serial.println("starting");
  setupLCD();
  printFragaria();

  setupButtons();
  setupScale();
  setupTemp();

  offset = loadOffset();
  lastCount = offset;
  lc.clearDisplay(0);
}

void loop() {
  milis = millis();
  if (scale.wait_ready_timeout(1000)) {
    long reading = scale.get_units(5);
    count = round(-reading / BEER_W) + offset;
    printNumber(count , 0, -1);
    if (lastCount != count) {
      sendData(temperature, count, milis);
      saveOffset(count);
      lastCount = count;
    }
  } else {
    printErr(0);
  }

  if (milis - lastTempRequest >= TEMP_DELAY) // waited long enough??
  {
    temperature = sensors.getTempCByIndex(0);
    sensors.requestTemperatures();
    lastTempRequest = millis();
    printNumber(temperature * 10, 4, 1);
    if (abs(int(lastTemp * 10) - int(temperature * 10)) >= 1) {
      sendData(temperature, count, milis);
      lastTemp = temperature;
    }
  }

  if (milis - lastBtnRead >= BTN_DELAY) // waited long enough??
  {
    plus = digitalRead(PLUS_PIN);
    minus = digitalRead(MINUS_PIN);

    if (plus == HIGH) {
      offset += 1;
      lastBtnRead = milis;
    }

    if (minus == HIGH) {
      offset -= 1;
      lastBtnRead = milis;
    }

    if (digitalRead(C_PLUS_PIN) == HIGH) {
      lastBtnRead = milis;
    }

    if (digitalRead(C_MINUS_PIN) == HIGH) {
      lastBtnRead = milis;
    }
  }


  //scale.power_down();			        // put the ADC in sleep mode
  //scale.power_up();

}
