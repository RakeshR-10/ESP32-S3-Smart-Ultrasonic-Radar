#include <Arduino.h>
#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ST7735.h>
#include <ESP32Servo.h>
#include <math.h>

// TFT PINS
#define TFT_CS    4
#define TFT_DC    15
#define TFT_RST   9
#define TFT_MOSI  11
#define TFT_SCLK  12

// SENSOR AND SERVO
#define TRIG_PIN  7
#define ECHO_PIN  16
#define SERVO_PIN 17

// DISPLAY
#define SCREEN_W 160
#define SCREEN_H 128

// RADAR
#define CX 80
#define CY 101
#define RADAR_R 60
#define MAX_DISTANCE 200

// COLORS
#define BG_COLOR     ST77XX_WHITE
#define TEXT_COLOR   ST77XX_BLACK
#define GRID_COLOR   0x07E0
#define SWEEP_COLOR  ST77XX_BLACK
#define OBJECT_COLOR ST77XX_RED

Adafruit_ST7735 tft(TFT_CS, TFT_DC, TFT_RST);
Servo radarServo;
GFXcanvas16 background(SCREEN_W, SCREEN_H);

int angle = 0;
int direction = 1;
int previousAngle = -1;
int distanceCm = -1;

int previousObjectX = -1;
int previousObjectY = -1;

unsigned long lastMove = 0;
unsigned long lastRead = 0;

const unsigned long SERVO_INTERVAL = 100;
const unsigned long SENSOR_INTERVAL = 100;

// --------------------------------------------------
// DRAW SEMICIRCLE
// --------------------------------------------------
template <typename Display>
void drawArc(Display &d, int radius, uint16_t color)
{
    for (int a = 0; a <= 180; a++)
    {
        float rad = a * PI / 180.0f;
        int x = CX + roundf(radius * cosf(rad));
        int y = CY - roundf(radius * sinf(rad));
        d.drawPixel(x, y, color);
    }
}

// --------------------------------------------------
// DRAW STATIC RADAR GRID
// --------------------------------------------------
template <typename Display>
void drawGrid(Display &d)
{
    drawArc(d, 15, GRID_COLOR);
    drawArc(d, 30, GRID_COLOR);
    drawArc(d, 45, GRID_COLOR);
    drawArc(d, RADAR_R, GRID_COLOR);

    d.drawLine(CX - RADAR_R, CY,
               CX + RADAR_R, CY, TEXT_COLOR);

    for (int a = 0; a <= 180; a += 30)
    {
        float rad = a * PI / 180.0f;
        int x = CX + roundf(RADAR_R * cosf(rad));
        int y = CY - roundf(RADAR_R * sinf(rad));
        d.drawLine(CX, CY, x, y, GRID_COLOR);
    }

    d.setTextColor(TEXT_COLOR);
    d.setTextSize(1);

    d.setCursor(5, 91);
    d.print("180");

    d.setCursor(75, 35);
    d.print("90");

    d.setCursor(140, 91);
    d.print("0");

    d.setCursor(5, 55);
    d.print("200");

    d.setCursor(17, 69);
    d.print("150");

    d.setCursor(35, 82);
    d.print("100");

    d.setCursor(58, 94);
    d.print("50");
}

// --------------------------------------------------
// CREATE SAVED BACKGROUND
// --------------------------------------------------
void createBackground()
{
    background.fillScreen(BG_COLOR);

    background.setTextColor(TEXT_COLOR);
    background.setTextSize(2);
    background.setCursor(5, 3);
    background.print("ULTRASONIC");

    background.setTextSize(1);
    background.setCursor(48, 21);
    background.print("RADAR");

    drawGrid(background);

    background.drawLine(0, 107, 159, 107, TEXT_COLOR);
}

// --------------------------------------------------
// RESTORE PIXEL FROM SAVED BACKGROUND
// --------------------------------------------------
void restorePixel(int x, int y)
{
    if (x < 0 || x >= SCREEN_W ||
        y < 0 || y >= SCREEN_H)
        return;

    uint16_t color =
        background.getBuffer()[y * SCREEN_W + x];

    tft.drawPixel(x, y, color);
}

// --------------------------------------------------
// RESTORE PREVIOUS SWEEP
// --------------------------------------------------
void restoreSweep(int a)
{
    float rad = a * PI / 180.0f;

    int x0 = CX;
    int y0 = CY;
    int x1 = CX + roundf(RADAR_R * cosf(rad));
    int y1 = CY - roundf(RADAR_R * sinf(rad));

    // Bresenham line restoration
    int dx = abs(x1 - x0);
    int dy = abs(y1 - y0);
    int sx = (x0 < x1) ? 1 : -1;
    int sy = (y0 < y1) ? 1 : -1;
    int err = dx - dy;

    while (true)
    {
        restorePixel(x0, y0);

        if (x0 == x1 && y0 == y1)
            break;

        int e2 = 2 * err;

        if (e2 > -dy)
        {
            err -= dy;
            x0 += sx;
        }

        if (e2 < dx)
        {
            err += dx;
            y0 += sy;
        }
    }
}

// --------------------------------------------------
// RESTORE PREVIOUS OBJECT MARKER
// --------------------------------------------------
void restoreObject()
{
    if (previousObjectX < 0 || previousObjectY < 0)
        return;

    for (int dy = -4; dy <= 4; dy++)
    {
        for (int dx = -4; dx <= 4; dx++)
        {
            if (dx * dx + dy * dy <= 16)
            {
                restorePixel(
                    previousObjectX + dx,
                    previousObjectY + dy
                );
            }
        }
    }

    previousObjectX = -1;
    previousObjectY = -1;
}

