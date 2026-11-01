#include <GxEPD2_BW.h>
#include <Fonts/FreeMonoBold9pt7b.h>

// WeAct Studio 4.2in b/w module (SSD1683 driver, panel code GDEY042T81). If
// your actual panel is a different model, swap this class for the matching
// one in GxEPD2/src/GxEPD2_display_selection_new_style.h.
const int csPin = 5;
const int dcPin = 17;
const int rstPin = 16;
const int busyPin = 4;

GxEPD2_BW<GxEPD2_420_GDEY042T81, GxEPD2_420_GDEY042T81::HEIGHT> display(
    GxEPD2_420_GDEY042T81(csPin, dcPin, rstPin, busyPin));

// Draws a placeholder screen, swap this out once a real image source exists
void drawPlaceholder() {
  display.setRotation(1);
  display.setFont(&FreeMonoBold9pt7b);
  display.setTextColor(GxEPD_BLACK);

  display.firstPage();
  do {
    display.fillScreen(GxEPD_WHITE);
    display.setCursor(10, 30);
    display.print("E-paper module ready");
    display.setCursor(10, 60);
    display.print("No image source yet");
  } while (display.nextPage());
}

void setup() {
  Serial.begin(9600);
  Serial.println("setup: start");

  display.init(115200, true, 2, false);
  Serial.println("setup: display initialized");

  drawPlaceholder();
  Serial.println("setup: placeholder drawn");

  // E-paper holds its image with no power, so the panel can go to sleep
  // between updates instead of staying powered
  display.hibernate();
  Serial.println("setup: display hibernating");
}

void loop() {
}
