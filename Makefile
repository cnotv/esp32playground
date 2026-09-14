# FQBN = Fully Qualified Board Name, format vendor:architecture:board[:options]
FQBN := esp32:esp32:esp32
PORT := /dev/cu.usbserial-0001
SKETCH := .

.PHONY: flash compile upload

flash: compile upload

compile:
	arduino-cli compile --fqbn $(FQBN) --output-dir build/esp32.esp32.esp32 $(SKETCH)

upload:
	arduino-cli upload -p $(PORT) --fqbn $(FQBN) $(SKETCH)
