# Drum Synth Circuit

Text diagram (Wokwi: `wokwi.toml` / `drum_synth/diagram.json`):

```
ESP32-S3 (S3 dev board)
  GPIO4  ──[BTN]── GND   (drum 1 / kick sample)
  GPIO5  ──[BTN]── GND   (drum 2 / snare)
  GPIO6  ──[BTN]── GND   (drum 3 / hat)
  GPIO7  ──[BTN]── GND   (drum 4 / pattern trigger)
  GPIO8  ──[BTN]── GND   (mode cycle: volume → tempo → filter → touch rot)
  GPIO2  ──[POT 10k wiper]── 3V3 / GND  (analog value)
  
  I2S (MAX98357A amp):
    GPIO16 = BCLK
    GPIO17 = LRCLK
    GPIO18 = DATA
    VCC = 3V3, GND = GND

4 buttons: 4-leg pushbutton — wire opposite corners (GPIO + GND); other 2 pins are paired terminal.
Pot: 10k between 3V3 and GND, wiper to GPIO2.
```
