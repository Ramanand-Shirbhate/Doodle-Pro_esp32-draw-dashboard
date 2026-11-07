
#include <TFT_eSPI.h>
#include <TouchScreen.h>
#include <math.h>

// ==========================================
// TFT DISPLAY
// ==========================================

TFT_eSPI tft = TFT_eSPI();

// ==========================================
// YOUR ACTUAL ESP32 TOUCHSCREEN PINS
// ==========================================

#define XP 12
#define XM 15
#define YP 33
#define YM 13

#define TFT_CS_PIN 33

TouchScreen ts = TouchScreen(XP, YP, XM, YM, 300);

// ==========================================
// TOUCH SETTINGS
// ==========================================

#define MIN_PRESSURE 200
#define MAX_PRESSURE 10000

#define REQUIRED_SAMPLES 120

// ==========================================
// CALIBRATION DATA
// ==========================================

const int POINTS = 8;

double rawX[POINTS];
double rawY[POINTS];

int targetX[POINTS];
int targetY[POINTS];

// Screen dimensions
int screenWidth;
int screenHeight;

// ==========================================
// RESTORE TFT PINS AFTER TOUCH READING
// ==========================================

void restoreDisplayPins() {
  pinMode(XP, OUTPUT);
  pinMode(XM, OUTPUT);
  pinMode(YP, OUTPUT);
  pinMode(YM, OUTPUT);

  digitalWrite(TFT_CS_PIN, LOW);
}

// ==========================================
// READ RAW TOUCH
// ==========================================

bool readRawTouch(int &x, int &y, int &pressure) {

  TSPoint p = ts.getPoint();

  // Restore TFT pins immediately
  restoreDisplayPins();

  if (p.z < MIN_PRESSURE || p.z > MAX_PRESSURE) {
    return false;
  }

  x = p.x;
  y = p.y;
  pressure = p.z;

  return true;
}

// ==========================================
// WAIT UNTIL FINGER/STYLUS IS RELEASED
// ==========================================

void waitForRelease() {

  int x, y, z;
  int stable = 0;

  while (stable < 5) {

    if (readRawTouch(x, y, z)) {
      stable = 0;
    } else {
      stable++;
    }

    delay(10);
  }
}

// ==========================================
// WAIT UNTIL SCREEN IS TOUCHED
// ==========================================

void waitForTouch() {

  int x, y, z;

  while (!readRawTouch(x, y, z)) {
    delay(10);
  }
}

// ==========================================
// DRAW CALIBRATION TARGET
// ==========================================

void drawTarget(int index) {

  tft.fillScreen(TFT_BLACK);

  tft.setTextDatum(TC_DATUM);
  tft.setTextColor(TFT_WHITE, TFT_BLACK);

  tft.drawString("TOUCH CALIBRATION", screenWidth / 2, 25, 4);

  char pointText[30];

  sprintf(pointText, "Point %d / %d", index + 1, POINTS);

  tft.drawString(pointText, screenWidth / 2, 65, 2);

  int x = targetX[index];
  int y = targetY[index];

  // Outer circle
  tft.drawCircle(x, y, 12, TFT_WHITE);

  // Inner circle
  tft.drawCircle(x, y, 5, TFT_RED);

  // Crosshair
  tft.drawLine(x - 18, y, x + 18, y, TFT_GREEN);
  tft.drawLine(x, y - 18, x, y + 18, TFT_GREEN);

  tft.fillCircle(x, y, 2, TFT_YELLOW);

  tft.setTextColor(TFT_CYAN, TFT_BLACK);
  tft.drawString("Touch and hold the crosshair", screenWidth / 2,
                 screenHeight - 35, 2);
}

// ==========================================
// CAPTURE ONE CALIBRATION POINT
// ==========================================

