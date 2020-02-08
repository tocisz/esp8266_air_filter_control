#include <ESP8266WiFi.h>        // Include the Wi-Fi library
#include <PubSubClient.h>
#include <Esp.h>
#include <DHTesp.h>

extern const char* ssid;     // The SSID (name) of the Wi-Fi network you want to connect to
extern const char* password; // The password of the Wi-Fi network
extern const char* mqttServer;
extern const int   mqttPort;
extern const char* mqttUser;
extern const char* mqttPassword;

enum power_state_e {
  OFF   = 0,
  ON_1  = 1,
  ON_UV = 2,
  ON_2  = 3,
  POWER_STATE_COUNT = 4
};

enum level_state_e {
  LVL0 = 0,
  LVL1 = 1,
  LVL2 = 2,
  LEVEL_STATE_COUNT = 3
};

enum power_state_pub_e {
  P0 = 0,
  P1 = 1,
  P2 = 2,
  P3 = 3
};

enum uv_state_pub_e {
  UV0 = 0,
  UV1 = 1
};

enum switch_state_e {
  SW0 = 0,
  SW1 = 1
};

boolean public_to_power(power_state_e power_int, power_state_pub_e power_pub, uv_state_pub_e uv_pub) {
  if (power_pub == P0 && power_int == OFF) return true;
  else if (power_pub >= P1 && uv_pub == UV0 && (power_int == ON_1 || power_int == ON_2)) return true;
  else if (power_pub >= P1 && uv_pub == UV1 && power_int == ON_UV) return true;
  return false;
}

boolean public_to_level(level_state_e level, power_state_pub_e power_pub) {
  if (power_pub == P0) return true; // when it's off level doesn't matter
  else if ((power_pub == P1 && level == LVL0) || (power_pub == P2 && level == LVL1)
        || (power_pub == P3 && level == LVL2)) return true;
  return false;
}

power_state_pub_e internal_to_power(power_state_e  power_state, level_state_e  level_state) {
  if (power_state == OFF) return P0;
  else switch (level_state) {
    case LVL0: return P1;
    case LVL1: return P2;
    case LVL2: return P3;
  }
  return P0;
}

uv_state_pub_e internal_to_uv(power_state_e power_state) {
  switch (power_state) {
    case ON_1:
    case ON_2: return UV0;
    case OFF: // default is UV = ON
    case ON_UV: return UV1;
  }
  return UV0;
}

power_state_pub_e requested_power_state = P0;
uv_state_pub_e    requested_uv_state    = UV1; // by default let's use UV
switch_state_e    requested_switch_state = SW0;

power_state_e  power_state = OFF;
level_state_e  level_state = LVL0;
switch_state_e switch_state = SW0;

boolean change_in_progress = false;
boolean request_dht = false;

#define ON_OFF_PIN 5
#define LEVEL_PIN 4
#define SWITCH_PIN 13
#define DHT_PIN 12

WiFiClient espClient;
PubSubClient client(espClient);
DHTesp dht;

const char *TOPIC_PRESENCE = "p/living/XXX"; // +id
const char *TOPIC_PRESENCE_INTERNAL = "p/living/XXX/i"; // +id
const char *TOPIC_PRESENCE_ONLINE = "p/living/XXX/online"; // +id
const char *TOPIC_PRESENCE_CMD = "preq";
const char *TOPIC_AIR_FILTER_CMD = "c/living/XXX"; // +id
const char *TOPIC_TEMP_HUMID = "s/living/t/XXX"; // +id

String my_name;

