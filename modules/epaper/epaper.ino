#include <WiFiManager.h>
#include <WebServer.h>
#include <ESPmDNS.h>
#include <LittleFS.h>
#include <Preferences.h>
#include <GxEPD2_3C.h>
#include <Fonts/FreeSans9pt7b.h>
#include <Fonts/FreeSans12pt7b.h>
#include <Fonts/FreeSansBold9pt7b.h>
#include <Fonts/FreeSansBold12pt7b.h>
#include <Fonts/FreeSansBold18pt7b.h>
#include <time.h>
#include "page.h"

// WeAct Studio 4.2in black/white/red module (SSD1683 driver, panel code
// GDEY042Z98). The b/w class GxEPD2_420_GDEY042T81 draws on this panel too,
// but its refresh waits are far too short for the three colour waveform
// (about 11 s here against about 1 s), so updates hit "Busy Timeout!" and the
// panel hangs mid transition. If your actual panel is a different model, swap
// this class for the matching one in
// GxEPD2/src/GxEPD2_display_selection_new_style.h.
const int csPin = 5;
// GPIO17 and GPIO16 are printed TX2 and RX2 on the 30-pin DevKit v1
const int dcPin = 17;
const int rstPin = 16;
const int busyPin = 4;

GxEPD2_3C<GxEPD2_420c_GDEY042Z98, GxEPD2_420c_GDEY042Z98::HEIGHT> display(
    GxEPD2_420c_GDEY042Z98(csPin, dcPin, rstPin, busyPin));

WiFiManager wifiManager;
WebServer server(80);
Preferences settings;

// Tuning knobs
const int screenRotation = 0;               // 0 or 2 for landscape (400 x 300), flip if the image is upside down
// Default for how long each screen stays up, changeable from the control
// page. Every change on a three colour panel is a full refresh that flashes
// for about 11 s, and the panel maker asks for at least 3 minutes between
// refreshes, so going lower is allowed but not kind to the panel.
const int defaultRotationMinutes = 3;
const int maxRotationMinutes = 120;
const uint16_t accentColour = GxEPD_RED;    // title bar, clock colon, current page dot
const char *timeZone = "CET-1CEST,M3.5.0,M10.5.0/3";  // Europe/Berlin, with daylight saving
const char *ntpServer = "pool.ntp.org";
const char *hostName = "epaper";            // reachable as http://epaper.local

// Screen 0 is the clock, screens 1 to 3 are the slots. The control page can
// shorten the rotation down to the clock alone.
const int maxScreens = 4;
const int headerHeight = 46;
int screenCount = maxScreens;
int rotationMinutes = defaultRotationMinutes;

// A stored image is two 1 bit planes of 400 x 300 pixels, black then red,
// rows top to bottom, most significant bit first, a set bit means ink
const size_t planeBytes = 400 * 300 / 8;
const size_t imageBytes = planeBytes * 2;
const size_t maxNotesBytes = 400;
const char *notesPath = "/notes.txt";
const char *uploadPath = "/upload.tmp";

int screenIndex = 0;
bool showNow = true;        // draw on the next loop pass instead of waiting for the timer
size_t uploadBytes = 0;
File uploadFile;

// Built in placeholder content, shown on a screen until an image is uploaded
// to its slot (or, for the notes, until notes are saved from the web page).
// All of it is invented, nothing here is a real forecast or a real event.
const int ICON_SUN = 0;
const int ICON_CLOUD = 1;
const int ICON_RAIN = 2;

struct ForecastDay {
  int icon;
  int high;
  int low;
};
const ForecastDay forecast[3] = {
  { ICON_SUN, 17, 9 },
  { ICON_CLOUD, 15, 8 },
  { ICON_RAIN, 12, 7 },
};

struct Event {
  const char *weekday;
  const char *date;
  const char *title;
  const char *place;
};
const Event events[4] = {
  { "SAT", "3 Oct", "Autumn flea market", "Weberplatz, Babelsberg" },
  { "SUN", "4 Oct", "Open air film night", "Sanssouci Park" },
  { "WED", "7 Oct", "Jazz in the quarter", "Dutch Quarter" },
  { "FRI", "9 Oct", "Night of the museums", "Old Market Square" },
};

const char *notes[5] = {
  "Water the plants",
  "Call landlord about heating",
  "Buy solder and M3 screws",
  "Bike service on Friday",
  "Back up the laptop",
};

