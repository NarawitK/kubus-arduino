#include <ArduinoJson.h>
#include "secret.h"
#include <SPI.h>
#include <WiFiNINA.h>
#include <TinyGPS++.h>

static const uint32_t GPSBaud = 9600;

const int ledPin =  LED_BUILTIN;
int ledState = LOW;
unsigned long previousMillis = 0;
const long blinkInterval = 1000;

TinyGPSPlus gps;

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
  Serial.begin(9600);
  Serial1.begin(GPSBaud);
  pinMode(ledPin, OUTPUT);

  // check for the WiFi module
  if (WiFi.status() == WL_NO_MODULE) {
    Serial.println("Communication with WiFi module failed!");

    // don't continue
    while (true);
  }
  connectToWifi();
  Serial.print("You're connected to the network");
  printCurrentNet();
  printWifiData();
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
  if (millis() > 5000 && gps.charsProcessed() < 10) {
    Serial.println("ERROR: not getting any GPS data!");
  }
}

void UpdateLocation() {
  checkWiFiConnection();
  if (client.connected()) {
    client.stop();
  }
  if (client.connect(server, 80)) {
    Serial.println("Generating POST Header");
    //char mode[] = "p1";
    const int capacity = JSON_OBJECT_SIZE(7);
    StaticJsonDocument<capacity> doc;
    //doc["postmode"] = mode;
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
    Serial.println();
    Serial.println("Uploaded");
  }
  else {
    Serial.println("UpdateLocationFailed.");
    if (client.connected()) {
      client.stop();
    }
  }
}

void GatherInfo() {
  Serial.print(F("Location: "));
  if (gps.location.isValid()) {
    latitude = gps.location.lat();
    Serial.print(gps.location.lat(), 6);
    Serial.print(F(","));
    longitude = gps.location.lng();
    Serial.println(gps.location.lng(), 6);
    Serial.print(F("Speed: "));
    spd = gps.speed.kmph();
    Serial.println(spd);
    Serial.print(F("Course: "));
    course = gps.course.deg();
    Serial.println(course);
  }
  else {
    Serial.print(F("Invalid "));
  }
  Serial.print(F("Date/Time: "));
  if (gps.date.isValid())
  {
    Serial.print(gps.date.day());
    Serial.print(F("/"));
    Serial.print(gps.date.month());
    Serial.print(F("/"));
    Serial.print(gps.date.year());
  }
  else
  {
    Serial.print(F("INVALID"));
  }

  Serial.print(F(" "));
  if (gps.time.isValid())
  {
    if (gps.time.hour() < 10) Serial.print(F("0"));
    Serial.print(gps.time.hour() + 7);
    Serial.print(F(":"));
    if (gps.time.minute() < 10) Serial.print(F("0"));
    Serial.print(gps.time.minute());
    Serial.print(F(":"));
    if (gps.time.second() < 10) Serial.print(F("0"));
    Serial.print(gps.time.second());
    Serial.print(F("."));
  }
  else
  {
    Serial.print(F("INVALID"));
  }
  Serial.println();
}

void connectToWifi() {
  while (status != WL_CONNECTED) {
    Serial.print("Attempting to connect to open SSID: ");
    BlinkLED();
    Serial.println(ssid_name);
    status = WiFi.begin(ssid_name, pass);
    // wait 5 secs. for connection
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
    printCurrentNet();
    printWifiData();
  }
}

void BlinkLED() {
  unsigned long currentMillis = millis();
  if (currentMillis - previousMillis >= blinkInterval) {
    previousMillis = currentMillis;
    // if the LED is off turn it on:
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

//Testing Purpose.
void printWifiData() {
  // print your board's IP address:
  IPAddress ip = WiFi.localIP();
  Serial.print("IP Address: ");
  Serial.println(ip);

  // print your subnet mask:
  IPAddress subnet = WiFi.subnetMask();
  Serial.print("NetMask: ");
  Serial.println(subnet);

  // print your gateway address:
  IPAddress gateway = WiFi.gatewayIP();
  Serial.print("Gateway: ");
  Serial.println(gateway);
}

void printCurrentNet() {
  // print the SSID of the network you're attached to:
  Serial.print("SSID: ");
  Serial.println(WiFi.SSID());
}
