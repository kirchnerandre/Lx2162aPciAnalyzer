KDIR ?= /lib/modules/$(shell uname -r)/build
MODULE_DIR := $(CURDIR)
BUILD_DIR := $(MODULE_DIR)/build

obj-m := Lx2162aPciAnalyzer.o
ccflags-y := -I$(src)

.PHONY: all clean

all:
	mkdir -p "$(BUILD_DIR)"
	$(MAKE) -C "$(KDIR)" M="$(MODULE_DIR)" MO="$(BUILD_DIR)" modules

clean:
	rm -rf "$(BUILD_DIR)"