// Segment bits: 0 top, 1 top right, 2 bottom right, 3 bottom, 4 bottom left,
// 5 top left, 6 middle
const uint8_t digitSegments[10] = { 0x3F, 0x06, 0x5B, 0x4F, 0x66, 0x6D, 0x7D, 0x07, 0x7F, 0x6F };

// Prints text horizontally centred on centreX, returns the x where it ended
int printCentred(const char *text, int centreX, int baselineY) {
  int16_t boundsX, boundsY;
  uint16_t width, height;
  display.getTextBounds(text, 0, baselineY, &boundsX, &boundsY, &width, &height);
  display.setCursor(centreX - width / 2 - boundsX, baselineY);
  display.print(text);
  return display.getCursorX();
}

// Prints text so that it ends at rightX
void printRightAligned(const char *text, int rightX, int baselineY) {
  int16_t boundsX, boundsY;
  uint16_t width, height;
  display.getTextBounds(text, 0, baselineY, &boundsX, &boundsY, &width, &height);
  display.setCursor(rightX - width - boundsX, baselineY);
  display.print(text);
}

// Draws a degree sign, the 7 bit GFX fonts have no glyph for it
void drawDegree(int x, int y, int radius) {
  display.drawCircle(x, y, radius, GxEPD_BLACK);
  display.drawCircle(x, y, radius - 1, GxEPD_BLACK);
}

// Draws a line roughly 3 pixels thick
void drawThickLine(int x0, int y0, int x1, int y1) {
  display.drawLine(x0, y0, x1, y1, GxEPD_BLACK);
  display.drawLine(x0 + 1, y0, x1 + 1, y1, GxEPD_BLACK);
  display.drawLine(x0 - 1, y0, x1 - 1, y1, GxEPD_BLACK);
  display.drawLine(x0, y0 + 1, x1, y1 + 1, GxEPD_BLACK);
  display.drawLine(x0, y0 - 1, x1, y1 - 1, GxEPD_BLACK);
}

// Draws one seven segment digit, a negative digit draws a dash
void drawDigit(int x, int y, int width, int height, int thickness, int digit) {
  uint8_t segments = digit < 0 ? 0x40 : digitSegments[digit];
  int half = (height - thickness) / 2;
  int radius = thickness / 2;
  int barWidth = width - 2 * thickness;
  int barHeight = half - thickness - 4;
  int rightX = x + width - thickness;

  if (segments & 0x01) display.fillRoundRect(x + thickness, y, barWidth, thickness, radius, GxEPD_BLACK);
  if (segments & 0x40) display.fillRoundRect(x + thickness, y + half, barWidth, thickness, radius, GxEPD_BLACK);
  if (segments & 0x08) display.fillRoundRect(x + thickness, y + height - thickness, barWidth, thickness, radius, GxEPD_BLACK);
  if (segments & 0x20) display.fillRoundRect(x, y + thickness + 2, thickness, barHeight, radius, GxEPD_BLACK);
  if (segments & 0x02) display.fillRoundRect(rightX, y + thickness + 2, thickness, barHeight, radius, GxEPD_BLACK);
  if (segments & 0x10) display.fillRoundRect(x, y + half + thickness + 2, thickness, barHeight, radius, GxEPD_BLACK);
  if (segments & 0x04) display.fillRoundRect(rightX, y + half + thickness + 2, thickness, barHeight, radius, GxEPD_BLACK);
}

// Draws a sun, cloud or rain icon centred on the given point
void drawWeatherIcon(int icon, int centreX, int centreY) {
  if (icon == ICON_SUN) {
    display.fillCircle(centreX, centreY, 15, GxEPD_BLACK);
    for (int ray = 0; ray < 8; ray++) {
      float angle = ray * PI / 4;
      drawThickLine(centreX + cos(angle) * 22, centreY + sin(angle) * 22,
                    centreX + cos(angle) * 33, centreY + sin(angle) * 33);
    }
    return;
  }

  int cloudY = icon == ICON_RAIN ? centreY - 10 : centreY;
  display.fillCircle(centreX - 19, cloudY + 6, 14, GxEPD_BLACK);
  display.fillCircle(centreX - 1, cloudY - 6, 20, GxEPD_BLACK);
  display.fillCircle(centreX + 19, cloudY + 6, 14, GxEPD_BLACK);
  display.fillRoundRect(centreX - 33, cloudY + 4, 66, 16, 8, GxEPD_BLACK);

  if (icon == ICON_RAIN) {
    for (int drop = -1; drop <= 1; drop++) {
      drawThickLine(centreX + drop * 20 + 4, cloudY + 28, centreX + drop * 20 - 4, cloudY + 44);
    }
  }
}

