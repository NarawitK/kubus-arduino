#include <ArduinoJson.h>
#include "secret.h"
#include <SPI.h>
#include <WiFiNINA.h>
#include <TinyGPS++.h>

static const uint32_t GPSBaud = 9600;
TinyGPSPlus gps;

const int ledPin =  LED_BUILTIN;
int ledState = LOW;
unsigned long previousMillis = 0;
const long blinkInterval = 1000;

char ssid_name[] = SECRET_SSID;
char pass[] = SECRET_PASS;
char server[] = SECRET_SERVER;
WiFiClient client;
int status = WL_IDLE_STATUS;
unsigned long lastConnectionTime = 0;
const unsigned long postingInterval = 2L * 1000L;

float latitude, longitude, spd, course;
int BusID = BUS_ID;

void setup() {
  // put your setup code here, to run once:
  Serial1.begin(GPSBaud);
  pinMode(ledPin, OUTPUT);

  // check for the WiFi module
  if (WiFi.status() == WL_NO_MODULE) {
    while (true);
  }
  connectToWifi();
}

void loop() {
  while (Serial1.available() > 0) {
    if (gps.encode(Serial1.read()) && gps.location.isUpdated() ) {
      GatherInfo();
      if (millis() - lastConnectionTime > postingInterval ) {
        UpdateLocation();
      }
    }
  }
}

void UpdateLocation() {
  checkWiFiConnection();
  if (client.connected()) {
    client.stop();
  }
  if (client.connect(server, 80)) {
    const int capacity = JSON_OBJECT_SIZE(7);
    StaticJsonDocument<capacity> doc;
    doc["bus_id"] = BusID;
    doc["latitude"] = latitude;
    doc["longitude"] = longitude;
    doc["course"] = course;
    doc["speed"] = spd;
    client.print("POST /kubus/api/v1/bus_post.php");
    //client.print("POST /kubus/api/v2/gps-update-location/");
    //client.print(BusID);
    client.println(" HTTP/1.1");
    client.print("Host: ");
    client.println(server);
    client.println("User-Agent: ArduinoWiFi/1.1");
    client.println("Access-Control-Allow-Origin: *");
    client.print("Content-Length: ");
    client.println(measureJson(doc));
    client.println("Content-Type: application/json");
    client.println("Connection: close");
    client.println();
    serializeJson(doc, client);
    lastConnectionTime = millis();
  }
  else {
    if (client.connected()) {
      client.stop();
    }
  }
}

void GatherInfo() {
  if (gps.location.isValid()) {
    latitude = gps.location.lat();
    longitude = gps.location.lng();
    spd = gps.speed.kmph();
    course = gps.course.deg();
  }
}

void connectToWifi() {
  while (status != WL_CONNECTED) {
    BlinkLED();
    status = WiFi.begin(ssid_name, pass);
    if (status == WL_CONNECTED) {
      LEDOn();
    }
  }
}

void checkWiFiConnection() {
  status = WiFi.status();
  if (status == WL_DISCONNECTED || status == WL_CONNECTION_LOST) {
    LEDOff();
    connectToWifi();
  }
}

void BlinkLED() {
  unsigned long currentMillis = millis();
  if (currentMillis - previousMillis >= blinkInterval) {
    previousMillis = currentMillis;
    if (ledState == LOW) {
      LEDOn();
    }
    else {
      LEDOff();
    }
  }
}
void LEDOn() {
  ledState = HIGH;
  digitalWrite(ledPin, ledState);
}
void LEDOff() {
  ledState = LOW;
  digitalWrite(ledPin, ledState);
}
