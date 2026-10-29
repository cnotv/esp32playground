// Minimal ESP32 MIDI-over-WiFi bridge for Yamaha EZ USB
// Note: this board (photo) has USB-serial, not USB-OTG Host.
// For native USB-MIDI from EZ, use ESP32-S3 USB-Host (TinyUSB) or
// a USB-MIDI-to-UART adapter feeding Serial here.
#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include <WiFiManager.h>

WiFiManager wm;

AsyncWebServer server(80);
AsyncWebSocket ws("/midi");
String lastNote = "none";

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
    request->send(200, "application/json", "{\"note\":\"" + lastNote + "\"}");
  });
  server.begin();
}

void loop() {
  // Placeholder: read MIDI bytes from Serial if using UART adapter
  // For USB-MIDI Host, replace with TinyUSB MIDI parser + ws.text()
  if (Serial.available() >= 3) {
    uint8_t b1 = Serial.read();
    uint8_t b2 = Serial.read();
    uint8_t b3 = Serial.read();
    if ((b1 & 0xF0) == 0x90 || (b1 & 0xF0) == 0x80) {
      char msg[32];
      snprintf(msg, sizeof(msg), "m:%02X:%02X:%02X", b1, b2, b3);
      lastNote = msg + 2; // "90:3C:7F", no "m:" prefix
      ws.textAll(msg); // push to all subscribed browsers
    }
  }
}
