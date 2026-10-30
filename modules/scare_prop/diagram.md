# Wiring diagram (ASCII)

Matches `diagram.json`. Jump-scare prop: when the ultrasonic distance sensor
sees something closer than 30cm, the two LEDs flicker, the speaker plays a
synthesized ghost moan, the servo jerks and the fan relay switches on, all
together. After 4s it stops, then ignores the sensor for 8s.

A phone can also drive it over WiFi (see "Phone control" below), but the
sensor trigger works without WiFi too.

**STOP before wiring the fan**: the fan cannot connect to a GPIO pin directly,
same reason a bare motor can't, it will draw far more current than the pin can
source. `D32` must go to an actual **relay MODULE**'s `IN` pin (a small board
with `VCC`/`GND`/`IN` and its own onboard transistor + flyback diode), not a
bare relay coil and not the motor itself. The `wokwi-ks2e-m-dc5` part in
`diagram.json` is a bare relay, Wokwi has no relay-module part, so it's a
stand-in for layout only, verify what you actually have physically matches a
relay module before connecting anything to the fan side. Some relay modules
are active-low (LOW turns the relay on), if the fan comes on backwards from
what you expect, flip the `HIGH`/`LOW` in `setFan()`. Leaving `D32`
unconnected is fine, everything else still works without a fan.

The NS4168 I2S amplifier has no Wokwi part, so the sound side is wiring
documentation only, the simulator will not play it.

```
       [======= BREADBOARD POWER RAILS =======]
       (+) 5V Rail   ================================= (Connects to ESP32 VIN, Servo V+, Amp V)
       (-) GND Rail  ================================= (Connects to ESP32 GND, Servo GND, Amp G, relay MODULE GND)

       +-----------------------------------------------+
       |            ESP32 DEVKIT V1 (30-pin)            |
       |                  [ USB PORT ]                  |
       |                                                 |
       |  3V3 [========]----> To HC-SR04 VCC            |
       |  GND [========]----> To HC-SR04 GND            |
       |  D26 [========]----> To HC-SR04 TRIG           |
       |  D27 [========]----> To HC-SR04 ECHO           |
       |  D14 [========]----> To 330ohm resistor ---> LED 1 A (anode)|
       |  D13 [========]----> To 330ohm resistor ---> LED 2 A (anode)|
       |  GND [========]----> To both LED C (cathode)   |
       |  D18 [========]----> To Amp BCL                |
       |  D19 [========]----> To Amp LRC                |
       |  D23 [========]----> To Amp DIN                |
       |  D33 [========]----> To Servo PWM              |
       |  GND [=======> GND Rail]                       |
       |  VIN [=======> 5V Rail]                        |
       |  D32 [========]----> To relay MODULE IN        |
       +-----------------------------------------------+

       +-----------------------------------------------+
       |        NS4168 I2S AMP (G V BCL LRC DIN)         |
       |  G   ---> GND Rail                             |
       |  V   ---> 5V Rail (louder than 3V3, 2.5 to 5.5V)|
       |  BCL ---> ESP32 D18                            |
       |  LRC ---> ESP32 D19                            |
       |  DIN ---> ESP32 D23                            |
       |  Speaker plugs into the amp's own 2-pin output.|
       |  It is a bridged output: NEVER connect either  |
       |  speaker wire to GND or to the ESP32.          |
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

The speaker and the servo both pull current spikes from the 5V rail at the
same moment. If the ESP32 resets when the scare starts, that is a brownout:
power the 5V rail from a separate 5V supply (shared GND), or lower `volume`
in the sketch.

## Phone control

First boot opens a WiFi network called `ScareSetup` for 2 minutes. Join it
from the phone, pick the home WiFi and enter its password. The Serial monitor
(`make monitor`) then prints the address to open, e.g. `192.168.1.42`. The
page has four buttons: Scare now, Ghost sound, Lights on/off, Sensor
armed/disarmed (disarm it while you set the prop up). If nobody joins within
2 minutes the prop runs on the sensor alone.

## Mounting behind cardboard

Keep the ESP32, breadboard, amp and resistors behind the cardboard, and only
run cable to the parts that must show at the front.

| Part at the front | Cable | Wires |
| --- | --- | --- |
| HC-SR04 sensor | 4 wires, an old Ethernet (CAT5/CAT6) cable is ideal | 3V3, GND, TRIG, ECHO |
| 2 LEDs | spare wires in the same Ethernet cable | D14, D13, shared GND |
| Servo | a standard servo extension lead (3 pin, plugs straight in) | V+, GND, PWM |
| Speaker | any 2 core cable, e.g. thin speaker wire or a spare twisted pair | amp output +/− only |

One Ethernet cable has 8 wires in 4 twisted pairs: use pair 1 for 3V3 and
GND, pair 2 for TRIG and ECHO, pair 3 for D14 and D13, and one wire of pair 4
as the LEDs' own GND. Keep the sensor run under about 2m.

How to fit it:

1. **Sensor**: cardboard blocks ultrasound, so cut two round holes about
   16mm wide, spaced like the ones on your board (measure centre to
   centre, about 26mm), and push the two silver
   transducers through from behind. Hot glue the board's edges, not the
   transducers.
2. **LEDs**: poke a 5mm hole with a pencil, the LED pushes in and holds. The
   330ohm resistors stay on the breadboard, only the LED goes to the front.
   Mark the long leg (anode) wire before you close it up.
3. **Joins**: solder and heat shrink at the part end is the most reliable. No
   soldering iron: crimp female Dupont ends onto the cable, or use Wago lever
   connectors.
4. **Strain relief**: tape or zip tie each cable to the cardboard a few cm from
   the part, so a tug pulls the cardboard, not the joint.
5. **Speaker**: face it at a hole or a patch of thin fabric, cardboard in
   front muffles it. A ring of hot glue around its rim against the cardboard
   stops rattling.
