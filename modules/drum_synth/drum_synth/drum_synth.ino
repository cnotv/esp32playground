/* ESP32-S3 Drum Synth — 5 buttons + pot + embedded WAVs
 * Buttons:  GPIO4(kick) 5(snare) 6(hat) 7(pattern) 8(mode cycle)
 * Pot:     GPIO2 -> active parameter (volume / tempo / filter / touch rot)
 * I2S:     GPIO16 BCLK / 17 LRCLK / 18 DATA -> MAX98357A
 *
 * Replace SAMPLE_KICK / SNARE / HAT / OPEN with converted 16-bit mono 22050Hz arrays.
 */
#include <Arduino.h>
#include <ESP_I2S.h>

static I2SClass i2s;

// ----- Pins -----
const uint8_t BTN[4] = {4, 5, 6, 7};
const uint8_t BTN_MODE = 8;
const uint8_t POT_PIN  = 2;         // ADC1_CH2

// ----- Mode cycle (pot target) -----
enum Mode { VOL=0, TEMPO, FILTER, TOUCH_ROT, MODE_COUNT };
Mode activeMode = VOL;

// ----- Dummy embedded WAV arrays — REPLACE WITH REAL PCM DATA -----
// Convert: ffmpeg -i kick.wav -ar 22050 -ac 1 -sample_fmt s16 kick.raw
// Then embed as: const int16_t SAMPLE_KICK[] = { ... };
// Here we use tiny stubs so it compiles; swap the arrays for real drums.
const int16_t SAMPLE_KICK[4]  = {0, 300, 800, 1200};
const int16_t SAMPLE_SNARE[4] = {0, 200, 400, 600};
const int16_t SAMPLE_HAT[4]   = {0, 100, 150, 250};
const int16_t SAMPLE_OPEN[4]  = {0, 500, 900, 1100};

const int16_t* SAMPLES[4] = {SAMPLE_KICK, SAMPLE_SNARE, SAMPLE_HAT, SAMPLE_OPEN};
const size_t S_LEN = sizeof(SAMPLE_KICK) / sizeof(int16_t);

// ----- Pattern (4-step loop) -----
const uint8_t PATTERN_STEPS[4] = {1, 2, 3, 1};  // snare, hat, open, kick (example)

// ----- I2S (ESP_I2S) -----

// ----- Setup -----
void setup() {
  Serial.begin(115200);
  for (uint8_t i = 0; i < 4; i++) pinMode(BTN[i], INPUT_PULLUP);
  pinMode(BTN_MODE, INPUT_PULLUP);

  analogReadResolution(12);
  analogSetAttenuation(ADC_11db);

  i2s.setPins(16, 17, 18);  // BCLK, LRCLK, DATA
  i2s.begin(I2S_MODE_STD, 22050, I2S_DATA_BIT_WIDTH_16BIT, I2S_SLOT_MODE_MONO);

  Serial.printf("Drum synth ready — mode=%d (VOL/TEMPO/FILTER/TOUCH)\n", activeMode);
}

// ----- Play one embedded sample -----
void playSample(const int16_t* s, size_t len, float gain) {
  int16_t buf[512];
  size_t pos = 0;
  for (size_t i = 0; i < len; i++) {
    int16_t v = (int16_t)(s[i] * gain);
    buf[pos++] = v;
    if (pos >= 512 || i == len - 1) {
      i2s.write(buf, pos * sizeof(int16_t));
      pos = 0;
    }
  }
}

// ----- Main loop -----
void loop() {
  static bool lastModeBtn = HIGH;
  static unsigned long lastHit = 0;

  // Mode cycle button (debounced)
  bool modeBtn = digitalRead(BTN_MODE);
  if (modeBtn == LOW && lastModeBtn == HIGH && millis() - lastHit > 300) {
    activeMode = (Mode)((activeMode + 1) % MODE_COUNT);
    Serial.printf("Mode -> %s\n", activeMode == VOL ? "VOL" : activeMode == TEMPO ? "TEMPO" : activeMode == FILTER ? "FILTER" : "TOUCH_ROT");
    lastHit = millis();
  }
  lastModeBtn = modeBtn;

  // Read pot once per loop; applies to active parameter
  int potRaw = analogRead(POT_PIN);  // 0..4095
  float potNorm = potRaw / 4095.0f;

  // Buttons 1-4: 1-3 = fixed drums, 4 = pattern trigger
  for (uint8_t i = 0; i < 4; i++) {
    if (digitalRead(BTN[i]) == LOW && millis() - lastHit > 200) {
      float gain = (activeMode == VOL) ? (0.3f + potNorm * 0.7f) : 1.0f;
      if (i == 3) {
        // Pattern trigger (4-step loop)
        for (uint8_t step = 0; step < 4; step++) {
          uint8_t drumIdx = PATTERN_STEPS[step];
          float tempo = (activeMode == TEMPO) ? (0.5f + potNorm * 2.0f) : 1.0f;
          playSample(SAMPLES[drumIdx], (size_t)(S_LEN / tempo), gain);
          delay(10);  // quick spacing; use tempo for proper timing
        }
      } else {
        float tempoMult = (activeMode == TEMPO) ? (0.5f + potNorm * 2.0f) : 1.0f;
        float filterCut = (activeMode == FILTER) ? potNorm : 1.0f;
        float touchSens = (activeMode == TOUCH_ROT) ? potNorm : 0.5f;
        // Apply simple scaling; for real filter use a filter on the pot value
        float mixGain = gain * filterCut * touchSens;
        playSample(SAMPLES[i], S_LEN, mixGain);
      }
      lastHit = millis();
      delay(150);  // debounce
    }
  }
}
