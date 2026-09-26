# FQBN = Fully Qualified Board Name, format vendor:architecture:board[:options]
FQBN := esp32:esp32:esp32
PORT := /dev/cu.usbserial-0001
# 115200 was unreliable (garbled serial) on this board/cable, 9600 is confirmed clean
BAUD := 9600

# Each entry here is a standalone sketch under modules/<dir> with its own
# setup()/loop(). Add a module by adding one line below, name is the Make
# target prefix (<name>-compile/-upload/-flash), dir is its folder under modules/.
MODULES := servo ir servoweb buzzerweb scareprop epaper
MODULE_DIR_servo := modules/servo
MODULE_DIR_ir := modules/ir
MODULE_DIR_servoweb := modules/servo_web
MODULE_DIR_buzzerweb := modules/buzzer_web
MODULE_DIR_scareprop := modules/scare_prop
MODULE_DIR_epaper := modules/epaper

.PHONY: monitor $(foreach m,$(MODULES),$(m)-compile $(m)-upload $(m)-flash)

monitor:
	arduino-cli monitor -p $(PORT) -c baudrate=$(BAUD)

define MODULE_TARGETS
$(1)-flash: $(1)-compile $(1)-upload

$(1)-compile:
	arduino-cli compile --fqbn $(FQBN) --output-dir $(2)/build/esp32.esp32.esp32 $(2)

$(1)-upload:
	arduino-cli upload -p $(PORT) --fqbn $(FQBN) $(2)
endef

$(foreach m,$(MODULES),$(eval $(call MODULE_TARGETS,$(m),$(MODULE_DIR_$(m)))))
