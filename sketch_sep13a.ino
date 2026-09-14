#include <LiquidCrystal.h>

// Define the exact pins you used on your ESP32
const int rs = 13, en = 12, d4 = 14, d5 = 27, d6 = 26, d7 = 25;
LiquidCrystal lcd(rs, en, d4, d5, d6, d7);

void setup() {
  // Initialize the LCD
  lcd.begin(16, 2);
  lcd.clear();
  
  // Print verification text
  lcd.setCursor(0, 0);
  lcd.print("Testing LCD...");
  lcd.setCursor(0, 1);
  lcd.print("Signal OK!");
}

void loop() {
  // Blinks a period to show the ESP32 code is actively looping
  lcd.setCursor(15, 1);
  lcd.print(".");
  delay(500);
  lcd.setCursor(15, 1);
  lcd.print(" ");
  delay(500);
}