// Draws the coloured title bar, with the current time on the right when known
void drawHeader(const char *title, const struct tm &now, bool hasTime) {
  display.fillRect(0, 0, display.width(), headerHeight, accentColour);
  display.setTextColor(GxEPD_WHITE);
  display.setFont(&FreeSansBold18pt7b);
  display.setCursor(14, 34);
  display.print(title);

  if (hasTime) {
    char clockText[6];
    strftime(clockText, sizeof(clockText), "%H:%M", &now);
    display.setFont(&FreeSansBold12pt7b);
    printRightAligned(clockText, display.width() - 14, 31);
  }
  display.setTextColor(GxEPD_BLACK);
}

// Draws the page dots, with a short label in the bottom left corner
void drawFooter(int screenIndex, const char *label) {
  for (int dot = 0; dot < screenCount; dot++) {
    int dotX = display.width() / 2 + (dot * 2 - (screenCount - 1)) * 9;
    if (dot == screenIndex) {
      display.fillCircle(dotX, 286, 4, accentColour);
    } else {
      display.drawCircle(dotX, 286, 4, GxEPD_BLACK);
    }
  }
  display.setFont(&FreeSans9pt7b);
  display.setCursor(12, 291);
  display.print(label);
}

// Draws the big clock with the weekday and date under it
void drawClock(const struct tm &now, bool hasTime) {
  const int digitWidth = 72, digitHeight = 150, thickness = 18, gap = 14, colonWidth = 34;
  const int top = 30;
  int x = (display.width() - (4 * digitWidth + 2 * gap + colonWidth)) / 2;

  int digits[4] = { -1, -1, -1, -1 };
  if (hasTime) {
    digits[0] = now.tm_hour / 10;
    digits[1] = now.tm_hour % 10;
    digits[2] = now.tm_min / 10;
    digits[3] = now.tm_min % 10;
  }

  drawDigit(x, top, digitWidth, digitHeight, thickness, digits[0]);
  x += digitWidth + gap;
  drawDigit(x, top, digitWidth, digitHeight, thickness, digits[1]);
  x += digitWidth;
  display.fillCircle(x + colonWidth / 2, top + digitHeight / 3, 8, accentColour);
  display.fillCircle(x + colonWidth / 2, top + digitHeight * 2 / 3, 8, accentColour);
  x += colonWidth;
  drawDigit(x, top, digitWidth, digitHeight, thickness, digits[2]);
  x += digitWidth + gap;
  drawDigit(x, top, digitWidth, digitHeight, thickness, digits[3]);

  char weekday[12] = "No time yet";
  char date[24] = "Check the WiFi";
  if (hasTime) {
    char month[12];
    strftime(weekday, sizeof(weekday), "%A", &now);
    strftime(month, sizeof(month), "%B", &now);
    snprintf(date, sizeof(date), "%d %s %d", now.tm_mday, month, now.tm_year + 1900);
  }
  display.setFont(&FreeSansBold18pt7b);
  printCentred(weekday, display.width() / 2, 226);
  display.setFont(&FreeSans12pt7b);
  printCentred(date, display.width() / 2, 258);
}

// Draws three forecast columns: day, icon, high and low
void drawForecast(const struct tm &now, bool hasTime) {
  drawHeader("Potsdam forecast", now, hasTime);
  const int columnWidth = display.width() / 3;

  for (int day = 0; day < 3; day++) {
    int centreX = columnWidth * day + columnWidth / 2;
    if (day > 0) {
      display.drawFastVLine(columnWidth * day, 62, 196, GxEPD_BLACK);
    }

    char label[12] = "Later";
    if (day == 0) strcpy(label, "Today");
    if (day == 1) strcpy(label, "Tomorrow");
    if (day == 2 && hasTime) {
      struct tm later = now;
      later.tm_mday += day;
      mktime(&later);
      strftime(label, sizeof(label), "%A", &later);
    }
    display.setFont(&FreeSansBold12pt7b);
    printCentred(label, centreX, 84);

    drawWeatherIcon(forecast[day].icon, centreX, 138);

    char temperature[12];
    snprintf(temperature, sizeof(temperature), "%d", forecast[day].high);
    display.setFont(&FreeSansBold18pt7b);
    int endX = printCentred(temperature, centreX - 5, 218);
    drawDegree(endX + 7, 197, 4);

    snprintf(temperature, sizeof(temperature), "low %d", forecast[day].low);
    display.setFont(&FreeSans12pt7b);
    endX = printCentred(temperature, centreX - 4, 250);
    drawDegree(endX + 5, 236, 3);
  }
}

