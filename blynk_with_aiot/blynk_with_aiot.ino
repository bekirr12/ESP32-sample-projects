#include <WiFi.h>
#include <BlynkSimpleEsp32.h>
#include <DHT.h>
#include <FastLED.h>

// blynk definitions
#define BLYNK_TEMPLATE_ID ""
#define BLYNK_TEMPLATE_NAME ""
#define BLYNK_AUTH_TOKEN ""
#define BLYNK_PRINT Serial

// DHT11
#define DHTPIN 8
#define DHTTYPE DHT11
DHT dht(DHTPIN, DHTTYPE);

// pins number
#define LED_PIN 35
#define FAN_PIN 10
#define s1pin 37

#define RGB_PIN 15
#define NUM_LEDS 1
CRGB leds[NUM_LEDS];

// Wi-Fi credentials
char ssid[] = "";
char pass[] = "";

BlynkTimer timer;

int fanSpeed = 0;
int redVal = 0;
int greenVal = 0;
int blueVal = 0;

/*
// V0 Led
BLYNK_WRITE(V0) {
  int ledState = param.asInt();
  digitalWrite(LED_PIN, ledState);
}
*/

// V1 Fan
BLYNK_WRITE(V1) {
  int percent = param.asInt();
  fanSpeed = map(percent, 0, 100, 0, 255);
  ledcWrite(FAN_PIN, fanSpeed);
}

// V2 temperature
void sendTemperature() {
  float temp = dht.readTemperature();
  if (!isnan(temp)) {
    Blynk.virtualWrite(V2, temp);
    Serial.print("Temperature Value: ");
    Serial.println(temp);
  } else {
    Serial.println("FAIL!");
  }
}

// V3 RGB red
BLYNK_WRITE(V3) {
  redVal = param.asInt();
  updateRGB();
}

// V4 RGB green
BLYNK_WRITE(V4) {
  greenVal = param.asInt();
  updateRGB();
}

// V5 blue
BLYNK_WRITE(V5) {
  blueVal = param.asInt();
  updateRGB();
}

void updateRGB() {
  leds[0] = CRGB(redVal, greenVal, blueVal);
  FastLED.show();

  Serial.printf("RGB: R=%d, G=%d, B=%d\n", redVal, greenVal, blueVal);
}

void setup() {
  Serial.begin(115200);

  pinMode(LED_PIN, OUTPUT);
  pinMode(s1pin, OUTPUT);
  digitalWrite(s1pin, HIGH);

  ledcAttach(FAN_PIN, 500, 8);

  // connect blynk
  Blynk.begin(BLYNK_AUTH_TOKEN, ssid, pass);
  dht.begin();

  FastLED.addLeds<NEOPIXEL, RGB_PIN>(leds, NUM_LEDS);
  leds[0] = CRGB::Black;
  FastLED.show();

  // Send data every 1 second
  timer.setInterval(1000L, sendTemperature);
}

void loop() {
  Blynk.run();
  timer.run();
}


