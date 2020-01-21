#include <ESP8266WiFi.h>        // Include the Wi-Fi library
#include <PubSubClient.h>
#include <Esp.h>

extern const char* ssid; // The SSID (name) of the Wi-Fi network you want to connect to
extern const char* password; // The password of the Wi-Fi network
extern const char* mqttServer;
extern const int mqttPort;

#define ON_OFF_PIN 5
#define LEVEL_PIN 4

WiFiClient espClient;
PubSubClient client(espClient);

const char *TOPIC_PRESENCE = "presence";
const char *TOPIC_PRESENCE_CMD = "presence_cmd";
const char *TOPIC_AIR_FILTER_CMD = "af_cmd";
const char *PING = "ping";

String my_name;

void callback(char* topic, byte* payload, unsigned int length) {
  if (strcmp(topic, TOPIC_PRESENCE_CMD) == 0) {
    // we subscribe only one topic, so no need to check it
    if (length == 0) {
      Serial.println(PING);
      publish_presence("pong");
    }
    // if (strlen(PING) == length && memcmp(payload, length) == 0)
  } else if (strcmp(topic, TOPIC_AIR_FILTER_CMD) == 0) {
    Serial.print("cmd:");
    Serial.println((char)payload[0]);
    if (payload[0] == 'P') {
      handleOnOff();
    } else if (payload[0] == 'L') {
      handleLevel();
    }
  } else {
    Serial.print("Unknown topic");
    Serial.println(topic);
  }
}

void setup() {
  pinMode(ON_OFF_PIN, OUTPUT);
  pinMode(LEVEL_PIN, OUTPUT);
  digitalWrite(ON_OFF_PIN, 0);
  digitalWrite(LEVEL_PIN, 0);

  Serial.begin(115200);         // Start the Serial communication to send messages to the computer
  delay(10);
  Serial.println('\n');

  my_name = "af_"+String(ESP.getChipId(), HEX)+":";
  Serial.print("My name is "); Serial.println(my_name);

  WiFi.begin(ssid, password);             // Connect to the network
  IPAddress ip(192,168,0,66);   
  IPAddress gateway(192,168,0,1);   
  IPAddress subnet(255,255,255,0);   
  WiFi.config(ip, gateway, subnet);
  Serial.print("Connecting to ");
  Serial.print(ssid); Serial.println(" ...");

  int i = 0;
  while (WiFi.status() != WL_CONNECTED) { // Wait for the Wi-Fi to connect
    delay(1000);
    Serial.print(++i); Serial.print(", ");
  }

  Serial.println('\n');
  Serial.print("Connected to ");
  Serial.println(WiFi.SSID());              // Tell us what network we're connected to
  Serial.print("IP address:\t");
  Serial.println(WiFi.localIP());           // Send the IP address of the ESP8266 to the computer

  client.setServer(mqttServer, mqttPort);
  client.setCallback(callback);
}

void publish_presence(String m) {
    String msg = my_name + m;  
    client.publish(TOPIC_PRESENCE, msg.c_str());
}

void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Create a random client ID
    String clientId = "af_" + WiFi.SSID();
    // Attempt to connect
    if (client.connect(clientId.c_str())) {
      Serial.println("connected");
      // Once connected, publish an announcement...
      publish_presence("connect");
      // Subscribe interesting topics
      client.subscribe(TOPIC_PRESENCE_CMD);
      client.subscribe(TOPIC_AIR_FILTER_CMD);
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}

void loop(void){
  while (!client.connected()) {
    reconnect();
  }
  client.loop();
}

void handleOnOff() {
  digitalWrite(ON_OFF_PIN,1);      // Change the state of the LED
  delay(50);
  digitalWrite(ON_OFF_PIN,0);      // Change the state of the LED
}

void handleLevel() {
  digitalWrite(LEVEL_PIN,1);      // Change the state of the LED
  delay(50);
  digitalWrite(LEVEL_PIN,0);      // Change the state of the LED
}
