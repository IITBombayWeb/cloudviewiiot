#include <WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include <NTPClient.h>
#include <WiFiUdp.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <WiFiAP.h>
#include "Wire.h"


#define ONE_WIRE_BUS 4 // DS18B20 DQ pin connected to digital pin 2
const int pressureInput = 34; // Select the analog input pin for the pressure transducer
const int baudRate = 9600; // Constant integer to set the baud rate for serial monitor
const int sensorReadDelay = 1000; // Constant integer to set the sensor read delay in milliseconds

// Wi-Fi credentials
const char* ssid = "Redmi";
const char* password = "12341234";

// MQTT broker details
const char* mqtt_server = "192.168.43.94";
const int mqtt_port = 1883;
const char* mqtt_topic = "temperature_pressure";

OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);

// Wi-Fi and MQTT client objects
WiFiClient espClient;
PubSubClient client(espClient);

// NTP client setup
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org", 19800, 60000); // 19800 seconds offset for IST

bool ntpConnected = false;

void setup_wifi() {
  delay(10);
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);

  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}

void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Attempt to connect
    if (client.connect("ESP32Client")) {
      Serial.println("connected");
      // Once connected, publish an announcement...
      // client.publish(mqtt_topic, "{\"status\": \"ESP32 connected\"}");
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}

void setup() {
  Serial.begin(9600);
  setup_wifi();
  client.setServer(mqtt_server, mqtt_port);
  timeClient.begin();
}

void loop() {
  if (!client.connected()) {
    reconnect();
  }
  client.loop();

  // Update NTP client and check for synchronization
  if (!ntpConnected && timeClient.update()) {
    Serial.println("NTP time synchronized");
    ntpConnected = true;
  }


  sensors.requestTemperatures(); // Send the command to get temperatures
  int temperature = sensors.getTempCByIndex(0); // Read temperature
  bool tempInvalid = false;

  if (isnan(temperature) || temperature < -200 || temperature > 1350) { // MAX6675 temperature range: -200 to 1350°C
    tempInvalid = true;
  }

  Serial.print("Temperature: ");
  if (tempInvalid) {
    Serial.println("NULL");
  } else {
    Serial.print(temperature);
    Serial.println("°C");
  }

  // Read pressure sensor value
  int sensorValue = analogRead(pressureInput); // Reads value from input pin and assigns to variable
  float voltage = sensorValue * (5.0/ 4095.0); // ESP32 analog input voltage range is 0-3.3V
  float pressure_value = ((((voltage-0.5) / (4.0))) * 0.8) + 0.1; // Adjust calculation based on your pressure sensor specifications
  //if (voltage < 0.5 && voltage > 0){
    //pressure_value = 0.1;
  //}
  bool pressureInvalid = false;

  if (pressure_value <= 0 || pressure_value > 1) { // Assuming pressure value is within the range of 0 to 1
    pressureInvalid = true;E
  }

  if (pressureInvalid) {
    Serial.println("NULL");
  } else {
    Serial.print("Pressure Sensor Value: ");
    Serial.println(pressure_value);
  }

  // Create JSON object
  StaticJsonDocument<200> doc;
  if (tempInvalid) {
    doc["Temperature"] = "NULL";
  } else {
    doc["Temperature"] = temperature;
  }

  if (pressureInvalid) {
    doc["Pressure"] = "NULL";
  } else {
    doc["Pressure"] = pressure_value;
  }

  doc["id"] = 1;

  // Get the current time
  if (ntpConnected) {
    time_t rawTime = timeClient.getEpochTime();
    struct tm * timeInfo = localtime(&rawTime);
    char timeBuffer[20]; // Buffer to store YYYY-MM-DD HH:MM:SS format
    strftime(timeBuffer, sizeof(timeBuffer), "%Y-%m-%d %H:%M:%S", timeInfo);
    doc["Time"] = timeBuffer;
  } else {
    doc["Time"] = "N/A"; // Indicate time not available if NTP not connected
  }

  // Serialize JSON to string
  char jsonBuffer[512];
  serializeJson(doc, jsonBuffer);

  // Publish the JSON string
  client.publish(mqtt_topic, jsonBuffer);

  // Print the JSON string to Serial Monitor
  Serial.println(jsonBuffer);

  // Wait before sending the next message
  delay(500);
}