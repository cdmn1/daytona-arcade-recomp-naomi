TARGET = naomi_input_probe.elf
OBJS = main.o

all: rm-elf $(TARGET)

include $(KOS_BASE)/Makefile.rules

$(TARGET): $(OBJS)
	kos-cc -o $@ $(OBJS)

clean: rm-elf
	-rm -f $(OBJS)

rm-elf:
	-rm -f $(TARGET)

.PHONY: all clean rm-elf
