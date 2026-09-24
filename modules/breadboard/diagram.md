# Wiring diagram (ASCII)

Matches `diagram.json`: ESP32, a micro servo, and an IR receiver, with the
servo and IR receiver seated on their own breadboard for power rails. There is
no single sketch that drives both parts together anymore (the IR receiver
module is dead, see `/memories/repo/hardware-debugging.md`, and its role was
replaced by `modules/servo_web/`), so `wokwi.toml` here points at
`modules/servo/`'s build just so the simulator has something valid to load.
Only the servo half will actually move if you press play, the IR receiver
wiring is documentation only. Pin numbers below are `modules/servo/servo.ino`
and `modules/ir/ir.ino`'s wiring combined, not a guess.

```
       [======= BREADBOARD POWER RAILS =======]
       (+) 5V Rail   ================================= (Connects to ESP32 VIN, Servo V+)
       (-) GND Rail  ================================= (Connects to ESP32 GND, Servo GND, IR receiver GND)

       +-----------------------------------------------+
       |            ESP32 DEVKIT V1 (30-pin)            |
       |                  [ USB PORT ]                  |
       |                                                 |
       |  (Left Side Pins)          (Right Side Pins)   |
       |  EN                        D23                 |
       |  VP (Input Only)           D22                 |
       |  VN (Input Only)           TX0                 |
       |  D34 (Input Only)          RX0                 |
       |  D35                       D21                 |
       |  D32                       D19                 |
       |  D33 [========]----> To Servo PWM              D18                 |
       |  D25                                           D5                 |
       |  D26                                            TX2                |
       |  D27                                            RX2                |
       |  D14                                    To IR receiver DAT <--[==] D4  |
       |  D13                                            D2                 |
       |  GND [=======> GND Rail]                        D15                |
       |  VIN [=======> 5V Rail]                         GND                |
       |                                                  3V3 [====]--> To IR receiver VCC |
       +-----------------------------------------------+

       +-----------------------------------------------+
       |               MICRO SERVO                      |
       |  V+  ---> DO NOT use ESP32/breadboard 5V rail  |
       |          (current spikes brown out the board)  |
       |          Use a separate 5V supply, shared GND  |
       |  GND ---> To GND Rail (shared ground only)     |
       |  PWM ---> To ESP32 D33                         |
       +-----------------------------------------------+

       +-----------------------------------------------+
       |               IR RECEIVER                      |
       |  VCC ---> ESP32 3V3 (not 5V, protects the GPIO)|
       |  GND ---> To GND Rail                          |
       |  DAT ---> To ESP32 D4                          |
       +-----------------------------------------------+
```

Real module wire colors (confirmed against the physical part, do not assume
the usual red/black/yellow convention): `green` = VCC, `yellow` = GND, `red` =
DAT.
