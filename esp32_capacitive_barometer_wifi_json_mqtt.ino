#include "heltec.h"
#include <Adafruit_BMP085.h>
#include <ArduinoJson.h>
#include <WiFi.h>
#include <PubSubClient.h>

#define BAND    868E6  //you can set band here directly,e.g. 868E6,915E6

Adafruit_BMP085 bmp;

const char* ssid = "YourWiFi_SSID";
const char* password = "********";

const char* mqttServer = "farmer.cloudmqtt.com";
const int	mqttPort = 14249;
const char* mqttUser = "xym***"; // register your user on cloudmqtt.com (it's free) and fill here
const char* mqttPassword = "O***-***";

int led = 25; // status LED that periodically shows our process

WiFiClient espClient;
PubSubClient client(espClient);

void setup() {
  // Board init..
  Heltec.begin(false /*DisplayEnable Enable*/, false /*LoRa Enable*/, true /*Serial Enable*/, true /*LoRa use PABOOST*/, BAND /*LoRa RF working band*/);
  delay(300);

  // Serial init for debug purposes..
  Serial.begin(9600);

  // WiFI init..
  WiFi.disconnect(true);
  delay(1000);
  WiFi.mode(WIFI_STA);
  WiFi.setAutoConnect(true);
  WiFi.begin(ssid,password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.println("Connecting to WiFi..");
  }
  Serial.println("Connected to the WiFi network");
  Serial.print("IP address:\t");
  Serial.println(WiFi.localIP());

  // MQTT connect..
  client.setServer(mqttServer, mqttPort);
  while (!client.connected()) {
    Serial.println("Connecting to MQTT...");
    if (client.connect("ESP32Client", mqttUser, mqttPassword)) {
      Serial.println("connected");
    } else {
      Serial.print("failed with state ");
      Serial.print(client.state());
      delay(2000);
    }
  }

  // Barometer init..
  if (!bmp.begin()) {
    Serial.println("Could not find a valid BMP085 sensor, check wiring!");
    while (1) {}
  }

  // LED init..
  pinMode(led, OUTPUT); 
}

// our main work here:
void loop() {
  digitalWrite(led, LOW);
  
  int temp = bmp.readTemperature();
  int pa = bmp.readPressure();
  int alt = bmp.readAltitude();
 
  // Barometer:
  Serial.print("Temp = "); Serial.print(temp); Serial.print(" C");
  Serial.print(" Pa = "); Serial.print(pa); Serial.print(" mbar");
  Serial.print(" Alt = "); Serial.print(alt); Serial.print(" meters");

  // Soil Moisture sensor:
  int soil = analogRead(A0);
  soil = map(soil,0,400,0,100);
  Serial.print(" Capacitive = "); Serial.print(soil); Serial.println(" %");

  // OK, we got all the data, preparing JSON structure to send..
  StaticJsonDocument<200> doc;

  doc["device"] = "ESP32";
  doc["sensorType"] = "Barometer";

  JsonArray data = doc.createNestedArray("values");
  data.add(temp);
  data.add(pa);
  data.add(alt);
  data.add(soil);
  
  Serial.println("Sending message to MQTT topic..");
  
  char buffer[512];
  size_t n = serializeJson(doc, buffer);
  Serial.println(buffer);

  digitalWrite(led, HIGH);
  
  if (!client.publish("esp/test", buffer, n)) {
    Serial.println("-> Error sending message");
  }

  digitalWrite(led, LOW);

  delay(1900); // to send updated data every 2 sec
}
