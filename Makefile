# Arduino Make file. Refer to https://github.com/sudar/Arduino-Makefile

BOARD_TAG    = leonardo
MONITOR_PORT = /dev/ttyACM0
USER_LIB_PATH = $(realpath libs)
ARDMK_DIR = $(realpath tools/Arduino-Makefile)

# Last
include $(ARDMK_DIR)/Arduino.mk

