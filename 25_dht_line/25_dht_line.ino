#include <Wire.h>
#include <WiFi.h>`n#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include <U8g2lib.h>

const int DHT_PIN = 14;
const int LIGHT_PIN = 33;
U8G2_SSD1306_128X64_NONAME_F_HW_I2C oled(U8G2_R0, U8X8_PIN_NONE);

 const char* WIFI_SSID = "YOUR_WIFI_SSID";
 const char* WIFI_PASSWORD = "YOUR_WIFI_PASSWORD";
 const char* THINGSPEAK_KEY = "YOUR_THINGSPEAK_WRITE_KEY";
const unsigned long THINGSPEAK_INTERVAL = 10000;
 const char* GOOGLE_SCRIPT_ID = "YOUR_GOOGLE_SCRIPT_ID";
 const char* GOOGLE_SHEET_ID = "YOUR_GOOGLE_SHEET_ID";
 const char* GOOGLE_SHEET_NAME = "data";const char* LINE_CHANNEL_ACCESS_TOKEN = "YOUR_LINE_CHANNEL_ACCESS_TOKEN";
 const char* LINE_USER_ID = "YOUR_LINE_USER_ID";
const int LINE_TEMPERATURE_LIMIT = 28;
const int LINE_HUMIDITY_LIMIT = 70;
bool lineAlertSent = false;
unsigned long lastThingSpeakUpload = 0;

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

void showWiFiStatus(const char* title, const char* detail) {
  oled.clearBuffer();
  oled.setFont(u8g2_font_6x10_tf);
  oled.drawStr(0, 14, title);
  oled.drawStr(0, 30, detail);
  oled.sendBuffer();
}

void connectWiFi() {
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.print("Connecting to WiFi: ");
  Serial.println(WIFI_SSID);

  uint8_t dotCount = 0;
  while (WiFi.status() != WL_CONNECTED) {
    char dots[16];
    uint8_t count = dotCount % 10;
    for (uint8_t index = 0; index < count; index++) dots[index] = '.';
    dots[count] = '\0';
    showWiFiStatus("WiFi connecting...", dots);
    Serial.print('.');
    delay(500);
    dotCount++;
  }

  String ipAddress = WiFi.localIP().toString();
  Serial.println();
  Serial.print("WiFi connected, IP: ");
  Serial.println(ipAddress);
  showWiFiStatus("WiFi connected", ipAddress.c_str());
  delay(1200);
}

void uploadThingSpeak(int temperature, int humidity, int lightPercent) {
  if (WiFi.status() != WL_CONNECTED) return;

  WiFiClient client;
  HTTPClient http;
  String url = String("http://api.thingspeak.com/update?api_key=") + THINGSPEAK_KEY
             + "&field1=" + temperature
             + "&field2=" + humidity
             + "&field3=" + lightPercent;

  Serial.println("Uploading to ThingSpeak...");
  if (http.begin(client, url)) {
    int responseCode = http.GET();
    Serial.print("ThingSpeak response: ");
    Serial.println(responseCode);
    if (responseCode > 0) {
      Serial.print("ThingSpeak entry: ");
      Serial.println(http.getString());
    }
    http.end();
  } else {
    Serial.println("ThingSpeak connection failed");
  }
}
String urlEncode(const char* message) {
  const char* hex = "0123456789abcdef";
  String encodedMessage;
  while (*message != '\0') {
    char character = *message++;
    if (('a' <= character && character <= 'z') ||
        ('A' <= character && character <= 'Z') ||
        ('0' <= character && character <= '9') ||
        character == '-' || character == '_' || character == '.' || character == '~') {
      encodedMessage += character;
    } else {
      encodedMessage += '%';
      encodedMessage += hex[(character >> 4) & 0x0F];
      encodedMessage += hex[character & 0x0F];
    }
  }
  return encodedMessage;
}

