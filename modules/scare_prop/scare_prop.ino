#include <ESP32Servo.h>
#include <ESP_I2S.h>
#include <WiFiManager.h>
#include <WebServer.h>

const int trigPin = 26;
const int echoPin = 27;
const int fanRelayPin = 32;  // relay MODULE's IN pin, not a bare relay coil, see diagram.md
const int ledPins[] = { 14, 13 };
const int ledCount = sizeof(ledPins) / sizeof(ledPins[0]);
const int servoPin = 33;

// NS4168 I2S amplifier, its pins are labelled BCL, LRC, DIN on the module
const int i2sBclkPin = 18;
const int i2sLrcPin = 19;
const int i2sDinPin = 23;

const int triggerDistanceCm = 30;         // closer than this triggers the scare
const unsigned long scareDurationMs = 4000;
const unsigned long cooldownMs = 8000;    // ignores new triggers for a while after a scare

const int sampleRate = 16000;
const float ghostSeconds = 3.0;
const float volume = 0.5;                 // 0.0 to 1.0, lower it if the speaker distorts or rattles

Servo myServo;
I2SClass i2s;
WiFiManager wifiManager;
WebServer server(80);

bool scaring = false;
bool armed = true;                        // false ignores the distance sensor, phone buttons still work
bool lightsOn = false;                    // manual light state from the phone, used outside a scare
volatile bool soundPlaying = false;
unsigned long scareStartedAt = 0;
unsigned long lastScareEndedAt = 0;
unsigned long lastFlickerAt = 0;

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

// Turns the fan relay on or off
void setFan(bool on) {
  digitalWrite(fanRelayPin, on ? HIGH : LOW);
}

// Turns all the LEDs on or off together
void setLights(bool on) {
  for (int i = 0; i < ledCount; i++) {
    digitalWrite(ledPins[i], on ? HIGH : LOW);
  }
}

// Randomly flickers each LED like a failing candle, called repeatedly during a scare
void flickerLights() {
  if (millis() - lastFlickerAt < 60) {
    return;
  }
  lastFlickerAt = millis();
  for (int i = 0; i < ledCount; i++) {
    digitalWrite(ledPins[i], random(4) != 0 ? HIGH : LOW);
  }
}

// Synthesizes a wailing "woooOOOooo" ghost moan and streams it to the I2S amp.
// Runs as its own task so the lights, servo and web page keep going meanwhile.
void ghostSoundTask(void*) {
  const int frameCount = sampleRate * ghostSeconds;
  const int chunkFrames = 128;
  int16_t buffer[chunkFrames * 2];
  float phase = 0;
  float breath = 0;

  for (int frame = 0; frame < frameCount;) {
    int n = 0;
    for (; n < chunkFrames && frame < frameCount; n++, frame++) {
      float t = (float)frame / sampleRate;
      // slow wail up and down, plus a quick vibrato wobble
      float pitch = 320 + 140 * sinf(TWO_PI * 0.35 * t) + 12 * sinf(TWO_PI * 5.5 * t);
      phase += TWO_PI * pitch / sampleRate;
      if (phase > TWO_PI) {
        phase -= TWO_PI;
      }
      // low-passed noise gives the moan a breathy, airy edge
      breath = breath * 0.97 + random(-1000, 1000) / 1000.0 * 0.03;
      float fade = min(1.0f, t / 0.5f) * min(1.0f, (ghostSeconds - t) / 0.8f);
      float sample = (0.7 * sinf(phase) + 0.2 * sinf(2 * phase) + 2.0 * breath) * fade * volume;
      int16_t value = constrain(sample, -1.0f, 1.0f) * 32767;
      buffer[n * 2] = value;      // left
      buffer[n * 2 + 1] = value;  // right, the amp may listen to either channel
    }
    i2s.write((uint8_t*)buffer, n * 2 * sizeof(int16_t));
  }

  soundPlaying = false;
  vTaskDelete(NULL);
}

// Starts the ghost sound in the background, ignored if it is already playing
void playGhost() {
  if (soundPlaying) {
    return;
  }
  soundPlaying = true;
  xTaskCreate(ghostSoundTask, "ghost", 4096, NULL, 1, NULL);
}