bool capturePoint(int index) {

  while (true) {

    waitForRelease();

    drawTarget(index);

    Serial.println();
    Serial.printf("Touch point %d at screen (%d, %d)\n",
                  index + 1, targetX[index], targetY[index]);

    Serial.println("Waiting for touch...");

    waitForTouch();

    // Allow the touch to stabilize
    delay(150);

    double sumX = 0;
    double sumY = 0;

    int count = 0;
    int missed = 0;

    int x, y, z;

    while (count < REQUIRED_SAMPLES) {

      if (readRawTouch(x, y, z)) {

        sumX += x;
        sumY += y;

        count++;
        missed = 0;

      } else {

        missed++;

        // Stop if the user releases the screen
        if (missed > 15) {
          break;
        }
      }

      delay(2);
    }

    // Retry if too few readings were captured
    if (count < 40) {

      tft.fillScreen(TFT_BLACK);
      tft.setTextDatum(MC_DATUM);
      tft.setTextColor(TFT_RED, TFT_BLACK);

      tft.drawString("Touch not stable!", screenWidth / 2,
                     screenHeight / 2, 2);

      Serial.println("Not enough readings. Please try again.");

      delay(1000);
      continue;
    }

    // Average raw readings
    rawX[index] = sumX / count;
    rawY[index] = sumY / count;

    Serial.printf("Captured %d readings\n", count);
    Serial.printf("RAW X = %.2f\n", rawX[index]);
    Serial.printf("RAW Y = %.2f\n", rawY[index]);

    waitForRelease();

    return true;
  }
}

// ==========================================
// SOLVE 3x3 MATRIX USING GAUSSIAN ELIMINATION
// ==========================================

bool solve3x3(double A[3][3],
              double b[3],
              double result[3]) {

  double matrix[3][4];

  // Create augmented matrix
  for (int i = 0; i < 3; i++) {

    for (int j = 0; j < 3; j++) {
      matrix[i][j] = A[i][j];
    }

    matrix[i][3] = b[i];
  }

  // Gaussian elimination with partial pivoting
  for (int col = 0; col < 3; col++) {

    int pivot = col;

    for (int row = col + 1; row < 3; row++) {

      if (fabs(matrix[row][col]) >
          fabs(matrix[pivot][col])) {

        pivot = row;
      }
    }

    if (fabs(matrix[pivot][col]) < 1e-12) {
      return false;
    }

    // Swap rows
    for (int j = 0; j < 4; j++) {

      double temp = matrix[col][j];

      matrix[col][j] = matrix[pivot][j];
      matrix[pivot][j] = temp;
    }

    // Normalize pivot row
    double divisor = matrix[col][col];

    for (int j = 0; j < 4; j++) {
      matrix[col][j] /= divisor;
    }

    // Eliminate other rows
    for (int row = 0; row < 3; row++) {

      if (row == col) {
        continue;
      }

      double factor = matrix[row][col];

      for (int j = 0; j < 4; j++) {

        matrix[row][j] -=
          factor * matrix[col][j];
      }
    }
  }

  // Extract answer
  for (int i = 0; i < 3; i++) {
    result[i] = matrix[i][3];
  }

  return true;
}

// ==========================================
// CALCULATE AFFINE CALIBRATION
// ==========================================

