#include <WiFi.h>
#include <esp_now.h>
#include <FastLED.h>

#define latchPin 34
#define clockPin 47
#define dataPin 48

#define TOUCH_PIN 13
#define TOUCHPAD_THRESHOLD 22000

#define RGB_PIN 15
#define NUM_LEDS 1
CRGB leds[NUM_LEDS];

// === LED Matrix Patterns ===
const uint8_t ROCK[8] = {
  B00000000, B00000000, B00111100, B00100100,
  B00100100, B00111100, B00000000, B00000000
};

const uint8_t PAPER[8] = {
  B11111111, B10000001, B10000001, B10000001,
  B10000001, B10000001, B10000001, B11111111
};

const uint8_t SCISSORS[8] = {
  B01100010, B01100100, B01101000, B00010000,
  B00010000, B01101000, B01100100, B01100010
};

const uint8_t HAPPY[8] = {
  B00100100, B00100100, B00000000, B10000001,
  B01000010, B00111100, B00000000, B00000000
};

const uint8_t SAD[8] = {
  B00100100, B00100100, B00000000, B00111100,
  B01000010, B10000001, B00000000, B00000000
};

const uint8_t DRAW[8] = {
  B00000000, B00100100, B01100110, B11111111,
  B11111111, B01100110, B00100100, B00000000
};

// === Game State ===
enum Choice { NONE = 0, ROCK_C, PAPER_C, SCISSORS_C };
Choice myChoice = NONE;
Choice opponentChoice = NONE;
bool resultShown = false;
bool readyForNextRound = true;

unsigned long myChoiceTime = 0;
unsigned long resultShownTime = 0;

// === Display ===
uint8_t displayBuffer[8] = {0};
int currentRow = 0;
unsigned long lastMatrixUpdate = 0;

// === ESP-NOW ===
uint8_t peerAddress[] = { 0x48, 0xCA, 0x43, 0x5E, 0xBB, 0xF0 }; // second device mac address (upload first device)
//uint8_t peerAddress[] = { 0x28, 0x37, 0x2F, 0x8E, 0x2F, 0xE4 }; // first device mac address (upload second device)

struct GamePacket {
  uint8_t choice;
};

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

void drawImage(const uint8_t image[8]) {
  memcpy(displayBuffer, image, 8);
}

void updateMatrix() {
  if (millis() - lastMatrixUpdate >= 1) {
    lastMatrixUpdate = millis();
    uint8_t rowData = (1 << currentRow);
    uint8_t colData = ~reverseBits(displayBuffer[currentRow]);

    shiftOutAll(colData, rowData);
    delayMicroseconds(190);
    shiftOutAll(0x00, 0x00);

    currentRow = (currentRow + 1) % 8;
  }
}

void onDataRecv(const esp_now_recv_info_t *info, const uint8_t *incomingData, int len) {
  GamePacket packet;
  memcpy(&packet, incomingData, sizeof(packet));
  opponentChoice = (Choice)packet.choice;
  Serial.println("[ESP-NOW] Opponent chose: " + String(opponentChoice));
}

void setupEspNow() {
  WiFi.mode(WIFI_STA);
  if (esp_now_init() != ESP_OK) {
    Serial.println("ESP-NOW Init Failed");
    return;
  }

  esp_now_register_recv_cb(onDataRecv);

  esp_now_peer_info_t peerInfo = {};
  memcpy(peerInfo.peer_addr, peerAddress, 6);
  peerInfo.channel = 0;
  peerInfo.encrypt = false;

  if (!esp_now_is_peer_exist(peerAddress)) {
    esp_now_add_peer(&peerInfo);
  }
}

void sendMyChoice(Choice choice) {
  GamePacket packet;
  packet.choice = (uint8_t)choice;
  esp_now_send(peerAddress, (uint8_t *)&packet, sizeof(packet));
  Serial.println("[ESP-NOW] Sent my choice: " + String(choice));
}

void decideWinner() {
  if (myChoice == opponentChoice) {
    drawImage(DRAW);
    leds[0] = CRGB::Yellow;
  } else if (
    (myChoice == ROCK_C && opponentChoice == SCISSORS_C) ||
    (myChoice == PAPER_C && opponentChoice == ROCK_C) ||
    (myChoice == SCISSORS_C && opponentChoice == PAPER_C)) {
    drawImage(HAPPY);
    leds[0] = CRGB::Green;
  } else {
    drawImage(SAD);
    leds[0] = CRGB::Red;
  }

  FastLED.show();
  resultShown = true;
  resultShownTime = millis();
}

void setup() {
  Serial.begin(115200);
  pinMode(latchPin, OUTPUT);
  pinMode(clockPin, OUTPUT);
  pinMode(dataPin, OUTPUT);
  randomSeed(esp_random());

  setupEspNow();
  drawImage((const uint8_t[8]){0});

  FastLED.addLeds<NEOPIXEL, RGB_PIN>(leds, NUM_LEDS);
  leds[0] = CRGB::Black;
  FastLED.show();
}

void loop() {
  updateMatrix();

  // If no selection is made and there is touch, make a selection
  if (readyForNextRound && myChoice == NONE && touchRead(TOUCH_PIN) > TOUCHPAD_THRESHOLD) {
    myChoice = (Choice)(random(1, 4));
    myChoiceTime = millis();

    switch (myChoice) {
      case ROCK_C: drawImage(ROCK); break;
      case PAPER_C: drawImage(PAPER); break;
      case SCISSORS_C: drawImage(SCISSORS); break;
      default: break;
    }

    sendMyChoice(myChoice);
    readyForNextRound = false;
  }

  // There are two elections and the result has not been shown yet
  if (myChoice != NONE && opponentChoice != NONE && !resultShown) {
    if (millis() - myChoiceTime >= 2000) {
      decideWinner();
    }
  }

  // Reset if touched again after game
  if (resultShown && touchRead(TOUCH_PIN) < TOUCHPAD_THRESHOLD && millis() - resultShownTime > 1000) {
    myChoice = NONE;
    opponentChoice = NONE;
    resultShown = false;
    readyForNextRound = true;
    drawImage((const uint8_t[8]){0});
    delay(500); // Do not leave the button pressed
    leds[0] = CRGB::Black;
    FastLED.show();
  }
}

