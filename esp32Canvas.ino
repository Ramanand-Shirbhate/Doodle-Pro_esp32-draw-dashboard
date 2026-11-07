
#include <TFT_eSPI.h>
#include <TouchScreen.h>
#include <math.h>

// ==========================================
// TFT DISPLAY
// ==========================================

TFT_eSPI tft = TFT_eSPI();

// ==========================================
// VERIFIED TOUCH PINS
// ==========================================

#define XP 12
#define XM 15
#define YP 33
#define YM 13

#define TFT_CS_PIN 33

TouchScreen ts = TouchScreen(XP, YP, XM, YM, 300);

#define MIN_PRESSURE 200
#define MAX_PRESSURE 10000

// ==========================================
// DISPLAY
// ==========================================

#define DISPLAY_ROTATION 0

int SCREEN_WIDTH = 320;
int SCREEN_HEIGHT = 480;

// ==========================================
// VERIFIED CALIBRATION
// ==========================================

const float CAL_X[3] = {
  0.412182419f,
 -0.001511499f,
 -63.990020399f
};

const float CAL_Y[3] = {
 -0.002097776f,
 -0.530821781f,
 527.559531235f
};

// ==========================================
// BRUSH SETTINGS
// ==========================================

// Radius 1 = approximately 3 pixels diameter
#define BRUSH_RADIUS 1

int previousX = -1;
int previousY = -1;

// ==========================================
// RESTORE DISPLAY PINS
// ==========================================

void restoreDisplayPins() {

  pinMode(XP, OUTPUT);
  pinMode(XM, OUTPUT);
  pinMode(YP, OUTPUT);
  pinMode(YM, OUTPUT);

  digitalWrite(TFT_CS_PIN, LOW);
}

// ==========================================
// READ CALIBRATED TOUCH
// ==========================================

bool readTouch(int &x, int &y) {

  TSPoint p = ts.getPoint();

  restoreDisplayPins();

  if (p.z < MIN_PRESSURE || p.z > MAX_PRESSURE) {
    return false;
  }

  float calibratedX =
    CAL_X[0] * p.x +
    CAL_X[1] * p.y +
    CAL_X[2];

  float calibratedY =
    CAL_Y[0] * p.x +
    CAL_Y[1] * p.y +
    CAL_Y[2];

  // Verified Y-axis correction
  float correction = 0.0f;

  if (calibratedY >= 200.0f && calibratedY < 240.0f) {
    correction = (calibratedY - 200.0f) * 0.5f;
  }
  else if (calibratedY >= 240.0f && calibratedY <= 440.0f) {
    correction = 20.0f;
  }
  else if (calibratedY > 440.0f && calibratedY < 460.0f) {
    correction = 460.0f - calibratedY;
  }

  calibratedY += correction;

  x = constrain(
    (int)roundf(calibratedX),
    0,
    SCREEN_WIDTH - 1
  );

  y = constrain(
    (int)roundf(calibratedY),
    0,
    SCREEN_HEIGHT - 1
  );

  return true;
}

// ==========================================
// DRAW BRUSH POINT
// ==========================================

void drawBrushPoint(int x, int y) {

  tft.fillCircle(
    x,
    y,
    BRUSH_RADIUS,
    TFT_WHITE
  );
}

// ==========================================
// DRAW CONTINUOUS LINE
// ==========================================

void drawBrushLine(
  int x0,
  int y0,
  int x1,
  int y1
) {

  int dx = x1 - x0;
  int dy = y1 - y0;

  int steps = max(abs(dx), abs(dy));

  if (steps == 0) {
    drawBrushPoint(x1, y1);
    return;
  }

  // Interpolate between touch readings
  // to prevent gaps while drawing quickly.

  for (int i = 0; i <= steps; i++) {

    int x = x0 + ((long)dx * i) / steps;
    int y = y0 + ((long)dy * i) / steps;

    drawBrushPoint(x, y);
  }
}

// ==========================================
// SETUP
// ==========================================

void setup() {

  Serial.begin(115200);

  analogReadResolution(10);

  tft.init();
  tft.setRotation(DISPLAY_ROTATION);

  SCREEN_WIDTH = tft.width();
  SCREEN_HEIGHT = tft.height();

  restoreDisplayPins();

  // Completely black canvas
  tft.fillScreen(TFT_BLACK);

  Serial.println("White drawing canvas ready.");
  Serial.printf(
    "Display: %d x %d\n",
    SCREEN_WIDTH,
    SCREEN_HEIGHT
  );
}

// ==========================================
// MAIN LOOP
// ==========================================

void loop() {

  int x, y;

  if (readTouch(x, y)) {

    if (previousX == -1 || previousY == -1) {

      // Start a new stroke
      drawBrushPoint(x, y);

    } else {

      // Continue the existing stroke
      drawBrushLine(
        previousX,
        previousY,
        x,
        y
      );
    }

    previousX = x;
    previousY = y;

  } else {

    // Touch released
    previousX = -1;
    previousY = -1;
  }

  delay(5);
}
