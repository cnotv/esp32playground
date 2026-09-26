#include <ESP32Servo.h>

const int trigPin = 26;
const int echoPin = 27;
const int fanRelayPin = 32;  // relay MODULE's IN pin, not a bare relay coil, see diagram.md
const int ledPin = 14;
const int buzzerPin = 25;
const int servoPin = 33;

const int triggerDistanceCm = 30;         // closer than this triggers the scare
const unsigned long scareDurationMs = 4000;
const unsigned long cooldownMs = 8000;    // ignores new triggers for a while after a scare

Servo myServo;
bool scaring = false;
unsigned long scareStartedAt = 0;
unsigned long lastScareEndedAt = 0;

// Sends an ultrasonic ping and returns the measured distance in centimeters, 0 if no echo
long readDistanceCm() {
  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);

  long durationUs = pulseIn(echoPin, HIGH, 30000);
  if (durationUs == 0) {
    return 0;
  }
  return durationUs / 58;
}

// Turns the fan relay and LED on or off together
void setFanAndLed(bool on) {
  digitalWrite(fanRelayPin, on ? HIGH : LOW);
  digitalWrite(ledPin, on ? HIGH : LOW);
}

// Plays a rising siren sweep on the buzzer
void playSiren() {
  for (int freq = 200; freq <= 1500; freq += 20) {
    tone(buzzerPin, freq);
    delay(8);
  }
  noTone(buzzerPin);
}

// Swings the servo to look startled, then settles back to rest
void jerkServo() {
  myServo.write(150);
  delay(300);
  myServo.write(30);
  delay(300);
  myServo.write(90);
}

// Runs the full jump-scare sequence: fan and LED on, siren, servo jerk
void startScare() {
  scaring = true;
  scareStartedAt = millis();
  Serial.println("scare: started");
  setFanAndLed(true);
  playSiren();
  jerkServo();
}

// Ends the scare, turning the fan and LED back off
void endScare() {
  scaring = false;
  setFanAndLed(false);
  lastScareEndedAt = millis();
  Serial.println("scare: ended");
}

void setup() {
  Serial.begin(9600);
  Serial.println("setup: start");

  pinMode(trigPin, OUTPUT);
  pinMode(echoPin, INPUT);
  pinMode(fanRelayPin, OUTPUT);
  pinMode(ledPin, OUTPUT);
  setFanAndLed(false);

  myServo.attach(servoPin);
  myServo.write(90);

  Serial.println("setup: ready");
}

void loop() {
  if (scaring) {
    if (millis() - scareStartedAt >= scareDurationMs) {
      endScare();
    }
    return;
  }

  if (millis() - lastScareEndedAt < cooldownMs) {
    return;
  }

  long distance = readDistanceCm();
  if (distance > 0 && distance < triggerDistanceCm) {
    Serial.print("scare: triggered at ");
    Serial.print(distance);
    Serial.println("cm");
    startScare();
  }

  delay(100);
}
