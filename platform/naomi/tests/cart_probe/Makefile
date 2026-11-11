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

TARGET = naomi_cart_probe.elf
ROM = naomi_cart_probe.bin
OBJS = main.o ../../code/naomi_cart.o

all: $(TARGET)

rom: $(ROM)

$(ROM): $(TARGET)
	naomibintool build $@ -e $<

include $(KOS_BASE)/Makefile.rules

$(TARGET): $(OBJS)
	kos-cc -o $@ $(OBJS)

clean:
	-rm -f $(TARGET) $(ROM) $(OBJS)

.PHONY: all rom clean
