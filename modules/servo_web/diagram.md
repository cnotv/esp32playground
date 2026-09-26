# Wiring diagram (ASCII)

Matches `diagram.json`: a single micro servo, no IR receiver, no other input
hardware. Control comes over WiFi instead, credentials are never hardcoded:
the sketch uses the `WiFiManager` library, on first boot (or if it can't
reconnect to a saved network) it opens a setup access point named
`ServoControlSetup`. Connect a phone to that AP, a captive portal pops up to
pick your home WiFi and enter its password, which WiFiManager saves on the
ESP32 for future boots. After that the board joins your normal WiFi, check
Serial for the IP it was assigned, then browse to it for the
Up/Down/Play-Pause/Go-to buttons.

```
       [======= BREADBOARD POWER RAILS =======]
       (+) 5V Rail   ================================= (Connects to ESP32 VIN, Servo V+)
       (-) GND Rail  ================================= (Connects to ESP32 GND, Servo GND)

       +-----------------------------------------------+
       |            ESP32 DEVKIT V1 (30-pin)            |
       |                  [ USB PORT ]                  |
       |                                                 |
       |  D33 [========]----> To Servo PWM              |
       |  GND [=======> GND Rail]                       |
       |  VIN [=======> 5V Rail]                        |
       +-----------------------------------------------+

       +-----------------------------------------------+
       |               MICRO SERVO                      |
       |  V+  ---> To 5V Rail                           |
       |  GND ---> To GND Rail                          |
       |  PWM ---> ESP32 D33                             |
       +-----------------------------------------------+
```
