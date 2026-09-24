#include <IRremote.hpp>

const int irPin = 4;

// Prints the decoded IR command over Serial
void logIrKeyToSerial(uint8_t command) {
  Serial.print("IR command: 0x");
  Serial.println(command, HEX);
}

// Reads a pending IR button press and logs it
void handleIrRemote() {
  if (!IrReceiver.decode()) {
    return;
  }

  logIrKeyToSerial(IrReceiver.decodedIRData.command);
  IrReceiver.resume();
}

void setup() {
  Serial.begin(9600);
  Serial.println("setup: start");

  IrReceiver.begin(irPin);
  Serial.println("setup: IR receiver started");
}

void loop() {
  handleIrRemote();
}
