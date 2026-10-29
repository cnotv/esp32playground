# yamaha_esp_bridge

Smallest module connecting Yamaha EZ USB MIDI -> ESP -> subscribed browsers.

## Note on option B (native Web MIDI)
True native `navigator.requestMIDIAccess()` requires the ESP to appear as a USB-MIDI device to your PC OS. The ESP in your photo (ESP32-WROOM) has USB-serial (CP2102/CH340), not USB-OTG. You cannot read Yamaha EZ USB-MIDI and present USB-MIDI device to PC simultaneously on the same USB port without ESP32-S3 OTG or an extra USB-MIDI adapter chip.

This module implements the practical equivalent: ESP reads MIDI (Serial adapter / TinyUSB Host) and pushes over WebSocket; browser subscribes on access (`index.html` load triggers `yamahaMIDIReady`). The `strudele_bridge.js` exposes events (`yamahaNote`) that `strudeleditor` can consume instead of native MIDI.

## Files
- `yamaha_esp_bridge/yamaha_esp_bridge.ino` — ESP32 sketch (WiFiManager + WebSocket server `ws://IP:80/midi`)
- `browser/strudele_bridge.js` — browser subscriber, fires `yamahaNote` events
- `browser/index.html` — loads bridge and shows subscription trigger

## Dependencies (Arduino Library Manager)
- WiFiManager (tzapu)
- ESP Async WebServer 3.x (ESP32Async)
- AsyncTCP 3.x (ESP32Async) — the old 1.1.4 fails to build against ESP Async WebServer 3.x

## Flashing
Folder name must match the `.ino` name, so open `yamaha_esp_bridge/yamaha_esp_bridge.ino` directly. On first boot join the `ESP_MIDIBridge` AP and pick your WiFi.

## Using it
Open `http://<esp-ip>/` — the sketch serves a test page that subscribes on load. Or open `browser/index.html` directly and enter the ESP IP when asked, then watch for `yamahaNote` events.

## Shortcut
`shortcut: native USB-MIDI host on ESP32 requires ESP32-S3 OTG or external adapter; this uses Serial proxy.`
