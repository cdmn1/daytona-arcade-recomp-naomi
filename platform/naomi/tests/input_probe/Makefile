# This target must never be built against a Dreamcast-only KOS environment.
ifeq ($(KOS_SUBARCH),)
$(error KOS_SUBARCH is unset. Source the Naomi KOS environ.sh first)
endif
ifneq ($(KOS_SUBARCH),naomi)
$(error Wrong KOS target '$(KOS_SUBARCH)'. This probe requires KOS_SUBARCH=naomi)
endif
ifeq ($(KOS_BASE),)
$(error KOS_BASE is unset. Source the Naomi KOS environ.sh first)
endif

NAOMIBINTOOL ?= $(KOS_BASE)/utils/naomibintool/naomibintool

$(NAOMIBINTOOL):
	$(MAKE) -C $(KOS_BASE)/utils/naomibintool CC=cc

TARGET = naomi_input_probe.elf
ROM = naomi_input_probe_v2.bin
OBJS = main.o

all: $(TARGET)

rom: $(ROM)

$(ROM): $(TARGET)
	$(NAOMIBINTOOL) build $@ -e $<

include $(KOS_BASE)/Makefile.rules

$(TARGET): $(OBJS)
	kos-cc -o $@ $(OBJS)

clean:
	-rm -f $(TARGET) $(ROM) $(OBJS)

.PHONY: all rom clean
