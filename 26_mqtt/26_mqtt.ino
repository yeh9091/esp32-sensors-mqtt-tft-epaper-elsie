#include <Wire.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include <U8g2lib.h>

const int DHT_PIN = 14;
const int LIGHT_PIN = 33;
const int LED_G_PIN = 15;
const int LED_Y_PIN = 2;
const int LED_R_PIN = 4;

 const char* WIFI_SSID = "YOUR_WIFI_SSID";
 const char* WIFI_PASSWORD = "YOUR_WIFI_PASSWORD";
const char* MQTT_HOST = "mqttgo.io";
const uint16_t MQTT_PORT = 1883;
const char* MQTT_TOPIC = "elsie/class305/data";
const unsigned long MQTT_INTERVAL = 10000;
const int TEMPERATURE_WARNING = 28;
const int HUMIDITY_WARNING = 70;

WiFiClient mqttNetworkClient;
PubSubClient mqttClient(mqttNetworkClient);
U8G2_SSD1306_128X64_NONAME_F_HW_I2C oled(U8G2_R0, U8X8_PIN_NONE);
unsigned long lastPublish = 0;
String mqttClientId;

bool waitForLevel(uint8_t level, unsigned long timeoutMicros) {
  unsigned long start = micros();
  while (digitalRead(DHT_PIN) != level) {
    if (micros() - start > timeoutMicros) return false;
  }
  return true;
}

bool readDHT11(int& temperature, int& humidity) {
  uint8_t data[5] = {0, 0, 0, 0, 0};
  pinMode(DHT_PIN, OUTPUT);
  digitalWrite(DHT_PIN, LOW);
  delay(20);
  digitalWrite(DHT_PIN, HIGH);
  delayMicroseconds(30);
  pinMode(DHT_PIN, INPUT_PULLUP);

  if (!waitForLevel(LOW, 100)) return false;
  if (!waitForLevel(HIGH, 100)) return false;
  if (!waitForLevel(LOW, 100)) return false;

  for (uint8_t bitIndex = 0; bitIndex < 40; bitIndex++) {
    if (!waitForLevel(HIGH, 100)) return false;
    unsigned long pulseStart = micros();
    if (!waitForLevel(LOW, 100)) return false;
    data[bitIndex / 8] <<= 1;
    if (micros() - pulseStart > 40) data[bitIndex / 8] |= 1;
  }

  if ((uint8_t)(data[0] + data[1] + data[2] + data[3]) != data[4]) return false;
  humidity = data[0];
  temperature = data[2];
  return true;
}

void drawThermometer(int x, int y) {
  oled.drawFrame(x + 5, y, 6, 15);
  oled.drawDisc(x + 8, y + 18, 5);
  oled.drawLine(x + 8, y + 5, x + 8, y + 18);
  oled.drawLine(x + 12, y + 4, x + 15, y + 4);
  oled.drawLine(x + 12, y + 9, x + 14, y + 9);
  oled.drawLine(x + 12, y + 14, x + 15, y + 14);
}

void drawDroplet(int x, int y) {
  oled.drawLine(x + 8, y, x + 1, y + 11);
  oled.drawLine(x + 8, y, x + 15, y + 11);
  oled.drawCircle(x + 8, y + 11, 7);
  oled.drawLine(x + 1, y + 11, x + 2, y + 15);
  oled.drawLine(x + 15, y + 11, x + 14, y + 15);
  oled.drawLine(x + 2, y + 15, x + 14, y + 15);
}

void drawSun(int x, int y) {
  oled.drawCircle(x + 8, y + 8, 5);
  oled.drawLine(x + 8, y, x + 8, y + 3);
  oled.drawLine(x + 8, y + 13, x + 8, y + 16);
  oled.drawLine(x, y + 8, x + 3, y + 8);
  oled.drawLine(x + 13, y + 8, x + 16, y + 8);
  oled.drawLine(x + 2, y + 2, x + 4, y + 4);
  oled.drawLine(x + 12, y + 12, x + 14, y + 14);
  oled.drawLine(x + 12, y + 4, x + 14, y + 2);
  oled.drawLine(x + 2, y + 14, x + 4, y + 12);
}

void drawRow(int y, const char* label, int value, const char* unit, void (*icon)(int, int)) {
  icon(3, y + 1);
  oled.setFont(u8g2_font_6x10_tf);
  oled.drawStr(25, y + 9, label);

  char valueText[8];
  snprintf(valueText, sizeof(valueText), "%d", value);
  oled.setFont(u8g2_font_logisoso18_tn);
  int valueWidth = oled.getStrWidth(valueText);
  oled.drawStr(75 - valueWidth, y + 21, valueText);

  oled.setFont(u8g2_font_6x10_tf);
  oled.drawStr(80, y + 17, unit);
}

