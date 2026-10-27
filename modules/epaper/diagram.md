# Wiring diagram (ASCII)

Matches `diagram.json`: ESP32 wired directly to a 4.2in SPI e-paper module
(WeAct Studio style, e.g. the AliExpress listing this variant was built from).
`epaper.ino` now exists and drives the real panel via **GxEPD2** (class
`GxEPD2_420c_GDEY042Z98`, the black/white/red panel; the b/w class
`GxEPD2_420_GDEY042T81` draws on it too but times out and hangs mid refresh,
measured on this board). It joins WiFi through a `WiFiManager` captive portal
(`EpaperSetup`), syncs the time over NTP, then rotates four screens: a clock,
a forecast, an event list and notes. Only the clock is real, the other three
show invented sample content until notes are saved or an image is uploaded
to that slot from the control page at `http://epaper.local`. Every
screen change is a full refresh of about 10 s, this panel has no fast partial
refresh. Wokwi still has no e-paper part, so the simulated diagram uses a
`wokwi-ili9341` as a stand-in wired to the real pin names below; it will not
render or behave like the real panel even though `wokwi.toml` now points at
this folder's own build, GxEPD2's SSD1683 commands aren't the ILI9341
protocol the stand-in part understands. Treat `diagram.json` in this folder as
a wiring reference only, not a working simulation, test on real hardware.

Pin names, pin order, and wire colours below are read off the real module's
back silkscreen and its 8-pin cable (pin 8 is `VCC` on the left of the
connector, pin 1 is `BUSY` on the right). The module labels its SPI pins `SDA`
and `SCL`, that is only WeAct's naming, the bus is SPI (`SDA` is MOSI, `SCL` is
SCK), not I2C.

The 30-pin DevKit v1 has no pins printed `D16` or `D17`. GPIO16 is labelled
`RX2` and GPIO17 is labelled `TX2` (between `D4` and `D5`), those are the pins
`epaper.ino` means by `rstPin = 16` and `dcPin = 17`.

```
       +-------------------------------------------------------+
       |               ESP32 DEVKIT V1 (30-pin)                |
       |                     [ USB PORT ]                      |
       |                                                       |
       |  3V3 [========]----> To e-paper VCC        (red)      |
       |  GND [========]----> To e-paper GND        (black)    |
       |  D23 [========]----> To e-paper SDA (MOSI) (yellow)   |
       |  D18 [========]----> To e-paper SCL (SCK)  (green)    |
       |  D5  [========]----> To e-paper CS         (blue)     |
       |  TX2 [========]----> To e-paper D/C        (white)    |
       |  RX2 [========]----> To e-paper RES        (orange)   |
       |  D4  [========]----> To e-paper BUSY       (purple)   |
       |                                                       |
       +-------------------------------------------------------+

                         |||||||| (8-pin cable)
                         vvvvvvvv

       +-------------------------------------------------------+
       |      WEACT STUDIO 4.2in EPAPER MODULE (400 x 300)     |
       |                                                       |
       |  8  VCC  (red)    ---> ESP32 3V3                      |
       |  7  GND  (black)  ---> ESP32 GND                      |
       |  6  SDA  (yellow) ---> ESP32 D23 (MOSI)               |
       |  5  SCL  (green)  ---> ESP32 D18 (SCK)                |
       |  4  CS   (blue)   ---> ESP32 D5                       |
       |  3  D/C  (white)  ---> ESP32 TX2 (GPIO17)             |
       |  2  RES  (orange) ---> ESP32 RX2 (GPIO16)             |
       |  1  BUSY (purple) ---> ESP32 D4                       |
       |                                                       |
       +-------------------------------------------------------+
```

Silkscreen notes from the module: `I/O: 3.3V`, so never drive its signal pins
from a 5V board. `VCC: 3.3V Only (SB3)` or `VCC: 3.3-5V (SB4+SB5)` depending on
which solder bridges are closed, check yours before feeding `VCC` anything
other than `3V3`. A second pair of pads selects `3-Lines SPI` or `4-Lines SPI`,
`epaper.ino` uses a `D/C` pin, so it needs 4-line mode.

Not simulated in Wokwi: the `BUSY` pin has no equivalent on the `wokwi-ili9341`
stand-in part, so `diagram.json` in this folder has no wire for it even though
the real module needs it wired to `D4`.
