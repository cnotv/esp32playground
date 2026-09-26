#include <WiFiManager.h>
#include <WebServer.h>

const int buzzerPin = 25;
const int noteDurationMs = 300;

WiFiManager wifiManager;
WebServer server(80);

// One octave of named notes mapped to their frequency in Hz
struct Note {
  const char* name;
  int frequency;
};

const Note notes[] = {
  { "C4", 262 },
  { "D4", 294 },
  { "E4", 330 },
  { "F4", 349 },
  { "G4", 392 },
  { "A4", 440 },
  { "B4", 494 },
  { "C5", 523 },
};
const int noteCount = sizeof(notes) / sizeof(notes[0]);

// Plays one note for a fixed duration
void playNote(int frequency) {
  tone(buzzerPin, frequency, noteDurationMs);
}

// Plays a rising siren sweep
void playSiren() {
  for (int freq = 200; freq <= 1500; freq += 20) {
    tone(buzzerPin, freq);
    delay(8);
  }
  noTone(buzzerPin);
}

// Plays a slow wobble between two low tones, like a ghost moaning
void playGhostMoan() {
  for (int i = 0; i < 6; i++) {
    tone(buzzerPin, 150);
    delay(150);
    tone(buzzerPin, 220);
    delay(150);
  }
  noTone(buzzerPin);
}

// Plays two low thumps like a heartbeat
void playHeartbeat() {
  tone(buzzerPin, 60, 120);
  delay(180);
  tone(buzzerPin, 60, 120);
  delay(180);
}

// Plays random frequency jumps for a glitchy screech
void playScreech() {
  for (int i = 0; i < 20; i++) {
    tone(buzzerPin, random(300, 3000));
    delay(30);
  }
  noTone(buzzerPin);
}

// Plays a slow descending drone
void playDoomDrone() {
  for (int freq = 400; freq >= 60; freq -= 10) {
    tone(buzzerPin, freq);
    delay(20);
  }
  noTone(buzzerPin);
}

// Builds the piano key buttons for the control page
String buildKeysHtml() {
  String html;
  for (int i = 0; i < noteCount; i++) {
    html += "<button onclick=\"fetch('/note?n=" + String(i) + "')\">" + notes[i].name + "</button>";
  }
  return html;
}

// Serves the control page with piano key buttons and scary sound effects
void handleRoot() {
  String html = "<!DOCTYPE html><html><head><title>Mini Piano</title>"
    "<meta name='viewport' content='width=device-width, initial-scale=1'>"
    "<style>body{font-family:sans-serif;text-align:center;padding-top:40px}"
    "button{font-size:22px;padding:16px 18px;margin:4px}</style></head><body>"
    "<h1>Mini Piano</h1><p>" + buildKeysHtml() + "</p>"
    "<h1>Scary Sounds</h1><p>"
    "<button onclick=\"fetch('/siren')\">Siren</button>"
    "<button onclick=\"fetch('/ghost')\">Ghost Moan</button>"
    "<button onclick=\"fetch('/heartbeat')\">Heartbeat</button>"
    "<button onclick=\"fetch('/screech')\">Screech</button>"
    "<button onclick=\"fetch('/doom')\">Doom Drone</button>"
    "</p></body></html>";
  server.send(200, "text/html", html);
}

// Route handler for a single piano key press
void handleNote() {
  if (!server.hasArg("n")) {
    server.send(400, "text/plain", "missing n");
    return;
  }

  int index = server.arg("n").toInt();
  if (index < 0 || index >= noteCount) {
    server.send(400, "text/plain", "bad note index");
    return;
  }

  playNote(notes[index].frequency);
  server.send(200, "text/plain", "ok");
}

// Wires up all the HTTP routes
void setupRoutes() {
  server.on("/", handleRoot);
  server.on("/note", handleNote);
  server.on("/siren", [](){ playSiren(); server.send(200, "text/plain", "ok"); });
  server.on("/ghost", [](){ playGhostMoan(); server.send(200, "text/plain", "ok"); });
  server.on("/heartbeat", [](){ playHeartbeat(); server.send(200, "text/plain", "ok"); });
  server.on("/screech", [](){ playScreech(); server.send(200, "text/plain", "ok"); });
  server.on("/doom", [](){ playDoomDrone(); server.send(200, "text/plain", "ok"); });
}

void setup() {
  Serial.begin(9600);
  Serial.println("setup: start");

  // Connects to a saved WiFi network, or opens a setup portal (AP named
  // "BuzzerSetup") to enter credentials if none are saved yet
  wifiManager.autoConnect("BuzzerSetup");
  Serial.print("setup: WiFi connected, browse to ");
  Serial.println(WiFi.localIP());

  setupRoutes();
  server.begin();
  Serial.println("setup: web server started");
}

void loop() {
  server.handleClient();
}