// Draws the event list: date on the left, title and place on the right
void drawEvents(const struct tm &now, bool hasTime) {
  drawHeader("Potsdam events", now, hasTime);
  const int rowHeight = 54;

  for (int row = 0; row < 4; row++) {
    int top = headerHeight + 4 + row * rowHeight;
    if (row > 0) {
      display.drawFastHLine(12, top, display.width() - 24, GxEPD_BLACK);
    }
    display.setFont(&FreeSansBold9pt7b);
    display.setCursor(14, top + 21);
    display.print(events[row].weekday);
    display.setFont(&FreeSans9pt7b);
    display.setCursor(14, top + 42);
    display.print(events[row].date);

    display.setFont(&FreeSansBold12pt7b);
    display.setCursor(92, top + 24);
    display.print(events[row].title);
    display.setFont(&FreeSans9pt7b);
    display.setCursor(92, top + 44);
    display.print(events[row].place);
  }
}

// Returns the notes saved from the web page, empty when there are none
String readNotes() {
  if (!LittleFS.exists(notesPath)) return "";
  File file = LittleFS.open(notesPath, "r");
  String text = file.readString();
  file.close();
  return text;
}

// Draws the notes as a bullet list, one bullet per line, five lines at most.
// Falls back to the built in sample notes when none are saved.
void drawNotes(const struct tm &now, bool hasTime) {
  drawHeader("Notes", now, hasTime);
  display.setFont(&FreeSans12pt7b);
  String saved = readNotes();
  int lineStart = 0;

  for (int row = 0; row < 5; row++) {
    String line = notes[row];
    if (saved.length() > 0) {
      int lineEnd = saved.indexOf('\n', lineStart);
      if (lineEnd < 0) lineEnd = saved.length();
      line = saved.substring(lineStart, lineEnd);
      lineStart = min(lineEnd + 1, (int)saved.length());
    }
    line.trim();
    if (line.length() == 0) continue;

    int baselineY = 88 + row * 40;
    display.fillCircle(24, baselineY - 8, 5, GxEPD_BLACK);
    display.setCursor(44, baselineY);
    display.print(line);
  }
}

// Path of the stored image for a slot, slots are numbered 1 to 3
String slotPath(int slot) {
  return "/slot" + String(slot) + ".bin";
}

// Draws the image stored for a slot, returns false when the slot has none
bool drawSlotImage(int slot) {
  static uint8_t plane[planeBytes];
  if (!LittleFS.exists(slotPath(slot))) return false;
  File file = LittleFS.open(slotPath(slot), "r");
  if (file.size() != imageBytes) {
    file.close();
    return false;
  }

  file.read(plane, planeBytes);
  display.drawBitmap(0, 0, plane, 400, 300, GxEPD_BLACK);
  file.read(plane, planeBytes);
  display.drawBitmap(0, 0, plane, 400, 300, GxEPD_RED);
  file.close();
  return true;
}

// Draws a plain two line status message, used before the rotation starts
void drawMessage(const char *title, const char *detail) {
  display.setFullWindow();
  display.firstPage();
  do {
    display.fillScreen(GxEPD_WHITE);
    display.setFont(&FreeSansBold18pt7b);
    printCentred(title, display.width() / 2, 140);
    display.setFont(&FreeSans9pt7b);
    printCentred(detail, display.width() / 2, 176);
  } while (display.nextPage());
}

