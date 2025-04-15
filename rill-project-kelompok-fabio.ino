#include <ESP8266WiFi.h>
#include <ThingerESP8266.h>
#include <UniversalTelegramBot.h>
#include <WiFiClientSecure.h>

// Thinger.io credentials
#define USERNAME "necro_sp12345"
#define DEVICE_ID "FireDetectorDevice"
#define DEVICE_CREDENTIAL "9CEY88PJ3Ed1yVH7"

// WiFi credentials
const char* WIFI_SSID = "glorp";
const char* WIFI_PASSWORD = "cciscute";

// Telegram Bot Token and Chat ID
#define BOT_TOKEN "your_bot_token_here"  // Replace with your bot token
#define CHAT_ID "your_chat_id_here"      // Replace with your chat ID

// Pin definitions
#define LED_PIN 13
#define BUZZER_PIN 12
#define IR_SENSOR_PIN 14

// Initialize Thinger.io client
ThingerESP8266 thing(USERNAME, DEVICE_ID, DEVICE_CREDENTIAL);

// Initialize Telegram bot
WiFiClientSecure client;
UniversalTelegramBot bot(BOT_TOKEN, client);

// Variables
bool fireDetected = false;
unsigned long lastDetectionTime = 0;
const unsigned long detectionInterval = 60000; // 1 minute cooldown between notifications
String fireStatusText = "✅ Sistem normal";

void setup() {
  Serial.begin(115200);

  pinMode(LED_PIN, OUTPUT);
  pinMode(BUZZER_PIN, OUTPUT);
  pinMode(IR_SENSOR_PIN, INPUT);

  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());

  client.setInsecure();

  thing["fire"] >> [](pson& out){
    out = fireStatusText;
  };

  bot.sendMessage(CHAT_ID, "\xF0\x9F\x94\xA5 Fire Detection System Activated\nDevice is now online and monitoring", "");
}

void loop() {
  thing.handle();

  int sensorValue = digitalRead(IR_SENSOR_PIN);

  if (sensorValue == LOW && !fireDetected) {
    fireDetected = true;
    lastDetectionTime = millis();

    fireStatusText = "\xF0\x9F\x9A\xA8 API TERDETEKSI!";

    String message = "\xF0\x9F\x9A\xA8 FIRE DETECTED! \xF0\x9F\x9A\xA8\n";
    message += "Time: " + getTimeStamp() + "\n";
    message += "Please check the area immediately!";
    bot.sendMessage(CHAT_ID, message, "");

    Serial.println("Fire detected! Alert sent.");

    // Aktifkan alarm setelah status dikirim ke dashboard
    digitalWrite(LED_PIN, HIGH);
    digitalWrite(BUZZER_PIN, HIGH);
  }
  else if (sensorValue == HIGH && fireDetected) {
    fireDetected = false;
    fireStatusText = "✅ Sistem normal";

    digitalWrite(LED_PIN, LOW);
    digitalWrite(BUZZER_PIN, LOW);

    if (millis() - lastDetectionTime > detectionInterval) {
      bot.sendMessage(CHAT_ID, "\xE2\x9C\x85 Fire situation cleared\nSystem returned to normal monitoring", "");
      Serial.println("Fire cleared. System normal.");
    }
  }

  if (millis() - lastDetectionTime > 1000) {
    int numNewMessages = bot.getUpdates(bot.last_message_received + 1);

    while (numNewMessages) {
      Serial.println("got response");
      handleNewMessages(numNewMessages);
      numNewMessages = bot.getUpdates(bot.last_message_received + 1);
    }
  }

  delay(100);
}

void handleNewMessages(int numNewMessages) {
  for (int i = 0; i < numNewMessages; i++) {
    String chat_id = String(bot.messages[i].chat_id);
    if (chat_id != CHAT_ID) {
      bot.sendMessage(chat_id, "Unauthorized user", "");
      continue;
    }

    String text = bot.messages[i].text;

    if (text == "/status") {
      String statusMessage = "\xF0\x9F\x94\xA5 Fire Detection System Status\n";
      statusMessage += fireDetected ? "\xF0\x9F\x9A\xA8 FIRE DETECTED!\n" : "\xE2\x9C\x85 System normal\n";
      statusMessage += "Last update: " + getTimeStamp();
      bot.sendMessage(CHAT_ID, statusMessage, "");
    }
    else if (text == "/silence") {
      digitalWrite(BUZZER_PIN, LOW);
      bot.sendMessage(CHAT_ID, "\xF0\x9F\x94\x95 Alarm silenced manually", "");
    }
    else if (text == "/help") {
      String helpMessage = "Available commands:\n";
      helpMessage += "/status - Check system status\n";
      helpMessage += "/silence - Silence the alarm\n";
      helpMessage += "/help - Show this help message";
      bot.sendMessage(CHAT_ID, helpMessage, "");
    }
  }
}

String getTimeStamp() {
  unsigned long seconds = millis() / 1000;
  unsigned long minutes = seconds / 60;
  unsigned long hours = minutes / 60;
  unsigned long days = hours / 24;

  hours %= 24;
  minutes %= 60;
  seconds %= 60;

  return String(days) + "d " + String(hours) + "h " + String(minutes) + "m " + String(seconds) + "s";
}
