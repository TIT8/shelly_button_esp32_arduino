#include <WiFi.h>
#include <PubSubClient.h>

#define BUTTON_PIN 26

const char* ssid = "REPLACE_WITH_YOUR_SSID";
const char* password = "REPLACE_WITH_YOUR_PASSWORD";
const char* mqtt_server = "YOUR_MQTT_BROKER_IP_ADDRESS";

WiFiClient espClient;
PubSubClient client(espClient);

bool button = 0;
bool state = 0;
bool current = 0;


void setup() {
  Serial.begin(115200);

  setup_wifi();
  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);

  pinMode(BUTTON_PIN, INPUT);
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

  for (int i = 0; i < length; i++) {
    Serial.print((char)message[i]);
    messageTemp += (char)message[i];
  }

  Serial.println();
}


void reconnect() {
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");

    if (client.connect("Esp32_id")) {
      Serial.println("connected");
      //client.subscribe("shellyplus1-blabla/status/switch:0");
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

  button = digitalRead(BUTTON_PIN);
  if (button == 1) {
    if (current == 0) {
      state = !state;
      Serial.println(state ? "1" : "0");
      client.publish("shellyplus1-blabla/command/switch:0", state ? "on" : "off");
    }
    current = 1;
  } else {
    current = 0;
  }

  delay(100);
}
