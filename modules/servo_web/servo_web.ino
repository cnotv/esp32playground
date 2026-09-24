#include <WiFiManager.h>
#include <WebServer.h>
#include <ESP32Servo.h>

WiFiManager wifiManager;

const int servoPin = 33;
Servo myServo;
int servoAngle = 0;
int servoStep = 5;
bool spinEnabled = false;

const int FIXED_ANGLE = 90;  // degrees the "go to" button jumps to, pick whatever position you want

WebServer server(80);

// Moves the servo one step, bouncing between 0 and 180 degrees
void sweepServo() {
  myServo.write(servoAngle);
  servoAngle += servoStep;
  if (servoAngle <= 0 || servoAngle >= 180) {
    servoStep = -servoStep;
  }
}

// Steps the servo up or down by a fixed amount, staying within 0 to 180 degrees
void moveServoBy(int amount) {
  spinEnabled = false;
  servoAngle += amount;
  if (servoAngle < 0) {
    servoAngle = 0;
  }
  if (servoAngle > 180) {
    servoAngle = 180;
  }
  myServo.write(servoAngle);
}

// Jumps the servo straight to a fixed angle
void moveServoTo(int angle) {
  spinEnabled = false;
  servoAngle = angle;
  myServo.write(servoAngle);
}

// Starts or stops the continuous sweep between 0 and 180 degrees
void toggleServoSpin() {
  spinEnabled = !spinEnabled;
}

// Serves the control page with buttons
void handleRoot() {
  String html = "<!DOCTYPE html><html><head><title>Servo Control</title>"
    "<meta name='viewport' content='width=device-width, initial-scale=1'>"
    "<style>body{font-family:sans-serif;text-align:center;padding-top:40px}"
    "button{font-size:24px;padding:16px 24px;margin:8px}</style></head><body>"
    "<h1>Servo Control</h1>"
    "<p><button onclick=\"fetch('/up')\">Up (+10)</button>"
    "<button onclick=\"fetch('/down')\">Down (-10)</button></p>"
    "<p><button onclick=\"fetch('/playpause')\">Play/Pause spin</button></p>"
    "<p><button onclick=\"fetch('/number')\">Go to " + String(FIXED_ANGLE) + "</button></p>"
    "</body></html>";
  server.send(200, "text/html", html);
}

// Route handler for the up button
void handleUp() {
  moveServoBy(10);
  server.send(200, "text/plain", "ok");
}

// Route handler for the down button
void handleDown() {
  moveServoBy(-10);
  server.send(200, "text/plain", "ok");
}

// Route handler for the play/pause button
void handlePlayPause() {
  toggleServoSpin();
  server.send(200, "text/plain", "ok");
}

// Route handler for the fixed-position button
void handleNumber() {
  moveServoTo(FIXED_ANGLE);
  server.send(200, "text/plain", "ok");
}

// Wires up all the HTTP routes
void setupRoutes() {
  server.on("/", handleRoot);
  server.on("/up", handleUp);
  server.on("/down", handleDown);
  server.on("/playpause", handlePlayPause);
  server.on("/number", handleNumber);
}

void setup() {
  Serial.begin(9600);
  Serial.println("setup: start");

  myServo.attach(servoPin);
  Serial.println("setup: servo attached");

  // Connects to a saved WiFi network, or opens a setup portal (AP named
  // "ServoControlSetup") to enter credentials if none are saved yet
  wifiManager.autoConnect("ServoControlSetup");
  Serial.print("setup: WiFi connected, browse to ");
  Serial.println(WiFi.localIP());

  setupRoutes();
  server.begin();
  Serial.println("setup: web server started");
}

void loop() {
  server.handleClient();

  if (spinEnabled) {
    sweepServo();
    delay(15);
  }
}