void drawCompactItem(int x, int y, const char* label, int value, const char* unit, void (*icon)(int, int)) {
  icon(x, y);
  oled.setFont(u8g2_font_6x10_tf);
  oled.drawStr(x + 21, y + 9, label);

  char valueText[8];
  snprintf(valueText, sizeof(valueText), "%d", value);
  oled.setFont(u8g2_font_logisoso16_tn);
  oled.drawStr(x + 21, y + 28, valueText);
  int valueWidth = oled.getStrWidth(valueText);

  oled.setFont(u8g2_font_6x10_tf);
  oled.drawStr(x + 23 + valueWidth, y + 26, unit);
}

void drawScreen(int temperature, int humidity, int lightPercent, bool dhtValid) {
  oled.clearBuffer();
  oled.setFont(u8g2_font_6x10_tf);
  oled.drawStr(39, 8, "MQTT SENSOR");
  oled.drawHLine(0, 10, 128);

  drawCompactItem(1, 12, "TEMP", dhtValid ? temperature : 0, "C", drawThermometer);
  drawCompactItem(67, 12, "HUMI", dhtValid ? humidity : 0, "%", drawDroplet);

  drawSun(3, 42);
  oled.setFont(u8g2_font_6x10_tf);
  oled.drawStr(25, 50, "LIGHT");
  char lightText[8];
  snprintf(lightText, sizeof(lightText), "%d", lightPercent);
  oled.setFont(u8g2_font_logisoso16_tn);
  oled.drawStr(25, 63, lightText);
  int lightWidth = oled.getStrWidth(lightText);
  oled.setFont(u8g2_font_6x10_tf);
  oled.drawStr(27 + lightWidth, 61, "%");

  oled.sendBuffer();
}

void setLeds(bool dhtValid, int temperature, int humidity) {
  bool abnormal = dhtValid && (temperature > TEMPERATURE_WARNING || humidity > HUMIDITY_WARNING);
  bool warning = dhtValid && !abnormal && (temperature >= 26 || humidity >= 61);
  digitalWrite(LED_G_PIN, dhtValid && !warning && !abnormal ? HIGH : LOW);
  digitalWrite(LED_Y_PIN, warning ? HIGH : LOW);
  digitalWrite(LED_R_PIN, abnormal || !dhtValid ? HIGH : LOW);
}

void connectWiFi() {
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.print("Connecting to WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print('.');
    delay(500);
  }
  Serial.println();
  Serial.print("WiFi IP: ");
  Serial.println(WiFi.localIP());
}

bool mqttConnect() {
  if (mqttClient.connected()) return true;
  mqttClient.setServer(MQTT_HOST, MQTT_PORT);
  Serial.print("Connecting to MQTT...");
  bool connected = mqttClient.connect(mqttClientId.c_str());
  Serial.println(connected ? " connected" : " failed");
  if (!connected) {
    Serial.print("MQTT state: ");
    Serial.println(mqttClient.state());
  }
  return connected;
}

bool mqttPublish(const String& payload) {
  if (!mqttConnect()) return false;
  return mqttClient.publish(MQTT_TOPIC, payload.c_str());
}

void setup() {
  Serial.begin(115200);
  pinMode(LED_G_PIN, OUTPUT);
  pinMode(LED_Y_PIN, OUTPUT);
  pinMode(LED_R_PIN, OUTPUT);
  digitalWrite(LED_G_PIN, LOW);
  digitalWrite(LED_Y_PIN, LOW);
  digitalWrite(LED_R_PIN, HIGH);
  analogReadResolution(12);
  analogSetPinAttenuation(LIGHT_PIN, ADC_11db);
  Wire.begin(21, 22);
  oled.begin();
  drawScreen(0, 0, 0, false);
  connectWiFi();
  mqttClientId = "esp32-" + String((uint32_t)esp_random(), HEX);
  Serial.print("MQTT Client ID: ");
  Serial.println(mqttClientId);
  mqttConnect();
}

void loop() {
  int temperature = 0;
  int humidity = 0;
  int lightRaw = analogRead(LIGHT_PIN);
  int lightPercent = constrain(map(lightRaw, 0, 4095, 0, 100), 0, 100);
  bool dhtValid = readDHT11(temperature, humidity);
  setLeds(dhtValid, temperature, humidity);
  drawScreen(temperature, humidity, lightPercent, dhtValid);

  if (dhtValid) {
    Serial.printf("Temperature: %d C, Humidity: %d %%, Light: %d %% (ADC %d)\n", temperature, humidity, lightPercent, lightRaw);
  } else {
    Serial.printf("DHT11 read failed, Light: %d %% (ADC %d)\n", lightPercent, lightRaw);
  }

  if (millis() - lastPublish >= MQTT_INTERVAL) {
    String payload = String("{\"temp\":") + temperature
                   + ",\"humi\":" + humidity
                   + ",\"light\":" + lightPercent + "}";
    if (mqttPublish(payload)) {
      Serial.print("MQTT publish ");
      Serial.print(MQTT_TOPIC);
      Serial.print(": ");
      Serial.println(payload);
    }
    lastPublish = millis();
  }

  mqttClient.loop();
  delay(1000);
}
