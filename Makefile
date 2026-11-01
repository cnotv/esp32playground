# FQBN = Fully Qualified Board Name, format vendor:architecture:board[:options]
FQBN := esp32:esp32:esp32
PORT := /dev/cu.usbserial-0001
# 115200 was unreliable (garbled serial) on this board/cable, 9600 is confirmed clean
BAUD := 9600

# Each entry here is a standalone sketch under modules/<dir> with its own
# setup()/loop(). Add a module by adding one line below, name is the Make
# target prefix (<name>-compile/-upload/-flash), dir is its folder under modules/.
MODULES := servo ir servoweb buzzerweb scareprop epaper yamaha
MODULE_DIR_servo := modules/servo
MODULE_DIR_ir := modules/ir
MODULE_DIR_servoweb := modules/servo_web
MODULE_DIR_buzzerweb := modules/buzzer_web
MODULE_DIR_scareprop := modules/scare_prop
MODULE_DIR_epaper := modules/epaper
MODULE_DIR_yamaha := modules/yamaha_esp_bridge/yamaha_esp_bridge

# The MIDI bridge runs on an ESP32-S3 (USB-OTG host), not the DevKit v1.
FQBN_yamaha := esp32:esp32:esp32s3
# No default port: pass it per run, e.g. make yamaha-flash PORT=/dev/cu.usbmodem...

.PHONY: monitor $(foreach m,$(MODULES),$(m)-compile $(m)-upload $(m)-flash)

monitor:
	arduino-cli monitor -p $(PORT) -c baudrate=$(BAUD)

define MODULE_TARGETS
$(1)-flash: $(1)-compile $(1)-upload

$(1)-compile:
	arduino-cli compile --fqbn $(or $(FQBN_$(1)),$(FQBN)) --output-dir $(2)/build/$(subst :,.,$(or $(FQBN_$(1)),$(FQBN))) $(2)

$(1)-upload:
	arduino-cli upload -p $(or $(PORT_$(1)),$(PORT)) --fqbn $(or $(FQBN_$(1)),$(FQBN)) $(2)
endef

$(foreach m,$(MODULES),$(eval $(call MODULE_TARGETS,$(m),$(MODULE_DIR_$(m)))))
