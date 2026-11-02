.RECIPEPREFIX := >
ifeq ($(strip $(DEVKITARM)),)
$(error Set DEVKITARM)
endif
DEVKITPRO ?= /opt/devkitpro
LIBNDS := $(DEVKITPRO)/libnds
CALICO := $(DEVKITPRO)/calico
CC := $(DEVKITARM)/bin/arm-none-eabi-gcc
NDSTOOL := $(DEVKITPRO)/tools/bin/ndstool
TARGET := elewiz
ARCH := -march=armv5te -mtune=arm946e-s -mthumb -mthumb-interwork
CFLAGS := -O2 -Wall -ffast-math -fomit-frame-pointer $(ARCH) -D__NDS__ -DARM9 -I$(LIBNDS)/include -I$(CALICO)/include
LDFLAGS := -specs=$(CALICO)/share/ds9.specs -g $(ARCH) -L$(LIBNDS)/lib -L$(CALICO)/lib

all: $(TARGET).nds
$(TARGET).elf: source/main.c
> $(CC) $(CFLAGS) $< $(LDFLAGS) -lnds9 -lcalico_ds9 -lm -o $@
$(TARGET).nds: $(TARGET).elf
> $(NDSTOOL) -c $@ -9 $< -b icon.bmp "ELEWIZ;Wizard Combo Flyer;Homebrew"
clean:
> rm -f *.elf *.nds
