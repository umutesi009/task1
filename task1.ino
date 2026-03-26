#include <LiquidCrystal_I2C.h>
#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include <Wire.h>

// ==========================
// WiFi
// ==========================
const char* ssid = "clusters";
const char* password = "1234567890@";

// ==========================
// MQTT
// ==========================
const char* mqttServer = "broker.hivemq.com";
const int mqttPort = 1883;

// Topics (matching dashboard)
const char* topicLCD = "nit/lcd/14";
const char* topicBuzzer = "nit/buzzer/14";

unsigned long lastPublish = 0;
const unsigned long publishInterval = 5000;
int counter = 0;

// ==========================
// Objects
// ==========================
WiFiClient espClient;
PubSubClient client(espClient);
LiquidCrystal_I2C lcd(0x27, 16, 2);

// ==========================
// Pins
// ==========================
const int buzzerPin = D7;

// ==========================
bool buzzerState = false;

// ==========================
// WiFi connect
// ==========================
void connectWiFi() {
  Serial.print("Connecting WiFi");

  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("\nWiFi Connected!");
}

// Tone function
void playTone() {
  tone(buzzerPin, 1000);   // 1kHz tone
  delay(300);
  noTone(buzzerPin);       // stop tone
  delay(50);
}

// ==========================
// MQTT callback
// ==========================
void mqttCallback(char* topic, byte* payload, unsigned int length) {

  char message[length + 1];
  for (int i = 0; i < length; i++) {
    message[i] = payload[i];
  }
  message[length] = '\0';

  Serial.print("Topic: ");
  Serial.println(topic);

  Serial.print("Message: ");
  Serial.println(message);

  // =====================
  // LCD CONTROL
  // =====================
  if (strcmp(topic, topicLCD) == 0) {
    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, message);
    const char* command = doc["command"];
    const char* value = doc["value"];
    lcd.clear();
    lcd.setCursor(0,0);
    lcd.print(value);
  }

  // =====================
  // BUZZER CONTROL
  // =====================
  if (strcmp(topic, topicBuzzer) == 0) {
    JsonDocument doc;
    DeserializationError error= deserializeJson(doc, message);
    const char* state = doc["state"];



    if (strcmp(state, "on") == 0) {
      buzzerState = true;
    }
    else if (strcmp(state, "off") == 0) {
      buzzerState = false;
    }

    // show buzzer status on LCD line 2
    lcd.setCursor(0, 1);
    lcd.print(buzzerState ? "Buzz: ON " : "Buzz: OFF");
  }
}

// ==========================
// MQTT connect
// ==========================
void connectMQTT() {
  while (!client.connected()) {

    String clientId = "mqtt_g14";
    clientId += String(ESP.getChipId(), HEX);

    if (client.connect(clientId.c_str())) {
      Serial.println("MQTT Connected");

      // subscribe to dashboard topics
      client.subscribe(topicLCD);
      client.subscribe(topicBuzzer);

    } else {
      Serial.println("Retrying MQTT...");
      delay(2000);
    }
  }
}





// ==========================
// SETUP
// ==========================
void setup() {
  Serial.begin(115200);
  Wire.begin();

  pinMode(buzzerPin, OUTPUT);
  digitalWrite(buzzerPin, LOW);





  connectWiFi();

  client.setServer(mqttServer, mqttPort);
  client.setCallback(mqttCallback);

  lcd.init();
  lcd.backlight();
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("Ready...");
}

// ==========================
// LOOP
// ==========================
void loop() {

  if (WiFi.status() != WL_CONNECTED) {
    connectWiFi();
  }

  if (!client.connected()) {
    connectMQTT();
  }

  client.loop();
  if (buzzerState) {
     playTone();
  }
}
