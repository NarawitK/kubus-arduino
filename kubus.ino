#include <ArduinoJson.h>
#include <SPI.h>
#include <WiFiNINA.h>
#include <TinyGPS++.h>
#include <SoftwareSerial.h>

static const int RXPin = 2, TXPin = 3;
static const uint32_t GPSBaud = 9600;

//TinyGPS++ Object
TinyGPSPlus gps;

// The serial connection to the GPS device
SoftwareSerial ss(RXPin, TXPin);
/*
   Real-Server
   char server[] = "158.108.207.4";
*/
char ssid_name[] = "Test";
char pass[] = "qjmg8259";
//char pass[] = "LVVM01277";
char server[] = "158.108.207.4";
WiFiClient client;
int status = WL_IDLE_STATUS;
unsigned long lastConnectionTime = 0;
const unsigned long postingInterval = 7L * 1000L;

float latitude, longitude, spd, course;
//Hard-coded busid by device
int BusID = 98;

void setup() {
  // put your setup code here, to run once:
  Serial.begin(9600);
  ss.begin(GPSBaud);

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
  checkWiFiConnection();
  while (ss.available() > 0) {
    if (gps.encode(ss.read()) && gps.location.isUpdated() ) {
      displayInfo();
      if (millis() - lastConnectionTime > postingInterval) {
        if (client.connected()) {
          client.stop();
        }
        UpdateLocation();
      }
    }
  }
  if (millis() > 5000 && gps.charsProcessed() < 10) {
    Serial.println("ERROR: not getting any GPS data!");
    while (millis() > 5000 && gps.charsProcessed() < 10);
  }
}

void UpdateLocation() {
  delay(1000);
  if (client.connect(server, 80)) {
    char mode[] = "p1";
    const int capacity = JSON_OBJECT_SIZE(8);
    StaticJsonDocument<capacity> doc;
    doc["postmode"] = mode;
    doc["bus_id"] = BusID;
    doc["latitude"] = latitude;
    doc["longitude"] = longitude;
    doc["course"] = course;
    doc["speed"] = spd;
    client.println("POST /kubus/api/bus_post.php HTTP/1.1");
    client.print("Host: ");
    client.println(server);
    client.println("User-Agent: ArduinoWiFi/1.1");
    client.println("Content-Type: application/json");
    client.print("Content-Length: ");
    client.println(measureJson(doc));
    client.println("Connection: close");
    client.println();
    serializeJson(doc, client);

    lastConnectionTime = millis();
    serializeJson(doc, Serial);
    Serial.println();
    Serial.println("Uploaded");
  }
  else {
    Serial.println("UpdateLocationFailed.");
  }
}

void displayInfo() {
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
    Serial.println(ssid_name);
    //status = WiFi.begin(ssid_name);
    status = WiFi.begin(ssid_name, pass);
    delay(10000);
    // wait 10 secs. for connection
    if (status == WL_CONNECTED) {
      digitalWrite(9, HIGH);
    }
  }
}

void checkWiFiConnection() {
  status = WiFi.status();
  if (status == WL_DISCONNECTED || status == WL_CONNECTION_LOST) {
    digitalWrite(9, LOW);
    Serial.println(status);
    connectToWifi();
    printCurrentNet();
    printWifiData();
  }
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
