PROGRAM = raw_dumper

EXTRA_COMPONENTS = $(abspath ../..)

include $(SDK_PATH)/common.mk

monitor:
	$(FILTEROUTPUT) --port $(ESPPORT) --baud 115200 --elf $(PROGRAM_OUT)
