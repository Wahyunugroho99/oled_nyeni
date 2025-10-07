#include <ESP8266WiFi.h>  // Untuk ESP32, ganti dengan: #include <WiFi.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include "all_frames.h"   // Pastikan: #define TOTAL_FRAMES 499, frames[], FRAME_WIDTH, FRAME_HEIGHT

// ====== OLED Setting ======
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET    -1
#define SCREEN_ADDRESS 0x3C
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// ====== WiFi & NTP ======
const char* ssid     = "Kos Bu Yanto";
const char* password = "09061984";
#include <WiFiUdp.h>
#include <NTPClient.h>
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org", 7 * 3600, 60000); // WIB (GMT+7), update tiap menit

// ====== Animasi Intro ======
#define FRAME_DELAY 42   // ms
bool animationDone = false;

// ====== Jam Otomatis Hidup/Mati ======
#define WAKE_INTERVAL_MS 120000UL   // 2 menit
#define SHOW_DURATION_MS 5000UL     // 5 detik

unsigned long lastWakeMillis = 0;
bool screenOn = false;
unsigned long screenOnSince = 0;

// Blink titik dua
bool colonState = true;
unsigned long lastColonToggle = 0;
#define COLON_INTERVAL 500

void setup() {
  Serial.begin(115200);

  // OLED
  if (!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
    Serial.println("SSD1306 allocation failed");
    while (1);
  }
  display.clearDisplay();
  display.display();

  // WiFi
  WiFi.begin(ssid, password);
  Serial.print("Connecting WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(400);
    Serial.print(".");
  }
  Serial.println("\nWiFi connected! IP: " + WiFi.localIP().toString());

  // NTP
  timeClient.begin();
  timeClient.update();

  // Awal: OLED ON untuk animasi
  screenOn = true;
  animationDone = false;
}

void showClock() {
  timeClient.update();
  String timeStr = timeClient.getFormattedTime(); // "HH:MM:SS"
  int sep = timeStr.lastIndexOf(':');
  String hh = timeStr.substring(0,2);
  String mm = timeStr.substring(3,5);

  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);
  display.setTextSize(3);

  // Jam besar tengah, blink colon
  int16_t x1, y1; uint16_t w, h;
  String jamText = hh + ":" + mm;
  display.getTextBounds(jamText, 0, 0, &x1, &y1, &w, &h);
  int xJ = (SCREEN_WIDTH - w) / 2;
  int yJ = (SCREEN_HEIGHT - h) / 2;

  display.setCursor(xJ, yJ);
  display.print(hh);

  if (colonState) display.print(":");
  else            display.print(" ");

  display.print(mm);

  display.display();
}

void loop() {
  static unsigned long lastFrameTime = 0;
  static int currentFrame = 0;
  unsigned long now = millis();

  // === Animasi Intro ===
  if (!animationDone) {
    if (now - lastFrameTime >= FRAME_DELAY) {
      lastFrameTime = now;
      display.clearDisplay();
      if (currentFrame < TOTAL_FRAMES) {
        display.drawBitmap(0, 0, frames[currentFrame], FRAME_WIDTH, FRAME_HEIGHT, SSD1306_WHITE);
        display.display();
      }
      currentFrame++;
      if (currentFrame >= TOTAL_FRAMES) {
        animationDone = true;
        display.clearDisplay();
        display.display();
        // OLED OFF, tunggu 2 menit otomatis hidup
        display.ssd1306_command(SSD1306_DISPLAYOFF);
        screenOn = false;
        lastWakeMillis = millis();
      }
    }
    return;
  }

  // === Mode Jam Digital Otomatis ON/OFF ===

  // Hidupkan setiap 2 menit (WAKE_INTERVAL_MS)
  if (!screenOn && (now - lastWakeMillis >= WAKE_INTERVAL_MS)) {
    display.ssd1306_command(SSD1306_DISPLAYON);
    screenOn = true;
    screenOnSince = now;
    lastWakeMillis = now;
    colonState = true;
    lastColonToggle = now;
    showClock();
  }

  // Selama layar ON, tampilkan jam + blink colon
  if (screenOn) {
    if (millis() - lastColonToggle > COLON_INTERVAL) {
      colonState = !colonState;
      lastColonToggle = millis();
      showClock();
    }
    // Matikan layar setelah 5 detik
    if (now - screenOnSince >= SHOW_DURATION_MS) {
      display.ssd1306_command(SSD1306_DISPLAYOFF);
      screenOn = false;
      // hitung 2 menit dari sekarang
      lastWakeMillis = now;
    }
  }

  delay(20);
}