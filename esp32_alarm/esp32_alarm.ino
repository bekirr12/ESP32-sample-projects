#include <WiFi.h>
#include <time.h>

// WiFi configuration
const char* ssid = "";
const char* password = "";

// Buzzer pin
const int buzzerPin = 35;

// Alarm state variables
int alarmHour = -1;
int alarmMinute = -1;
bool alarmActive = false;
bool alarmManuallyStopped = false;
String inputBuffer = "";

// Notes
#define NOTE_C5 523
#define NOTE_D5 587
#define NOTE_E5 659
#define NOTE_F5 698
#define NOTE_G5 784
#define REST     0

void setup() {
  Serial.begin(115200);

  // Attach PWM to the buzzer pin using the new LEDC API
  ledcAttach(buzzerPin, 2700, 8); // 2.7kHz, 8-bit resolution

  // Connect to Wi-Fi
  WiFi.begin(ssid, password);
  Serial.print("Connecting to Wi-Fi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWi-Fi connected!");

  // Synchronize time using NTP (UTC+3 for Turkey)
  configTime(3 * 3600, 0, "pool.ntp.org", "time.nist.gov");

  struct tm timeinfo;
  while (!getLocalTime(&timeinfo)) {
    Serial.println("Fetching time...");
    delay(1000);
  }

  Serial.println("Time synchronized!");
  Serial.println("Enter alarm time:");
}

// Sample melody (you can replace this with any melody you like)
void playAlarmMelody() {
  int melody[] = {
    NOTE_C5, NOTE_D5, NOTE_E5, NOTE_F5,
    NOTE_G5, REST,
    NOTE_G5, NOTE_F5, NOTE_E5, NOTE_D5,
    NOTE_C5
  };
  int durations[] = {
    150, 150, 150, 150,
    300, 100,
    150, 150, 150, 150,
    300
  };

  for (int i = 0; i < sizeof(melody) / sizeof(int); i++) {
    int freq = melody[i];
    if (freq == REST) {
      ledcWriteTone(buzzerPin, 0); // stop sound
    } else {
      ledcWriteTone(buzzerPin, freq);
    }
    delay(durations[i]);
  }

  ledcWriteTone(buzzerPin, 0); // stop sound after melody
}

void loop() {
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo)) {
    Serial.println("Unable to get time!");
    return;
  }

  int hour = timeinfo.tm_hour;
  int minute = timeinfo.tm_min;
  int second = timeinfo.tm_sec;

  // Read user input from serial monitor
  while (Serial.available()) {
    char c = Serial.read();
    if (c == '\n') {
      inputBuffer.trim();

      if (inputBuffer.equalsIgnoreCase("stop")) {
        alarmManuallyStopped = true;
        ledcWriteTone(buzzerPin, 0);
        Serial.println("Alarm stopped.");
      } else if (inputBuffer.indexOf(':') != -1) {
        int colonIndex = inputBuffer.indexOf(':');
        String hourStr = inputBuffer.substring(0, colonIndex);
        String minStr = inputBuffer.substring(colonIndex + 1);

        alarmHour = hourStr.toInt();
        alarmMinute = minStr.toInt();
        alarmActive = true;
        alarmManuallyStopped = false;

        Serial.printf("New alarm set: %02d:%02d\n", alarmHour, alarmMinute);
      }

      inputBuffer = "";
    } else {
      inputBuffer += c;
    }
  }

  //Serial.printf("Current time: %02d:%02d:%02d\n", hour, minute, second);

  // Trigger the alarm if current time matches and it hasn't been stopped
  if (alarmActive && !alarmManuallyStopped &&
      hour == alarmHour && minute == alarmMinute) {
    playAlarmMelody();
  } else {
    ledcWriteTone(buzzerPin, 0); // keep buzzer silent
  }

  delay(1000);
}