void calculateCalibration() {

  double A[3][3] = {0};

  double bX[3] = {0};
  double bY[3] = {0};

  // Build normal equations using all eight points
  for (int i = 0; i < POINTS; i++) {

    double v[3] = {
      rawX[i],
      rawY[i],
      1.0
    };

    for (int row = 0; row < 3; row++) {

      for (int col = 0; col < 3; col++) {

        A[row][col] += v[row] * v[col];
      }

      bX[row] += v[row] * targetX[i];
      bY[row] += v[row] * targetY[i];
    }
  }

  double calX[3];
  double calY[3];

  bool solvedX = solve3x3(A, bX, calX);
  bool solvedY = solve3x3(A, bY, calY);

  if (!solvedX || !solvedY) {

    Serial.println("ERROR: Calibration calculation failed.");
    return;
  }

  // ========================================
  // PRINT FINAL CALIBRATION COEFFICIENTS
  // ========================================

  Serial.println();
  Serial.println("==================================");
  Serial.println("       CALIBRATION COMPLETE");
  Serial.println("==================================");

  Serial.println();
  Serial.println("COPY THESE VALUES:");
  Serial.println();

  Serial.println("X COEFFICIENTS:");

  Serial.printf(
    "float calX[3] = { %.9f, %.9f, %.9f };\n",
    calX[0], calX[1], calX[2]
  );

  Serial.println();
  Serial.println("Y COEFFICIENTS:");

  Serial.printf(
    "float calY[3] = { %.9f, %.9f, %.9f };\n",
    calY[0], calY[1], calY[2]
  );

  // ========================================
  // PRINT INDIVIDUAL POINT ERRORS
  // ========================================

  Serial.println();
  Serial.println("POINT VALIDATION:");

  double totalError = 0;

  for (int i = 0; i < POINTS; i++) {

    double predictedX =
      calX[0] * rawX[i] +
      calX[1] * rawY[i] +
      calX[2];

    double predictedY =
      calY[0] * rawX[i] +
      calY[1] * rawY[i] +
      calY[2];

    double errorX = predictedX - targetX[i];
    double errorY = predictedY - targetY[i];

    double error = sqrt(
      errorX * errorX +
      errorY * errorY
    );

    totalError += error;

    Serial.printf(
      "Point %d | Target: (%d,%d) | Predicted: (%.1f,%.1f) | Error: %.1f px\n",
      i + 1,
      targetX[i],
      targetY[i],
      predictedX,
      predictedY,
      error
    );
  }

  Serial.printf(
    "\nAverage calibration error: %.2f pixels\n",
    totalError / POINTS
  );

  Serial.println();
  Serial.println("Send the complete Serial Monitor output.");
}

// ==========================================
// SETUP
// ==========================================

void setup() {

  Serial.begin(115200);

  // Keep Wi-Fi disabled during calibration
  analogReadResolution(10);

  // Initialize TFT
  tft.init();

  // Portrait orientation
  tft.setRotation(0);

  screenWidth = tft.width();
  screenHeight = tft.height();

  restoreDisplayPins();

  // Eight calibration targets
  targetX[0] = 10;
  targetY[0] = 10;

  targetX[1] = 10;
  targetY[1] = screenHeight / 2;

  targetX[2] = 10;
  targetY[2] = screenHeight - 10;

  targetX[3] = screenWidth / 2;
  targetY[3] = 10;

  targetX[4] = screenWidth / 2;
  targetY[4] = screenHeight - 10;

  targetX[5] = screenWidth - 10;
  targetY[5] = 10;

  targetX[6] = screenWidth - 10;
  targetY[6] = screenHeight / 2;

  targetX[7] = screenWidth - 10;
  targetY[7] = screenHeight - 10;

  Serial.println();
  Serial.println("ESP32 8-POINT TOUCH CALIBRATION");
  Serial.println("--------------------------------");

  Serial.printf("Display: %d x %d\n",
                screenWidth, screenHeight);

  Serial.println("Touch each target and hold steady.");

  delay(1500);

  // Capture all eight points
  for (int i = 0; i < POINTS; i++) {
    capturePoint(i);
  }

  // Calculate affine calibration
  calculateCalibration();

  tft.fillScreen(TFT_BLACK);
  tft.setTextDatum(MC_DATUM);

  tft.setTextColor(TFT_GREEN, TFT_BLACK);

  tft.drawString("CALIBRATION COMPLETE",
                 screenWidth / 2,
                 screenHeight / 2 - 15, 2);

  tft.setTextColor(TFT_WHITE, TFT_BLACK);

  tft.drawString("Check Serial Monitor",
                 screenWidth / 2,
                 screenHeight / 2 + 20, 2);
}

// ==========================================
// LOOP
// ==========================================

void loop() {

  // Calibration runs only once.
  // Nothing else is required here.

}