// --------------------------------------------------
// RADAR ENDPOINT
// --------------------------------------------------
void getEndpoint(int a, int radius, int &x, int &y)
{
    float rad = a * PI / 180.0f;

    x = CX + roundf(radius * cosf(rad));
    y = CY - roundf(radius * sinf(rad));
}

// --------------------------------------------------
// DRAW CURRENT SWEEP
// --------------------------------------------------
void drawSweep(int a)
{
    int x, y;
    getEndpoint(a, RADAR_R, x, y);

    tft.drawLine(CX, CY, x, y, SWEEP_COLOR);
}

// --------------------------------------------------
// READ ULTRASONIC DISTANCE
// --------------------------------------------------
int readDistance()
{
    digitalWrite(TRIG_PIN, LOW);
    delayMicroseconds(3);

    digitalWrite(TRIG_PIN, HIGH);
    delayMicroseconds(10);

    digitalWrite(TRIG_PIN, LOW);

    unsigned long duration =
        pulseIn(ECHO_PIN, HIGH, 25000);

    if (duration == 0)
        return -1;

    int cm = (int)(duration * 0.0343f / 2.0f);

    if (cm < 2 || cm > MAX_DISTANCE)
        return -1;

    return cm;
}

// --------------------------------------------------
// DISPLAY ANGLE AND ECHO
// --------------------------------------------------
void drawReadings()
{
    tft.fillRect(0, 108, SCREEN_W, 20, BG_COLOR);

    tft.setTextColor(TEXT_COLOR);
    tft.setTextSize(1);

    // ANGLE MOVED SLIGHTLY LEFT
    tft.setCursor(0, 110);
    tft.print("ANG:");

    tft.setTextSize(2);
    tft.setCursor(24, 109);

    if (angle < 100)
        tft.print(" ");

    tft.print(angle);

    tft.setTextSize(1);
    tft.setCursor(51, 114);
    tft.print("deg");

    // ECHO MOVED RIGHT
    tft.setCursor(82, 110);
    tft.print("E:");

    tft.setTextSize(2);
    tft.setCursor(101, 109);

    if (distanceCm < 0)
    {
        tft.print("--");
    }
    else
    {
        tft.print(distanceCm);

        // Display distance unit beside the echo value
        tft.setTextSize(1);
        tft.setCursor(137, 114);
        tft.print("cm");
    }
}

// --------------------------------------------------
// DRAW DETECTED OBJECT
// --------------------------------------------------
void drawDetectedObject()
{
    if (distanceCm < 0)
        return;

    int radius = map(
        distanceCm, 0, MAX_DISTANCE, 0, RADAR_R
    );

    radius = constrain(radius, 0, RADAR_R);

    int x, y;
    getEndpoint(angle, radius, x, y);

    tft.fillCircle(x, y, 3, OBJECT_COLOR);

    previousObjectX = x;
    previousObjectY = y;
}

// --------------------------------------------------
// STARTUP SCREEN
// --------------------------------------------------
void showStartupScreen()
{
    tft.fillScreen(BG_COLOR);

    tft.setTextColor(TEXT_COLOR);
    tft.setTextSize(2);

    tft.setCursor(8, 40);
    tft.print("SRI ESHWAR");

    tft.setCursor(35, 65);
    tft.print("RADAR");

    delay(2000);
}

// --------------------------------------------------
// SETUP
// --------------------------------------------------
void setup()
{
    Serial.begin(115200);

    pinMode(TRIG_PIN, OUTPUT);
    pinMode(ECHO_PIN, INPUT);
    digitalWrite(TRIG_PIN, LOW);

    SPI.begin(TFT_SCLK, -1, TFT_MOSI, TFT_CS);

    tft.initR(INITR_BLACKTAB);
    tft.setRotation(1);
    tft.setSPISpeed(27000000);

    showStartupScreen();

    radarServo.setPeriodHertz(50);
    radarServo.attach(SERVO_PIN, 500, 2400);
    radarServo.write(0);

    delay(500);

    createBackground();

    tft.drawRGBBitmap(
        0, 0,
        background.getBuffer(),
        SCREEN_W,
        SCREEN_H
    );

    previousAngle = -1;
    lastMove = millis();
    lastRead = millis();

    Serial.println("Ultrasonic Radar Started");
}

// --------------------------------------------------
// MAIN LOOP
// --------------------------------------------------
void loop()
{
    unsigned long now = millis();

    // Read distance periodically
    if (now - lastRead >= SENSOR_INTERVAL)
    {
        lastRead = now;
        distanceCm = readDistance();

        Serial.print("Angle: ");
        Serial.print(angle);
        Serial.print(" | Distance: ");

        if (distanceCm >= 0)
        {
            Serial.print(distanceCm);
            Serial.println(" cm");
        }
        else
        {
            Serial.println("No echo");
        }
    }

    // SLOW SWEEP; BACKGROUND IS NOT REDRAWN
    if (now - lastMove >= SERVO_INTERVAL)
    {
        lastMove = now;

        // Restore previous marker first
        restoreObject();

        // Restore previous sweep line
        if (previousAngle >= 0)
            restoreSweep(previousAngle);

        // One degree per update
        angle += direction;

        if (angle >= 180)
        {
            angle = 180;
            direction = -1;
        }
        else if (angle <= 0)
        {
            angle = 0;
            direction = 1;
        }

        radarServo.write(angle);

        drawSweep(angle);
        drawDetectedObject();
        drawReadings();

        previousAngle = angle;
    }
}