// Swings the servo to look startled, then settles back to rest
void jerkServo() {
  myServo.write(150);
  delay(300);
  myServo.write(30);
  delay(300);
  myServo.write(90);
}

// Runs the full jump-scare sequence: fan and lights on, ghost sound, servo jerk
void startScare() {
  scaring = true;
  scareStartedAt = millis();
  Serial.println("scare: started");
  setFan(true);
  setLights(true);
  playGhost();
  jerkServo();
}

// Ends the scare, the fan goes off and the lights go back to their manual state
void endScare() {
  scaring = false;
  setFan(false);
  setLights(lightsOn);
  lastScareEndedAt = millis();
  Serial.println("scare: ended");
}

// Describes the current state for the phone page
String statusText() {
  return String(armed ? "armed" : "disarmed") + ", lights " + (lightsOn ? "on" : "off");
}

// Serves the phone control page
void handleRoot() {
  String html = "<!DOCTYPE html><html><head><title>Scare Prop</title>"
    "<meta name='viewport' content='width=device-width, initial-scale=1'>"
    "<style>body{font-family:sans-serif;text-align:center;padding-top:40px;background:#111;color:#eee}"
    "button{font-size:22px;padding:16px 18px;margin:6px;width:80%}</style></head><body>"
    "<h1>Scare Prop</h1><p id='s'>" + statusText() + "</p>"
    "<button onclick=\"go('/scare')\">Scare now</button>"
    "<button onclick=\"go('/ghost')\">Ghost sound</button>"
    "<button onclick=\"go('/lights')\">Lights on/off</button>"
    "<button onclick=\"go('/arm')\">Sensor armed/disarmed</button>"
    "<script>function go(p){fetch(p).then(r=>r.text()).then(t=>s.textContent=t)}</script>"
    "</body></html>";
  server.send(200, "text/html", html);
}

// Wires up all the HTTP routes, each replies with the new status text
void setupRoutes() {
  server.on("/", handleRoot);
  server.on("/scare", [](){
    if (!scaring) {
      startScare();
    }
    server.send(200, "text/plain", statusText());
  });
  server.on("/ghost", [](){ playGhost(); server.send(200, "text/plain", statusText()); });
  server.on("/lights", [](){
    lightsOn = !lightsOn;
    if (!scaring) {
      setLights(lightsOn);
    }
    server.send(200, "text/plain", statusText());
  });
  server.on("/arm", [](){ armed = !armed; server.send(200, "text/plain", statusText()); });
}

void setup() {
  Serial.begin(9600);
  Serial.println("setup: start");

  pinMode(trigPin, OUTPUT);
  pinMode(echoPin, INPUT);
  pinMode(fanRelayPin, OUTPUT);
  for (int i = 0; i < ledCount; i++) {
    pinMode(ledPins[i], OUTPUT);
  }
  setFan(false);
  setLights(false);

  myServo.attach(servoPin);
  myServo.write(90);

  i2s.setPins(i2sBclkPin, i2sLrcPin, i2sDinPin);
  if (!i2s.begin(I2S_MODE_STD, sampleRate, I2S_DATA_BIT_WIDTH_16BIT, I2S_SLOT_MODE_STEREO)) {
    Serial.println("setup: I2S init failed, no sound");
  }

  // Connects to a saved WiFi network, or opens a setup portal (AP named
  // "ScareSetup") for 2 minutes. If nobody configures it the prop still runs
  // on the distance sensor alone, just without the phone page.
  wifiManager.setConfigPortalTimeout(120);
  if (wifiManager.autoConnect("ScareSetup")) {
    Serial.print("setup: WiFi connected, browse to ");
    Serial.println(WiFi.localIP());
    setupRoutes();
    server.begin();
  } else {
    Serial.println("setup: no WiFi, running sensor-only");
  }

  Serial.println("setup: ready");
}

void loop() {
  server.handleClient();

  if (scaring) {
    flickerLights();
    if (millis() - scareStartedAt >= scareDurationMs) {
      endScare();
    }
    return;
  }

  if (!armed || millis() - lastScareEndedAt < cooldownMs) {
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
