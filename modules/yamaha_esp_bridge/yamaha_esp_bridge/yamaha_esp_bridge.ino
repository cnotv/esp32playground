// ESP32-S3 MIDI-over-WiFi bridge for Yamaha EZ USB.
// The S3 hosts the keyboard on its USB-OTG port; notes are pushed to
// subscribed browsers over WebSocket.
#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include <WiFiManager.h>
#include <ESP32_Host_MIDI.h>
#include <USBConnection.h>

WiFiManager wm;

AsyncWebServer server(80);
AsyncWebSocket ws("/midi");
USBConnection usbHost;
char lastNote[16] = "none";
uint64_t lastEventIndex = 0;
bool usbReady = false;

// Served at "/" so opening http://<esp-ip>/ gives a working page (same origin as the WS).
const char PAGE[] PROGMEM = R"HTML(<!DOCTYPE html>
<html><head><meta charset="utf-8"><title>Yamaha EZ ESP Bridge</title></head>
<body>
<h2>Yamaha EZ over WiFi</h2>
<p>Status: <span id="s">connecting...</span></p>
<pre id="log"></pre>
<script>
const ws = new WebSocket(`ws://${location.hostname}/midi`);
document.getElementById("s").textContent = location.hostname;
ws.onopen = () => {
  document.getElementById("s").textContent = "connected";
  window.dispatchEvent(new CustomEvent("yamahaMIDIReady"));
};
ws.onmessage = (e) => {
  if (!e.data || !e.data.startsWith("m:")) return;
  const [b1, b2, b3] = e.data.split(":").slice(1).map((x) => parseInt(x, 16));
  document.getElementById("log").textContent = `status ${b1.toString(16)} note ${b2} vel ${b3}`;
  window.dispatchEvent(new CustomEvent("yamahaNote", {
    detail: { status: b1 & 0xF0, note: b2, vel: b3, raw: [b1, b2, b3] }
  }));
};
ws.onclose = () => { document.getElementById("s").textContent = "disconnected"; };
</script>
</body></html>)HTML";

void onWsEvent(AsyncWebSocket *server, AsyncWebSocketClient *client,
               AwsEventType type, void *arg, uint8_t *data, size_t len) {
  if (type == WS_EVT_CONNECT) {
    // subscription triggered = browser accessed
    Serial.println("Browser subscribed");
    client->text("welcome:midi_bridge_active");
  }
}

void emitNote(uint8_t status, uint8_t note, uint8_t velocity) {
  char msg[16];
  snprintf(msg, sizeof(msg), "m:%02X:%02X:%02X", (unsigned)status, (unsigned)note, (unsigned)velocity);
  snprintf(lastNote, sizeof(lastNote), "%s", msg + 2); // "90:3C:7F", no "m:" prefix
  ws.textAll(msg);
}

void setup() {
  Serial.begin(115200);
  // Captive portal "ESP_MIDIBridge" on first boot / after failed connect.
  if (!wm.autoConnect("ESP_MIDIBridge")) {
    Serial.println("WiFi config failed, restarting");
    ESP.restart();
  }
  Serial.println(WiFi.localIP());

  ws.onEvent(onWsEvent);
  server.addHandler(&ws);
  server.on("/", WebRequestMethod::HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send_P(200, "text/html", PAGE);
  });
  // Plain GET /midi shows the last note; ws above only claims Upgrade: websocket requests.
  server.on("/midi", WebRequestMethod::HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(200, "application/json",
                  String("{\"note\":\"") + lastNote + "\",\"usb\":" + (usbReady ? "true" : "false") +
                    ",\"device\":" + (usbHost.isConnected() ? "true" : "false") + "}");
  });
  server.begin();

  usbReady = usbHost.begin();
  if (!usbReady) {
    Serial.print("USB host init failed: ");
    Serial.println(usbHost.getLastError());
  }
  midiHandler.addTransport(&usbHost);
  midiHandler.begin();
}

void loop() {
  midiHandler.task();

  // The handler queue is append-only; skip events already forwarded.
  for (const auto &ev : midiHandler.getQueue()) {
    if (ev.index <= lastEventIndex) continue;
    lastEventIndex = ev.index;
    if (ev.statusCode == MIDI_NOTE_ON) {
      emitNote(MIDI_NOTE_ON | ev.channel0, ev.noteNumber, ev.velocity7);
    } else if (ev.statusCode == MIDI_NOTE_OFF) {
      emitNote(MIDI_NOTE_OFF | ev.channel0, ev.noteNumber, 0);
    }
  }
}
