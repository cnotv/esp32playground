# Wiring diagram (ASCII)

Matches `diagram.json`. Autonomous jump-scare prop: an ultrasonic distance
sensor triggers a fan, an LED, a siren on the buzzer, and a servo jerk when
something gets close, no WiFi, no phone needed. Intended physical build:
a bright LED and the fan positioned under a loose bag/cloth, so the fan
flutters it while the LED lights it up from underneath.

**STOP before wiring the fan**: the fan cannot connect to a GPIO pin directly,
same reason a bare motor can't, it will draw far more current than the pin can
source. `D32` must go to an actual **relay MODULE**'s `IN` pin (a small board
with `VCC`/`GND`/`IN` and its own onboard transistor + flyback diode), not a
bare relay coil and not the motor itself. The `wokwi-ks2e-m-dc5` part in
`diagram.json` is a bare relay, Wokwi has no relay-module part, so it's a
stand-in for layout only, verify what you actually have physically matches a
relay module before connecting anything to the fan side. Some relay modules
are active-low (LOW turns the relay on), if the fan comes on backwards from
what you expect, flip the `HIGH`/`LOW` in `setFanAndLed()`.

```
       [======= BREADBOARD POWER RAILS =======]
       (+) 5V Rail   ================================= (Connects to ESP32 VIN, Servo V+)
       (-) GND Rail  ================================= (Connects to ESP32 GND, Servo GND, relay MODULE GND)

       +-----------------------------------------------+
       |            ESP32 DEVKIT V1 (30-pin)            |
       |                  [ USB PORT ]                  |
       |                                                 |
       |  3V3 [========]----> To HC-SR04 VCC            |
       |  GND [========]----> To HC-SR04 GND            |
       |  D26 [========]----> To HC-SR04 TRIG           |
       |  D27 [========]----> To HC-SR04 ECHO           |
       |  D14 [========]----> To 330ohm resistor ---> LED A (anode)|
       |  GND [========]----> To LED C (cathode)        |
       |  D25 [========]----> To Buzzer pin 1           |
       |  GND [========]----> To Buzzer pin 2           |
       |  D33 [========]----> To Servo PWM              |
       |  GND [=======> GND Rail]                       |
       |  VIN [=======> 5V Rail]                        |
       |  D32 [========]----> To relay MODULE IN        |
       +-----------------------------------------------+

       +-----------------------------------------------+
       |               MICRO SERVO                      |
       |  V+  ---> To 5V Rail, DO NOT use a real 5V     |
       |          rail powered from the ESP32 itself,   |
       |          current spikes brown out the board,   |
       |          use a separate 5V supply, shared GND  |
       |  GND ---> To GND Rail                          |
       |  PWM ---> To ESP32 D33                         |
       +-----------------------------------------------+

       +-----------------------------------------------+
       |          RELAY MODULE (not a bare relay)       |
       |  VCC ---> a suitable supply per the module's   |
       |          own rating (check its silkscreen)     |
       |  GND ---> To GND Rail                           |
       |  IN  ---> ESP32 D32                             |
       |  The fan's own power runs through the relay's  |
       |  switched contacts (COM/NO), from its own       |
       |  separate power source, never from the ESP32.  |
       +-----------------------------------------------+
```
