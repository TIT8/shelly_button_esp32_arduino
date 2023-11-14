#include <WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include <ESPmDNS.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>


#define BUTTON_PIN 26

const char* ssid = "REPLACE_WITH_YOUR_SSID";
const char* password = "REPLACE_WITH_YOUR_PASSWORD";
const char* mqtt_server = "YOUR_MQTT_BROKER_IP_ADDRESS";

WiFiClient espClient;
PubSubClient client(espClient);

volatile bool state = 0;
bool current = 0;
bool button_current = 0;
bool button_last = 0;

unsigned long previous_millis = 0UL;
unsigned long interval = 75UL;


void setup() {
  Serial.begin(115200);

  setup_wifi();
  client.setServer(mqtt_server, 1883);  // Change the port if needed
  client.setCallback(callback);
  client.setKeepAlive(30);

  ArduinoOTA
    .onStart([]() {
      String type;
      if (ArduinoOTA.getCommand() == U_FLASH)
        type = "sketch";
      else  // U_SPIFFS
        type = "filesystem";

      // NOTE: if updating SPIFFS this would be the place to unmount SPIFFS using SPIFFS.end()
      Serial.println("Start updating " + type);
    })
    .onEnd([]() {
      Serial.println("\nEnd");
    })
    .onProgress([](unsigned int progress, unsigned int total) {
      Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
    })
    .onError([](ota_error_t error) {
      Serial.printf("Error[%u]: ", error);
      if (error == OTA_AUTH_ERROR) Serial.println("Auth Failed");
      else if (error == OTA_BEGIN_ERROR) Serial.println("Begin Failed");
      else if (error == OTA_CONNECT_ERROR) Serial.println("Connect Failed");
      else if (error == OTA_RECEIVE_ERROR) Serial.println("Receive Failed");
      else if (error == OTA_END_ERROR) Serial.println("End Failed");
    });

  ArduinoOTA.begin();

  pinMode(BUTTON_PIN, INPUT_PULLUP);
}


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


void callback(char* topic, byte* message, unsigned int length) {
  Serial.print("Message arrived on topic: ");
  Serial.print(topic);
  Serial.print(". Message: ");
  String messageTemp;
  StaticJsonDocument<400> json; // The size can be less

  for (int i = 0; i < length; i++) {
    Serial.print((char)message[i]);
    messageTemp += (char)message[i];
  }
  Serial.println();

  DeserializationError error = deserializeJson(json, messageTemp.c_str());
  if (error) {
    Serial.print(F("deserializeJson() failed: "));
    Serial.println(error.f_str());
    return;
  }

  state = json["output"];  // Aware of the current state of the light
}


void reconnect() {
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");

    if (client.connect("Esp32_id")) {
      Serial.println("connected");
      client.subscribe("shellyplus1-<YOUR_SHELLY_ID>/status/switch:0", 1);
      client.publish("shellyplus1-<YOUR_SHELLY_ID>/command/switch:0", "status_update");
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}


void loop() {
  if (!client.connected()) {
    reconnect();
  }
  client.loop();

  button_current = digitalRead(BUTTON_PIN);
  if (button_current != button_last) {
    previous_millis = millis();
  }

  if ((millis() - previous_millis) > interval) {  // Debouncing

    if (!button_current) {
      if (!current) {
        Serial.println(state ? "on" : "off");
        state = !state;
        client.publish("shellyplus1-<YOUR_SHELLY_ID>/command/switch:0", state ? "on" : "off");
      }
      current = 1;
    } else {
      current = 0;
    }
  }

  button_last = button_current;

  ArduinoOTA.handle();
}
