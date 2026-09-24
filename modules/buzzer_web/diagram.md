# Wiring diagram (ASCII)

Matches `diagram.json`: a single passive (piezo) buzzer, no other hardware.
Control comes over WiFi: the ESP32 connects to your home WiFi via the
`WiFiManager` captive portal (setup AP named `BuzzerSetup`, no credentials
hardcoded), then serves a page with one button per note (`C4` to `C5`), each
press plays that note for 300ms using `tone()`.

```
       +-----------------------------------------------+
       |            ESP32 DEVKIT V1 (30-pin)            |
       |                  [ USB PORT ]                  |
       |                                                 |
       |  D25 [========]----> To Buzzer pin 1           |
       |  GND [========]----> To Buzzer pin 2           |
       +-----------------------------------------------+

       +-----------------------------------------------+
       |           PASSIVE (PIEZO) BUZZER               |
       |  1 ---> ESP32 D25 (tone signal)                |
       |  2 ---> ESP32 GND                              |
       +-----------------------------------------------+
```
