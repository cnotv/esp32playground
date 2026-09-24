# Wiring diagram (ASCII)

Matches `diagram.json`: a single micro servo on pin 33 (adjusted from the
Elegoo example's original pin 18 to match the physical wiring already in
place), nothing else. Standalone reference sketch, not part of the main
`sketch_sep13a` build.

```
       +-----------------------------------------------+
       |            ESP32 DEVKIT V1 (30-pin)            |
       |                  [ USB PORT ]                  |
       |                                                 |
       |  D33 [========]----> To Servo PWM              |
       |  GND [========]----> To Servo GND              |
       |  VIN [========]----> To Servo V+                |
       +-----------------------------------------------+

       +-----------------------------------------------+
       |               MICRO SERVO                      |
       |  V+  ---> ESP32 VIN                            |
       |  GND ---> ESP32 GND                             |
       |  PWM ---> ESP32 D33                             |
       +-----------------------------------------------+
```
