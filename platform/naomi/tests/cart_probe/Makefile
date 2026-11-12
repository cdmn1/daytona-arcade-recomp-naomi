ifeq ($(KOS_SUBARCH),)
$(error KOS_SUBARCH is unset. Source the Naomi KOS environ.sh first)
endif
ifneq ($(KOS_SUBARCH),naomi)
$(error Wrong KOS target '$(KOS_SUBARCH)'. This probe requires KOS_SUBARCH=naomi)
endif
ifeq ($(KOS_BASE),)
$(error KOS_BASE is unset. Source the Naomi KOS environ.sh first)
endif

CFLAGS += -I../../include
NAOMIBINTOOL ?= $(KOS_BASE)/utils/naomibintool/naomibintool

$(NAOMIBINTOOL):
	$(MAKE) -C $(KOS_BASE)/utils/naomibintool CC=cc

TARGET = naomi_cart_probe.elf
BASE_ROM = naomi_cart_probe.base.bin
ROMFS = naomi_cart_probe.romfs
ROM = naomi_cart_probe_v1.bin
OBJS = main.o ../../code/naomi_cart.o ../../code/naomi_romfs.o

all: $(TARGET)

rom: $(ROM)

.PHONY: FORCE_ROMFS
FORCE_ROMFS:
$(ROMFS): FORCE_ROMFS
	python3 ../../tools/romfsgen.py $@ assets

$(BASE_ROM): $(TARGET) $(NAOMIBINTOOL)
	$(NAOMIBINTOOL) build $@ -d "CDMN" -t "Naomi NetDIMM Probe" -e $(TARGET)

$(ROM): $(BASE_ROM) $(ROMFS)
	python3 ../../tools/append_romfs.py $< $(ROMFS) $@

include $(KOS_BASE)/Makefile.rules

$(TARGET): $(OBJS)
	kos-cc -o $@ $(OBJS)

clean:
	-rm -f $(TARGET) $(BASE_ROM) $(ROMFS) $(ROM) $(OBJS)

.PHONY: all rom clean
