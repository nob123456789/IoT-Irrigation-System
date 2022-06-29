String myAPIkey = "ENTER API KEY HERE";  

#include <Wire.h>
#include "DS1307.h"
#include <SoftwareSerial.h>

SoftwareSerial ESP8266(2, 3); // Rx,  Tx

DS1307 clock; // define a object of DS1307 class

const int pump1 = 4; // pump1 set to digital pin 4
const int pump2 = 5;

// Define moisture sensors
const int MOISTURE_1 = A0;
const int MOISTURE_2 = A1;
const int MOISTURE_3 = A2;

// moisture values
int moist1Value = 0;
int moist2Value = 0;
int moist3Value = 0;

int moistAvg = 0; // average of all sensor values

int dryLimit = 420; // how dry you will allow your plants to be on average

// esp8266 stuff
unsigned char check_connection=0;
unsigned char times_check=0;
boolean error;
bool moisted = false;

void setup()
{
  Serial.begin(9600); 
  ESP8266.begin(9600); 
  pinMode(pump1, OUTPUT);
  pinMode(pump2, OUTPUT);
  clock.begin();
  ESP8266.println("AT+RST");

  delay(2000);
  Serial.println("Connecting to Wifi");

  // try to connect to wifi
  while(check_connection==0){
    Serial.print(".");
    // PUT SSID AND PASSWORD BELOW!!!
    ESP8266.print("AT+CWJAP=\"Wifi Network's name\",\"PassWord\"\r\n");
    ESP8266.setTimeout(5000);
    
    if(ESP8266.find("WIFI CONNECTED\r\n")==1) {
      Serial.println("WIFI CONNECTED");
      break;
    }
    times_check++;
    
    if(times_check>3) {
      times_check=0;
      Serial.println("Trying to Reconnect..");
    }
  }
}
void loop()
{
  measureAndWater();
  writeThingSpeak();

  // Once a day at 11:00:01, water the plants regardless of moisture levels indicated
  switch (clock.hour)
  {
  case 11: // at 11AM
    switch (clock.minute)
    {
    case 0: // at 0 minutes
      switch (clock.second)
      {
      case 1: // at 1 second
        waterPlants();
      }
    }
  }
}

// funcs below:
// watering the plants
void waterPlants() {
  digitalWrite(pump1, HIGH); // turn the pump1 on (HIGH is the voltage level)
  digitalWrite(pump2, LOW);
  delay(10000);               // wait for 10 seconds
  digitalWrite(pump1, LOW);  // turn the pump1 off by making the voltage LOW
  digitalWrite(pump2, LOW);
  delay(1000);               // wait for 1 second
  moisted = true;
}

void measureAndWater() {
  // read the analog value of the sensors:
  clock.getTime();
  switch (clock.second) {
  case 59:
    delay(2);
    moist1Value = analogRead(MOISTURE_1);
    moist2Value = analogRead(MOISTURE_2);
    moist3Value = analogRead(MOISTURE_3);

    // average
    moistAvg = (moist1Value + moist2Value + moist3Value) / 3;
    delay(2);
    }
   if (moistAvg > dryLimit){
    waterPlants();
  }
}

void writeThingSpeak(void)
{
  startThingSpeakCmd();
  // prepare string
  String getStr = "GET /update?api_key=";
  getStr += myAPIkey;
  getStr +="&field1=";
  getStr += String(moist1Value);
  getStr +="&field2=";
  getStr += String(moist2Value);
  getStr +="&field3=";
  getStr += String(moist3Value);
  if (moisted = true) {
    getStr +="&field4=";
    getStr +="Watered";
    moisted = false;
  }
  getStr += "\r\n\r\n";
  GetThingspeakcmd(getStr);
}


void startThingSpeakCmd(void)
{
  ESP8266.flush();
  String cmd = "AT+CIPSTART=\"TCP\",\"";
  cmd += "184.106.153.149"; // api.thingspeak.com IP address
  cmd += "\",80";
  ESP8266.println(cmd);
  Serial.print("Start Commands: ");
  Serial.println(cmd);

  if(ESP8266.find("Error"))
  {
    Serial.println("AT+CIPSTART error");
    return;
  }
}


String GetThingspeakcmd(String getStr) {
  String cmd = "AT+CIPSEND=";
  cmd += String(getStr.length());
  ESP8266.println(cmd);
  Serial.println(cmd);

  if(ESP8266.find(">"))
  {
    ESP8266.print(getStr);
    Serial.println(getStr);
    delay(500);
    String messageBody = "";
    while (ESP8266.available()) 
    {
      String line = ESP8266.readStringUntil('\n');
      if (line.length() == 1) 
      { 
        messageBody = ESP8266.readStringUntil('\n');
      }
    }
    Serial.print("MessageBody received: ");
    Serial.println(messageBody);
    return messageBody;
  }
  else
  {
    ESP8266.println("AT+CIPCLOSE");
    Serial.println("AT+CIPCLOSE"); 
  } 
}
