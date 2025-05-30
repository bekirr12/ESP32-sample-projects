#define latchPin 34
#define clockPin 47
#define dataPin 48

#define TOUCH_PIN 13
#define TOUCHPAD_THRESHOLD 22000

const byte ROCK[8] = {
  B00000000,
  B00000000,
  B00111100,
  B00111100,
  B00111100,
  B00111100,
  B00000000,
  B00000000
};

const byte PAPER[8] = {
  B11111111,
  B11111111,
  B11000011,
  B11000011,
  B11000011,
  B11000011,
  B11111111,
  B11111111
};

const byte SCISSORS[8] = {
  B01100010,
  B01100100,
  B01101000,
  B00010000,
  B00010000,
  B01101000,
  B01100100,
  B01100010
};

uint8_t currentPattern[8] = {0};

unsigned long lastUpdateTime = 0;
int currentRow = 0;
unsigned long lastTouchTime = 0;
const unsigned long debounceDelay = 1000;

void setup() {
  pinMode(latchPin, OUTPUT);
  pinMode(clockPin, OUTPUT);
  pinMode(dataPin, OUTPUT);
  Serial.begin(115200);
}

void loop() {
  updateMatrix();

  int touchVal = touchRead(TOUCH_PIN);
  if (touchVal > TOUCHPAD_THRESHOLD && (millis() - lastTouchTime > debounceDelay)) {
    lastTouchTime = millis();

    int choice = random(0, 3);
    Serial.print("Seçim: ");
    Serial.println(choice);

    if (choice == 0) memcpy(currentPattern, ROCK, 8);
    else if (choice == 1) memcpy(currentPattern, PAPER, 8);
    else memcpy(currentPattern, SCISSORS, 8);
  }
}

void updateMatrix() {
  unsigned long now = millis();
  if (now - lastUpdateTime >= 1) {
    lastUpdateTime = now;

    uint8_t patternRow = currentPattern[currentRow];
    uint8_t reversedPattern = reverseBits(patternRow);
    uint8_t rowData = (1 << currentRow);
    uint8_t colData = ~reversedPattern;

    shiftOutAll(colData, rowData);
    delayMicroseconds(190);
    shiftOutAll(0x00, 0x00);

    currentRow++;
    if (currentRow >= 8) currentRow = 0;
  }
}

void shiftOutAll(uint8_t colData, uint8_t rowData) {
  digitalWrite(latchPin, LOW);
  shiftOut(dataPin, clockPin, MSBFIRST, colData);
  shiftOut(dataPin, clockPin, MSBFIRST, rowData);
  digitalWrite(latchPin, HIGH);
}

uint8_t reverseBits(uint8_t b) {
  b = (b & 0xF0) >> 4 | (b & 0x0F) << 4;
  b = (b & 0xCC) >> 2 | (b & 0x33) << 2;
  b = (b & 0xAA) >> 1 | (b & 0x55) << 1;
  return b;
}

