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

char ssid[] = "RDM";
//char pass[] = "LVVM01277";
char server[] = "192.168.137.1";
WiFiClient client;
int status = WL_IDLE_STATUS;

unsigned long lastConnectionTime = 0;
const unsigned long postingInterval = 7L * 1000L;

float latitude, longitude, spd;
char BusID[2] = "1";

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

  while (status != WL_CONNECTED) {
    //No encryption.
    Serial.print("Attempting to connect to open SSID: ");
    Serial.println(ssid);
    status = WiFi.begin(ssid);
    //status = WiFi.begin(ssid, pass);
    // wait 10 secs. for connection

    delay(10000);
  }

  Serial.print("You're connected to the network");
  printCurrentNet();
  printWifiData();
}

void loop() {
  // put your main code here, to run repeatedly:
  while (ss.available() > 0) {
    if (gps.encode(ss.read()) && gps.location.isUpdated()) {
      displayInfo();
      if (millis() - lastConnectionTime > postingInterval) {
        UpdateLocation();
      }
    }
  }
  if (millis() > 5000 && gps.charsProcessed() < 10) {
    Serial.println("ERROR: not getting any GPS data!");
    while (millis() > 5000 && gps.charsProcessed() < 10);
  }

  while (client.available()) {
    char c = client.read();
    Serial.write(c);
  }
  delay(1000);
}

void UpdateLocation() {
  client.stop();
  if (client.connect(server, 80)) {
    char mode[2] = "P1";
    const int capacity = JSON_OBJECT_SIZE(6);
    StaticJsonDocument<capacity> doc;
    doc["postmode"] = mode;
    doc["bus_id"] = BusID;
    doc["latitude"] = latitude;
    doc["longitude"] = longitude;
    doc["speed"] = spd;
    doc["step"] = 100;
    client.println("POST /kubus/api/index.php HTTP/1.1");
    client.print("Host: ");
    client.println(server);
    client.println("User-Agent: ArduinoWiFi/1.1");
    client.println("Content-Type: application/json");
    client.print("Content-Length: ");
    client.println(measureJson(doc));
    client.println("Connection: close");
    //Terminate Header
    client.println();

    client.print("?postmode=");
    client.println(mode);
    client.print(",data=");
    serializeJson(doc, client);

    lastConnectionTime = millis();
    Serial.println("Uploaded");
  }
  else {
    Serial.println("ConnectionFailed.");
  }
}

void displayInfo() {
  Serial.print(F("Location: "));
  if (gps.location.isValid()) {
    Serial.print(gps.location.lat(), 6);
    Serial.print(F(","));
    Serial.println(gps.location.lng(), 6);
    Serial.print("Speed: ");
    spd = (gps.speed.kmph());
    Serial.println(spd);
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

void printWifiData() {
  // print your board's IP address:
  IPAddress ip = WiFi.localIP();
  Serial.print("IP Address: ");
  Serial.println(ip);
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
