# drum_synth

Standalone drum synth for ESP32-S3: 4 buttons trigger embedded drum samples; 5th button cycles pot mode (volume/tempo/filter/touch rotation); potentiometer on GPIO2.

## Wiring
- Buttons (momentary, opposite pins): GPIO4 / 5 / 6 / 7 (drums), GPIO8 (mode)
- Pot (10k): GPIO2 (volume/tempo/filter/touch mode)
- I2S amp MAX98357A: GPIO16 BCLK / 17 LRCLK / 18 DATA

## Dependencies
- `esp32:esp32:esp32s3` core
- `esp_i2s` (built-in)

## Flashing
`arduino-cli compile --fqbn esp32:esp32:esp32s3 modules/drum_synth/drum_synth`

## Note
`drum_synth.ino` contains dummy WAV arrays — replace with real 16-bit mono 22050 Hz PCM arrays (use `ffmpeg -ar 22050 -ac 1 -f s16le`).
