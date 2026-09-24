# Wiring diagram (ASCII)

Matches `diagram.json`: an IR receiver only, nothing else. Standalone
reference sketch, not part of the main `sketch_sep13a` build. Pin numbers
match the real physical wiring already confirmed working (see
`/memories/repo/hardware-debugging.md`): `DATA` on `D4`, `VCC` from `3V3`
(not 5V, protects the GPIO), `GND` shared.

```
       +-----------------------------------------------+
       |            ESP32 DEVKIT V1 (30-pin)            |
       |                  [ USB PORT ]                  |
       |                                                 |
       |  D4  [========]----> To IR receiver DAT        |
       |  GND [========]----> To IR receiver GND        |
       |  3V3 [========]----> To IR receiver VCC        |
       +-----------------------------------------------+

       +-----------------------------------------------+
       |               IR RECEIVER                      |
       |  VCC ---> ESP32 3V3 (not 5V, protects the GPIO)|
       |  GND ---> ESP32 GND                             |
       |  DAT ---> ESP32 D4                              |
       +-----------------------------------------------+
```