void callback(char* topic, byte* payload, unsigned int length) {
  if (strcmp(topic, TOPIC_PRESENCE_CMD) == 0) {
    // we subscribe only one topic, so no need to check it
    if (length == 0) {
      Serial.println("preq");
      publish_presence();
    }
  } else if (strcmp(topic, TOPIC_PRESENCE_INTERNAL) == 0) {
    if (length == 3) {
      Serial.println("init after reset");
      Serial.print((char)payload[0]);
      Serial.print((char)payload[1]);
      Serial.println((char)payload[2]);
      power_state  = (power_state_e) (payload[0]-'0');
      level_state  = (level_state_e) (payload[1]-'0');
      switch_state = (switch_state_e)(payload[2]-'0');
      // after reset assume requested state is as it is
      requested_power_state = internal_to_power(power_state, level_state);
      requested_uv_state = internal_to_uv(power_state);
      requested_switch_state = switch_state;
      // we are done, so unsubscribe
      client.unsubscribe(TOPIC_PRESENCE_INTERNAL);
    }
  } else if (strcmp(topic, TOPIC_AIR_FILTER_CMD) == 0) {
    Serial.print("cmd:");
    Serial.print((char)payload[0]);
    if (length == 2 && payload[0] == 'P') {
      Serial.println((char)payload[1]);
      int req_int = payload[1]-'0';
      if (req_int >= P0 && req_int <= P3) {
        requested_power_state = (power_state_pub_e)req_int;
        change_in_progress = true;
      }
    } else if (length == 2 && payload[0] == 'U') {
      Serial.println((char)payload[1]);
      int req_int = payload[1]-'0';
      if (req_int >= UV0 && req_int <= UV1) {
        requested_uv_state = (uv_state_pub_e)req_int;
        change_in_progress = true;
      }
    } else if (length == 2 && payload[0] == 'S') {
      Serial.println((char)payload[1]);
      int req_int = payload[1]-'0';
      if (req_int >= SW0 && req_int <= SW1) {
        requested_switch_state = (switch_state_e)req_int;
      }
    } else if (length == 1 && payload[0] == 'D') {
      Serial.println();
      request_dht = true;
    }
  } else {
    Serial.print("Unknown topic");
    Serial.println(topic);
  }
}

void setup() {
  pinMode(ON_OFF_PIN, OUTPUT);
  pinMode(LEVEL_PIN, OUTPUT);
  pinMode(SWITCH_PIN, OUTPUT);
  digitalWrite(ON_OFF_PIN, 0);
  digitalWrite(LEVEL_PIN, 0);
  digitalWrite(SWITCH_PIN, switch_state);

  Serial.begin(115200);         // Start the Serial communication to send messages to the computer
  delay(10);
  Serial.println('\n');

  my_name = "af_"+String(ESP.getChipId(), HEX);
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

  // Initialize temperature sensor
  dht.setup(DHT_PIN, DHTesp::DHT22);
}

void publish_presence() {
    client.beginPublish(TOPIC_PRESENCE, 3, false);
    client.write('0'+requested_power_state);
    client.write('0'+requested_uv_state);
    client.write('0'+switch_state);
    client.endPublish();

    client.beginPublish(TOPIC_PRESENCE_INTERNAL, 3, true);
    client.write('0'+power_state);
    client.write('0'+level_state);
    client.write('0'+switch_state);
    client.endPublish();
}

void publish_online() {
  client.publish(TOPIC_PRESENCE_ONLINE, "1", true); // online state is retained
}

void publish_temp_humid(float temp, float humid) {
  String msg = String(temp,1) + ";" + String(humid,1);
  Serial.println(msg);
  client.publish(TOPIC_TEMP_HUMID, msg.c_str());
}

void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Attempt to connect
    if (client.connect(my_name.c_str(), mqttUser, mqttPassword,
        TOPIC_PRESENCE_ONLINE, 1, true, "0")) { // last will is to say that it's offline now
      Serial.println("connected");
      // Once connected, publish an announcement...
      publish_online();
      // Subscribe interesting topics
      client.subscribe(TOPIC_PRESENCE_INTERNAL, 1);
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

  // Press some buttons when needed
  if (!public_to_power(power_state, requested_power_state, requested_uv_state)) {
    advancePowerState();
  } else if (!public_to_level(level_state, requested_power_state)) {
    advanceLevelState();
  } else if (change_in_progress) {
    publish_presence();
    change_in_progress = false;
  }

  // External switch
  if (requested_switch_state != switch_state) {
    digitalWrite(SWITCH_PIN, requested_switch_state);
    switch_state = requested_switch_state;
    publish_presence();
  }

  // DHT
  if (request_dht) {
    publish_temp_humid(dht.getTemperature(), dht.getHumidity());
    request_dht = false;
  }
}

void advancePowerState() {
  Serial.println("Press P");
  digitalWrite(ON_OFF_PIN,1);
  delay(50);
  digitalWrite(ON_OFF_PIN,0);
  delay(200);
  power_state = (power_state_e)((power_state+1) % POWER_STATE_COUNT);
}

void advanceLevelState() {
  Serial.println("Press L");
  digitalWrite(LEVEL_PIN,1);
  delay(50);
  digitalWrite(LEVEL_PIN,0);
  delay(200);
  level_state = (level_state_e)((level_state+1) % LEVEL_STATE_COUNT);
}
