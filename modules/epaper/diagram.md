# Wiring diagram (ASCII)

Matches `diagram.json`: ESP32 wired directly to a 4.2in SPI e-paper module
(WeAct Studio style, e.g. the AliExpress listing this variant was built from).
Wokwi has no e-paper part, so the simulated diagram uses a `wokwi-ili9341` as a
stand-in wired to the real pin names below; it will not render or behave like
the real panel, and does not run the real GxEPD2/SSD1683 firmware protocol.
Treat `diagram.json` in this folder as a wiring reference only, not a working
simulation. `wokwi.toml` points at `modules/servo/`'s build just so the
simulator has something valid to load, that firmware has nothing to do with
this display and won't drive it.

```
       +-----------------------------------------------+
       |            ESP32 DEVKIT V1 (30-pin)            |
       |                  [ USB PORT ]                  |
       |                                                 |
       |  3V3 [========]----> To e-paper VCC             |
       |  GND [========]----> To e-paper GND             |
       |  D23 [========]----> To e-paper DIN (MOSI)      |
       |  D18 [========]----> To e-paper CLK (SCK)       |
       |  D5  [========]----> To e-paper CS              |
       |  D17 [========]----> To e-paper DC              |
       |  D16 [========]----> To e-paper RST             |
       |  D4  [========]----> To e-paper BUSY            |
       |                                                 |
       +-----------------------------------------------+

                             |||||| (SPI + control lines)
                             vvvvvv

       +-----------------------------------------------+
       |         4.2in SPI E-PAPER MODULE               |
       |  VCC  ---> ESP32 3V3 (module is 3.3V logic)    |
       |  GND  ---> ESP32 GND                            |
       |  DIN  ---> ESP32 D23 (MOSI)                     |
       |  CLK  ---> ESP32 D18 (SCK)                      |
       |  CS   ---> ESP32 D5                             |
       |  DC   ---> ESP32 D17                            |
       |  RST  ---> ESP32 D16                            |
       |  BUSY ---> ESP32 D4                             |
       +-----------------------------------------------+
```

Not simulated in Wokwi: the `BUSY` pin has no equivalent on the `wokwi-ili9341`
stand-in part, so `diagram.json` in this folder has no wire for it even though
the real module needs it wired to `D4`.