// Draws one screen of the rotation, always as a full refresh, this panel has
// no fast partial refresh. Screen 0 is the clock, screens 1 to 3 show the
// image uploaded to that slot, or their built in screen when there is none.
void drawScreen(int screenIndex) {
  struct tm now;
  bool hasTime = getLocalTime(&now, 0);
  bool hasNotes = LittleFS.exists(notesPath);
  String address = WiFi.isConnected() ? WiFi.localIP().toString() : String("no WiFi");

  display.setFullWindow();

  display.firstPage();
  do {
    display.fillScreen(GxEPD_WHITE);
    if (screenIndex == 0) {
      drawClock(now, hasTime);
      drawFooter(screenIndex, address.c_str());
    } else if (!drawSlotImage(screenIndex)) {
      if (screenIndex == 1) drawForecast(now, hasTime);
      if (screenIndex == 2) drawEvents(now, hasTime);
      if (screenIndex == 3) drawNotes(now, hasTime);
      drawFooter(screenIndex, screenIndex == 3 && hasNotes ? "" : "sample data");
    }
  } while (display.nextPage());

  // Put the panel controller into deep sleep between updates, the image stays
  // up with no power and the driver wakes it through RES on the next update
  display.hibernate();

  Serial.print("loop: drew screen ");
  Serial.print(screenIndex);
  Serial.println(hasTime ? ", time synced" : ", time NOT synced");
}

// Jumps the rotation to a screen and draws it on the next loop pass, so a
// change made over HTTP shows up straight away
void showScreen(int index) {
  screenIndex = index;
  showNow = true;
}

// Slot number from the last character of the request path, e.g. /slot/2
int requestSlot() {
  return server.uri().charAt(server.uri().length() - 1) - '0';
}

// Serves the control page
void handleRoot() {
  server.send_P(200, "text/html", pageHtml);
}

// Returns the saved notes as plain text
void handleNotesGet() {
  server.send(200, "text/plain", readNotes());
}

// Saves the notes from the "text" form field, empty text restores the samples
void handleNotesPost() {
  String text = server.arg("text");
  if (text.length() > maxNotesBytes) {
    server.send(400, "text/plain", "Notes are too long, the limit is 400 characters");
    return;
  }
  if (text.length() == 0) {
    LittleFS.remove(notesPath);
  } else {
    File file = LittleFS.open(notesPath, "w");
    file.print(text);
    file.close();
  }
  server.send(200, "text/plain", "saved");
  showScreen(3);
}

// Returns the two settings as JSON
void handleSettingsGet() {
  server.send(200, "application/json",
              "{\"minutes\":" + String(rotationMinutes) + ",\"screens\":" + String(screenCount) + "}");
}

// Saves the rotation time and the number of screens from the "minutes" and
// "screens" form fields. Applies from the next screen change, without forcing
// a refresh.
void handleSettingsPost() {
  int minutes = server.arg("minutes").toInt();
  int screens = server.arg("screens").toInt();
  if (minutes < 1 || minutes > maxRotationMinutes || screens < 1 || screens > maxScreens) {
    server.send(400, "text/plain", "minutes must be 1 to 120 and screens 1 to 4");
    return;
  }
  rotationMinutes = minutes;
  screenCount = screens;
  settings.putInt("minutes", minutes);
  settings.putInt("screens", screens);
  if (screenIndex >= screenCount) screenIndex = 0;
  server.send(200, "text/plain", "saved");
}

// Receives an image upload chunk by chunk into a temporary file. Bytes past
// the expected size are counted but not written, so an oversized upload
// cannot fill the flash.
void handleSlotUpload() {
  HTTPUpload &upload = server.upload();
  if (upload.status == UPLOAD_FILE_START) {
    uploadBytes = 0;
    uploadFile = LittleFS.open(uploadPath, "w");
  } else if (upload.status == UPLOAD_FILE_WRITE) {
    uploadBytes += upload.currentSize;
    if (uploadFile && uploadBytes <= imageBytes) {
      uploadFile.write(upload.buf, upload.currentSize);
    }
  } else if (uploadFile) {
    uploadFile.close();
  }
}

// Runs after an upload finished: keeps the image only if its size is right
void handleSlotPost() {
  int slot = requestSlot();
  bool isValid = uploadBytes == imageBytes;
  uploadBytes = 0;
  if (!isValid) {
    LittleFS.remove(uploadPath);
    server.send(400, "text/plain", "Image must be exactly 30000 bytes: a 15000 byte black plane then a 15000 byte red plane, 400 x 300 at 1 bit per pixel");
    return;
  }
  LittleFS.remove(slotPath(slot));
  LittleFS.rename(uploadPath, slotPath(slot));
  server.send(200, "text/plain", "saved");
  showScreen(slot);
}

