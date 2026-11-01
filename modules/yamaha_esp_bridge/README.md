# yamaha_esp_bridge

Smallest module connecting Yamaha EZ USB MIDI -> ESP32-S3 -> subscribed browsers.

The S3 has USB-OTG, so it hosts the keyboard directly (`ESP32_Host_MIDI`) and pushes notes over WebSocket; the browser subscribes on access (`index.html` load triggers `yamahaMIDIReady`). The `strudele_bridge.js` exposes events (`yamahaNote`) that `strudeleditor` can consume instead of native MIDI.

This sketch targets ESP32-S3/S2/P4 (USB-OTG host). A classic ESP32 has no OTG and cannot run it.

## Files
- `yamaha_esp_bridge/yamaha_esp_bridge.ino` — ESP32-S3 sketch (WiFiManager + USB host + WebSocket server `ws://IP:80/midi`)
- `browser/strudele_bridge.js` — browser subscriber, fires `yamahaNote` events
- `browser/index.html` — loads bridge and shows subscription trigger

## Dependencies (Arduino Library Manager)
- WiFiManager (tzapu)
- ESP Async WebServer 3.x (ESP32Async)
- AsyncTCP 3.x (ESP32Async) — the old 1.1.4 fails to build against ESP Async WebServer 3.x
- ESP32_Host_MIDI (sauloverissimo) — USB-MIDI host on the S3 OTG port

## Flashing
Board: an ESP32-S3. `USB Mode` = Hardware CDC and JTAG, `USB CDC On Boot` = Disabled (both defaults); the USB host driver takes the OTG PHY at boot and logs go to UART0. Plug the keyboard into the S3's USB-OTG port. On first boot join the `ESP_MIDIBridge` AP and pick your WiFi.

Power: the OTG connector is the keyboard link, so feed the board from 5V/VIN (or use a second USB port if your board has one). It cannot be powered by the same port it hosts from.

## Using it
Open `http://<esp-ip>/` — the sketch serves a test page that subscribes on load. Or open `browser/index.html` directly and enter the ESP IP when asked, then watch for `yamahaNote` events.

## Shortcut
`shortcut: one USB port cannot both power the S3 and host the keyboard; use 5V/VIN or a two-port S3 board.`
