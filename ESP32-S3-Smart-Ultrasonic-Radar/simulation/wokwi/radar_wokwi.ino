#include <Arduino.h>
#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ILI9341.h>
#include <ESP32Servo.h>
#include <math.h>

#define TFT_CS 4
#define TFT_DC 15
#define TFT_RST 9
#define TFT_MOSI 11
#define TFT_SCLK 12
#define TRIG_PIN 7
#define ECHO_PIN 16
#define SERVO_PIN 17

#define SCREEN_W 320
#define SCREEN_H 240
#define CX 160
#define CY 190
#define RADAR_R 105
#define MAX_DISTANCE 200

#define BG_COLOR ILI9341_WHITE
#define TEXT_COLOR ILI9341_BLACK
#define GRID_COLOR 0x07E0
#define SWEEP_COLOR ILI9341_BLACK
#define OBJECT_COLOR ILI9341_RED

Adafruit_ILI9341 tft(TFT_CS, TFT_DC, TFT_RST);
Servo radarServo;
GFXcanvas16 background(SCREEN_W, SCREEN_H);

int angle = 0, direction = 1, previousAngle = -1;
int distanceCm = -1, previousObjectX = -1, previousObjectY = -1;
unsigned long lastMove = 0, lastRead = 0;
const unsigned long SERVO_INTERVAL = 100;
const unsigned long SENSOR_INTERVAL = 120;

template <typename Display>
void drawArc(Display &d, int radius, uint16_t color) {
  for (int a = 0; a <= 180; a++) {
    float rad = a * PI / 180.0f;
    d.drawPixel(CX + lroundf(radius * cosf(rad)),
                CY - lroundf(radius * sinf(rad)), color);
  }
}

template <typename Display>
void drawGrid(Display &d) {
  drawArc(d, 26, GRID_COLOR); drawArc(d, 52, GRID_COLOR);
  drawArc(d, 78, GRID_COLOR); drawArc(d, RADAR_R, GRID_COLOR);
  d.drawLine(CX - RADAR_R, CY, CX + RADAR_R, CY, TEXT_COLOR);
  for (int a = 0; a <= 180; a += 30) {
    float rad = a * PI / 180.0f;
    d.drawLine(CX, CY, CX + lroundf(RADAR_R * cosf(rad)),
               CY - lroundf(RADAR_R * sinf(rad)), GRID_COLOR);
  }
  d.setTextColor(TEXT_COLOR); d.setTextSize(1);
  d.setCursor(42, 174); d.print("180");
  d.setCursor(154, 72); d.print("90");
  d.setCursor(268, 174); d.print("0");
  d.setCursor(38, 110); d.print("200");
  d.setCursor(55, 132); d.print("150");
  d.setCursor(80, 154); d.print("100");
  d.setCursor(120, 174); d.print("50");
}

void createBackground() {
  background.fillScreen(BG_COLOR);
  background.setTextColor(TEXT_COLOR); background.setTextSize(2);
  background.setCursor(10, 8); background.print("ULTRASONIC RADAR");
  background.setTextSize(1); background.setCursor(10, 32);
  background.print("ESP32-S3 OBJECT SCANNER");
  drawGrid(background);
  background.drawLine(0, 211, SCREEN_W - 1, 211, TEXT_COLOR);
}

void restorePixel(int x, int y) {
  if (x < 0 || x >= SCREEN_W || y < 0 || y >= SCREEN_H) return;
  tft.drawPixel(x, y, background.getBuffer()[y * SCREEN_W + x]);
}

void getEndpoint(int a, int radius, int &x, int &y) {
  float rad = a * PI / 180.0f;
  x = CX + lroundf(radius * cosf(rad));
  y = CY - lroundf(radius * sinf(rad));
}

void restoreSweep(int a) {
  int x0 = CX, y0 = CY, x1, y1;
  getEndpoint(a, RADAR_R, x1, y1);
  int dx = abs(x1-x0), dy = abs(y1-y0);
  int sx = (x0 < x1) ? 1 : -1, sy = (y0 < y1) ? 1 : -1;
  int err = dx - dy;
  while (true) {
    restorePixel(x0, y0);
    if (x0 == x1 && y0 == y1) break;
    int e2 = 2 * err;
    if (e2 > -dy) { err -= dy; x0 += sx; }
    if (e2 < dx) { err += dx; y0 += sy; }
  }
}

