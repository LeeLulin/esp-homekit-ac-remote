PROGRAM ?= cooler
#PROGRAM_SRC_DIR = src

EXTRA_COMPONENTS = \
	extras/http-parser \
	extras/dhcpserver \
	extras/dht \
	$(abspath components/ir) \
	$(abspath components/wolfssl) \
	$(abspath components/cjson) \
	$(abspath components/homekit)

FLASH_SIZE ?= 32
FLASH_MODE ?= dout
FLASH_SPEED ?= 40
HOMEKIT_SPI_FLASH_BASE_ADDR ?= 0x7A000
 
EXTRA_CFLAGS += -I../.. -DHOMEKIT_SHORT_APPLE_UUIDS -DLWIP_HTTPD_CGI=1 -DLWIP_HTTPD_SSI=1 -I./fsdata
 
include $(abspath ../../sdk/esp-open-rtos/common.mk)

monitor:
	$(FILTEROUTPUT) --port $(ESPPORT) --baud 115200 --elf $(PROGRAM_OUT)
