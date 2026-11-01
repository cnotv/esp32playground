# Drum Synth Circuit

Built on a breadboard (ESP32-S3 sits on the rail, buttons/pot/I2S amp jump to rows).

Text diagram (Wokwi: `wokwi.toml` / `drum_synth/diagram.json`):

```
Breadboard layout:
[ESP32-S3] seated on 3V3 / GND / signal rows (left side)
  GPIO4  ──[BTN]── GND rail
  GPIO5  ──[BTN]── GND rail
  GPIO6  ──[BTN]── GND rail
  GPIO7  ──[BTN]── GND rail   (pattern trigger)
  GPIO8  ──[BTN]── GND rail   (mode cycle)
  GPIO2  ──[POT wiper]── 3V3 / GND rails

  I2S MAX98357A amp (side of breadboard):
    GPIO16 = BCLK
    GPIO17 = LRCLK
    GPIO18 = DATA
    VCC/GND → breadboard rails

4 buttons: wire opposite corners (GPIO + GND); other 2 pins paired terminal — do not bridge same-side pins.
Pot: 10k between 3V3 and GND, wiper to GPIO2.
Power: feed S3 via VIN/5V (single USB port is host link, not power source).
```
