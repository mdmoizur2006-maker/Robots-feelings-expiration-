#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_ADXL345_U.h>
#include <Adafruit_SSD1306.h>
#include <Adafruit_GFX.h>
#include <DHT.h>

// OLED display setup
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// Accelerometer
Adafruit_ADXL345_Unified accel = Adafruit_ADXL345_Unified(12345);

// DHT
#define DHTPIN 27
#define DHTTYPE DHT11
DHT dht(DHTPIN, DHTTYPE);

// Touch pin
#define TOUCH_PIN 4
#define TOUCH_THRESHOLD 30

// Internal states
unsigned long lastBlink = 0;
bool eyesOpen = true;
float lastX = 0, lastY = 0, lastZ = 0;
const float THRESHOLD = 2.0;

bool heartEyeAnimationActive = false;

void drawHeartEyeFace(bool eyesVisible) {
  display.clearDisplay();
  if (eyesVisible) {
    drawHeart(40, 28, 10);
    drawHeart(88, 28, 10);
  } else {
    display.fillCircle(40, 28, 10, SSD1306_BLACK);
    display.fillCircle(88, 28, 10, SSD1306_BLACK);
  }

  // Small smile below eyes
  display.fillCircle(64, 52, 8, SSD1306_WHITE);
  display.fillRect(56, 44, 16, 8, SSD1306_BLACK);

  display.display();
}

void drawHeart(int x, int y, int s) {
  int r = s / 2;
  display.fillCircle(x - r, y, r, SSD1306_WHITE);
  display.fillCircle(x + r, y, r, SSD1306_WHITE);
  display.fillTriangle(x - s, y, x + s, y, x, y + s * 1.5, SSD1306_WHITE);
}

void drawEyesOpen() {
  display.clearDisplay();
  display.fillCircle(40, 30, 10, SSD1306_WHITE);
  display.fillCircle(88, 30, 10, SSD1306_WHITE);
  display.fillCircle(40, 30, 4, SSD1306_BLACK);
  display.fillCircle(88, 30, 4, SSD1306_BLACK);
  display.display();
}

void drawEyesClosed() {
  display.clearDisplay();
  display.drawLine(30, 30, 50, 30, SSD1306_WHITE);
  display.drawLine(78, 30, 98, 30, SSD1306_WHITE);
  display.display();
}

void drawSleepAnimation() {
  const int zX[] = {70, 80, 90};
  const int zY[] = {40, 30, 20};
  for (int i = 0; i < 3; i++) {
    display.clearDisplay();
    drawEyesClosed();
    display.setCursor(zX[i], zY[i]);
    display.setTextSize(1);
    display.setTextColor(SSD1306_WHITE);
    display.print("Z");
    display.display();
    delay(400);
  }
}

void drawAngryFace() {
  display.clearDisplay();
  display.drawLine(28, 20, 45, 30, SSD1306_WHITE);
  display.drawLine(81, 30, 98, 20, SSD1306_WHITE);
  display.drawTriangle(34, 32, 44, 32, 39, 42, SSD1306_WHITE);
  display.drawTriangle(84, 32, 94, 32, 89, 42, SSD1306_WHITE);
  display.display();
  delay(1000);
}

void drawHappyBlinkingFace(bool open) {
  display.clearDisplay();
  if (open) drawEyesOpen();
  else drawEyesClosed();

  display.drawCircle(64, 50, 10, SSD1306_WHITE);
  display.fillRect(54, 40, 20, 10, SSD1306_BLACK);
  display.display();
}

bool isMatch(float x, float y, float z, float tx, float ty, float tz) {
  return abs(x - tx) < THRESHOLD && abs(y - ty) < THRESHOLD && abs(z - tz) < THRESHOLD;
}

bool isShaking(float x, float y, float z) {
  float dx = abs(x - lastX);
  float dy = abs(y - lastY);
  float dz = abs(z - lastZ);
  lastX = x;
  lastY = y;
  lastZ = z;
  return (dx > 4 || dy > 4 || dz > 4);
}

void setup() {
  Serial.begin(115200);
  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println("OLED not found");
    while (1);
  }
  if (!accel.begin()) {
    Serial.println("ADXL not found");
    while (1);
  }
  accel.setRange(ADXL345_RANGE_16_G);
  dht.begin();

  display.clearDisplay();
  display.setCursor(0, 0);
  display.setTextColor(SSD1306_WHITE);
  display.println("Booted!");
  display.display();
  delay(1000);
}

void loop() {
  int touchValue = touchRead(TOUCH_PIN);
  sensors_event_t event;
  accel.getEvent(&event);
  float x = event.acceleration.x;
  float y = event.acceleration.y;
  float z = event.acceleration.z;

  if (touchValue < TOUCH_THRESHOLD) {
    heartEyeAnimationActive = true;
    static bool blinkOn = true;
    static unsigned long lastHeartBlink = 0;
    if (millis() - lastHeartBlink > 500) {
      blinkOn = !blinkOn;
      lastHeartBlink = millis();
    }
    drawHeartEyeFace(blinkOn);
    return;
  }

  // Reset heart mode
  heartEyeAnimationActive = false;

  if (isShaking(x, y, z)) {
    drawAngryFace();
    return;
  }

  if (isMatch(x, y, z, -3, 0, 84)) {
    drawSleepAnimation();
    return;
  }

  if (isMatch(x, y, z, -4, -11, 76)) {
    static bool happyOpen = true;
    static unsigned long lastHappy = 0;
    if (millis() - lastHappy > 800) {
      happyOpen = !happyOpen;
      lastHappy = millis();
    }
    drawHappyBlinkingFace(happyOpen);
    return;
  }

  // Default blinking
  if (millis() - lastBlink > 1000) {
    eyesOpen = !eyesOpen;
    lastBlink = millis();
  }

  if (eyesOpen) drawEyesOpen();
  else drawEyesClosed();
}