void restoreObject() {
  if (previousObjectX < 0 || previousObjectY < 0) return;
  for (int dy = -5; dy <= 5; dy++)
    for (int dx = -5; dx <= 5; dx++)
      if (dx*dx + dy*dy <= 25)
        restorePixel(previousObjectX + dx, previousObjectY + dy);
  previousObjectX = previousObjectY = -1;
}

int readDistance() {
  digitalWrite(TRIG_PIN, LOW); delayMicroseconds(3);
  digitalWrite(TRIG_PIN, HIGH); delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);
  unsigned long duration = pulseIn(ECHO_PIN, HIGH, 25000);
  if (duration == 0) return -1;
  int cm = (int)(duration * 0.0343f / 2.0f);
  if (cm < 2 || cm > MAX_DISTANCE) return -1;
  return cm;
}

void drawReadings() {
  tft.fillRect(0, 212, SCREEN_W, 28, BG_COLOR);
  tft.setTextColor(TEXT_COLOR); tft.setTextSize(2);
  tft.setCursor(8, 218); tft.print("ANGLE:");
  tft.setCursor(90, 218); tft.print(angle); tft.print(" deg");
  tft.setCursor(190, 218); tft.print("ECHO:");
  tft.setCursor(255, 218);
  if (distanceCm < 0) tft.print("--");
  else {
    tft.print(distanceCm);
    tft.setTextSize(1); tft.setCursor(296, 224); tft.print("cm");
  }
}

void drawSweep(int a) {
  int x, y; getEndpoint(a, RADAR_R, x, y);
  tft.drawLine(CX, CY, x, y, SWEEP_COLOR);
}

void drawDetectedObject() {
  if (distanceCm < 0) return;
  int radius = constrain(map(distanceCm, 0, MAX_DISTANCE, 0, RADAR_R), 0, RADAR_R);
  int x, y; getEndpoint(angle, radius, x, y);
  tft.fillCircle(x, y, 4, OBJECT_COLOR);
  previousObjectX = x; previousObjectY = y;
}

void showStartupScreen() {
  tft.fillScreen(BG_COLOR); tft.setTextColor(TEXT_COLOR);
  tft.setTextSize(3); tft.setCursor(38, 85); tft.print("SRI ESHWAR");
  tft.setCursor(100, 125); tft.print("RADAR");
  delay(1500);
}

void setup() {
  Serial.begin(115200);
  pinMode(TRIG_PIN, OUTPUT); pinMode(ECHO_PIN, INPUT);
  digitalWrite(TRIG_PIN, LOW);
  SPI.begin(TFT_SCLK, -1, TFT_MOSI, TFT_CS);
  tft.begin(); tft.setRotation(1);
  showStartupScreen();
  radarServo.setPeriodHertz(50);
  radarServo.attach(SERVO_PIN, 500, 2400);
  radarServo.write(0); delay(300);
  createBackground();
  tft.drawRGBBitmap(0, 0, background.getBuffer(), SCREEN_W, SCREEN_H);
  lastMove = millis(); lastRead = millis();
  Serial.println("Ultrasonic Radar Started");
}

void loop() {
  unsigned long now = millis();
  if (now - lastRead >= SENSOR_INTERVAL) {
    lastRead = now; distanceCm = readDistance();
    Serial.print("Angle: "); Serial.print(angle); Serial.print(" | Distance: ");
    if (distanceCm >= 0) { Serial.print(distanceCm); Serial.println(" cm"); }
    else Serial.println("No echo");
  }
  if (now - lastMove >= SERVO_INTERVAL) {
    lastMove = now;
    restoreObject();
    if (previousAngle >= 0) restoreSweep(previousAngle);
    angle += direction;
    if (angle >= 180) { angle = 180; direction = -1; }
    else if (angle <= 0) { angle = 0; direction = 1; }
    radarServo.write(angle);
    drawSweep(angle); drawDetectedObject(); drawReadings();
    previousAngle = angle;
  }
}