void uploadGoogleSheet(int temperature, int humidity, int lightPercent) {
  if (WiFi.status() != WL_CONNECTED) return;

  WiFiClientSecure client;
  client.setInsecure();
  HTTPClient https;
  String data = String(temperature) + "," + String(humidity) + "," + String(lightPercent);
  String url = String("https://script.google.com/macros/s/") + GOOGLE_SCRIPT_ID
             + "/exec?type=insert&dateInclude=1&sheetId=" + GOOGLE_SHEET_ID
             + "&sheetTag=" + urlEncode(GOOGLE_SHEET_NAME)
             + "&data=" + urlEncode(data.c_str());

  Serial.println("Uploading to Google Sheet...");
  if (https.begin(client, url)) {
    https.setFollowRedirects(HTTPC_FORCE_FOLLOW_REDIRECTS);
    int responseCode = https.GET();
    Serial.print("Google Sheet response: ");
    Serial.println(responseCode);
    if (responseCode > 0) {
      Serial.print("Google Sheet result: ");
      Serial.println(https.getString());
    }
    https.end();
  } else {
    Serial.println("Google Sheet connection failed");
  }
}
void sendLineNotification(int temperature, int humidity, int lightPercent) {
  if (WiFi.status() != WL_CONNECTED) return;

  WiFiClientSecure client;
  client.setInsecure();
  HTTPClient https;
  String message = String("\\u74B0\\u5883\\u7570\\u5E38\\u8B66\\u544A\\n\\u6EAB\\u5EA6: ") + temperature + " C\\n\\u6FD5\\u5EA6: " + humidity
                 + " %\\n\\u4EAE\\u5EA6: " + lightPercent + " %";
  String body = String("{\"to\":\"") + LINE_USER_ID
              + "\",\"messages\":[{\"type\":\"text\",\"text\":\"" + message + "\"}]}";

  Serial.println("Sending LINE notification...");
  if (https.begin(client, "https://api.line.me/v2/bot/message/push")) {
    https.addHeader("Authorization", String("Bearer ") + LINE_CHANNEL_ACCESS_TOKEN);
    https.addHeader("Content-Type", "application/json");
    int responseCode = https.POST(body);
    Serial.print("LINE response: ");
    Serial.println(responseCode);
    if (responseCode > 0) {
      Serial.print("LINE result: ");
      Serial.println(https.getString());
    }
    https.end();
  } else {
    Serial.println("LINE connection failed");
  }
}
void drawTopItem(int x, int y, const char* label, int value, const char* unit, void (*icon)(int, int)) {
  icon(x, y);
  oled.setFont(u8g2_font_6x10_tf);
  oled.drawStr(x + 22, y + 9, label);

  char valueText[8];
  snprintf(valueText, sizeof(valueText), "%d", value);
  oled.setFont(u8g2_font_logisoso16_tn);
  oled.drawStr(x + 22, y + 27, valueText);
  int valueWidth = oled.getStrWidth(valueText);

  oled.setFont(u8g2_font_6x10_tf);
  oled.drawStr(x + 24 + valueWidth, y + 25, unit);
}

void drawScreen(int temperature, int humidity, int lightPercent, bool dhtValid) {
  oled.clearBuffer();
  int shownTemperature = dhtValid ? temperature : 0;
  int shownHumidity = dhtValid ? humidity : 0;

  drawTopItem(1, 1, "TEMP", shownTemperature, "C", drawThermometer);
  drawTopItem(67, 1, "HUMI", shownHumidity, "%", drawDroplet);

  drawSun(4, 40);
  oled.setFont(u8g2_font_6x10_tf);
  oled.drawStr(28, 48, "LIGHT");
  char lightText[8];
  snprintf(lightText, sizeof(lightText), "%d", lightPercent);
  oled.setFont(u8g2_font_logisoso16_tn);
  oled.drawStr(28, 63, lightText);
  int lightWidth = oled.getStrWidth(lightText);
  oled.setFont(u8g2_font_6x10_tf);
  oled.drawStr(30 + lightWidth, 61, "%");

  oled.sendBuffer();
}

void setup() {
  Serial.begin(115200);
  analogReadResolution(12);
  analogSetPinAttenuation(LIGHT_PIN, ADC_11db);
  Wire.begin(21, 22);
  oled.begin();
  connectWiFi();
  drawScreen(0, 0, 0, false);
  lastThingSpeakUpload = millis();
}

void loop() {
  int temperature = 0;
  int humidity = 0;
  int lightRaw = analogRead(LIGHT_PIN);
  int lightPercent = map(lightRaw, 0, 4095, 0, 100);
  lightPercent = constrain(lightPercent, 0, 100);
  bool dhtValid = readDHT11(temperature, humidity);  

  if (WiFi.status() != WL_CONNECTED) {
    connectWiFi();
  }
  bool lineAbnormal = dhtValid &&
    (temperature > LINE_TEMPERATURE_LIMIT || humidity > LINE_HUMIDITY_LIMIT);
  if (lineAbnormal && !lineAlertSent) {
    showWiFiStatus("Sending...", "LINE notification");
    sendLineNotification(temperature, humidity, lightPercent);
    lineAlertSent = true;
    drawScreen(temperature, humidity, lightPercent, dhtValid);
  } else if (!lineAbnormal) {
    lineAlertSent = false;
  }

  if (dhtValid) {
    Serial.printf("Temperature: %d C, Humidity: %d %%, Light: %d %% (ADC %d)\n", temperature, humidity, lightPercent, lightRaw);
  } else {
    Serial.printf("DHT11 read failed, Light: %d %% (ADC %d)\n", lightPercent, lightRaw);
  }

  drawScreen(temperature, humidity, lightPercent, dhtValid);

  if (millis() - lastThingSpeakUpload >= THINGSPEAK_INTERVAL) {
    showWiFiStatus("Uploading...", "ThingSpeak");
    uploadThingSpeak(temperature, humidity, lightPercent);
    showWiFiStatus("Uploading...", "Google Sheet");
    uploadGoogleSheet(temperature, humidity, lightPercent);
    lastThingSpeakUpload = millis();
    drawScreen(temperature, humidity, lightPercent, dhtValid);
  }

  delay(1000);
}