// Returns the stored image of a slot, in the same format it was uploaded in
void handleSlotGet() {
  int slot = requestSlot();
  if (!LittleFS.exists(slotPath(slot))) {
    server.send(404, "text/plain", "This slot has no image");
    return;
  }
  File file = LittleFS.open(slotPath(slot), "r");
  server.streamFile(file, "application/octet-stream");
  file.close();
}

// Removes the stored image of a slot, its built in screen comes back
void handleSlotDelete() {
  int slot = requestSlot();
  LittleFS.remove(slotPath(slot));
  server.send(200, "text/plain", "removed");
  showScreen(slot);
}

// Registers the HTTP routes and starts the server. No authentication: anyone
// on the same network can change the content.
//   GET    /          control page
//   GET    /notes     saved notes as plain text
//   POST   /notes     form field "text", empty restores the sample notes
//   GET    /slot/N    stored image of slot N (1 to 3), 404 when empty
//   POST   /slot/N    multipart file of exactly 30000 bytes, see imageBytes
//   DELETE /slot/N    removes the image, the built in screen comes back
//   GET    /settings  {"minutes":3,"screens":4}
//   POST   /settings  form fields "minutes" (1 to 120) and "screens" (1 to 4)
void startServer() {
  server.on("/", HTTP_GET, handleRoot);
  server.on("/notes", HTTP_GET, handleNotesGet);
  server.on("/notes", HTTP_POST, handleNotesPost);
  server.on("/settings", HTTP_GET, handleSettingsGet);
  server.on("/settings", HTTP_POST, handleSettingsPost);
  for (int slot = 1; slot < maxScreens; slot++) {
    String path = "/slot/" + String(slot);
    server.on(path, HTTP_GET, handleSlotGet);
    server.on(path, HTTP_POST, handleSlotPost, handleSlotUpload);
    server.on(path, HTTP_DELETE, handleSlotDelete);
  }
  server.begin();
}

void setup() {
  // 80 MHz is the slowest clock the WiFi radio works with, and draws clearly
  // less than the default 240 MHz
  setCpuFrequencyMhz(80);
  Serial.begin(9600);
  Serial.println("setup: start");

  // Formats the flash file system on first use
  LittleFS.begin(true);
  settings.begin("epaper");
  rotationMinutes = settings.getInt("minutes", defaultRotationMinutes);
  screenCount = settings.getInt("screens", maxScreens);

  // First argument is the baud rate GxEPD2 uses for its own diagnostics, keep
  // it the same as Serial.begin above or the monitor turns to garbage
  display.init(9600, true, 2, false);
  display.setRotation(screenRotation);
  display.setTextColor(GxEPD_BLACK);
  display.setTextWrap(false);
  Serial.println("setup: display initialized");

  if (!wifiManager.getWiFiIsSaved()) {
    drawMessage("WiFi setup needed", "Join the WiFi \"EpaperSetup\" and pick your network");
  }

  // Opens a "EpaperSetup" access point with a captive portal on first boot
  // (or if saved credentials fail), gives up after 3 minutes so the screens
  // still rotate without a clock
  wifiManager.setConfigPortalTimeout(180);
  if (wifiManager.autoConnect("EpaperSetup")) {
    // Modem sleep: the radio naps between access point beacons, the web page
    // stays reachable, replies are just a little slower
    WiFi.setSleep(true);
    MDNS.begin(hostName);
    startServer();
    Serial.print("setup: WiFi connected, browse to http://");
    Serial.print(hostName);
    Serial.print(".local or http://");
    Serial.println(WiFi.localIP());
    configTzTime(timeZone, ntpServer);
    struct tm now;
    Serial.println(getLocalTime(&now, 10000) ? "setup: time synced" : "setup: time sync failed");
  } else {
    Serial.println("setup: no WiFi, clock will stay blank");
  }
}

void loop() {
  static unsigned long lastDrawMillis = 0;
  server.handleClient();

  if (showNow || millis() - lastDrawMillis >= rotationMinutes * 60000UL) {
    showNow = false;
    drawScreen(screenIndex);
    screenIndex = (screenIndex + 1) % screenCount;
    lastDrawMillis = millis();
  }
  // Lets the CPU idle between requests instead of spinning
  delay(10);
}
