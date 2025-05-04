#include <SoftwareSerial.h>
#include <Wire.h>
#include <math.h>

#include "Adafruit_Sensor.h"
#include "Adafruit_AM2320.h"
#include <Adafruit_BMP085.h>
#include <LiquidCrystal_I2C.h>

#include "Forecaster.h"

#define seaLevelPressure_hPa 1013.25

String HOST = "api.thingspeak.com";
String PORT = "80";
String AP = "XXXXX"; // Your AP here
String PASS = "XXXXX"; // Your password here

int successOrder = 0;

String API = "XXXXXXX"; // Your write API on thingspeak here
String field = "field1";

Adafruit_BMP085 bmp;
SoftwareSerial esp(7, 6);
Adafruit_AM2320 am2320 = Adafruit_AM2320();
Forecaster cond;
LiquidCrystal_I2C lcd(0x27, 16, 2);

uint16_t showorder = 0;

void setup() {
  Serial.begin(9600);
  esp.begin(9600);

  if (!bmp.begin()) {
    Serial.println("BMP180 NOT FOUND!");
    while (1) {}
  }

  lcd.init();
  lcd.backlight();
  lcd.setCursor(0, 0);
  lcd.print("WEATHER STATION");

  sendCommand("AT", "OK", 5);
  sendCommand("AT+CWMODE=1", "OK", 5);
  sendCommand("AT+CWJAP=\""+AP+"\",\""+PASS+"\"", "OK", 15);

  successOrder = 0;

  while (!Serial) {
    delay(10);
  }

  am2320.begin();
}

void loop() {
  float temp = am2320.readTemperature();
  float humid = am2320.readHumidity();
  float pressure = bmp.readPressure();
  
  static uint32_t tmr;
  static uint32_t data_millis;
  static boolean first = false;
  static boolean didSetH = false;

  if (!didSetH)
  {
   cond.setH(bmp.readAltitude(seaLevelPressure_hPa * 100));
   didSetH = true;
  }

  if (millis() - tmr >= 30*60*1000ul) { // Updates pressure every 30 minutes, 3 hours interval
    tmr = millis();
    cond.addP(pressure, temp);
    Serial.println(cond.getCast());
  }

  lcd.setCursor(0, 1);
  //Serial.println(requestSendForecast);

  if (millis() - data_millis >= 2*60*1000ul) {
    boolean done = sendDataESP(temp, humid, pressure);

    if (!first) {
      lcd.print("Sending data....");
      first = true;
    }

    if (done) {
      data_millis = millis();
      clearLCDLine(1);
      lcd.print("SUCCESS");
      first = false;

      delay(2000);
    }
  }
  else 
  {
    clearLCDLine(1);

    switch (showorder++) {
      case 0: lcd.print("Tmp:" + String(temp) + "*C"); break;
      case 1: lcd.print("Hum:" + String(humid) + "%"); break;
      case 2: lcd.print("Prs:" + String(pressure/100) + "hPa"); break;
      case 3: lcd.print("P:" + getForecastText(round(cond.getCast()))); showorder = 0; break;
    }

    delay(3000);
  }
}

boolean sendDataESP(float temp, float humid, float pressure)
{
  String s_tmp = String(temp);
  String s_hum = String(humid);
  String s_prs = String(pressure);
  int id = round(cond.getCast());
  String id_cast = String(id);

  String getData = "GET /update?api_key="+ API;

  String byteLen = String(
    36 + 8 * 4
    + s_tmp.length() + s_hum.length() + s_prs.length() + id_cast.length()
    + 4
    );

  boolean success = false;

  switch (successOrder) {
    case 0: success = sendCommand("AT", "OK", 5);break;
    case 1: success = sendCommand("AT+RST", "OK", 10);break;
    case 2: success = sendCommand("AT+CIPMUX=1", "OK", 5); break;
    case 3: success = sendCommand("AT+CIPSTART=0,\"TCP\",\""+ HOST +"\","+ PORT, "OK", 15); break;
    case 4: success = sendCommand("AT+CIPSEND=0," + byteLen, ">", 4); break;
    case 5: {
      String tosend = getData +"&"+ field +"=" + s_tmp +"&field2=" + s_hum +"&field3=" + s_prs += "&field4=" + id_cast;

      Serial.println(tosend);
      esp.println(tosend);
      delay(1500);
      esp.println();
      delay(1500);
      success = true;
      break;
    }//case 6: success = sendCommand("AT+CIPSEND=0," + byteLen, ">", 4); break;
    //case 5: esp.println(getData +"&field1=" + String(humid)); delay(1500); success = true; break;
    case 6: sendCommand("AT+CIPCLOSE=0", "OK", 5); success = true; break;
  }

    if (success) {
    if (successOrder++ == 6)
      {
        successOrder -= 7;
        return true;
      }
  } else {
    successOrder = 0;
  }

  delay(1000);
  return false;
}

boolean sendCommand(String command, char readReplay[], int maxInterval) {
  boolean found = false;
  int interval = 0;

  Serial.print(command);
  Serial.print(" ");

  while(interval < maxInterval) {
    esp.println(command);
    if(esp.find(readReplay))
    {
      found = true;
      break;
    }

    interval++;
  }
  
  if(found) {
    Serial.println("SUCCESS");
    return true;
  }

  Serial.println("FAIL");
  return false;
}

void clearLCDLine(int line) {
  for(int n = 0; n < 16; n++)
  {
    lcd.setCursor(n,line);
    lcd.print(" ");
  }
  lcd.setCursor(0,line);
}

String getForecastText(int zambrettiNumber) {
  switch(zambrettiNumber)   
    {
    case 1:
        return "STABLE FINE";
    case 2:
        return "FINE";
    case 3:
        return "UNSTABLE FINE";
    case 4:
        return "DEVELOPING";
    case 5:
        return "SHOWERS";
    case 6:
        return "UNSETTLED";
    case 7:
        return "RAIN";
    case 8:
        return "UNSETTLED RAIN";
    case 9:
        return "BAD RAIN";
    case 10:
        return "SETTLED FINE";
    case 11:
        return "FINE";
    case 12:
        return "UNSTABLE FINE";
    case 13:
        return "FAIRLY FINE";
    case 14:
        return "SHOWERY";
    case 15:
        return "SOME RAIN";
    case 16:
        return "UNSETTLED RAIN";
    case 17:
        return "FREQUENT RAIN";
    case 18:
        return "BAD RAIN";
    case 19:
        return "STORMY RAIN";
    case 20:
        return "STABLE FINE";
    case 21:
        return "FINE";
    case 22:
        return "TURNING FINE";
    case 23:
        return "IMPROVING";
    case 24:
        return "SHOWERS, FINE";
    case 25:
        return "SHOWERS, IMPR.";
    case 26:
        return "CHANGEABLE";
    case 27:
        return "UNSETTLED, CLEAR";
    case 28:
        return "UNSETTLED, IMPR.";
    case 29:
        return "SHORT FINE";
    case 30:
        return "V. UNSETTLED";
    case 31:
        return "STORMY, IMPR.";
    case 32:
        return "STORMY, RAIN";
    default:
        return "DATA TOO SMALL";
    }
}
