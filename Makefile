FQBN      = m5stack:esp32:m5stack_dial
PORT      = /dev/cu.usbmodem1101
BUILD_DIR = build
SKETCH    = .
ESPTOOL   = $(HOME)/Library/Arduino15/packages/m5stack/tools/esptool_py/5.1.0/esptool

.PHONY: all compile flash verify bootloader clean list

all: compile flash

compile:
	arduino-cli compile --fqbn $(FQBN) --build-path $(BUILD_DIR) $(SKETCH)

flash:
	arduino-cli upload -p $(PORT) --fqbn $(FQBN) --verify --build-path $(BUILD_DIR)

verify:
	$(ESPTOOL) --port $(PORT) --baud 921600 verify-flash \
	  0x0000 $(BUILD_DIR)/m5dail_proj.ino.bootloader.bin \
	  0x8000 $(BUILD_DIR)/m5dail_proj.ino.partitions.bin \
	  0x10000 $(BUILD_DIR)/m5dail_proj.ino.bin

bootloader:
	$(ESPTOOL) --port $(PORT) --baud 921600 write-flash 0x0000 $(BUILD_DIR)/m5dail_proj.ino.bootloader.bin

clean:
	rm -rf $(BUILD_DIR)

list:
	arduino-cli board list
